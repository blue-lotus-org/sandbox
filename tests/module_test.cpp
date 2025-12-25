/**
 * @file module_test.cpp
 * @brief Tests for module classes.
 */

#include <gtest/gtest.h>
#include "modules/interface/IModule.h"
#include "modules/isolation/Namespaces.h"
#include "modules/isolation/Cgroups.h"
#include "modules/security/Caps.h"
#include "core/ConfigParser.h"

using namespace sandbox;

TEST(ModuleTest, NamespacesGetInfo) {
    Namespaces ns;
    EXPECT_EQ(ns.getName(), "namespaces");
    EXPECT_EQ(ns.getVersion(), "1.0.0");
    EXPECT_EQ(ns.getType(), "isolation");
    EXPECT_FALSE(ns.isEnabled() == false);  // Always enabled
    EXPECT_FALSE(ns.getDependencies().size() > 0);  // No dependencies
}

TEST(ModuleTest, CgroupsGetInfo) {
    Cgroups cg;
    EXPECT_EQ(cg.getName(), "cgroups");
    EXPECT_EQ(cg.getVersion(), "1.0.0");
    EXPECT_EQ(cg.getType(), "isolation");
    EXPECT_FALSE(cg.isEnabled() == false);
}

TEST(ModuleTest, ModuleStateTransitions) {
    Namespaces ns;
    ConfigParser parser(ConfigParser::createDefaultConfig());
    auto config = parser.parse();

    // Initial state
    EXPECT_EQ(ns.getState(), ModuleState::UNINITIALIZED);

    // Initialize
    bool result = ns.initialize(config);
    EXPECT_TRUE(result);
    EXPECT_EQ(ns.getState(), ModuleState::INITIALIZED);

    // Cleanup
    result = ns.cleanup();
    EXPECT_TRUE(result);
    EXPECT_EQ(ns.getState(), ModuleState::STOPPED);
}

TEST(ConfigParserTest, UIDMapParsing) {
    std::string json = R"({
        "sandbox": {
            "name": "test",
            "command": ["/bin/bash"]
        },
        "resources": {
            "memory_mb": 512
        },
        "isolation": {
            "uid_map": {
                "host_uid": 1000,
                "container_uid": 0,
                "count": 1
            },
            "gid_map": {
                "host_gid": 1000,
                "container_gid": 0,
                "count": 1
            }
        }
    })";

    ConfigParser parser(json);
    auto config = parser.parse();

    EXPECT_EQ(config.isolation.uid_map.host_uid, 1000);
    EXPECT_EQ(config.isolation.uid_map.container_uid, 0);
    EXPECT_EQ(config.isolation.uid_map.count, 1);
    EXPECT_EQ(config.isolation.gid_map.host_gid, 1000);
    EXPECT_EQ(config.isolation.gid_map.container_gid, 0);
    EXPECT_EQ(config.isolation.gid_map.count, 1);
}

TEST(ConfigParserTest, CapabilitiesParsing) {
    std::string json = R"({
        "sandbox": {
            "name": "test",
            "command": ["/bin/bash"]
        },
        "resources": {
            "memory_mb": 512
        },
        "security": {
            "capabilities": ["CAP_NET_BIND_SERVICE", "CAP_SYS_TIME"]
        }
    })";

    ConfigParser parser(json);
    auto config = parser.parse();

    EXPECT_EQ(config.security.capabilities.size(), 2);
    EXPECT_EQ(config.security.capabilities[0], "CAP_NET_BIND_SERVICE");
    EXPECT_EQ(config.security.capabilities[1], "CAP_SYS_TIME");
}

TEST(ConfigParserTest, BindMountsParsing) {
    std::string json = R"({
        "sandbox": {
            "name": "test",
            "command": ["/bin/bash"]
        },
        "resources": {
            "memory_mb": 512
        },
        "mounts": {
            "bind_mounts": [
                {"source": "/tmp", "target": "/tmp", "read_only": false},
                {"source": "/data", "target": "/data", "read_only": true}
            ]
        }
    })";

    ConfigParser parser(json);
    auto config = parser.parse();

    EXPECT_EQ(config.mounts.bind_mounts.size(), 2);
    EXPECT_EQ(config.mounts.bind_mounts[0].source, "/tmp");
    EXPECT_FALSE(config.mounts.bind_mounts[0].read_only);
    EXPECT_TRUE(config.mounts.bind_mounts[1].read_only);
}
