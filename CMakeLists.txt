cmake_minimum_required(VERSION 3.16)
project(sandbox VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build Type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find required packages
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)

# Fetch nlohmann/json header-only library
include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# Main executable
add_executable(sandbox
    src/main.cpp
    src/core/Logger.cpp
    src/core/ConfigParser.cpp
    src/core/SandboxManager.cpp
    src/modules/interface/IModule.cpp
    src/modules/filesystem/RootFS.cpp
    src/modules/filesystem/Mounts.cpp
    src/modules/isolation/Namespaces.cpp
    src/modules/isolation/Cgroups.cpp
    src/modules/security/Seccomp.cpp
    src/modules/security/Caps.cpp
    src/modules/ai/AIAgent.cpp
    src/utils/Syscalls.cpp
)

target_include_directories(sandbox PRIVATE
    src/
    ${CURL_INCLUDE_DIRS}
)

target_link_libraries(sandbox PRIVATE
    Threads::Threads
    ${CURL_LIBRARIES}
    nlohmann_json::nlohmann_json
)

# Enable strict warnings
target_compile_options(sandbox PRIVATE
    -Wall -Wextra -Wpedantic
    -Werror=return-type
    -Werror=uninitialized
)

# Installation
install(TARGETS sandbox DESTINATION bin)
install(DIRECTORY config/ DESTINATION etc/sandbox)
install(DIRECTORY docs/ DESTINATION share/doc/sandbox)

# Testing (optional)
enable_testing()
add_subdirectory(tests)
