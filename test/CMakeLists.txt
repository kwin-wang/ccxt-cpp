cmake_minimum_required(VERSION 3.10)

# Enable testing
enable_testing()

# Find GTest package
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Add test executable
add_executable(ccxt_tests
    base_tests.cpp
    exchange_tests.cpp
)

# Link against GTest and your library
target_link_libraries(ccxt_tests
    ${GTEST_BOTH_LIBRARIES}
    ccxt
    pthread
)

# Add tests
add_test(NAME ccxt_tests COMMAND ccxt_tests)
