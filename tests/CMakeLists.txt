# Fetch GoogleTest
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

# Test executable
add_executable(sandbox_tests
    test_main.cpp
    config_parser_test.cpp
    module_test.cpp
)

target_link_libraries(sandbox_tests PRIVATE
    sandbox
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(sandbox_tests)
