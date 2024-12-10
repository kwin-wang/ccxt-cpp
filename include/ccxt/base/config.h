#pragma once

#include <string>
#include <map>

namespace ccxt {

struct Config {
    std::string apiKey;
    std::string secret;
    std::string password;
    std::map<std::string, std::string> options;

    Config() = default;
};

} // namespace ccxt
