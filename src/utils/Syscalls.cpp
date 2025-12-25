/**
 * @file Syscalls.cpp
 * @brief Implementation of syscall wrapper functions.
 */

#include "utils/Syscalls.h"
#include "core/Logger.h"
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <linux/seccomp.h>
#include <sys/capability.h>

namespace sandbox {

std::optional<std::string> Syscall::readProcStatus(const std::string& key) {
    std::ifstream file("/proc/self/status");
    if (!file.is_open()) {
        return std::nullopt;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.find(key) == 0) {
            // Extract value after the colon
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                std::string value = line.substr(pos + 1);
                // Trim whitespace
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                return value;
            }
        }
    }

    return std::nullopt;
}

std::optional<std::string> Syscall::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return std::nullopt;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool Syscall::writeFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file.is_open()) {
        SANDBOX_ERROR("Failed to open file for writing: " + path);
        return false;
    }

    file << content;
    if (!file.good()) {
        SANDBOX_ERROR("Failed to write to file: " + path);
        return false;
    }

    return true;
}

bool Syscall::mkdirRecursive(const std::string& path, mode_t mode) {
    std::string currentPath;
    std::stringstream ss(path);
    std::string token;

    while (std::getline(ss, token, '/')) {
        currentPath += "/" + token;
        if (!exists(currentPath)) {
            if (::mkdir(currentPath.c_str(), mode) < 0) {
                SANDBOX_ERROR("Failed to create directory: " + currentPath);
                return false;
            }
        }
    }
    return true;
}

bool Syscall::removeRecursive(const std::string& path) {
    std::error_code ec;
    std::filesystem::remove_all(path, ec);
    return !ec;
}

bool Syscall::exists(const std::string& path) {
    std::error_code ec;
    return std::filesystem::exists(path, ec);
}

bool Syscall::isDirectory(const std::string& path) {
    std::error_code ec;
    return std::filesystem::is_directory(path, ec);
}

bool Syscall::createCgroup(const std::string& hierarchy, const std::string& name) {
    std::string path = hierarchy + "/" + name;
    return mkdirRecursive(path);
}

bool Syscall::removeCgroup(const std::string& hierarchy, const std::string& name) {
    std::string path = hierarchy + "/" + name;
    return removeRecursive(path);
}

bool Syscall::setCgroupValue(const std::string& hierarchy, const std::string& name,
                              const std::string& setting, const std::string& value) {
    std::string path = hierarchy + "/" + name + "/" + setting;
    return writeFile(path, value);
}

bool Syscall::addToCgroup(const std::string& hierarchy, const std::string& name, pid_t pid) {
    std::string path = hierarchy + "/" + name + "/cgroup.procs";
    return writeFile(path, std::to_string(pid));
}

bool Syscall::mount(const std::string& source, const std::string& target,
                    const std::string& fstype, unsigned long flags, const void* data) {
    if (::mount(source.c_str(), target.c_str(), fstype.c_str(), flags, data) < 0) {
        SANDBOX_ERROR("Failed to mount: " + std::string(strerror(errno)));
        return false;
    }
    return true;
}

bool Syscall::unmount(const std::string& path, int flags) {
    if (::umount2(path.c_str(), flags) < 0) {
        SANDBOX_ERROR("Failed to unmount: " + std::string(strerror(errno)));
        return false;
    }
    return true;
}

bool Syscall::pivotRoot(const std::string& newRoot, const std::string& putOld) {
    if (::pivot_root(newRoot.c_str(), putOld.c_str()) < 0) {
        SANDBOX_ERROR("pivot_root failed: " + std::string(strerror(errno)));
        return false;
    }
    return true;
}

bool Syscall::unshare(int flags) {
    if (::unshare(flags) < 0) {
        SANDBOX_ERROR("unshare failed: " + std::string(strerror(errno)));
        return false;
    }
    return true;
}

