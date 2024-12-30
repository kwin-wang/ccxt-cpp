cmake_minimum_required(VERSION 3.10)
project(ccxt VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CURL_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
)

# Base source files
set(BASE_SOURCES
    src/base/exchange.cpp
    src/base/errors.cpp
    src/base/precise.cpp
    src/base/websocket_client.cpp
)

# Exchange source files - only include implemented exchanges
set(EXCHANGE_SOURCES
    src/exchanges/binance.cpp
)

#
set(EXChange_WS_SOURCES
    src/exchanges/ws/binance_ws.cpp
)


# Add library
add_library(ccxt
    ${BASE_SOURCES}
    ${EXCHANGE_SOURCES}
    ${EXChange_WS_SOURCES}
)

# Link libraries
target_link_libraries(ccxt
    PRIVATE
    ${CURL_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
)

# Install
install(TARGETS ccxt
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ccxt
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# Add test subdirectory if it exists
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test)
    add_subdirectory(test)
endif()
