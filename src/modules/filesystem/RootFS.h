/**
 * @file RootFS.h
 * @brief Root filesystem management module.
 *
 * This header defines the RootFS class that manages the root filesystem
 * for the sandbox, including debootstrap integration and pivot_root.
 */

#ifndef SANDBOX_ROOTFS_H
#define SANDBOX_ROOTFS_H

#include "modules/interface/IModule.h"
#include "core/ConfigParser.h"
#include <string>

namespace sandbox {

/**
 * @class RootFS
 * @brief Manages the root filesystem for the sandbox.
 *
 * The RootFS class handles the creation, setup, and mounting of
 * the root filesystem used by the sandbox. It supports debootstrap
 * for creating minimal Debian/Ubuntu environments and implements
 * pivot_root for proper filesystem isolation.
 */
class RootFS : public IModule {
public:
    /**
     * @brief Construct a RootFS module.
     */
    RootFS();

    /**
     * @brief Destructor.
     */
    ~RootFS() override = default;

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
     * @brief Get the root filesystem path.
     * @return The rootfs path.
     */
    std::string getRootPath() const;

    /**
     * @brief Check if rootfs exists.
     * @return true if exists.
     */
    bool exists() const;

    /**
     * @brief Bootstrap a new rootfs using debootstrap.
     * @param config The sandbox configuration.
     * @return true if successful.
     */
    bool bootstrap(const SandboxConfiguration& config);

private:
    /**
     * @brief Check if the rootfs needs to be created.
     * @return true if needs creation.
     */
    bool needsBootstrap(const SandboxConfiguration& config);

    /**
     * @brief Run debootstrap to create the rootfs.
     * @param config The sandbox configuration.
     * @return true if successful.
     */
    bool runDebootstrap(const SandboxConfiguration& config);

    /**
     * @brief Set up the rootfs mounts in the child process.
     * @param config The sandbox configuration.
     * @return true if successful.
     */
    bool setupMounts(const SandboxConfiguration& config);

    /**
     * @brief Perform pivot_root.
     * @param newRoot The new root path.
     * @param putOld The path for the old root.
     * @return true if successful.
     */
    bool doPivotRoot(const std::string& newRoot, const std::string& putOld);

    ModuleState state_;
    SandboxConfiguration config_;
    std::string rootPath_;
    std::string oldRootPath_;
    bool bootstrapRequired_;
};

} // namespace sandbox

#endif // SANDBOX_ROOTFS_H
