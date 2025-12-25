/**
 * @file main.cpp
 * @brief Main entry point for the sandbox platform.
 *
 * This file implements the command-line interface and orchestrates
 * the initialization and execution of the sandbox platform.
 */

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstring>
#include <getopt.h>
#include <unistd.h>

#include "core/Logger.h"
#include "core/ConfigParser.h"
#include "core/SandboxManager.h"
#include "modules/interface/IModule.h"
#include "modules/isolation/Namespaces.h"
#include "modules/isolation/Cgroups.h"
#include "modules/security/Seccomp.h"
#include "modules/security/Caps.h"
#include "modules/filesystem/RootFS.h"
#include "modules/filesystem/Mounts.h"
#include "modules/ai/AIAgent.h"

using namespace sandbox;

/**
 * @brief Print usage information.
 * @param programName The name of the program.
 */
void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [OPTIONS] COMMAND [ARGS...]\n\n"
              << "Options:\n"
              << "  -c, --config FILE     Configuration file path\n"
              << "  -n, --name NAME       Sandbox instance name\n"
              << "  -h, --help            Show this help message\n"
              << "  -v, --version         Show version information\n"
              << "  -d, --debug           Enable debug logging\n"
              << "  --ai                  Enable AI module\n\n"
              << "Commands:\n"
              << "  run                   Run a command in the sandbox\n"
              << "  exec                  Execute a command in a running sandbox\n"
              << "  list                  List running sandboxes\n"
              << "  stop                  Stop a running sandbox\n\n"
              << "Examples:\n"
              << "  " << programName << " run --config /etc/sandbox/default.json -- /bin/bash\n"
              << "  " << programName << " run -n mysandbox -- /bin/ls -la\n"
              << "  " << programName << " --ai run -c config.json -- echo 'Hello'\n";
}

/**
 * @brief Print version information.
 */
void printVersion() {
    std::cout << "sandbox version 1.0.0\n"
              << "Build with C++20\n\n"
              << "Copyright (c) 2025 lotuschain.org\n";
}

/**
 * @brief Register all default modules with the manager.
 * @param manager The sandbox manager.
 */
void registerDefaultModules(SandboxManager& manager) {
    // Register core modules
    manager.registerModule(std::make_unique<Namespaces>());
    manager.registerModule(std::make_unique<Cgroups>());
    manager.registerModule(std::make_unique<Seccomp>());
    manager.registerModule(std::make_unique<Caps>());
    manager.registerModule(std::make_unique<RootFS>());
    manager.registerModule(std::make_unique<Mounts>());

    // Register AI module
    manager.registerModule(std::make_unique<AIAgent>());
}

/**
 * @brief Parse command line arguments.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @param configPath Output path to config file.
 * @param sandboxName Output sandbox name.
 * @param enableAI Output AI enable flag.
 * @param command Output command to execute.
 * @return true if parsing succeeded.
 */
bool parseArgs(int argc, char* argv[],
               std::string& configPath,
               std::string& sandboxName,
               bool& enableAI,
               std::vector<std::string>& command) {
    static struct option longOptions[] = {
        {"config", required_argument, nullptr, 'c'},
        {"name", required_argument, nullptr, 'n'},
        {"help", no_argument, nullptr, 'h'},
        {"version", no_argument, nullptr, 'v'},
        {"debug", no_argument, nullptr, 'd'},
        {"ai", no_argument, nullptr, 'a'},
        {nullptr, 0, nullptr, 0}
    };

    int opt;
    bool hasCommand = false;

    while ((opt = getopt_long(argc, argv, "c:n:hvd", longOptions, nullptr)) != -1) {
        switch (opt) {
            case 'c':
                configPath = optarg;
                break;
            case 'n':
                sandboxName = optarg;
                break;
            case 'h':
                printUsage(argv[0]);
                exit(0);
            case 'v':
                printVersion();
                exit(0);
            case 'd':
                Logger::getInstance().setLevel(LogLevel::DEBUG);
                break;
            case 'a':
                enableAI = true;
                break;
            default:
                std::cerr << "Unknown option: " << opt << "\n";
                return false;
        }
    }

    // Collect remaining arguments as command
    for (int i = optind; i < argc; ++i) {
        command.push_back(argv[i]);
        hasCommand = true;
    }

    if (!hasCommand) {
        std::cerr << "No command specified\n";
        return false;
    }

    return true;
}

/**
 * @brief Main entry point.
 */
int main(int argc, char* argv[]) {
    std::string configPath;
    std::string sandboxName = "default";
    bool enableAI = false;
    std::vector<std::string> command;

    // Parse command line arguments
    if (!parseArgs(argc, argv, configPath, sandboxName, enableAI, command)) {
        printUsage(argv[0]);
        return 1;
    }

    // Load configuration
    SandboxConfiguration config;
    if (!configPath.empty()) {
        ConfigParser parser(configPath);
        if (!parser.isValidConfigFile(configPath)) {
            std::cerr << "Invalid configuration file: " << configPath << "\n";
            return 1;
        }
        config = parser.parse();
    } else {
        // Use default configuration
        config = ConfigParser::createDefaultConfig();
    }

    // Override name if specified
    if (!sandboxName.empty()) {
        config.sandbox.name = sandboxName;
    }

    // Enable AI if requested
    if (enableAI) {
        config.ai_module.enabled = true;
    }

    // Initialize logger
    Logger::getInstance().initialize(
        stringToLogLevel(config.logging.level),
        config.logging.output,
        config.logging.log_file
    );

    SANDBOX_INFO("Starting sandbox platform");
    SANDBOX_INFO("Command: " + command[0]);

    // Create sandbox manager
    SandboxManager manager;
    manager.setConfig(config);

    // Register default modules
    registerDefaultModules(manager);

    // Update command in config
    config.sandbox.command = command;
    manager.setConfig(config);

    // Run the sandbox
    SandboxResult result = manager.run();

    // Output result
    if (result.success) {
        SANDBOX_INFO("Sandbox executed successfully");
    } else {
        SANDBOX_ERROR("Sandbox execution failed: " + result.errorMessage);
    }

    // Print output
    if (!result.stdout.empty()) {
        std::cout << result.stdout;
    }

    // Shutdown logger
    Logger::getInstance().shutdown();

    return result.exitCode;
}
