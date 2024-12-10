#pragma once

#include <functional>
#include <map>
#include <string>
#include "config.h"
#include "exchange.h"

namespace ccxt {

class ExchangeRegistry {
public:
    using Creator = std::function<Exchange*(const Config&)>;
    
    class Factory {
    public:
        Factory(const std::string& name, Creator creator) {
            ExchangeRegistry::instance().registerExchange(name, creator);
        }
    };

    static ExchangeRegistry& instance() {
        static ExchangeRegistry registry;
        return registry;
    }

    void registerExchange(const std::string& name, Creator creator) {
        creators_[name] = creator;
    }

    Exchange* createExchange(const std::string& name, const Config& config = Config()) {
        auto it = creators_.find(name);
        if (it != creators_.end()) {
            return it->second(config);
        }
        return nullptr;
    }

private:
    ExchangeRegistry() = default;
    std::map<std::string, Creator> creators_;
};

} // namespace ccxt
