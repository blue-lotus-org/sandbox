/**
 * @file ConfigParser.cpp
 * @brief Implementation of the ConfigParser class.
 */

#include "core/ConfigParser.h"
#include "core/Logger.h"
#include <fstream>
#include <algorithm>

namespace sandbox {

ConfigParser::ConfigParser(const std::filesystem::path& configPath)
    : configPath_(configPath)
    , useFile_(true)
{
}

ConfigParser::ConfigParser(const std::string& jsonContent)
    : configPath_("")
    , useFile_(false)
{
    try {
        json_ = json::parse(jsonContent);
    } catch (const json::parse_error& e) {
        throw std::runtime_error("Failed to parse JSON: " + std::string(e.what()));
    }
}

bool ConfigParser::isValidConfigFile(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        return false;
    }

    if (!std::filesystem::is_regular_file(path)) {
        return false;
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    try {
        json j;
        file >> j;
        return j.contains("sandbox") && j.contains("resources");
    } catch (const json::parse_error&) {
        return false;
    }
}

std::filesystem::path ConfigParser::getDefaultConfigPath() {
    const char* envPath = std::getenv("SANDBOX_CONFIG_PATH");
    if (envPath) {
        return std::filesystem::path(envPath);
    }

    // Check common locations
    std::vector<std::string> candidates = {
        "/etc/sandbox/default.json",
        "/var/lib/sandbox/config.json",
        "./config/default.json",
        "../config/default.json"
    };

    for (const auto& candidate : candidates) {
        std::filesystem::path p(candidate);
        if (isValidConfigFile(p)) {
            return p;
        }
    }

    return "";
}

SandboxConfiguration ConfigParser::createDefaultConfig() {
    SandboxConfiguration config;

    // Sandbox config
    config.sandbox.name = "sandbox-default";
    config.sandbox.hostname = "sandbox-container";
    config.sandbox.rootfs_path = "/var/lib/sandbox/rootfs/ubuntu_focal";
    config.sandbox.command = {"/bin/bash"};
    config.sandbox.auto_bootstrap = false;
    config.sandbox.distro = "ubuntu";
    config.sandbox.release = "focal";

    // Resources config
    config.resources.memory_mb = 512;
    config.resources.cpu_quota_percent = 50;
    config.resources.max_pids = 100;
    config.resources.enable_swap = false;

    // Isolation config
    config.isolation.namespaces = {"pid", "net", "ipc", "uts", "mount", "user"};
    config.isolation.uid_map = {1000, 0, 1};
    config.isolation.gid_map = {1000, 0, 1};

    // Security config
    config.security.capabilities = {};
    config.security.seccomp_policy = "default";
    config.security.seccomp_profile_path = "";

    // Mounts config
    config.mounts.bind_mounts = {{"/tmp", "/tmp", false}};

    // AI module config
    config.ai_module.enabled = false;
    config.ai_module.provider = "openai";
    config.ai_module.api_key_env = "OPENAI_API_KEY";
    config.ai_module.base_url = "https://api.openai.com/v1";
    config.ai_module.model = "gpt-4-turbo";
    config.ai_module.temperature = 0.2;
    config.ai_module.max_tokens = 1000;
    config.ai_module.system_prompt = "You are a sandbox assistant that helps analyze and configure sandbox environments.";
    config.ai_module.auto_report_errors = true;

    // Logging config
    config.logging.level = "info";
    config.logging.output = "stdout";
    config.logging.log_file = "/var/log/sandbox/sandbox.log";

    return config;
}

void ConfigParser::parseJson() {
    if (useFile_) {
        std::ifstream file(configPath_);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open config file: " + configPath_.string());
        }

        try {
            file >> json_;
        } catch (const json::parse_error& e) {
            throw std::runtime_error("Failed to parse config file: " + std::string(e.what()));
        }
    }
}

void ConfigParser::validate() {
    if (!json_.contains("sandbox")) {
        throw std::runtime_error("Config must contain 'sandbox' section");
    }

    if (!json_.contains("resources")) {
        throw std::runtime_error("Config must contain 'resources' section");
    }

    // Validate sandbox section
    const auto& sandbox = json_["sandbox"];
    if (!sandbox.contains("command")) {
        throw std::runtime_error("Sandbox config must contain 'command'");
    }

    // Validate resources section
    const auto& resources = json_["resources"];
    if (!resources.contains("memory_mb")) {
        throw std::runtime_error("Resources config must contain 'memory_mb'");
    }
}

