/**
 * @file Cgroups.h
 * @brief Cgroups resource limiting module.
 *
 * This header defines the Cgroups class that implements resource
 * limits using Linux cgroups v2.
 */

#ifndef SANDBOX_CGROUPS_H
#define SANDBOX_CGROUPS_H

#include "modules/interface/IModule.h"
#include "core/ConfigParser.h"

namespace sandbox {

/**
 * @class Cgroups
 * @brief Implements cgroup-based resource limiting.
 *
 * The Cgroups class manages the creation and configuration of
 * cgroups for controlling CPU, memory, PID, and other resource
 * limits for processes running in the sandbox.
 */
class Cgroups : public IModule {
public:
    /**
     * @brief Construct a Cgroups module.
     * @param cgroupPath Path to the cgroup hierarchy (default: /sys/fs/cgroup).
     */
    explicit Cgroups(const std::string& cgroupPath = "/sys/fs/cgroup");

    /**
     * @brief Destructor.
     */
    ~Cgroups() override = default;

    // IModule interface
    std::string getName() const override;
    std::string getVersion() const override;
    ModuleState getState() const override;
    bool initialize(const SandboxConfiguration& config) override;
    bool prepareChild(const SandboxConfiguration& config, pid_t childPid) override;
    bool applyChild(const SandboxConfiguration& config) override;
    int execute(const SandboxConfiguration& config) override;
    bool cleanup() override;
    std::vector<std::string> getDependencies() const override;
    bool isEnabled() const override;
    std::string getDescription() const override;
    std::string getType() const override;

    /**
     * @brief Get the cgroup path for this sandbox.
     * @return The cgroup path.
     */
    std::string getCgroupPath() const;

    /**
     * @brief Get the cgroup name.
     * @return The cgroup name.
     */
    std::string getCgroupName() const;

private:
    /**
     * @brief Create the cgroup.
     * @param config The sandbox configuration.
     * @return true if successful.
     */
    bool createCgroup(const SandboxConfiguration& config);

    /**
     * @brief Set memory limits.
     * @param config The sandbox configuration.
     * @return true if successful.
     */
    bool setMemoryLimits(const SandboxConfiguration& config);

    /**
     * @brief Set CPU limits.
     * @param config The sandbox configuration.
     * @return true if successful.
     */
    bool setCpuLimits(const SandboxConfiguration& config);

    /**
     * @brief Set PID limits.
     * @param config The sandbox configuration.
     * @return true if successful.
     */
    bool setPidLimits(const SandboxConfiguration& config);

    ModuleState state_;
    SandboxConfiguration config_;
    std::string cgroupPath_;
    std::string cgroupName_;
    std::string cgroupFullPath_;
};

} // namespace sandbox

#endif // SANDBOX_CGROUPS_H
