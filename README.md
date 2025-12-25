# Sandbox Platform

A modern, modular sandbox environment built in C++ using native Linux kernel primitives for secure isolation.

## Features

- **Easy to Use**: Simple CLI interface with sensible defaults
- **Easy Install/Uninstall**: One-click installation script, clean removal
- **Easy Config**: JSON-based configuration with documentation
- **Well Documented**: Comprehensive guides and API documentation
- **Modular Architecture**: Plugin-based design with interchangeable components
- **AI-Ready**: Built-in AI module with OpenAI-compatible API support

## Core Technologies

- **debootstrap**: Minimal root filesystem creation
- **chroot/pivot_root**: Filesystem isolation
- **Linux Namespaces**: Process, network, mount, UTS, IPC, and user isolation
- **cgroups v2**: Resource limits (CPU, memory, PID)
- **seccomp BPF**: System call filtering
- **Linux Capabilities**: Fine-grained privilege management

## Quick Start

### Installation

```bash
# Clone the repository
cd sandbox

# Install dependencies and build
chmod +x scripts/install_deps.sh
sudo ./scripts/install_deps.sh
```

### Basic Usage

```bash
# Run a command in the sandbox
sandbox run -- /bin/bash

# Run with custom configuration
sandbox run -c /path/to/config.json -- /bin/bash -c "echo Hello"

# Run with AI assistance
sandbox --ai run -c config.json -- /bin/bash
```

### Configuration

Create a JSON configuration file:

```json
{
  "sandbox": {
    "name": "my-sandbox",
    "hostname": "sandbox-container",
    "rootfs_path": "/var/lib/sandbox/rootfs/ubuntu_focal",
    "auto_bootstrap": true,
    "distro": "ubuntu",
    "release": "focal"
  },
  "resources": {
    "memory_mb": 512,
    "cpu_quota_percent": 50,
    "max_pids": 100
  },
  "isolation": {
    "namespaces": ["pid", "net", "ipc", "uts", "mount", "user"]
  },
  "security": {
    "capabilities": [],
    "seccomp_policy": "default"
  },
  "ai_module": {
    "enabled": true,
    "api_key_env": "OPENAI_API_KEY"
  }
}
```
- distro: yours
- release: like trixi for debian 13

## Architecture

### Module System

The sandbox platform uses a plugin-based architecture where each isolation
feature is implemented as a separate module:

```
┌─────────────────────────────────────────────┐
│              SandboxManager                 │
├─────────────────────────────────────────────┤
│  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │Namespace │  │ Cgroups  │  │ Seccomp  │   │
│  │ Module   │  │  Module  │  │  Module  │   │
│  └──────────┘  └──────────┘  └──────────┘   │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │  Caps    │  │  RootFS  │  │ Mounts   │   │
│  │ Module   │  │  Module  │  │  Module  │   │
│  └──────────┘  └──────────┘  └──────────┘   │
│  ┌──────────┐                               │
│  │   AI     │                               │
│  │ Module   │                               │
│  └──────────┘                               │
└─────────────────────────────────────────────┘
```

### Module Interfaces

All modules implement the `IModule` interface:

```cpp
class IModule {
    virtual bool initialize(const SandboxConfiguration& config);
    virtual bool prepareChild(const SandboxConfiguration& config, pid_t childPid);
    virtual bool applyChild(const SandboxConfiguration& config);
    virtual int execute(const SandboxConfiguration& config);
    virtual bool cleanup();
};
```

## AI Module

The AI module provides OpenAI-compatible integration for:

- **Error Analysis**: Diagnose sandbox errors and suggest fixes
- **Policy Generation**: Generate seccomp policies for commands
- **Configuration Optimization**: Optimize resource settings for workloads

### Configuration

```json
{
  "ai_module": {
    "enabled": true,
    "provider": "openai",
    "api_key_env": "OPENAI_API_KEY",
    "base_url": "https://api.openai.com/v1",
    "model": "gpt-4-turbo",
    "temperature": 0.2,
    "auto_report_errors": true
  }
}
```
- any ai provider and model, if following openai-compatible method

### Usage

```cpp
#include "modules/ai/AIAgent.h"

AIAgent agent;
agent.initialize(config);

AIResponse response = agent.analyzeError("Permission denied", {"chmod operation"});
std::cout << response.content << std::endl;
```

## Development

### Building

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
make install
```

### Testing

```bash
cd build
ctest
```

### Adding New Modules

1. Create a new class inheriting from `IModule`
2. Implement all required interface methods
3. Register the module with `SandboxManager::registerModule()`

Example:

```cpp
class MyModule : public IModule {
    std::string getName() const override { return "my-module"; }
    bool initialize(const SandboxConfiguration& config) override { /* ... */ }
    // ... implement other methods
};

manager.registerModule(std::make_unique<MyModule>());
```

## Security Considerations

The sandbox platform implements multiple layers of security:

1. **Namespace Isolation**: Processes cannot see or interact with host resources
2. **Resource Limits**: Prevent denial-of-service through resource exhaustion
3. **System Call Filtering**: Restrict dangerous system calls
4. **Capability Dropping**: Remove unnecessary privileges

Always review and test your configuration before running untrusted code.

## Requirements

- Linux kernel 5.4+ (for cgroups v2)
- GCC 11+ or Clang 14+
- CMake 3.16+
- libcurl
- libseccomp
- libcap

## License

MIT License - see LICENSE file for details.\
This is **as/is** and use by your knowledge and at your own risks.

## Contributing

Contributions are welcome! 

---

> BLUE LOTUS INNOVATION LAB