void ConfigParser::applyDefaults() {
    config_ = createDefaultConfig();

    // Apply sandbox settings
    if (json_.contains("sandbox")) {
        const auto& sandbox = json_["sandbox"];
        if (sandbox.contains("name")) config_.sandbox.name = sandbox["name"];
        if (sandbox.contains("hostname")) config_.sandbox.hostname = sandbox["hostname"];
        if (sandbox.contains("rootfs_path")) config_.sandbox.rootfs_path = sandbox["rootfs_path"];
        if (sandbox.contains("command")) {
            for (const auto& cmd : sandbox["command"]) {
                config_.sandbox.command.push_back(cmd.get<std::string>());
            }
        }
        if (sandbox.contains("auto_bootstrap")) config_.sandbox.auto_bootstrap = sandbox["auto_bootstrap"];
        if (sandbox.contains("distro")) config_.sandbox.distro = sandbox["distro"];
        if (sandbox.contains("release")) config_.sandbox.release = sandbox["release"];
    }

    // Apply resources settings
    if (json_.contains("resources")) {
        const auto& resources = json_["resources"];
        if (resources.contains("memory_mb")) config_.resources.memory_mb = resources["memory_mb"];
        if (resources.contains("cpu_quota_percent")) config_.resources.cpu_quota_percent = resources["cpu_quota_percent"];
        if (resources.contains("max_pids")) config_.resources.max_pids = resources["max_pids"];
        if (resources.contains("enable_swap")) config_.resources.enable_swap = resources["enable_swap"];
    }

    // Apply isolation settings
    if (json_.contains("isolation")) {
        const auto& isolation = json_["isolation"];
        if (isolation.contains("namespaces")) {
            config_.isolation.namespaces.clear();
            for (const auto& ns : isolation["namespaces"]) {
                config_.isolation.namespaces.push_back(ns.get<std::string>());
            }
        }
        if (isolation.contains("uid_map")) {
            const auto& uidMap = isolation["uid_map"];
            if (uidMap.contains("host_uid")) config_.isolation.uid_map.host_uid = uidMap["host_uid"];
            if (uidMap.contains("container_uid")) config_.isolation.uid_map.container_uid = uidMap["container_uid"];
            if (uidMap.contains("count")) config_.isolation.uid_map.count = uidMap["count"];
        }
        if (isolation.contains("gid_map")) {
            const auto& gidMap = isolation["gid_map"];
            if (gidMap.contains("host_gid")) config_.isolation.gid_map.host_gid = gidMap["host_gid"];
            if (gidMap.contains("container_gid")) config_.isolation.gid_map.container_gid = gidMap["container_gid"];
            if (gidMap.contains("count")) config_.isolation.gid_map.count = gidMap["count"];
        }
    }

    // Apply security settings
    if (json_.contains("security")) {
        const auto& security = json_["security"];
        if (security.contains("capabilities")) {
            config_.security.capabilities.clear();
            for (const auto& cap : security["capabilities"]) {
                config_.security.capabilities.push_back(cap.get<std::string>());
            }
        }
        if (security.contains("seccomp_policy")) config_.security.seccomp_policy = security["seccomp_policy"];
        if (security.contains("seccomp_profile_path")) config_.security.seccomp_profile_path = security["seccomp_profile_path"];
    }

    // Apply mounts settings
    if (json_.contains("mounts")) {
        const auto& mounts = json_["mounts"];
        if (mounts.contains("bind_mounts")) {
            config_.mounts.bind_mounts.clear();
            for (const auto& mount : mounts["bind_mounts"]) {
                BindMount bm;
                bm.source = mount.value("source", "");
                bm.target = mount.value("target", "");
                bm.read_only = mount.value("read_only", false);
                config_.mounts.bind_mounts.push_back(bm);
            }
        }
    }

    // Apply AI module settings
    if (json_.contains("ai_module")) {
        const auto& ai = json_["ai_module"];
        if (ai.contains("enabled")) config_.ai_module.enabled = ai["enabled"];
        if (ai.contains("provider")) config_.ai_module.provider = ai["provider"];
        if (ai.contains("api_key_env")) config_.ai_module.api_key_env = ai["api_key_env"];
        if (ai.contains("base_url")) config_.ai_module.base_url = ai["base_url"];
        if (ai.contains("model")) config_.ai_module.model = ai["model"];
        if (ai.contains("temperature")) config_.ai_module.temperature = ai["temperature"];
        if (ai.contains("max_tokens")) config_.ai_module.max_tokens = ai["max_tokens"];
        if (ai.contains("system_prompt")) config_.ai_module.system_prompt = ai["system_prompt"];
        if (ai.contains("auto_report_errors")) config_.ai_module.auto_report_errors = ai["auto_report_errors"];
    }

    // Apply logging settings
    if (json_.contains("logging")) {
        const auto& logging = json_["logging"];
        if (logging.contains("level")) config_.logging.level = logging["level"];
        if (logging.contains("output")) config_.logging.output = logging["output"];
        if (logging.contains("log_file")) config_.logging.log_file = logging["log_file"];
    }
}

SandboxConfiguration ConfigParser::parse() {
    parseJson();
    validate();
    applyDefaults();
    return config_;
}

const json& ConfigParser::getJson() const {
    return json_;
}

} // namespace sandbox
