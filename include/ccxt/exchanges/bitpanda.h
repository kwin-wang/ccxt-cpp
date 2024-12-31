#pragma once

#include "ccxt/exchanges/onetrading.h"

namespace ccxt {

class Bitpanda : public OneTrading {
public:
    Bitpanda();
    ~Bitpanda() override = default;

protected:
    std::string sign(const std::string& path, const std::string& api = "public",
               const std::string& method = "GET", const json& params = json::object(),
               const std::map<std::string, std::string>& headers = {}, const json& body = nullptr) override;

private:
    std::string get_signature(const std::string& timestamp, const std::string& method,
                        const std::string& path, const std::string& body = "");
    std::string get_nonce();
};

} // namespace ccxt
