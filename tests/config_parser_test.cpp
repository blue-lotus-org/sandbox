/**
 * @file config_parser_test.cpp
 * @brief Tests for ConfigParser class.
 */

#include <gtest/gtest.h>
#include "core/ConfigParser.h"

using namespace sandbox;

TEST(ConfigParserTest, CreateDefaultConfig) {
    auto config = ConfigParser::createDefaultConfig();

    EXPECT_EQ(config.sandbox.name, "sandbox-default");
    EXPECT_EQ(config.sandbox.hostname, "sandbox-container");
    EXPECT_EQ(config.resources.memory_mb, 512);
    EXPECT_EQ(config.resources.cpu_quota_percent, 50);
    EXPECT_TRUE(config.ai_module.enabled == false);
}

TEST(ConfigParserTest, ParseValidJson) {
    std::string json = R"({
        "sandbox": {
            "name": "test-sandbox",
            "hostname": "test-container",
            "command": ["/bin/bash"]
        },
        "resources": {
            "memory_mb": 1024,
            "cpu_quota_percent": 75
        }
    })";

    ConfigParser parser(json);
    auto config = parser.parse();

    EXPECT_EQ(config.sandbox.name, "test-sandbox");
    EXPECT_EQ(config.resources.memory_mb, 1024);
    EXPECT_EQ(config.resources.cpu_quota_percent, 75);
}

TEST(ConfigParserTest, OverrideDefaults) {
    std::string json = R"({
        "sandbox": {
            "name": "custom-sandbox"
        },
        "resources": {
            "memory_mb": 2048
        }
    })";

    ConfigParser parser(json);
    auto config = parser.parse();

    // Should have default values where not overridden
    EXPECT_EQ(config.sandbox.name, "custom-sandbox");
    EXPECT_EQ(config.resources.memory_mb, 2048);
    // Should keep default for unspecified values
    EXPECT_EQ(config.resources.max_pids, 100);
}

TEST(ConfigParserTest, InvalidJson) {
    std::string invalidJson = "{ invalid json }";

    ConfigParser parser(invalidJson);
    EXPECT_THROW(parser.parse(), std::runtime_error);
}

TEST(ConfigParserTest, MissingRequiredSection) {
    std::string json = R"({
        "memory_mb": 1024
    })";

    ConfigParser parser(json);
    EXPECT_THROW(parser.parse(), std::runtime_error);
}
