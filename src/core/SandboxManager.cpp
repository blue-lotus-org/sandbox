/**
 * @file SandboxManager.cpp
 * @brief Implementation of the SandboxManager class.
 */

#include "core/SandboxManager.h"
#include "core/Logger.h"
#include "modules/interface/IModule.h"
#include <chrono>
#include <thread>
#include <csignal>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <fcntl.h>

namespace sandbox {

SandboxManager::SandboxManager()
    : state_(SandboxState::CREATED)
    , childPid_(-1)
{
    pipeFd_[0] = -1;
    pipeFd_[1] = -1;
}

SandboxManager::~SandboxManager() {
    stop(1000);
    cleanupModules();
}

bool SandboxManager::loadConfig(const std::filesystem::path& configPath) {
    try {
        ConfigParser parser(configPath);
        config_ = parser.parse();
        return true;
    } catch (const std::exception& e) {
        SANDBOX_ERROR("Failed to load config: " + std::string(e.what()));
        return false;
    }
}

void SandboxManager::setConfig(const SandboxConfiguration& config) {
    config_ = config;
}

const SandboxConfiguration& SandboxManager::getConfig() const {
    return config_;
}

bool SandboxManager::registerModule(std::unique_ptr<IModule> module) {
    if (!module) {
        SANDBOX_ERROR("Cannot register null module");
        return false;
    }

    std::string name = module->getName();
    if (modules_.count(name)) {
        SANDBOX_WARNING("Module " + name + " already registered, replacing");
    }

    modules_[name] = std::move(module);
    SANDBOX_INFO("Registered module: " + name);
    return true;
}

bool SandboxManager::unregisterModule(const std::string& name) {
    auto it = modules_.find(name);
    if (it == modules_.end()) {
        return false;
    }

    modules_.erase(it);
    SANDBOX_INFO("Unregistered module: " + name);
    return true;
}

IModule* SandboxManager::getModule(const std::string& name) {
    auto it = modules_.find(name);
    return it != modules_.end() ? it->second.get() : nullptr;
}

const std::map<std::string, IModule*>& SandboxManager::getModules() const {
    static std::map<std::string, IModule*> result;
    result.clear();
    for (const auto& [name, module] : modules_) {
        result[name] = module.get();
    }
    return result;
}

void SandboxManager::initializeLogger() {
    LogLevel level = stringToLogLevel(config_.logging.level);
    Logger::getInstance().initialize(level, config_.logging.output, config_.logging.log_file);
}

SandboxResult SandboxManager::run() {
    auto startTime = std::chrono::steady_clock::now();

    SandboxResult result;
    result.exitCode = -1;
    result.success = false;
    result.childPid = -1;

    SANDBOX_INFO("Starting sandbox: " + config_.sandbox.name);
    setState(SandboxState::INITIALIZING);

    // Resolve module dependencies
    resolveDependencies();

    // Initialize modules
    if (!initializeModules()) {
        result.errorMessage = "Failed to initialize modules";
        setState(SandboxState::ERROR);
        return result;
    }

    // Create pipe for output capture
    if (pipe(pipeFd_) < 0) {
        result.errorMessage = "Failed to create pipe";
        SANDBOX_ERROR(result.errorMessage);
        setState(SandboxState::ERROR);
        return result;
    }

    // Fork child process
    SANDBOX_INFO("Forking child process");
    childPid_ = fork();

    if (childPid_ < 0) {
        result.errorMessage = "Failed to fork process";
        SANDBOX_ERROR(result.errorMessage);
        close(pipeFd_[0]);
        close(pipeFd_[1]);
        setState(SandboxState::ERROR);
        return result;
    }

    if (childPid_ == 0) {
        // Child process
        close(pipeFd_[0]);  // Close read end

        // Set process title
        prctl(PR_SET_NAME, config_.sandbox.name.c_str(), 0, 0, 0);

        int exitCode = executeChild();
        _exit(exitCode);
    }

    // Parent process
    close(pipeFd_[1]);  // Close write end
    result.childPid = childPid_;
    setState(SandboxState::RUNNING);
    SANDBOX_INFO("Child process started with PID: " + std::to_string(childPid_));

    // Prepare child process (move to cgroups, etc.)
    if (!prepareChildProcess()) {
        SANDBOX_ERROR("Failed to prepare child process");
        kill(childPid_, SIGKILL);
    }

    // Wait for child to exit
    int status = 0;
    pid_t waitedPid = waitpid(childPid_, &status, 0);

    // Read output from pipe
    char buffer[4096];
    ssize_t bytesRead;
    while ((bytesRead = read(pipeFd_[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0';
        result.stdout += buffer;
    }
    close(pipeFd_[0]);

    if (waitedPid == childPid_) {
        if (WIFEXITED(status)) {
            result.exitCode = WEXITSTATUS(status);
            result.success = (result.exitCode == 0);
        } else if (WIFSIGNALED(status)) {
            result.exitCode = -WTERMSIG(status);
            result.errorMessage = "Killed by signal: " + std::to_string(WTERMSIG(status));
            result.success = false;
        }
    }

    setState(SandboxState::STOPPING);
    cleanupModules();
    setState(SandboxState::STOPPED);

    auto endTime = std::chrono::steady_clock::now();
    result.executionTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime).count();

    SANDBOX_INFO("Sandbox execution completed in " + std::to_string(result.executionTimeMs) + "ms");
    SANDBOX_INFO("Exit code: " + std::to_string(result.exitCode));

    return result;
}

std::future<SandboxResult> SandboxManager::runAsync() {
    return std::async(std::launch::async, [this]() {
        return run();
    });
}

bool SandboxManager::stop(int timeoutMs) {
    if (childPid_ < 0) {
        return true;
    }

    SANDBOX_INFO("Stopping sandbox (timeout: " + std::to_string(timeoutMs) + "ms)");

    // Send SIGTERM first
    kill(childPid_, SIGTERM);

    // Wait for graceful shutdown
    int attempts = timeoutMs / 100;
    for (int i = 0; i < attempts; ++i) {
        int status = 0;
        pid_t result = waitpid(childPid_, &status, WNOHANG);
        if (result != 0) {
            // Process has exited
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Force kill if still running
    SANDBOX_WARNING("Graceful shutdown failed, sending SIGKILL");
    kill(childPid_, SIGKILL);
    waitpid(childPid_, nullptr, 0);

    return true;
}

SandboxState SandboxManager::getState() const {
    return state_;
}

pid_t SandboxManager::getChildPid() const {
    return childPid_;
}

bool SandboxManager::isRunning() const {
    return state_ == SandboxState::RUNNING && childPid_ > 0;
}

void SandboxManager::registerDefaultModules() {
    // Default modules will be registered by the main application
    SANDBOX_DEBUG("Default modules registration point");
}

void SandboxManager::resolveDependencies() {
    // Build dependency graph and resolve execution order
    executionOrder_.clear();

    // Simple dependency resolution (topological sort)
    std::set<std::string> visited;
    std::set<std::string> temp;

    std::function<void(const std::string&)> dfs = [&](const std::string& name) {
        if (temp.count(name)) {
            SANDBOX_WARNING("Circular dependency detected: " + name);
            return;
        }
        if (visited.count(name)) {
            return;
        }

        IModule* module = getModule(name);
        if (!module) {
            SANDBOX_WARNING("Module not found for dependency resolution: " + name);
            return;
        }

        temp.insert(name);

        for (const auto& dep : module->getDependencies()) {
            dfs(dep);
        }

        temp.erase(name);
        visited.insert(name);
        executionOrder_.push_back(module);
    };

    for (const auto& [name, module] : modules_) {
        if (!visited.count(name)) {
            dfs(name);
        }
    }

    SANDBOX_INFO("Resolved execution order with " + std::to_string(executionOrder_.size()) + " modules");
}

std::vector<IModule*> SandboxManager::getExecutionOrder() {
    return executionOrder_;
}

bool SandboxManager::initializeModules() {
    for (IModule* module : executionOrder_) {
        SANDBOX_INFO("Initializing module: " + module->getName());

        if (!module->initialize(config_)) {
            SANDBOX_ERROR("Failed to initialize module: " + module->getName());
            return false;
        }

        SANDBOX_DEBUG("Module " + module->getName() + " initialized successfully");
    }

    return true;
}

bool SandboxManager::prepareChildProcess() {
    for (IModule* module : executionOrder_) {
        if (!module->prepareChild(config_, childPid_)) {
            SANDBOX_ERROR("Failed to prepare module: " + module->getName());
            return false;
        }
    }
    return true;
}

int SandboxManager::executeChild() {
    try {
        // Apply child-side module configurations
        for (IModule* module : executionOrder_) {
            if (!module->applyChild(config_)) {
                SANDBOX_ERROR("Failed to apply child configuration for module: " + module->getName());
                return 1;
            }
        }

        // Execute modules
        return execute(config_);
    } catch (const std::exception& e) {
        SANDBOX_ERROR("Exception in child process: " + std::string(e.what()));
        return 1;
    }
}

bool SandboxManager::cleanupModules() {
    bool success = true;

    // Cleanup in reverse order
    for (auto it = executionOrder_.rbegin(); it != executionOrder_.rend(); ++it) {
        IModule* module = *it;
        SANDBOX_INFO("Cleaning up module: " + module->getName());

        if (!module->cleanup()) {
            SANDBOX_ERROR("Failed to cleanup module: " + module->getName());
            success = false;
        }
    }

    executionOrder_.clear();
    childPid_ = -1;

    return success;
}

void SandboxManager::setState(SandboxState state) {
    state_ = state;
    SANDBOX_DEBUG("Sandbox state changed to: " + std::to_string(static_cast<int>(state)));
}

} // namespace sandbox
