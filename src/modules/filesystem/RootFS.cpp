/**
 * @file RootFS.cpp
 * @brief Implementation of the RootFS class.
 */

#include "modules/filesystem/RootFS.h"
#include "utils/Syscalls.h"
#include "core/Logger.h"
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>

namespace sandbox {

RootFS::RootFS()
    : state_(ModuleState::UNINITIALIZED)
    , bootstrapRequired_(false)
{
}

std::string RootFS::getName() const {
    return "rootfs";
}

std::string RootFS::getVersion() const {
    return "1.0.0";
}

ModuleState RootFS::getState() const {
    return state_;
}

bool RootFS::initialize(const SandboxConfiguration& config) {
    SANDBOX_INFO("Initializing RootFS module");
    config_ = config;

    rootPath_ = config.sandbox.rootfs_path;
    oldRootPath_ = "/oldroot";

    SANDBOX_DEBUG("Rootfs path: " + rootPath_);

    // Check if we need to bootstrap
    bootstrapRequired_ = config.sandbox.auto_bootstrap && !exists();

    if (bootstrapRequired_) {
        SANDBOX_INFO("Rootfs does not exist, bootstrap required");
        if (!bootstrap(config)) {
            SANDBOX_ERROR("Failed to bootstrap rootfs");
            return false;
        }
    }

    // Verify rootfs exists
    if (!exists()) {
        SANDBOX_ERROR("Rootfs does not exist: " + rootPath_);
        return false;
    }

    state_ = ModuleState::INITIALIZED;
    SANDBOX_INFO("RootFS module initialized successfully");

    return true;
}

bool RootFS::prepareChild(const SandboxConfiguration& config, pid_t childPid) {
    // Prepare rootfs in parent process (mounts are done in child)
    SANDBOX_DEBUG("Preparing rootfs for child process");
    return true;
}

bool RootFS::applyChild(const SandboxConfiguration& config) {
    SANDBOX_INFO("Setting up root filesystem");

    // Set up mounts before pivot_root
    if (!setupMounts(config)) {
        SANDBOX_ERROR("Failed to setup mounts");
        return false;
    }

    // Create old root directory
    std::string oldRootDir = rootPath_ + oldRootPath_;
    if (!Syscall::mkdirRecursive(oldRootDir)) {
        SANDBOX_ERROR("Failed to create old root directory");
        return false;
    }

    // Perform pivot_root
    if (!doPivotRoot(rootPath_, oldRootDir)) {
        SANDBOX_ERROR("Failed to pivot_root");
        return false;
    }

    // Now we're in the new root, unmount the old root
    if (!Syscall::unmount(oldRootPath_, MNT_DETACH)) {
        SANDBOX_WARNING("Failed to unmount old root");
    }

    // Mount essential filesystems
    if (!Syscall::mount("proc", "/proc", "proc", MS_NOSUID | MS_NOEXEC | MS_NODEV, nullptr)) {
        SANDBOX_ERROR("Failed to mount /proc");
        return false;
    }

    if (!Syscall::mount("sysfs", "/sys", "sysfs", MS_NOSUID | MS_NOEXEC | MS_NODEV, nullptr)) {
        SANDBOX_WARNING("Failed to mount /sys");
    }

    if (!Syscall::mount("tmpfs", "/dev", "tmpfs",
                        MS_NOSUID | MS_STRICTATIME, "mode=755")) {
        SANDBOX_WARNING("Failed to mount /dev");
    }

    state_ = ModuleState::RUNNING;
    return true;
}

int RootFS::execute(const SandboxConfiguration& config) {
    return 0;
}

bool RootFS::cleanup() {
    SANDBOX_DEBUG("Cleaning up RootFS module");
    state_ = ModuleState::STOPPED;
    return true;
}

std::vector<std::string> RootFS::getDependencies() const {
    return {};
}

bool RootFS::isEnabled() const {
    return true;
}

std::string RootFS::getDescription() const {
    return "Manages the root filesystem for the sandbox using pivot_root and debootstrap.";
}

std::string RootFS::getType() const {
    return "filesystem";
}

std::string RootFS::getRootPath() const {
    return rootPath_;
}

bool RootFS::exists() const {
    return Syscall::exists(rootPath_);
}

bool RootFS::bootstrap(const SandboxConfiguration& config) {
    SANDBOX_INFO("Bootstrapping rootfs: " + config.sandbox.distro + " " + config.sandbox.release);

    // Fork to run debootstrap
    pid_t pid = fork();
    if (pid < 0) {
        SANDBOX_ERROR("Failed to fork for debootstrap");
        return false;
    }

    if (pid == 0) {
        // Child process
        const char* distro = config.sandbox.distro.c_str();
        const char* release = config.sandbox.release.c_str();
        const char* path = config.sandbox.rootfs_path.c_str();
        const char* mirror = "http://archive.ubuntu.com/ubuntu/";

        // Execute debootstrap
        execlp("debootstrap", "debootstrap",
               "--arch=amd64",
               "--variant=minbase",
               release,
               path,
               mirror,
               (char*)nullptr);

        // If we get here, debootstrap failed
        _exit(1);
    }

    // Parent process
    int status = 0;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        SANDBOX_INFO("Bootstrap completed successfully");
        return true;
    }

    SANDBOX_ERROR("Bootstrap failed with status: " + std::to_string(WEXITSTATUS(status)));
    return false;
}

bool RootFS::needsBootstrap(const SandboxConfiguration& config) {
    if (!config.sandbox.auto_bootstrap) {
        return false;
    }
    return !exists();
}

bool RootFS::setupMounts(const SandboxConfiguration& config) {
    // Ensure the rootfs has necessary directories
    std::vector<std::string> requiredDirs = {
        "/bin", "/etc", "/home", "/lib", "/lib64", "/media",
        "/mnt", "/opt", "/root", "/sbin", "/srv", "/tmp",
        "/usr", "/var"
    };

    for (const auto& dir : requiredDirs) {
        std::string fullPath = rootPath_ + dir;
        if (!Syscall::isDirectory(fullPath)) {
            Syscall::mkdirRecursive(fullPath);
        }
    }

    return true;
}

bool RootFS::doPivotRoot(const std::string& newRoot, const std::string& putOld) {
    // First, make newRoot a mount point
    if (!Syscall::mount(newRoot, newRoot, "", MS_BIND | MS_REC, nullptr)) {
        SANDBOX_ERROR("Failed to bind mount newRoot to itself");
        return false;
    }

    // Perform pivot_root
    if (!Syscall::pivotRoot(newRoot, putOld)) {
        SANDBOX_ERROR("pivot_root failed");
        return false;
    }

    // Change to new root directory
    if (chdir("/") < 0) {
        SANDBOX_ERROR("Failed to chdir to new root");
        return false;
    }

    return true;
}

} // namespace sandbox
