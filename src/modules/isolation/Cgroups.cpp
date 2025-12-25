/**
 * @file Cgroups.cpp
 * @brief Implementation of the Cgroups class.
 */

#include "modules/isolation/Cgroups.h"
#include "utils/Syscalls.h"
#include "core/Logger.h"
#include <sstream>
#include <cstdlib>

namespace sandbox {

Cgroups::Cgroups(const std::string& cgroupPath)
    : state_(ModuleState::UNINITIALIZED)
    , cgroupPath_(cgroupPath)
{
}

std::string Cgroups::getName() const {
    return "cgroups";
}

std::string Cgroups::getVersion() const {
    return "1.0.0";
}

ModuleState Cgroups::getState() const {
    return state_;
}

bool Cgroups::initialize(const SandboxConfiguration& config) {
    SANDBOX_INFO("Initializing Cgroups module");
    config_ = config;

    // Generate cgroup name
    cgroupName_ = "sandbox-" + config.sandbox.name + "-" +
                  std::to_string(getpid());

    // Full path to the cgroup
    cgroupFullPath_ = cgroupPath_ + "/" + cgroupName_;

    SANDBOX_DEBUG("Cgroup path: " + cgroupFullPath_);

    // Create the cgroup in parent process
    if (!createCgroup(config)) {
        SANDBOX_ERROR("Failed to create cgroup");
        return false;
    }

    state_ = ModuleState::INITIALIZED;
    SANDBOX_INFO("Cgroups module initialized successfully");

    return true;
}

bool Cgroups::prepareChild(const SandboxConfiguration& config, pid_t childPid) {
    SANDBOX_DEBUG("Adding child process " + std::to_string(childPid) + " to cgroup");

    // Move the child process to our cgroup
    if (!Syscall::addToCgroup(cgroupPath_, cgroupName_, childPid)) {
        SANDBOX_ERROR("Failed to add child to cgroup");
        return false;
    }

    return true;
}

bool Cgroups::applyChild(const SandboxConfiguration& config) {
    // Cgroup configuration happens in the parent
    return true;
}

int Cgroups::execute(const SandboxConfiguration& config) {
    // This module doesn't execute anything directly
    return 0;
}

bool Cgroups::cleanup() {
    SANDBOX_DEBUG("Cleaning up Cgroups module");

    // Remove the cgroup
    if (!cgroupFullPath_.empty()) {
        Syscall::removeCgroup(cgroupPath_, cgroupName_);
    }

    state_ = ModuleState::STOPPED;
    return true;
}

std::vector<std::string> Cgroups::getDependencies() const {
    return {};
}

bool Cgroups::isEnabled() const {
    return true;
}

std::string Cgroups::getDescription() const {
    return "Implements cgroup v2 resource limits for CPU, memory, and PID counts.";
}

std::string Cgroups::getType() const {
    return "isolation";
}

std::string Cgroups::getCgroupPath() const {
    return cgroupPath_;
}

std::string Cgroups::getCgroupName() const {
    return cgroupName_;
}

bool Cgroups::createCgroup(const SandboxConfiguration& config) {
    SANDBOX_INFO("Creating cgroup: " + cgroupFullPath_);

    // Create the cgroup directory
    if (!Syscall::mkdirRecursive(cgroupFullPath_)) {
        SANDBOX_ERROR("Failed to create cgroup directory");
        return false;
    }

    // Set memory limits
    if (!setMemoryLimits(config)) {
        SANDBOX_ERROR("Failed to set memory limits");
        return false;
    }

    // Set CPU limits
    if (!setCpuLimits(config)) {
        SANDBOX_ERROR("Failed to set CPU limits");
        return false;
    }

    // Set PID limits
    if (!setPidLimits(config)) {
        SANDBOX_ERROR("Failed to set PID limits");
        return false;
    }

    return true;
}

bool Cgroups::setMemoryLimits(const SandboxConfiguration& config) {
    long long memoryBytes = static_cast<long long>(config.resources.memory_mb) * 1024 * 1024;

    // Set memory limit
    if (!Syscall::setCgroupValue(cgroupPath_, cgroupName_, "memory.max",
                                 std::to_string(memoryBytes))) {
        SANDBOX_ERROR("Failed to set memory.max");
        return false;
    }

    SANDBOX_DEBUG("Memory limit set to " + std::to_string(config.resources.memory_mb) + " MB");

    // Set swap limit if enabled
    if (config.resources.enable_swap) {
        if (!Syscall::setCgroupValue(cgroupPath_, cgroupName_, "memory.swap.max", "0")) {
            SANDBOX_WARNING("Failed to set memory.swap.max");
        }
    }

    // Set memory high watermark (triggers memory pressure)
    if (!Syscall::setCgroupValue(cgroupPath_, cgroupName_, "memory.high",
                                 std::to_string(memoryBytes * 8 / 10))) {
        SANDBOX_WARNING("Failed to set memory.high");
    }

    return true;
}

bool Cgroups::setCpuLimits(const SandboxConfiguration& config) {
    // CPU quota is specified as a percentage (e.g., 50 = 50% of one CPU)
    // cgroups v2 uses cpu.max with format: "quota period"
    // Default period is 100000 microseconds (100ms)

    long long quota = config.resources.cpu_quota_percent * 1000;  // Convert % to microseconds

    if (!Syscall::setCgroupValue(cgroupPath_, cgroupName_, "cpu.max",
                                 std::to_string(quota) + " 100000")) {
        SANDBOX_ERROR("Failed to set cpu.max");
        return false;
    }

    SANDBOX_DEBUG("CPU quota set to " + std::to_string(config.resources.cpu_quota_percent) + "%");

    return true;
}

bool Cgroups::setPidLimits(const SandboxConfiguration& config) {
    if (config.resources.max_pids > 0) {
        if (!Syscall::setCgroupValue(cgroupPath_, cgroupName_, "pids.max",
                                     std::to_string(config.resources.max_pids))) {
            SANDBOX_ERROR("Failed to set pids.max");
            return false;
        }

        SANDBOX_DEBUG("Max PIDs set to " + std::to_string(config.resources.max_pids));
    }

    return true;
}

} // namespace sandbox
