/**
 * @file Mounts.h
 * @brief Bind mounts and volume management module.
 *
 * This header defines the Mounts class that handles bind mounts
 * and volume configuration for the sandbox.
 */

#ifndef SANDBOX_MOUNTS_H
#define SANDBOX_MOUNTS_H

#include "modules/interface/IModule.h"
#include "core/ConfigParser.h"
#include <vector>
#include <string>

namespace sandbox {

/**
 * @struct MountInfo
 * @brief Information about an active mount.
 */
struct MountInfo {
    std::string source;
    std::string target;
    std::string fstype;
    unsigned long flags;
    bool readOnly;
};

/**
 * @class Mounts
 * @brief Manages bind mounts and volumes for the sandbox.
 *
 * The Mounts class handles the configuration and application of
 * bind mounts and volumes from the sandbox configuration.
 */
class Mounts : public IModule {
public:
    /**
     * @brief Construct a Mounts module.
     */
    Mounts();

    /**
     * @brief Destructor.
     */
    ~Mounts() override = default;

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
     * @brief Get the list of active mounts.
     * @return Vector of active mounts.
     */
    std::vector<MountInfo> getActiveMounts() const;

private:
    /**
     * @brief Apply a single bind mount.
     * @param mount The mount configuration.
     * @return true if successful.
     */
    bool applyBindMount(const BindMount& mount);

    /**
     * @brief Ensure the mount target exists.
     * @param target The target path.
     * @return true if successful.
     */
    bool ensureMountTarget(const std::string& target);

    ModuleState state_;
    SandboxConfiguration config_;
    std::vector<MountInfo> activeMounts_;
};

} // namespace sandbox

#endif // SANDBOX_MOUNTS_H
