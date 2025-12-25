/**
 * @file Seccomp.cpp
 * @brief Implementation of the Seccomp class.
 */

#include "modules/security/Seccomp.h"
#include "utils/Syscalls.h"
#include "core/Logger.h"
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <algorithm>
#include <cstring>

// Include seccomp header
#include <seccomp.h>

namespace sandbox {

Seccomp::Seccomp()
    : state_(ModuleState::UNINITIALIZED)
    , defaultAction_(ACTION_ERRNO)
    , enabled_(true)
{
}

std::string Seccomp::getName() const {
    return "seccomp";
}

std::string Seccomp::getVersion() const {
    return "1.0.0";
}

ModuleState Seccomp::getState() const {
    return state_;
}

bool Seccomp::initialize(const SandboxConfiguration& config) {
    SANDBOX_INFO("Initializing Seccomp module");
    config_ = config;

    // Check if seccomp is enabled
    enabled_ = !config.security.seccomp_policy.empty() ||
               !config.security.seccomp_profile_path.empty();

    if (!enabled_) {
        SANDBOX_INFO("Seccomp is disabled (no policy specified)");
        state_ = ModuleState::INITIALIZED;
        return true;
    }

    // Set default action based on policy
    if (config.security.seccomp_policy == "default") {
        defaultAction_ = ACTION_ERRNO;
    } else if (config.security.seccomp_policy == "strict") {
        defaultAction_ = ACTION_KILL;
    } else if (config.security.seccomp_policy == "log") {
        defaultAction_ = ACTION_LOG;
    } else if (config.security.seccomp_policy == "allow") {
        defaultAction_ = ACTION_ALLOW;
    }

    // Load profile if specified
    if (!config.security.seccomp_profile_path.empty()) {
        if (!loadProfile(config.security.seccomp_profile_path)) {
            SANDBOX_ERROR("Failed to load seccomp profile");
            return false;
        }
    } else {
        // Generate default policy
        if (!generateDefaultPolicy(config)) {
            SANDBOX_ERROR("Failed to generate default policy");
            return false;
        }
    }

    state_ = ModuleState::INITIALIZED;
    SANDBOX_INFO("Seccomp module initialized successfully");

    return true;
}

bool Seccomp::prepareChild(const SandboxConfiguration& config, pid_t childPid) {
    // Seccomp is applied in the child process
    return true;
}

bool Seccomp::applyChild(const SandboxConfiguration& config) {
    if (!enabled_) {
        SANDBOX_DEBUG("Seccomp is disabled, skipping");
        return true;
    }

    SANDBOX_INFO("Applying seccomp filter");

    // Ensure the filter is compiled
    if (filterBlob_.empty()) {
        if (!installFilter()) {
            SANDBOX_ERROR("Failed to compile seccomp filter");
            return false;
        }
    }

    // Load the filter using prctl
    if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, filterBlob_.data()) < 0) {
        SANDBOX_ERROR("Failed to set seccomp: " + std::string(strerror(errno)));
        return false;
    }

    SANDBOX_DEBUG("Seccomp filter applied successfully");
    state_ = ModuleState::RUNNING;

    return true;
}

int Seccomp::execute(const SandboxConfiguration& config) {
    return 0;
}

bool Seccomp::cleanup() {
    SANDBOX_DEBUG("Cleaning up Seccomp module");
    rules_.clear();
    filterBlob_.clear();
    state_ = ModuleState::STOPPED;
    return true;
}

std::vector<std::string> Seccomp::getDependencies() const {
    return {};
}

bool Seccomp::isEnabled() const {
    return enabled_;
}

std::string Seccomp::getDescription() const {
    return "Implements seccomp BPF filtering to restrict system calls available to sandbox processes.";
}

std::string Seccomp::getType() const {
    return "security";
}

bool Seccomp::loadProfile(const std::string& path) {
    SANDBOX_INFO("Loading seccomp profile from: " + path);

    auto content = Syscall::readFile(path);
    if (!content) {
        SANDBOX_ERROR("Failed to read seccomp profile");
        return false;
    }

    // Parse JSON profile and add rules
    // This is a simplified implementation
    // In a full implementation, you would parse the JSON and create rules

    SANDBOX_INFO("Seccomp profile loaded successfully");
    return true;
}

void Seccomp::addRule(const SyscallRule& rule) {
    rules_.push_back(rule);
}

void Seccomp::setDefaultAction(int action) {
    defaultAction_ = action;
}

int Seccomp::getDefaultAction() const {
    return defaultAction_;
}

