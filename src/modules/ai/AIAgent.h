/**
 * @file AIAgent.h
 * @brief AI module for OpenAI-compatible API integration.
 *
 * This header defines the AIAgent class that provides AI-powered
 * functionality for the sandbox platform, including error analysis,
 * configuration suggestions, and dynamic policy generation.
 */

#ifndef SANDBOX_AI_AGENT_H
#define SANDBOX_AI_AGENT_H

#include "modules/interface/IModule.h"
#include "core/ConfigParser.h"
#include <string>
#include <vector>
#include <curl/curl.h>

namespace sandbox {

/**
 * @struct AIResponse
 * @brief Represents a response from the AI agent.
 */
struct AIResponse {
    std::string content;
    int statusCode;
    std::string errorMessage;
    bool success;
};

/**
 * @struct AIPrompt
 * @brief Represents a prompt to send to the AI.
 */
struct AIPrompt {
    std::string systemPrompt;
    std::string userPrompt;
    std::vector<std::string> context;
    double temperature;
    int maxTokens;
};

/**
 * @class AIAgent
 * @brief Implements OpenAI-compatible AI integration.
 *
 * The AIAgent class provides AI-powered capabilities for the sandbox,
 * including error analysis, configuration optimization, and dynamic
 * security policy generation. It implements an OpenAI-compatible API
 * interface.
 */
class AIAgent : public IModule {
public:
    /**
     * @brief Construct an AIAgent module.
     */
    explicit AIAgent();

    /**
     * @brief Destructor.
     */
    ~AIAgent() override;

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
     * @brief Send a prompt to the AI API.
     * @param prompt The prompt to send.
     * @return AIResponse from the API.
     */
    AIResponse sendPrompt(const AIPrompt& prompt);

    /**
     * @brief Analyze an error message and suggest a fix.
     * @param errorMessage The error to analyze.
     * @param context Additional context information.
     * @return AIResponse with suggestions.
     */
    AIResponse analyzeError(const std::string& errorMessage,
                            const std::vector<std::string>& context = {});

    /**
     * @brief Generate a seccomp policy for a given command.
     * @param command The command to generate a policy for.
     * @return AIResponse with the policy.
     */
    AIResponse generateSeccompPolicy(const std::string& command);

    /**
     * @brief Optimize sandbox configuration.
     * @param currentConfig The current configuration.
     * @param workloadDescription Description of the workload.
     * @return AIResponse with optimized configuration.
     */
    AIResponse optimizeConfiguration(const SandboxConfiguration& currentConfig,
                                     const std::string& workloadDescription);

private:
    /**
     * @brief Initialize cURL handle.
     * @return true if successful.
     */
    bool initCurl();

    /**
     * @brief Cleanup cURL handle.
     */
    void cleanupCurl();

    /**
     * @brief Get API key from environment.
     * @return The API key or empty string.
     */
    std::string getApiKey() const;

    /**
     * @brief Build the JSON payload for API request.
     * @param prompt The prompt to send.
     * @return JSON string payload.
     */
    std::string buildPayload(const AIPrompt& prompt);

    /**
     * @brief Parse the API response.
     * @param response The raw response string.
     * @return Parsed AIResponse.
     */
    AIResponse parseResponse(const std::string& response);

    /**
     * @brief cURL write callback.
     * @param contents Data received.
     * @param size Element size.
     * @param nmemb Number of elements.
     * @param userp User pointer.
     * @return Bytes processed.
     */
    static size_t writeCallback(char* contents, size_t size, size_t nmemb, void* userp);

    ModuleState state_;
    SandboxConfiguration config_;
    CURL* curlHandle_;
    std::string apiKey_;
    std::string baseUrl_;
    std::string model_;
    std::string systemPrompt_;
    std::string responseBuffer_;
};

} // namespace sandbox

#endif // SANDBOX_AI_AGENT_H
