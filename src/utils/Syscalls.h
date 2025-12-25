/**
 * @file Syscalls.h
 * @brief Low-level syscall wrappers for the sandbox platform.
 *
 * This header provides safe wrappers around Linux system calls
 * used by the sandbox platform. All wrappers use RAII principles
 * and provide error handling.
 */

#ifndef SANDBOX_SYSCALLS_H
#define SANDBOX_SYSCALLS_H

#include <string>
#include <vector>
#include <optional>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <sched.h>
#include <sys/syscall.h>
#include <errno.h>

namespace sandbox {

/**
 * @class ScopedFd
 * @brief RAII wrapper for file descriptors.
 */
class ScopedFd {
public:
    explicit ScopedFd(int fd = -1) : fd_(fd) {}
    ~ScopedFd() { close(); }

    ScopedFd(const ScopedFd&) = delete;
    ScopedFd& operator=(const ScopedFd&) = delete;

    ScopedFd(ScopedFd&& other) noexcept : fd_(other.fd_) {
        other.fd_ = -1;
    }

    ScopedFd& operator=(ScopedFd&& other) noexcept {
        if (this != &other) {
            close();
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    void reset(int fd = -1) {
        close();
        fd_ = fd;
    }

    int get() const { return fd_; }
    int release() {
        int fd = fd_;
        fd_ = -1;
        return fd;
    }

    bool isValid() const { return fd_ >= 0; }
    explicit operator bool() const { return isValid(); }

private:
    void close() {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }

    int fd_;
};

/**
 * @namespace Syscall
 * @brief Namespace containing syscall wrapper functions.
 */
namespace Syscall {

/**
 * @brief Read from /proc/self/status safely.
 * @param key The key to look for.
 * @return The value if found, nullopt otherwise.
 */
std::optional<std::string> readProcStatus(const std::string& key);

/**
 * @brief Read a file safely.
 * @param path Path to the file.
 * @return File contents if successful.
 */
std::optional<std::string> readFile(const std::string& path);

/**
 * @brief Write to a file safely.
 * @param path Path to the file.
 * @param content Content to write.
 * @return true if successful.
 */
bool writeFile(const std::string& path, const std::string& content);

/**
 * @brief Create a directory recursively.
 * @param path Path to create.
 * @param mode Permission mode.
 * @return true if successful.
 */
bool mkdirRecursive(const std::string& path, mode_t mode = 0755);

/**
 * @brief Remove a directory recursively.
 * @param path Path to remove.
 * @return true if successful.
 */
bool removeRecursive(const std::string& path);

/**
 * @brief Check if a path exists.
 * @param path Path to check.
 * @return true if exists.
 */
bool exists(const std::string& path);

/**
 * @brief Check if a path is a directory.
 * @param path Path to check.
 * @return true if is directory.
 */
bool isDirectory(const std::string& path);

/**
 * @brief Create a cgroup.
 * @param hierarchy Path to cgroup hierarchy.
 * @param name Name of the cgroup.
 * @return true if successful.
 */
bool createCgroup(const std::string& hierarchy, const std::string& name);

/**
 * @brief Remove a cgroup.
 * @param hierarchy Path to cgroup hierarchy.
 * @param name Name of the cgroup.
 * @return true if successful.
 */
bool removeCgroup(const std::string& hierarchy, const std::string& name);

/**
 * @brief Set a cgroup value.
 * @param hierarchy Path to cgroup hierarchy.
 * @param name Name of the cgroup.
 * @param setting Setting name.
 * @param value Value to set.
 * @return true if successful.
 */
bool setCgroupValue(const std::string& hierarchy, const std::string& name,
                    const std::string& setting, const std::string& value);

/**
 * @brief Add a process to a cgroup.
 * @param hierarchy Path to cgroup hierarchy.
 * @param name Name of the cgroup.
 * @param pid Process ID to add.
 * @return true if successful.
 */
bool addToCgroup(const std::string& hierarchy, const std::string& name, pid_t pid);

/**
 * @brief Mount a filesystem.
 * @param source Mount source.
 * @param target Mount target.
 * @param fstype Filesystem type.
 * @param flags Mount flags.
 * @param data Mount options.
 * @return true if successful.
 */
bool mount(const std::string& source, const std::string& target,
           const std::string& fstype, unsigned long flags, const void* data);

/**
 * @brief Unmount a path.
 * @param path Path to unmount.
 * @param flags Unmount flags.
 * @return true if successful.
 */
bool unmount(const std::string& path, int flags = 0);

/**
 * @brief Pivot root.
 * @param newRoot New root path.
 * @param putOld Path to put old root.
 * @return true if successful.
 */
bool pivotRoot(const std::string& newRoot, const std::string& putOld);

/**
 * @brief Apply unshare with flags.
 * @param flags Unshare flags.
 * @return true if successful.
 */
bool unshare(int flags);

/**
 * @brief Clone a new process with flags.
 * @param flags Clone flags.
 * @param stack Stack size for child.
 * @return PID of child in parent, 0 in child, -1 on error.
 */
pid_t cloneWithFlags(int flags, void* stack = nullptr);

/**
 * @brief Set hostname.
 * @param hostname Hostname to set.
 * @return true if successful.
 */
bool setHostname(const std::string& hostname);

/**
 * @brief Create a veth pair.
 * @param veth1 Name for first interface.
 * @param veth2 Name for second interface.
 * @return true if successful.
 */
bool createVethPair(const std::string& veth1, const std::string& veth2);

/**
 * @brief Set interface up.
 * @param interface Interface name.
 * @return true if successful.
 */
bool interfaceUp(const std::string& interface);

/**
 * @brief Set interface IP address.
 * @param interface Interface name.
 * @param ip IP address.
 * @return true if successful.
 */
bool setInterfaceIP(const std::string& interface, const std::string& ip);

/**
 * @brief Create network namespace.
 * @param nsName Name of namespace.
 * @return true if successful.
 */
bool createNetNs(const std::string& nsName);

/**
 * @brief Move interface to namespace.
 * @param interface Interface name.
 * @param nsName Namespace name.
 * @return true if successful.
 */
bool moveInterfaceToNs(const std::string& interface, const std::string& nsName);

/**
 * @brief Write to /proc/self/setgroups.
 * @param content Content to write.
 * @return true if successful.
 */
bool writeProcSetgroups(const std::string& content);

/**
 * @brief Write to /proc/self/uid_map.
 * @param content Content to write.
 * @return true if successful.
 */
bool writeProcUidMap(const std::string& content);

/**
 * @brief Write to /proc/self/gid_map.
 * @param content Content to write.
 * @return true if successful.
 */
bool writeProcGidMap(const std::string& content);

/**
 * @brief Load a seccomp profile.
 * @param path Path to the profile.
 * @param action Default action.
 * @return true if successful.
 */
bool loadSeccompProfile(const std::string& path, int action = SECCOMP_RET_ERRNO);

/**
 * @brief Drop capabilities.
 * @param capabilities List of capabilities to drop.
 * @return true if successful.
 */
bool dropCapabilities(const std::vector<std::string>& capabilities);

/**
 * @brief Check if a capability is allowed.
 * @param cap Capability name.
 * @return true if allowed.
 */
bool hasCapability(const std::string& cap);

/**
 * @brief Execute a command.
 * @param path Path to command.
 * @param argv Argument vector.
 * @param envp Environment variables.
 * @return Exit status.
 */
int execve(const std::string& path, char* const argv[], char* const envp[]);

} // namespace Syscall

} // namespace sandbox

#endif // SANDBOX_SYSCALLS_H
