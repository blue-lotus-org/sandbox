# API Reference
This is the developer guide :)

## Configuration

### SandboxConfiguration

Complete configuration structure for the sandbox platform.

```cpp
struct SandboxConfiguration {
    SandboxConfig sandbox;
    ResourcesConfig resources;
    IsolationConfig isolation;
    SecurityConfig security;
    MountsConfig mounts;
    AIModuleConfig ai_module;
    LoggingConfig logging;
};
```

### SandboxConfig

Core sandbox settings.

```cpp
struct SandboxConfig {
    std::string name;           // Sandbox instance name
    std::string hostname;       // Container hostname
    std::string rootfs_path;    // Path to root filesystem
    std::vector<std::string> command;  // Command to execute
    bool auto_bootstrap;        // Auto-create rootfs with debootstrap
    std::string distro;         // Distribution (ubuntu, debian)
    std::string release;        // Release version (focal, jammy, etc.)
};
```

### ResourcesConfig

Resource limits configuration.

```cpp
struct ResourcesConfig {
    int memory_mb;           // Memory limit in MB
    int cpu_quota_percent;   // CPU quota as percentage (50 = 50%)
    int max_pids;            // Maximum number of PIDs
    bool enable_swap;        // Enable swap limits
};
```

### IsolationConfig

Namespace and isolation configuration.

```cpp
struct IsolationConfig {
    std::vector<std::string> namespaces;  // Namespace types to enable
    UIDMap uid_map;        // UID mapping
    GIDMap gid_map;        // GID mapping
};

struct UIDMap {
    int host_uid;
    int container_uid;
    int count;
};
```

Available namespaces: `pid`, `net`, `ipc`, `uts`, `mount`, `user`

### SecurityConfig

Security settings.

```cpp
struct SecurityConfig {
    std::vector<std::string> capabilities;  // Capabilities to keep
    std::string seccomp_policy;             // Policy type
    std::string seccomp_profile_path;       // Custom policy path
};
```

Seccomp policies: `default`, `strict`, `log`, `allow`

### MountsConfig

Bind mount configuration.

```cpp
struct MountsConfig {
    std::vector<BindMount> bind_mounts;
    std::vector<std::string> volumes;
};

struct BindMount {
    std::string source;
    std::string target;
    bool read_only;
};
```

### AIModuleConfig

AI module configuration.

```cpp
struct AIModuleConfig {
    bool enabled;
    std::string provider;
    std::string api_key_env;
    std::string base_url;
    std::string model;
    double temperature;
    int max_tokens;
    std::string system_prompt;
    bool auto_report_errors;
};
```

## Core Classes

### SandboxManager

The main orchestrator class for the sandbox platform.

```cpp
class SandboxManager {
public:
    bool loadConfig(const std::filesystem::path& configPath);
    void setConfig(const SandboxConfiguration& config);
    const SandboxConfiguration& getConfig() const;

    bool registerModule(std::unique_ptr<IModule> module);
    bool unregisterModule(const std::string& name);
    IModule* getModule(const std::string& name);

    SandboxResult run();
    std::future<SandboxResult> runAsync();
    bool stop(int timeoutMs = 5000);

    SandboxState getState() const;
    bool isRunning() const;
};
```

### ConfigParser

Configuration file parser.

```cpp
class ConfigParser {
public:
    explicit ConfigParser(const std::filesystem::path& configPath);
    explicit ConfigParser(const std::string& jsonContent);

    SandboxConfiguration parse();
    const json& getJson() const;

    static bool isValidConfigFile(const std::filesystem::path& path);
    static SandboxConfiguration createDefaultConfig();
};
```

### Logger

Thread-safe logging facility.

```cpp
class Logger {
public:
    static Logger& getInstance();

    void initialize(LogLevel level, const std::string& output,
                    const std::string& logFile = "");

    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void critical(const std::string& message);

    void setLevel(LogLevel level);
    void flush();
    void shutdown();
};
```

## Module Interface

### IModule

Base interface for all sandbox modules.

```cpp
class IModule {
public:
    virtual ~IModule() = default;

    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual ModuleState getState() const = 0;

    virtual bool initialize(const SandboxConfiguration& config) = 0;
    virtual bool prepareChild(const SandboxConfiguration& config, pid_t childPid) = 0;
    virtual bool applyChild(const SandboxConfiguration& config) = 0;
    virtual int execute(const SandboxConfiguration& config) = 0;
    virtual bool cleanup() = 0;

    virtual std::vector<std::string> getDependencies() const = 0;
    virtual bool isEnabled() const = 0;
    virtual std::string getDescription() const = 0;
    virtual std::string getType() const = 0;
};
```

