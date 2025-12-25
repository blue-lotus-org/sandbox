/**
 * @file IModule.h
 * @brief Base interface for all sandbox modules.
 *
 * This header defines the IModule interface that all sandbox modules
 * must implement. It provides a standardized lifecycle interface for
 * module initialization, execution, and cleanup.
 */

#ifndef SANDBOX_IMODULE_H
#define SANDBOX_IMODULE_H

#include <string>
#include <vector>
#include "../core/ConfigParser.h"

namespace sandbox {

/**
 * @enum ModuleState
 * @brief Represents the current state of a module.
 */
enum class ModuleState {
    UNINITIALIZED,   ///< Module has not been initialized
    INITIALIZED,     ///< Module is initialized
    RUNNING,         ///< Module is actively running
    STOPPING,        ///< Module is in the process of stopping
    STOPPED,         ///< Module has stopped
    ERROR            ///< Module encountered an error
};

/**
 * @class IModule
 * @brief Abstract base class for all sandbox modules.
 *
 * The IModule interface defines the standard lifecycle methods that
 * all modules must implement. This ensures consistent behavior across
 * different module types and enables the SandboxManager to manage
 * modules uniformly.
 *
 * Modules follow a three-phase lifecycle:
 * 1. Initialization (pre-start) - Runs in parent process before cloning
 * 2. Application (child) - Runs in child process after namespace isolation
 * 3. Cleanup - Runs in parent process after child exits
 */
class IModule {
public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~IModule() = default;

    /**
     * @brief Get the name of this module.
     * @return The module's name as a string.
     */
    virtual std::string getName() const = 0;

    /**
     * @brief Get the version of this module.
     * @return The module's version string.
     */
    virtual std::string getVersion() const = 0;

    /**
     * @brief Get the current state of this module.
     * @return The current ModuleState.
     */
    virtual ModuleState getState() const = 0;

    /**
     * @brief Initialize the module (parent context).
     *
     * This method is called in the parent process before the child
     * process is forked. Use this for setting up resources that
     * need to persist across the fork boundary.
     *
     * @param config Reference to the sandbox configuration.
     * @return true if initialization succeeded, false otherwise.
     */
    virtual bool initialize(const SandboxConfiguration& config) = 0;

    /**
     * @brief Prepare the module for child process (parent context).
     *
     * Called after fork but before exec. Use this to perform any
     * final setup that needs to happen in the parent process.
     *
     * @param config Reference to the sandbox configuration.
     * @param childPid PID of the child process.
     * @return true if preparation succeeded, false otherwise.
     */
    virtual bool prepareChild(const SandboxConfiguration& config, pid_t childPid) = 0;

    /**
     * @brief Apply module configuration in the child process.
     *
     * This method is called in the child process after namespace
     * isolation but before executing the target command. Use this
     * for operations that must happen inside the isolated environment.
     *
     * @param config Reference to the sandbox configuration.
     * @return true if application succeeded, false otherwise.
     */
    virtual bool applyChild(const SandboxConfiguration& config) = 0;

    /**
     * @brief Execute the module's main functionality.
     *
     * Called after applyChild, this method runs the module's main
     * functionality in the child process. For most modules, this
     * will be the execution of the target command.
     *
     * @param config Reference to the sandbox configuration.
     * @return Exit code or status from the module.
     */
    virtual int execute(const SandboxConfiguration& config) = 0;

    /**
     * @brief Clean up the module (parent context).
     *
     * Called after the child process exits. Use this to release
     * resources and perform final cleanup.
     *
     * @return true if cleanup succeeded, false otherwise.
     */
    virtual bool cleanup() = 0;

    /**
     * @brief Get dependencies of this module.
     *
     * Returns a list of module names that must be initialized
     * and executed before this module.
     *
     * @return Vector of dependency names.
     */
    virtual std::vector<std::string> getDependencies() const = 0;

    /**
     * @brief Check if this module is enabled.
     * @return true if enabled, false otherwise.
     */
    virtual bool isEnabled() const = 0;

    /**
     * @brief Get a description of this module.
     * @return Module description string.
     */
    virtual std::string getDescription() const = 0;

    /**
     * @brief Get the type identifier for this module.
     * @return The module type string.
     */
    virtual std::string getType() const = 0;
};

/**
 * @class IModuleFactory
 * @brief Factory interface for creating module instances.
 *
 * The IModuleFactory interface allows for dynamic module creation
 * and registration, supporting plugin-based architecture.
 */
class IModuleFactory {
public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~IModuleFactory() = default;

    /**
     * @brief Create a new module instance.
     * @return Pointer to the new module instance.
     */
    virtual IModule* create() const = 0;

    /**
     * @brief Get the type name this factory creates.
     * @return The type name.
     */
    virtual std::string getTypeName() const = 0;
};

/**
 * @brief Helper function to check if module state indicates running.
 * @param state The module state to check.
 * @return true if the module is in a running state.
 */
inline bool isRunningState(ModuleState state) {
    return state == ModuleState::RUNNING ||
           state == ModuleState::INITIALIZED;
}

} // namespace sandbox

#endif // SANDBOX_IMODULE_H
