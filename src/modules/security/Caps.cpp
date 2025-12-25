/**
 * @file Caps.cpp
 * @brief Implementation of the Caps class.
 */

#include "modules/security/Caps.h"
#include "utils/Syscalls.h"
#include "core/Logger.h"
#include <sys/capability.h>
#include <unistd.h>
#include <string.h>

namespace sandbox {

Caps::Caps()
    : state_(ModuleState::UNINITIALIZED)
    , ambientCapsEnabled_(false)
{
}

std::string Caps::getName() const {
    return "caps";
}

std::string Caps::getVersion() const {
    return "1.0.0";
}

ModuleState Caps::getState() const {
    return state_;
}

bool Caps::initialize(const SandboxConfiguration& config) {
    SANDBOX_INFO("Initializing Caps module");
    config_ = config;

    grantedCapabilities_ = config.security.capabilities;

    SANDBOX_DEBUG("Requested capabilities: " + std::to_string(grantedCapabilities_.size()));
    for (const auto& cap : grantedCapabilities_) {
        SANDBOX_DEBUG("  - " + cap);
    }

    state_ = ModuleState::INITIALIZED;
    SANDBOX_INFO("Caps module initialized successfully");

    return true;
}

bool Caps::prepareChild(const SandboxConfiguration& config, pid_t childPid) {
    // Capabilities are applied in the child
    return true;
}

bool Caps::applyChild(const SandboxConfiguration& config) {
    SANDBOX_INFO("Applying capabilities");

    // Get current capabilities
    cap_t caps = cap_get_proc();
    if (!caps) {
        SANDBOX_ERROR("Failed to get process capabilities");
        return false;
    }

    // Clear all capabilities
    cap_clear(caps);

    // Add requested capabilities
    cap_value_t capList[64];
    int capCount = 0;

    for (const auto& capName : config.security.capabilities) {
        int capNum = capabilityFromName(capName);
        if (capNum >= 0) {
            capList[capCount++] = static_cast<cap_value_t>(capNum);
        } else {
            SANDBOX_WARNING("Unknown capability: " + capName);
        }
    }

    if (capCount > 0) {
        if (cap_set_flag(caps, CAP_EFFECTIVE, capCount, capList, CAP_SET) < 0) {
            SANDBOX_ERROR("Failed to set effective capabilities");
            cap_free(caps);
            return false;
        }

        if (cap_set_flag(caps, CAP_PERMITTED, capCount, capList, CAP_SET) < 0) {
            SANDBOX_ERROR("Failed to set permitted capabilities");
            cap_free(caps);
            return false;
        }

        if (cap_set_flag(caps, CAP_INHERITABLE, capCount, capList, CAP_SET) < 0) {
            SANDBOX_ERROR("Failed to set inheritable capabilities");
            cap_free(caps);
            return false;
        }
    }

    // Apply the capability set
    if (cap_set_proc(caps) < 0) {
        SANDBOX_ERROR("Failed to set process capabilities: " + std::string(strerror(errno)));
        cap_free(caps);
        return false;
    }

    // Also set ambient capabilities if supported (Linux 4.3+)
    // Ambient capabilities are passed through execve for non-setuid binaries
    for (int i = 0; i < capCount; ++i) {
        if (cap_update_ambient(capList[i], CAP_SET) < 0) {
            SANDBOX_WARNING("Failed to set ambient capability: " + std::to_string(capList[i]));
        } else {
            ambientCapsEnabled_ = true;
        }
    }

    cap_free(caps);

    SANDBOX_DEBUG("Capabilities applied successfully");
    state_ = ModuleState::RUNNING;

    return true;
}

int Caps::execute(const SandboxConfiguration& config) {
    return 0;
}

bool Caps::cleanup() {
    SANDBOX_DEBUG("Cleaning up Caps module");
    grantedCapabilities_.clear();
    state_ = ModuleState::STOPPED;
    return true;
}

std::vector<std::string> Caps::getDependencies() const {
    return {};
}

bool Caps::isEnabled() const {
    return true; // Always enabled if registered
}

std::string Caps::getDescription() const {
    return "Manages Linux capabilities for fine-grained privilege control in the sandbox.";
}

std::string Caps::getType() const {
    return "security";
}

bool Caps::dropAllCapabilities() {
    cap_t caps = cap_init();
    if (!caps) {
        return false;
    }

    bool result = (cap_set_proc(caps) == 0);
    cap_free(caps);
    return result;
}

bool Caps::addAmbientCapability(const std::string& cap) {
    int capNum = capabilityFromName(cap);
    if (capNum < 0) {
        return false;
    }

    return (cap_update_ambient(capNum, CAP_SET) == 0);
}

bool Caps::hasCapability(const std::string& cap) {
    cap_t caps = cap_get_proc();
    if (!caps) {
        return false;
    }

    int capNum = capabilityFromName(cap);
    if (capNum < 0) {
        cap_free(caps);
        return false;
    }

    cap_flag_value_t value;
    bool result = (cap_get_flag(caps, capNum, CAP_EFFECTIVE, &value) == 0 && value == CAP_SET);

    cap_free(caps);
    return result;
}

int Caps::capabilityFromName(const std::string& name) {
    // Map common capability names to numbers
    if (name == "CAP_CHOWN") return CAP_CHOWN;
    if (name == "CAP_DAC_OVERRIDE") return CAP_DAC_OVERRIDE;
    if (name == "CAP_DAC_READ_SEARCH") return CAP_DAC_READ_SEARCH;
    if (name == "CAP_FOWNER") return CAP_FOWNER;
    if (name == "CAP_FSETID") return CAP_FSETID;
    if (name == "CAP_KILL") return CAP_KILL;
    if (name == "CAP_SETGID") return CAP_SETGID;
    if (name == "CAP_SETUID") return CAP_SETUID;
    if (name == "CAP_SETPCAP") return CAP_SETPCAP;
    if (name == "CAP_LINUX_IMMUTABLE") return CAP_LINUX_IMMUTABLE;
    if (name == "CAP_NET_BIND_SERVICE") return CAP_NET_BIND_SERVICE;
    if (name == "CAP_NET_BROADCAST") return CAP_NET_BROADCAST;
    if (name == "CAP_NET_ADMIN") return CAP_NET_ADMIN;
    if (name == "CAP_NET_RAW") return CAP_NET_RAW;
    if (name == "CAP_IPC_LOCK") return CAP_IPC_LOCK;
    if (name == "CAP_IPC_OWNER") return CAP_IPC_OWNER;
    if (name == "CAP_SYS_MODULE") return CAP_SYS_MODULE;
    if (name == "CAP_SYS_RAWIO") return CAP_SYS_RAWIO;
    if (name == "CAP_SYS_CHROOT") return CAP_SYS_CHROOT;
    if (name == "CAP_SYS_PTRACE") return CAP_SYS_PTRACE;
    if (name == "CAP_SYS_PACCT") return CAP_SYS_PACCT;
    if (name == "CAP_SYS_ADMIN") return CAP_SYS_ADMIN;
    if (name == "CAP_SYS_BOOT") return CAP_SYS_BOOT;
    if (name == "CAP_SYS_NICE") return CAP_SYS_NICE;
    if (name == "CAP_SYS_RESOURCE") return CAP_SYS_RESOURCE;
    if (name == "CAP_SYS_TIME") return CAP_SYS_TIME;
    if (name == "CAP_SYS_TTY_CONFIG") return CAP_SYS_TTY_CONFIG;
    if (name == "CAP_MKNOD") return CAP_MKNOD;
    if (name == "CAP_LEASE") return CAP_LEASE;
    if (name == "CAP_AUDIT_WRITE") return CAP_AUDIT_WRITE;
    if (name == "CAP_AUDIT_CONTROL") return CAP_AUDIT_CONTROL;
    if (name == "CAP_SETFCAP") return CAP_SETFCAP;

    return -1;
}

std::vector<int> Caps::getKeepCapabilities(const SandboxConfiguration& config) {
    std::vector<int> caps;

    for (const auto& capName : config.security.capabilities) {
        int capNum = capabilityFromName(capName);
        if (capNum >= 0) {
            caps.push_back(capNum);
        }
    }

    return caps;
}

} // namespace sandbox