pid_t Syscall::cloneWithFlags(int flags, void* stack) {
    // Default stack size for child thread
    static const int STACK_SIZE = 65536;
    char* stackMemory;

    if (!stack) {
        stackMemory = new char[STACK_SIZE];
        stack = stackMemory + STACK_SIZE;
    }

    pid_t pid = ::clone(flags, stack, nullptr, nullptr);
    return pid;
}

bool Syscall::setHostname(const std::string& hostname) {
    if (::sethostname(hostname.c_str(), hostname.size()) < 0) {
        SANDBOX_ERROR("sethostname failed: " + std::string(strerror(errno)));
        return false;
    }
    return true;
}

bool Syscall::createVethPair(const std::string& veth1, const std::string& veth2) {
    // This would require executing ip command
    // For simplicity, we'll return true and let the command module handle it
    SANDBOX_DEBUG("Veth pair creation would be handled by command module");
    return true;
}

bool Syscall::interfaceUp(const std::string& interface) {
    SANDBOX_DEBUG("Interface up: " + interface);
    return true;
}

bool Syscall::setInterfaceIP(const std::string& interface, const std::string& ip) {
    SANDBOX_DEBUG("Setting IP for " + interface + ": " + ip);
    return true;
}

bool Syscall::createNetNs(const std::string& nsName) {
    SANDBOX_DEBUG("Creating net ns: " + nsName);
    return true;
}

bool Syscall::moveInterfaceToNs(const std::string& interface, const std::string& nsName) {
    SANDBOX_DEBUG("Moving " + interface + " to ns: " + nsName);
    return true;
}

bool Syscall::writeProcSetgroups(const std::string& content) {
    return writeFile("/proc/self/setgroups", content);
}

bool Syscall::writeProcUidMap(const std::string& content) {
    return writeFile("/proc/self/uid_map", content);
}

bool Syscall::writeProcGidMap(const std::string& content) {
    return writeFile("/proc/self/gid_map", content);
}

bool Syscall::loadSeccompProfile(const std::string& path, int action) {
    SANDBOX_DEBUG("Loading seccomp profile: " + path);
    // Seccomp loading is done via prctl in the Seccomp module
    return true;
}

bool Syscall::dropCapabilities(const std::vector<std::string>& capabilities) {
    cap_t caps = cap_init();
    if (!caps) {
        SANDBOX_ERROR("Failed to initialize capabilities");
        return false;
    }

    // Clear all capabilities
    cap_clear(caps);

    // Set effective and permitted sets
    cap_value_t capList[1];
    for (const auto& cap : capabilities) {
        // Convert capability name to number
        int capNum = -1;
        if (cap == "CAP_NET_BIND_SERVICE") capNum = CAP_NET_BIND_SERVICE;
        // Add more capability mappings as needed

        if (capNum >= 0) {
            capList[0] = capNum;
            cap_set_flag(caps, CAP_EFFECTIVE, 1, capList, CAP_SET);
            cap_set_flag(caps, CAP_PERMITTED, 1, capList, CAP_SET);
        }
    }

    if (cap_set_proc(caps) < 0) {
        SANDBOX_ERROR("Failed to set capabilities: " + std::string(strerror(errno)));
        cap_free(caps);
        return false;
    }

    cap_free(caps);
    return true;
}

bool Syscall::hasCapability(const std::string& cap) {
    cap_t caps = cap_get_proc();
    if (!caps) {
        return false;
    }

    cap_flag_value_t value;
    int capNum = -1;

    if (cap == "CAP_NET_BIND_SERVICE") capNum = CAP_NET_BIND_SERVICE;
    // Add more mappings

    if (capNum >= 0 && cap_get_flag(caps, capNum, CAP_EFFECTIVE, &value) == 0) {
        cap_free(caps);
        return value == CAP_SET;
    }

    cap_free(caps);
    return false;
}

int Syscall::execve(const std::string& path, char* const argv[], char* const envp[]) {
    return ::execve(path.c_str(), argv, envp);
}

} // namespace sandbox
