/**
 * @file Seccomp.h
 * @brief Seccomp system call filtering module.
 *
 * This header defines the Seccomp class that implements system call
 * filtering using Linux seccomp with BPF filters.
 */

#ifndef SANDBOX_SECCOMP_H
#define SANDBOX_SECCOMP_H

#include "modules/interface/IModule.h"
#include "core/ConfigParser.h"
#include <vector>
#include <string>

namespace sandbox {

/**
 * @struct SyscallRule
 * @brief Represents a seccomp syscall rule.
 */
struct SyscallRule {
    std::string name;      ///< System call name
    int action;            ///< Action to take
    int argCount;          ///< Number of arguments to match (0 = any)
    std::vector<uint64_t> args;  ///< Arguments to match
};

/**
 * @class Seccomp
 * @brief Implements seccomp BPF filtering for system calls.
 *
 * The Seccomp class manages the creation and installation of
 * seccomp filters to restrict the system calls that processes
 * in the sandbox can execute. It supports both whitelist and
 * blacklist modes with configurable policies.
 */
class Seccomp : public IModule {
public:
    /**
     * @brief Seccomp action constants.
     */
    enum {
        ACTION_KILL = SECCOMP_RET_KILL,           ///< Kill the process
        ACTION_TRAP = SECCOMP_RET_TRAP,           ///< Send SIGSYS
        ACTION_ERRNO = SECCOMP_RET_ERRNO,         ///< Return error
        ACTION_TRACE = SECCOMP_RET_TRACE,         ///< Pass to tracer
        ACTION_LOG = SECCOMP_RET_LOG,             ///< Log only
        ACTION_ALLOW = SECCOMP_RET_ALLOW          ///< Allow the call
    };

    /**
     * @brief Construct a Seccomp module.
     */
    Seccomp();

    /**
     * @brief Destructor.
     */
    ~Seccomp() override = default;

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
     * @brief Load a seccomp profile from a file.
     * @param path Path to the profile file.
     * @return true if loaded successfully.
     */
    bool loadProfile(const std::string& path);

    /**
     * @brief Add a rule to the filter.
     * @param rule The rule to add.
     */
    void addRule(const SyscallRule& rule);

    /**
     * @brief Set the default action.
     * @param action The default action.
     */
    void setDefaultAction(int action);

    /**
     * @brief Get the default action.
     * @return The default action.
     */
    int getDefaultAction() const;

private:
    /**
     * @brief Generate the default policy.
     * @param config The sandbox configuration.
     * @return true if successful.
     */
    bool generateDefaultPolicy(const SandboxConfiguration& config);

    /**
     * @brief Load the default allowlist policy.
     * @return true if successful.
     */
    bool loadDefaultAllowlist();

    /**
     * @brief Load the default denylist policy.
     * @return true if successful.
     */
    bool loadDefaultDenylist();

    /**
     * @brief Compile and install the filter.
     * @return true if successful.
     */
    bool installFilter();

    ModuleState state_;
    SandboxConfiguration config_;
    int defaultAction_;
    std::vector<SyscallRule> rules_;
    std::vector<char> filterBlob_;
    bool enabled_;
};

} // namespace sandbox

#endif // SANDBOX_SECCOMP_H
