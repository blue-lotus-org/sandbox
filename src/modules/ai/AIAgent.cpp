/**
 * @file AIAgent.cpp
 * @brief Implementation of the AIAgent class.
 */

#include "modules/ai/AIAgent.h"
#include "core/Logger.h"
#include "nlohmann/json.hpp"
#include <cstdlib>
#include <cstring>
#include <sstream>

using json = nlohmann::json;

namespace sandbox {

AIAgent::AIAgent()
    : state_(ModuleState::UNINITIALIZED)
    , curlHandle_(nullptr)
{
}

AIAgent::~AIAgent() {
    cleanupCurl();
}

std::string AIAgent::getName() const {
    return "ai-agent";
}

std::string AIAgent::getVersion() const {
    return "1.0.0";
}

ModuleState AIAgent::getState() const {
    return state_;
}

bool AIAgent::initialize(const SandboxConfiguration& config) {
    SANDBOX_INFO("Initializing AI Agent module");
    config_ = config;

    if (!config.ai_module.enabled) {
        SANDBOX_INFO("AI module is disabled");
        state_ = ModuleState::INITIALIZED;
        return true;
    }

    // Get configuration
    baseUrl_ = config.ai_module.base_url;
    model_ = config.ai_module.model;
    systemPrompt_ = config.ai_module.system_prompt;
    apiKey_ = getApiKey();

    if (apiKey_.empty()) {
        SANDBOX_WARNING("AI API key not found, module will be disabled");
        state_ = ModuleState::INITIALIZED;
        return true;
    }

    // Initialize cURL
    if (!initCurl()) {
        SANDBOX_ERROR("Failed to initialize cURL for AI module");
        return false;
    }

    state_ = ModuleState::INITIALIZED;
    SANDBOX_INFO("AI Agent module initialized successfully");
    SANDBOX_DEBUG("Using model: " + model_);
    SANDBOX_DEBUG("API endpoint: " + baseUrl_);

    return true;
}

bool AIAgent::prepareChild(const SandboxConfiguration& config, pid_t childPid) {
    // AI module doesn't need to do anything in parent
    return true;
}

bool AIAgent::applyChild(const SandboxConfiguration& config) {
    // AI module operations happen on demand, not during sandbox execution
    return true;
}

int AIAgent::execute(const SandboxConfiguration& config) {
    // AI module doesn't execute anything
    return 0;
}

bool AIAgent::cleanup() {
    SANDBOX_DEBUG("Cleaning up AI Agent module");
    cleanupCurl();
    state_ = ModuleState::STOPPED;
    return true;
}

std::vector<std::string> AIAgent::getDependencies() const {
    return {};
}

bool AIAgent::isEnabled() const {
    return config_.ai_module.enabled && !apiKey_.empty();
}

std::string AIAgent::getDescription() const {
    return "Provides AI-powered analysis, error diagnosis, and configuration optimization.";
}

std::string AIAgent::getType() const {
    return "ai";
}

AIResponse AIAgent::sendPrompt(const AIPrompt& prompt) {
    AIResponse response;
    response.success = false;
    response.statusCode = 0;

    if (!isEnabled()) {
        response.errorMessage = "AI module is not enabled or API key not configured";
        return response;
    }

    // Build the request payload
    std::string payload = buildPayload(prompt);

    // Set up cURL request
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, ("Authorization: Bearer " + apiKey_).c_str());

