#pragma once

#include <string>
#include <map>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include "types.h"
#include "config.h"

namespace ccxt {

using json = nlohmann::json;
using String = std::string;

class ExchangeBase {
public:
    ExchangeBase(const Config& config = Config()) : config(config) {}
    virtual ~ExchangeBase() = default;

    // Basic exchange properties
    std::string id;
    std::string name;
    std::vector<std::string> countries;
    std::string version;
    int rateLimit;
    bool pro;
    bool certified;
    std::map<std::string, std::map<std::string, std::string>> urls;
    std::map<std::string, std::optional<bool>> has;
    std::map<std::string, std::string> timeframes;
    long long lastRestRequestTimestamp;

protected:
    Config config;
};

} // namespace ccxt
