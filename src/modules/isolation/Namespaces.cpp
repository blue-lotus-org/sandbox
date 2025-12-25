/**
 * @file Namespaces.cpp
 * @brief Implementation of the Namespaces class.
 */

#include "modules/isolation/Namespaces.h"
#include "utils/Syscalls.h"
#include "core/Logger.h"
#include <sched.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <cstring>

namespace sandbox {

Namespaces::Namespaces()
    : state_(ModuleState::UNINITIALIZED)
    , userNsEnabled_(false)
{
}

std::string Namespaces::getName() const {
    return "namespaces";
}

std::string Namespaces::getVersion() const {
    return "1.0.0";
}

ModuleState Namespaces::getState() const {
    return state_;
}

bool Namespaces::initialize(const SandboxConfiguration& config) {
    SANDBOX_INFO("Initializing Namespaces module");
    config_ = config;

    // Check which namespaces are requested
    userNsEnabled_ = hasNamespace("user", config);

    state_ = ModuleState::INITIALIZED;
    SANDBOX_INFO("Namespaces module initialized successfully");
    SANDBOX_DEBUG("User namespace enabled: " + std::string(userNsEnabled_ ? "yes" : "no"));

    return true;
}

bool Namespaces::prepareChild(const SandboxConfiguration& config, pid_t childPid) {
    SANDBOX_DEBUG("Preparing namespace configuration in parent process");
    // Parent-side namespace setup is minimal
    // Most namespace configuration happens in the child
    return true;
}

bool Namespaces::applyChild(const SandboxConfiguration& config) {
    SANDBOX_INFO("Applying namespace isolation");

    // Apply user namespace mappings if enabled
    if (userNsEnabled_) {
        if (!applyUserNamespace(config)) {
            SANDBOX_ERROR("Failed to apply user namespace mappings");
            return false;
        }
    }

    // Mount /proc if PID namespace is enabled
    if (hasNamespace("pid", config)) {
        SANDBOX_DEBUG("Mounting /proc for PID namespace");
        if (!Syscall::mount("proc", "/proc", "proc", MS_NOSUID | MS_NOEXEC | MS_NODEV, nullptr)) {
            SANDBOX_ERROR("Failed to mount /proc");
            return false;
        }
    }

    // Mount /sys if mount namespace is enabled
    if (hasNamespace("mount", config)) {
        SANDBOX_DEBUG("Mounting /sys for mount namespace");
        if (!Syscall::mount("sysfs", "/sys", "sysfs", MS_NOSUID | MS_NOEXEC | MS_NODEV, nullptr)) {
            SANDBOX_WARNING("Failed to mount /sys");
        }
    }

    // Set hostname if UTS namespace is enabled
    if (hasNamespace("uts", config)) {
        SANDBOX_DEBUG("Setting hostname for UTS namespace");
        std::string hostname = config.sandbox.hostname;
        if (!Syscall::setHostname(hostname)) {
            SANDBOX_WARNING("Failed to set hostname");
        }
    }

    state_ = ModuleState::RUNNING;
    return true;
}

int Namespaces::execute(const SandboxConfiguration& config) {
    // This module doesn't execute anything directly
    // The next module in the chain will execute
    return 0;
}

bool Namespaces::cleanup() {
    SANDBOX_DEBUG("Cleaning up Namespaces module");
    state_ = ModuleState::STOPPED;
    return true;
}

std::vector<std::string> Namespaces::getDependencies() const {
    return {};
}

bool Namespaces::isEnabled() const {
    return true; // Always enabled if registered
}

std::string Namespaces::getDescription() const {
    return "Implements Linux namespace isolation for process, network, mount, UTS, IPC, and user namespaces.";
}

std::string Namespaces::getType() const {
    return "isolation";
}

bool Namespaces::applyUserNamespace(const SandboxConfiguration& config) {
    // Write to /proc/self/setgroups first (required for user namespaces)
    if (!Syscall::writeProcSetgroups("deny")) {
        SANDBOX_WARNING("Failed to write /proc/self/setgroups");
    }

    // Write UID map
    std::string uidMap = std::to_string(config.isolation.uid_map.container_uid) + " " +
                         std::to_string(config.isolation.uid_map.host_uid) + " " +
                         std::to_string(config.isolation.uid_map.count);
    if (!Syscall::writeProcUidMap(uidMap)) {
        SANDBOX_ERROR("Failed to write UID map");
        return false;
    }
    SANDBOX_DEBUG("UID map: " + uidMap);

    // Write GID map
    std::string gidMap = std::to_string(config.isolation.gid_map.container_gid) + " " +
                         std::to_string(config.isolation.gid_map.host_gid) + " " +
                         std::to_string(config.isolation.gid_map.count);
    if (!Syscall::writeProcGidMap(gidMap)) {
        SANDBOX_ERROR("Failed to write GID map");
        return false;
    }
    SANDBOX_DEBUG("GID map: " + gidMap);

    return true;
}

bool Namespaces::hasNamespace(const std::string& nsName, const SandboxConfiguration& config) {
    const auto& namespaces = config.isolation.namespaces;
    return std::find(namespaces.begin(), namespaces.end(), nsName) != namespaces.end();
}

int Namespaces::getNamespaceFlag(const std::string& nsName) {
    if (nsName == "pid") return CLONE_NEWPID;
    if (nsName == "net") return CLONE_NEWNET;
    if (nsName == "ipc") return CLONE_NEWIPC;
    if (nsName == "uts") return CLONE_NEWUTS;
    if (nsName == "mount") return CLONE_NEWNS;
    if (nsName == "user") return CLONE_NEWUSER;
    return 0;
}

} // namespace sandbox
