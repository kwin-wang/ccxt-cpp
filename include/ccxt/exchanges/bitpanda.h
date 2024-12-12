#pragma once

#include "ccxt/exchanges/onetrading.h"

namespace ccxt {

class Bitpanda : public OneTrading {
public:
    Bitpanda();
    ~Bitpanda() override = default;

protected:
    String sign(const String& path, const String& api = "public",
               const String& method = "GET", const json& params = json::object(),
               const std::map<String, String>& headers = {}, const json& body = nullptr) override;

private:
    String get_signature(const String& timestamp, const String& method,
                        const String& path, const String& body = "");
    String get_nonce();
};

} // namespace ccxt
