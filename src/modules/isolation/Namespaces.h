/**
 * @file Namespaces.h
 * @brief Linux namespaces isolation module.
 *
 * This header defines the Namespaces class that implements process
 * and resource isolation using Linux namespaces.
 */

#ifndef SANDBOX_NAMESPACES_H
#define SANDBOX_NAMESPACES_H

#include "modules/interface/IModule.h"
#include "core/ConfigParser.h"

namespace sandbox {

/**
 * @class Namespaces
 * @brief Implements namespace-based isolation for the sandbox.
 *
 * The Namespaces class manages the creation and configuration of
 * Linux namespaces including PID, network, mount, UTS, IPC, and user
 * namespaces. It provides comprehensive isolation of processes and
 * system resources.
 */
class Namespaces : public IModule {
public:
    /**
     * @brief Construct a Namespaces module.
     */
    Namespaces();

    /**
     * @brief Destructor.
     */
    ~Namespaces() override = default;

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

private:
    /**
     * @brief Apply user namespace mappings.
     * @param config The sandbox configuration.
     * @return true if successful.
     */
    bool applyUserNamespace(const SandboxConfiguration& config);

    /**
     * @brief Check if a namespace type is requested.
     * @param nsName Name of the namespace.
     * @param config The configuration.
     * @return true if namespace is requested.
     */
    bool hasNamespace(const std::string& nsName, const SandboxConfiguration& config);

    /**
     * @brief Get the clone flag for a namespace type.
     * @param nsName Name of the namespace.
     * @return The clone flag, or 0 if not found.
     */
    int getNamespaceFlag(const std::string& nsName);

    ModuleState state_;
    SandboxConfiguration config_;
    bool userNsEnabled_;
};

} // namespace sandbox

#endif // SANDBOX_NAMESPACES_H
