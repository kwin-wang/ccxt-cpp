#pragma once

#include <string>
#include <map>
#include <nlohmann/json.hpp>
#include <fstream>
#include <boost/coroutine2/coroutine.hpp>
using json = nlohmann::json;
using AsyncPullType = boost::coroutines2::coroutine<json>::pull_type;
namespace ccxt {
struct Config {
    std::string apiKey;
    std::string secret;
    std::string password;
    std::map<std::string, std::string> options;
    std::string hostname;
    int rateLimit;  // default 50
    bool pro;       // default false    
    void loadRest(const std::string& filename)
    {
        std::ifstream file(filename);
        json_rest = json::parse(file);
    }

    void loadWs(const std::string& filename)
    {
        std::ifstream file(filename);
        json_ws = json::parse(file);
    }
    
    json json_rest;
    json json_ws;
    Config() = default;
};

} // namespace ccxt