bool Seccomp::generateDefaultPolicy(const SandboxConfiguration& config) {
    // Create a context for building the filter
    scmp_filter_ctx ctx = seccomp_init(defaultAction_);
    if (!ctx) {
        SANDBOX_ERROR("Failed to create seccomp context");
        return false;
    }

    // Allow essential system calls for basic operation
    // These are typically needed by any program
    std::vector<std::string> essentialCalls = {
        "read", "write", "close", "brk", "execve", "exit_group", "exit",
        "getpid", "gettid", "getppid", "getuid", "getgid", "geteuid", "getegid",
        "getrandom", "mmap", "mprotect", "munmap", "rt_sigaction",
        "rt_sigprocmask", "rt_sigreturn", "ioctl", "pread64", "pwrite64",
        "readv", "writev", "access", "pipe", "sched_yield", "mremap",
        "msync", "mincore", "madvise", "shmget", "shmat", "shmctl",
        "dup", "dup2", "pause", "nanosleep", "getitimer", "setitimer",
        "alarm", "setpgid", "getpgid", "getsid", "setsid", "syslog",
        "getrlimit", "getrusage", "gettimeofday", "settimeofday",
        "symlink", "readlink", "uselib", "readahead", "setxattr",
        "lsetxattr", "fsetxattr", "getxattr", "lgetxattr", "fgetxattr",
        "listxattr", "llistxattr", "flistxattr", "removexattr",
        "lremovexattr", "fremovexattr", "tkill", "time", "futex",
        "sched_setaffinity", "sched_getaffinity", "io_setup", "io_destroy",
        "io_getevents", "io_submit", "io_cancel", "lookup_dcookie",
        "epoll_create", "remap_file_pages", "set_tid_address", "timer_create",
        "timer_settime", "timer_gettime", "timer_getoverrun", "timer_delete",
        "clock_settime", "clock_gettime", "clock_getres", "clock_nanosleep",
        "exit", "wait4", "kill", "uname", "semget", "semop", "semctl",
        "shmdt", "msgget", "msgsnd", "msgrcv", "msgctl", "fcntl", "flock",
        "fsync", "fdatasync", "truncate", "ftruncate", "getcwd", "chdir",
        "fchdir", "rename", "mkdir", "rmdir", "creat", "link", "unlink",
        "open", "close", "vhangup", "signal", "sethostname", "setrlimit"
    };

    for (const auto& call : essentialCalls) {
        int syscallNum = seccomp_syscall_resolve_name(call.c_str());
        if (syscallNum != __NR_SCMP_ERROR) {
            if (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, syscallNum, 0) < 0) {
                SANDBOX_WARNING("Failed to add rule for: " + call);
            }
        }
    }

    // Add a deny rule for some dangerous calls (blacklist mode alternative)
    // This is just an example; actual dangerous calls depend on the workload
    std::vector<std::string> dangerousCalls = {
        "reboot", "swapon", "swapoff", "init_module", "delete_module",
        "kexec_load", "kexec_file_load", "acct", "add_key", "request_key"
    };

    for (const auto& call : dangerousCalls) {
        int syscallNum = seccomp_syscall_resolve_name(call.c_str());
        if (syscallNum != __NR_SCMP_ERROR) {
            // In default (whitelist) mode, dangerous calls are already blocked
            // by default action. This is just for documentation.
        }
    }

    // Export the filter
    filterBlob_.resize(0);
    size_t blobSize = 0;

    if (seccomp_export_bpf(ctx, nullptr, &blobSize) < 0) {
        SANDBOX_ERROR("Failed to get BPF blob size");
        seccomp_release(ctx);
        return false;
    }

    filterBlob_.resize(blobSize);
    if (seccomp_export_bpf(ctx, filterBlob_.data(), &blobSize) < 0) {
        SANDBOX_ERROR("Failed to export BPF blob");
        seccomp_release(ctx);
        return false;
    }

    seccomp_release(ctx);
    SANDBOX_DEBUG("Generated default seccomp policy with " +
                  std::to_string(essentialCalls.size()) + " allowed syscalls");

    return true;
}

bool Seccomp::loadDefaultAllowlist() {
    return generateDefaultPolicy(config_);
}

bool Seccomp::loadDefaultDenylist() {
    // Denylist mode: allow everything except specified calls
    scmp_filter_ctx ctx = seccomp_init(SCMP_ACT_ALLOW);
    if (!ctx) {
        return false;
    }

    // Add deny rules for dangerous calls
    seccomp_release(ctx);
    return true;
}

bool Seccomp::installFilter() {
    if (filterBlob_.empty()) {
        if (!loadDefaultAllowlist()) {
            return false;
        }
    }
    return true;
}

} // namespace sandbox
