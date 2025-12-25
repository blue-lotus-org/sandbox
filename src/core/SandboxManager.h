/**
 * @file SandboxManager.h
 * @brief Main orchestrator for the sandbox platform.
 *
 * This header defines the SandboxManager class that coordinates
 * all sandbox modules and manages the lifecycle of sandbox instances.
 */

#ifndef SANDBOX_SANDBOX_MANAGER_H
#define SANDBOX_SANDBOX_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <future>
#include <memory>
#include <sys/types.h>
#include "ConfigParser.h"
#include "modules/interface/IModule.h"

namespace sandbox {

/**
 * @enum SandboxState
 * @brief Represents the current state of the sandbox.
 */
enum class SandboxState {
    CREATED,        ///< Sandbox instance created
    INITIALIZING,   ///< Initializing modules
    PREPARING,      ///< Preparing child process
    RUNNING,        ///< Child process is running
    STOPPING,       ///< Stopping the sandbox
    STOPPED,        ///< Sandbox has stopped
    ERROR           ///< Sandbox encountered an error
};

/**
 * @struct SandboxResult
 * @brief Contains the result of a sandbox execution.
 */
struct SandboxResult {
    int exitCode;                  ///< Exit code of the sandbox process
    bool success;                  ///< Whether the sandbox ran successfully
    std::string errorMessage;      ///< Error message if failed
    long executionTimeMs;          ///< Execution time in milliseconds
    std::string stdout;            ///< Captured stdout
    std::string stderr;            ///< Captured stderr
    pid_t childPid;                ///< PID of the child process
};

/**
 * @class SandboxManager
 * @brief Orchestrates the lifecycle of a sandbox instance.
 *
 * The SandboxManager is the core component of the sandbox platform.
 * It manages module registration, initialization order, process forking,
 * and cleanup. It follows the facade pattern to provide a simple
 * interface to the complex underlying system.
 */
class SandboxManager {
public:
    /**
     * @brief Construct a SandboxManager.
     */
    SandboxManager();

    /**
     * @brief Destructor.
     */
    ~SandboxManager();

    /**
     * @brief Load a configuration file.
     * @param configPath Path to the configuration file.
     * @return true if loaded successfully, false otherwise.
     */
    bool loadConfig(const std::filesystem::path& configPath);

    /**
     * @brief Set configuration directly.
     * @param config The sandbox configuration.
     */
    void setConfig(const SandboxConfiguration& config);

    /**
     * @brief Get the current configuration.
     * @return Reference to the current configuration.
     */
    const SandboxConfiguration& getConfig() const;

    /**
     * @brief Register a module with the manager.
     * @param module Pointer to the module to register.
     * @return true if registered successfully.
     */
    bool registerModule(std::unique_ptr<IModule> module);

    /**
     * @brief Unregister a module by name.
     * @param name The name of the module to unregister.
     * @return true if unregistered successfully.
     */
    bool unregisterModule(const std::string& name);

    /**
     * @brief Get a module by name.
     * @param name The name of the module.
     * @return Pointer to the module, or nullptr if not found.
     */
    IModule* getModule(const std::string& name);

    /**
     * @brief Get all registered modules.
     * @return Map of module names to module pointers.
     */
    const std::map<std::string, IModule*>& getModules() const;

    /**
     * @brief Run the sandbox with the current configuration.
     * @return SandboxResult containing the execution result.
     */
    SandboxResult run();

    /**
     * @brief Run the sandbox asynchronously.
     * @return Future containing the SandboxResult.
     */
    std::future<SandboxResult> runAsync();

    /**
     * @brief Stop the running sandbox.
     * @param timeoutMs Maximum time to wait for graceful shutdown.
     * @return true if stopped successfully.
     */
    bool stop(int timeoutMs = 5000);

    /**
     * @brief Get the current state of the sandbox.
     * @return The current SandboxState.
     */
    SandboxState getState() const;

    /**
     * @brief Get the PID of the child process.
     * @return The child PID, or -1 if not running.
     */
    pid_t getChildPid() const;

    /**
     * @brief Check if the sandbox is running.
     * @return true if the sandbox is running.
     */
    bool isRunning() const;

    /**
     * @brief Initialize the logger with configuration settings.
     */
    void initializeLogger();

    /**
     * @brief Register default modules.
     *
     * Registers all built-in modules that are part of the
     * sandbox platform.
     */
    void registerDefaultModules();

private:
    bool initializeModules();
    bool prepareChildProcess();
    int executeChild();
    bool cleanupModules();
    void resolveDependencies();
    std::vector<IModule*> getExecutionOrder();
    void setState(SandboxState state);

    SandboxConfiguration config_;
    SandboxState state_;
    std::map<std::string, std::unique_ptr<IModule>> modules_;
    std::vector<IModule*> executionOrder_;
    pid_t childPid_;
    int pipeFd_[2];  ///< Pipe for capturing output
};

} // namespace sandbox

#endif // SANDBOX_SANDBOX_MANAGER_H
