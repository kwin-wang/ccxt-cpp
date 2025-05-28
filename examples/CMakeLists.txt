cmake_minimum_required(VERSION 3.15)  
project(okx_ticker_example)  
  
set(CMAKE_CXX_STANDARD 17)  
set(CMAKE_CXX_STANDARD_REQUIRED ON)  
  
# 查找依赖库  
find_package(Boost REQUIRED COMPONENTS system thread)  
find_package(OpenSSL REQUIRED)  
find_package(nlohmann_json REQUIRED)  
find_package(CURL REQUIRED)  
  
# 添加主项目  
add_subdirectory(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/ccxt)  
  
# 包含头文件目录  
include_directories(  
    ${CMAKE_SOURCE_DIR}/include  
    ${CMAKE_SOURCE_DIR}  
    ${CURL_INCLUDE_DIRS}  
    ${OPENSSL_INCLUDE_DIR}  
    ${Boost_INCLUDE_DIRS}  
)  
  
# 添加可执行文件  
add_executable(okx_ticker_example  
    okx_ticker_example.cpp          
    ${CMAKE_SOURCE_DIR}/src/exchanges/okx.cpp  
    ${CMAKE_SOURCE_DIR}/src/exchanges/ws/okx_ws.cpp  
    ${CMAKE_SOURCE_DIR}/src/base/websocket_client.cpp  
    ${CMAKE_SOURCE_DIR}/src/base/exchange.cpp  
)  
  
# 链接库  
target_link_libraries(okx_ticker_example  
    ccxt  
    ${Boost_LIBRARIES}  
    OpenSSL::SSL  
    OpenSSL::Crypto  
    nlohmann_json::nlohmann_json  
    ${CURL_LIBRARIES}  
    pthread  
)