/**
 * @file Mounts.cpp
 * @brief Implementation of the Mounts class.
 */

#include "modules/filesystem/Mounts.h"
#include "utils/Syscalls.h"
#include "core/Logger.h"
#include <sys/mount.h>
#include <unistd.h>

namespace sandbox {

Mounts::Mounts()
    : state_(ModuleState::UNINITIALIZED)
{
}

std::string Mounts::getName() const {
    return "mounts";
}

std::string Mounts::getVersion() const {
    return "1.0.0";
}

ModuleState Mounts::getState() const {
    return state_;
}

bool Mounts::initialize(const SandboxConfiguration& config) {
    SANDBOX_INFO("Initializing Mounts module");
    config_ = config;

    SANDBOX_DEBUG("Configured bind mounts: " + std::to_string(config.mounts.bind_mounts.size()));
    for (const auto& mount : config.mounts.bind_mounts) {
        SANDBOX_DEBUG("  - " + mount.source + " -> " + mount.target +
                     (mount.read_only ? " (ro)" : " (rw)"));
    }

    state_ = ModuleState::INITIALIZED;
    SANDBOX_INFO("Mounts module initialized successfully");

    return true;
}

bool Mounts::prepareChild(const SandboxConfiguration& config, pid_t childPid) {
    // Mounts are applied in the child process
    return true;
}

bool Mounts::applyChild(const SandboxConfiguration& config) {
    SANDBOX_INFO("Applying bind mounts");

    for (const auto& mount : config.mounts.bind_mounts) {
        if (!applyBindMount(mount)) {
            SANDBOX_ERROR("Failed to apply bind mount: " + mount.source + " -> " + mount.target);
            return false;
        }

        // Track the mount
        MountInfo info;
        info.source = mount.source;
        info.target = mount.target;
        info.fstype = "bind";
        info.flags = MS_BIND;
        info.readOnly = mount.read_only;
        activeMounts_.push_back(info);
    }

    state_ = ModuleState::RUNNING;
    SANDBOX_INFO("Bind mounts applied successfully");

    return true;
}

int Mounts::execute(const SandboxConfiguration& config) {
    return 0;
}

bool Mounts::cleanup() {
    SANDBOX_DEBUG("Cleaning up Mounts module");

    // Unmount in reverse order
    for (auto it = activeMounts_.rbegin(); it != activeMounts_.rend(); ++it) {
        SANDBOX_DEBUG("Unmounting: " + it->target);
        Syscall::unmount(it->target);
    }

    activeMounts_.clear();
    state_ = ModuleState::STOPPED;
    return true;
}

std::vector<std::string> Mounts::getDependencies() const {
    return {"rootfs"};
}

bool Mounts::isEnabled() const {
    return !config_.mounts.bind_mounts.empty();
}

std::string Mounts::getDescription() const {
    return "Manages bind mounts and volumes for the sandbox filesystem.";
}

std::string Mounts::getType() const {
    return "filesystem";
}

std::vector<MountInfo> Mounts::getActiveMounts() const {
    return activeMounts_;
}

bool Mounts::applyBindMount(const BindMount& mount) {
    SANDBOX_DEBUG("Applying bind mount: " + mount.source + " -> " + mount.target);

    // Ensure the source exists
    if (!Syscall::exists(mount.source)) {
        SANDBOX_WARNING("Bind mount source does not exist, creating: " + mount.source);
        Syscall::mkdirRecursive(mount.source);
    }

    // Ensure the target directory exists
    if (!ensureMountTarget(mount.target)) {
        SANDBOX_ERROR("Failed to create mount target: " + mount.target);
        return false;
    }

    // First, do a bind mount
    if (!Syscall::mount(mount.source, mount.target, "bind", MS_BIND, nullptr)) {
        SANDBOX_ERROR("Failed to bind mount: " + mount.source);
        return false;
    }

    // If read-only, remount with read-only flag
    if (mount.read_only) {
        if (!Syscall::mount("", mount.target, "bind", MS_BIND | MS_REMOUNT | MS_RDONLY, nullptr)) {
            SANDBOX_WARNING("Failed to remount as read-only: " + mount.target);
        }
    }

    return true;
}

bool Mounts::ensureMountTarget(const std::string& target) {
    // Target path is relative to new root after pivot_root
    if (target.empty() || target == "/") {
        return true; // Root is always available
    }

    return Syscall::mkdirRecursive(target);
}

} // namespace sandbox