    curl_easy_setopt(curlHandle_, CURLOPT_URL, (baseUrl_ + "/chat/completions").c_str());
    curl_easy_setopt(curlHandle_, CURLOPT_POST, 1);
    curl_easy_setopt(curlHandle_, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curlHandle_, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curlHandle_, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curlHandle_, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(curlHandle_, CURLOPT_TIMEOUT, 30);

    responseBuffer_.clear();

    // Execute request
    CURLcode res = curl_easy_perform(curlHandle_);

    if (res != CURLE_OK) {
        response.errorMessage = curl_easy_strerror(res);
        response.statusCode = -1;
        SANDBOX_ERROR("AI API request failed: " + response.errorMessage);
    } else {
        long httpCode = 0;
        curl_easy_getinfo(curlHandle_, CURLINFO_RESPONSE_CODE, &httpCode);
        response.statusCode = static_cast<int>(httpCode);

        if (httpCode == 200) {
            response = parseResponse(responseBuffer_);
        } else {
            response.errorMessage = "HTTP " + std::to_string(httpCode);
            response.success = false;
        }
    }

    curl_slist_free_all(headers);

    return response;
}

AIResponse AIAgent::analyzeError(const std::string& errorMessage,
                                 const std::vector<std::string>& context) {
    AIPrompt prompt;
    prompt.systemPrompt = systemPrompt_;
    prompt.temperature = config_.ai_module.temperature;
    prompt.maxTokens = config_.ai_module.max_tokens;

    std::stringstream ss;
    ss << "Analyze the following sandbox error and suggest a solution:\n\n";
    ss << "Error: " << errorMessage << "\n\n";

    if (!context.empty()) {
        ss << "Context:\n";
        for (const auto& c : context) {
            ss << "- " << c << "\n";
        }
    }

    ss << "\nProvide a brief explanation of the error and how to resolve it.";
    prompt.userPrompt = ss.str();

    return sendPrompt(prompt);
}

AIResponse AIAgent::generateSeccompPolicy(const std::string& command) {
    AIPrompt prompt;
    prompt.systemPrompt = "You are a security expert specializing in seccomp policies for container sandboxing.";
    prompt.temperature = config_.ai_module.temperature;
    prompt.maxTokens = config_.ai_module.max_tokens;

    std::stringstream ss;
    ss << "Generate a seccomp policy (JSON format) for the following command running in a sandbox:\n\n";
    ss << "Command: " << command << "\n\n";
    ss << "The policy should:\n";
    ss << "1. Allow essential system calls for basic operation\n";
    ss << "2. Block dangerous system calls that are not needed\n";
    ss << "3. Be in the standard seccomp-bpf JSON format\n\n";
    ss << "Output only the JSON policy, no explanations.";

    prompt.userPrompt = ss.str();

    return sendPrompt(prompt);
}

AIResponse AIAgent::optimizeConfiguration(const SandboxConfiguration& currentConfig,
                                          const std::string& workloadDescription) {
    AIPrompt prompt;
    prompt.systemPrompt = "You are a container security and performance optimization expert.";
    prompt.temperature = config_.ai_module.temperature;
    prompt.maxTokens = config_.ai_module.max_tokens;

    std::stringstream ss;
    ss << "Optimize the sandbox configuration for the following workload:\n\n";
    ss << "Workload: " << workloadDescription << "\n\n";
    ss << "Current Configuration:\n";
    ss << "- Memory: " << currentConfig.resources.memory_mb << " MB\n";
    ss << "- CPU: " << currentConfig.resources.cpu_quota_percent << "%\n";
    ss << "- Namespaces: ";
    for (const auto& ns : currentConfig.isolation.namespaces) {
        ss << ns << " ";
    }
    ss << "\n";

    ss << "\nProvide optimized configuration values (JSON format) with explanations.";
    prompt.userPrompt = ss.str();

    return sendPrompt(prompt);
}

bool AIAgent::initCurl() {
    curlHandle_ = curl_easy_init();
    return curlHandle_ != nullptr;
}

void AIAgent::cleanupCurl() {
    if (curlHandle_) {
        curl_easy_cleanup(curlHandle_);
        curlHandle_ = nullptr;
    }
}

std::string AIAgent::getApiKey() const {
    const char* envKey = std::getenv(config_.ai_module.api_key_env.c_str());
    return envKey ? std::string(envKey) : "";
}

std::string AIAgent::buildPayload(const AIPrompt& prompt) {
    json payload;
    json messages = json::array();

    // System message
    if (!prompt.systemPrompt.empty()) {
        messages.push_back({
            {"role", "system"},
            {"content", prompt.systemPrompt}
        });
    }

    // User message
    json userMessage;
    userMessage["role"] = "user";

    std::stringstream content;
    content << prompt.userPrompt;

    // Add context if provided
    if (!prompt.context.empty()) {
        content << "\n\nContext information:\n";
        for (const auto& c : prompt.context) {
            content << "- " << c << "\n";
        }
    }

    userMessage["content"] = content.str();
    messages.push_back(userMessage);

    payload["messages"] = messages;
    payload["model"] = model_;
    payload["temperature"] = prompt.temperature;
    payload["max_tokens"] = prompt.maxTokens;

    return payload.dump();
}

AIResponse AIAgent::parseResponse(const std::string& response) {
    AIResponse result;
    result.success = false;
    result.statusCode = 200;

    try {
        json resp = json::parse(response);

        if (resp.contains("choices") && !resp["choices"].empty()) {
            result.content = resp["choices"][0]["message"]["content"].get<std::string>();
            result.success = true;
        } else if (resp.contains("error")) {
            result.errorMessage = resp["error"]["message"].get<std::string>();
            result.success = false;
        } else {
            result.errorMessage = "Unexpected response format";
            result.success = false;
        }
    } catch (const json::parse_error& e) {
        result.errorMessage = "Failed to parse response: " + std::string(e.what());
        result.success = false;
    }

    return result;
}

size_t AIAgent::writeCallback(char* contents, size_t size, size_t nmemb, void* userp) {
    AIAgent* agent = static_cast<AIAgent*>(userp);
    size_t totalSize = size * nmemb;

    agent->responseBuffer_.append(contents, totalSize);
    return totalSize;
}

} // namespace sandbox
