/**
 * @file ConfigParser.h
 * @brief Configuration parser for the sandbox platform.
 *
 * This header defines the ConfigParser class that reads and validates
 * JSON configuration files for the sandbox platform. It uses nlohmann/json
 * for JSON parsing and provides structured access to configuration values.
 */

#ifndef SANDBOX_CONFIG_PARSER_H
#define SANDBOX_CONFIG_PARSER_H

#include <string>
#include <vector>
#include <optional>
#include <filesystem>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace sandbox {

/**
 * @struct UIDMap
 * @brief Represents a UID mapping configuration.
 */
struct UIDMap {
    int host_uid;
    int container_uid;
    int count;
};

/**
 * @struct GIDMap
 * @brief Represents a GID mapping configuration.
 */
struct GIDMap {
    int host_gid;
    int container_gid;
    int count;
};

/**
 * @struct BindMount
 * @brief Represents a bind mount configuration.
 */
struct BindMount {
    std::string source;
    std::string target;
    bool read_only;
};

/**
 * @struct SandboxConfig
 * @brief Core sandbox configuration.
 */
struct SandboxConfig {
    std::string name;
    std::string hostname;
    std::string rootfs_path;
    std::vector<std::string> command;
    bool auto_bootstrap;
    std::string distro;
    std::string release;
};

/**
 * @struct ResourcesConfig
 * @brief Resource limits configuration.
 */
struct ResourcesConfig {
    int memory_mb;
    int cpu_quota_percent;
    int max_pids;
    bool enable_swap;
};

/**
 * @struct IsolationConfig
 * @brief Namespace and isolation configuration.
 */
struct IsolationConfig {
    std::vector<std::string> namespaces;
    UIDMap uid_map;
    GIDMap gid_map;
};

/**
 * @struct SecurityConfig
 * @brief Security-related configuration.
 */
struct SecurityConfig {
    std::vector<std::string> capabilities;
    std::string seccomp_policy;
    std::string seccomp_profile_path;
};

/**
 * @struct MountsConfig
 * @brief Mount configuration.
 */
struct MountsConfig {
    std::vector<BindMount> bind_mounts;
    std::vector<std::string> volumes;
};

/**
 * @struct AIModuleConfig
 * @brief AI module configuration.
 */
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

/**
 * @struct LoggingConfig
 * @brief Logging configuration.
 */
struct LoggingConfig {
    std::string level;
    std::string output;
    std::string log_file;
};

/**
 * @struct SandboxConfiguration
 * @brief Complete sandbox configuration container.
 */
struct SandboxConfiguration {
    SandboxConfig sandbox;
    ResourcesConfig resources;
    IsolationConfig isolation;
    SecurityConfig security;
    MountsConfig mounts;
    AIModuleConfig ai_module;
    LoggingConfig logging;
};

/**
 * @class ConfigParser
 * @brief Parses and validates sandbox configuration files.
 *
 * The ConfigParser class handles reading JSON configuration files,
 * validating their structure, and providing typed access to configuration
 * values. It supports default values for optional settings.
 */
class ConfigParser {
public:
    /**
     * @brief Construct a ConfigParser with a configuration file path.
     * @param configPath Path to the JSON configuration file.
     */
    explicit ConfigParser(const std::filesystem::path& configPath);

    /**
     * @brief Construct a ConfigParser with JSON content.
     * @param jsonContent Raw JSON string content.
     */
    explicit ConfigParser(const std::string& jsonContent);

    /**
     * @brief Destructor.
     */
    ~ConfigParser() = default;

    /**
     * @brief Parse and validate the configuration.
     * @return The parsed configuration structure.
     * @throws std::runtime_error if parsing fails.
     */
    SandboxConfiguration parse();

    /**
     * @brief Get the raw JSON object.
     * @return Reference to the parsed JSON.
     */
    const json& getJson() const;

    /**
     * @brief Check if a configuration file exists and is valid.
     * @param path Path to check.
     * @return true if valid, false otherwise.
     */
    static bool isValidConfigFile(const std::filesystem::path& path);

    /**
     * @brief Get the default configuration path.
     * @return Default config file path.
     */
    static std::filesystem::path getDefaultConfigPath();

    /**
     * @brief Create a minimal default configuration.
     * @return Default SandboxConfiguration.
     */
    static SandboxConfiguration createDefaultConfig();

private:
    void parseJson();
    void validate();
    void applyDefaults();

    json json_;
    SandboxConfiguration config_;
    std::filesystem::path configPath_;
    bool useFile_;
};

} // namespace sandbox

#endif // SANDBOX_CONFIG_PARSER_H
