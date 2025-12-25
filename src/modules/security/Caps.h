/**
 * @file Caps.h
 * @brief Linux capabilities management module.
 *
 * This header defines the Caps class that implements fine-grained
 * privilege management using Linux capabilities.
 */

#ifndef SANDBOX_CAPS_H
#define SANDBOX_CAPS_H

#include "modules/interface/IModule.h"
#include "core/ConfigParser.h"
#include <vector>
#include <string>

namespace sandbox {

/**
 * @class Caps
 * @brief Manages Linux capabilities for the sandbox.
 *
 * The Caps class implements capability-based privilege dropping,
 * allowing fine-grained control over which privileges are granted
 * to processes running in the sandbox.
 */
class Caps : public IModule {
public:
    /**
     * @brief Construct a Caps module.
     */
    Caps();

    /**
     * @brief Destructor.
     */
    ~Caps() override = default;

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
     * @brief Drop all capabilities.
     * @return true if successful.
     */
    bool dropAllCapabilities();

    /**
     * @brief Add a capability to the ambient set.
     * @param cap The capability name.
     * @return true if successful.
     */
    bool addAmbientCapability(const std::string& cap);

    /**
     * @brief Check if a capability is present.
     * @param cap The capability name.
     * @return true if present.
     */
    bool hasCapability(const std::string& cap);

private:
    /**
     * @brief Convert capability name to number.
     * @param name The capability name.
     * @return The capability number, or -1 if not found.
     */
    int capabilityFromName(const std::string& name);

    /**
     * @brief Get the list of capabilities to keep.
     * @param config The sandbox configuration.
     * @return Vector of capability numbers to keep.
     */
    std::vector<int> getKeepCapabilities(const SandboxConfiguration& config);

    ModuleState state_;
    SandboxConfiguration config_;
    std::vector<std::string> grantedCapabilities_;
    bool ambientCapsEnabled_;
};

} // namespace sandbox

#endif // SANDBOX_CAPS_H