### Module Lifecycle

Modules follow a three-phase lifecycle:

1. **Initialization** (parent context): Set up resources before fork
2. **Prepare Child** (parent context): Final setup after fork
3. **Apply Child** (child context): Configure isolation in child
4. **Execute** (child context): Run the main module logic
5. **Cleanup** (parent context): Release resources after exit

## Built-in Modules

### Namespaces

```cpp
class Namespaces : public IModule {
    // Enables: PID, Network, Mount, UTS, IPC, User namespaces
};
```

### Cgroups

```cpp
class Cgroups : public IModule {
    // Sets: memory.max, cpu.max, pids.max
};
```

### Seccomp

```cpp
class Seccomp : public IModule {
    bool loadProfile(const std::string& path);
    void addRule(const SyscallRule& rule);
    void setDefaultAction(int action);
};
```

### Caps

```cpp
class Caps : public IModule {
    bool dropAllCapabilities();
    bool addAmbientCapability(const std::string& cap);
};
```

### RootFS

```cpp
class RootFS : public IModule {
    std::string getRootPath() const;
    bool bootstrap(const SandboxConfiguration& config);
};
```

### Mounts

```cpp
class Mounts : public IModule {
    std::vector<MountInfo> getActiveMounts() const;
};
```

### AIAgent

```cpp
class AIAgent : public IModule {
    AIResponse sendPrompt(const AIPrompt& prompt);
    AIResponse analyzeError(const std::string& errorMessage,
                           const std::vector<std::string>& context = {});
    AIResponse generateSeccompPolicy(const std::string& command);
    AIResponse optimizeConfiguration(const SandboxConfiguration& config,
                                     const std::string& workload);
};

struct AIResponse {
    std::string content;
    int statusCode;
    std::string errorMessage;
    bool success;
};
```

## Usage Examples

### Basic Sandbox Execution

```cpp
#include "core/SandboxManager.h"
#include "core/ConfigParser.h"

int main() {
    // Load configuration
    ConfigParser parser("/etc/sandbox/default.json");
    auto config = parser.parse();

    // Create and run sandbox
    SandboxManager manager;
    manager.registerDefaultModules();
    manager.setConfig(config);

    auto result = manager.run();

    if (result.success) {
        std::cout << "Sandbox executed successfully\n";
        std::cout << "Output: " << result.stdout << "\n";
    }

    return result.exitCode;
}
```

### Custom Module Registration

```cpp
#include "core/SandboxManager.h"

class CustomModule : public IModule {
    // Implement IModule interface
};

int main() {
    SandboxManager manager;
    manager.registerModule(std::make_unique<CustomModule>());

    // Register built-in modules
    manager.registerModule(std::make_unique<Namespaces>());
    manager.registerModule(std::make_unique<Cgroups>());

    auto result = manager.run();
    return result.exitCode;
}
```

### Using the AI Module

```cpp
#include "modules/ai/AIAgent.h"

int main() {
    SandboxConfiguration config = /* ... */;
    config.ai_module.enabled = true;
    config.ai_module.api_key_env = "OPENAI_API_KEY";

    AIAgent agent;
    agent.initialize(config);

    if (agent.isEnabled()) {
        auto response = agent.analyzeError(
            "seccomp: SECCOMP_RET_ALLOW unknown syscall",
            {"Running nginx container"}
        );

        if (response.success) {
            std::cout << "AI Suggestion: " << response.content << "\n";
        }
    }

    return 0;
}
```

## Error Handling

### SandboxResult

```cpp
struct SandboxResult {
    int exitCode;              // Exit code of sandbox process
    bool success;              // Whether execution succeeded
    std::string errorMessage;  // Error message if failed
    long executionTimeMs;      // Execution time in milliseconds
    std::string stdout;        // Captured stdout
    std::string stderr;        // Captured stderr
    pid_t childPid;            // PID of child process
};
```

### Common Errors

| Error Code | Description |
|------------|-------------|
| 1 | General initialization failure |
| 2 | Configuration error |
| 3 | Module initialization failed |
| 4 | Fork failed |
| 5 | Child process error |
| 6 | Cleanup failed |

## Thread Safety

The sandbox platform is designed for single-threaded initialization with
optional async execution via `runAsync()`. Once execution begins, modules
should not be modified concurrently.

The Logger class is thread-safe and can be used from multiple threads.
