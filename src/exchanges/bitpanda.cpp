#include "ccxt/exchanges/bitpanda.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Bitpanda::Bitpanda() : onetrading() {
    id = "bitpanda";
    name = "Bitpanda Pro";
    version = "v1";
    certified = true;
    pro = true;
    hasPublicAPI = true;
    hasPrivateAPI = true;
    hasFiatAPI = true;
    hasMarginAPI = false;
    hasFuturesAPI = false;

    // Initialize URLs
    baseUrl = "https://api.exchange.bitpanda.com/public/v1";
    urls = {
        {"logo", "https://user-images.githubusercontent.com/51840849/87591171-9a377d80-c6f0-11ea-94ac-97a126eac3bc.jpg"},
        {"api", {
            {"public", "https://api.exchange.bitpanda.com/public/v1"},
            {"private", "https://api.exchange.bitpanda.com/public/v1"}
        }},
        {"www", "https://www.bitpanda.com/en/pro"},
        {"doc", {
            "https://developers.bitpanda.com/exchange/",
            "https://api.exchange.bitpanda.com/public/v1"
        }},
        {"fees", "https://www.bitpanda.com/en/pro/fees"}
    };

    // Set up API endpoints
    api = {
        {"public", {
            {"GET", {
                "time",
                "currencies",
                "candlesticks/{instrument_code}",
                "fees",
                "instruments",
                "order-book/{instrument_code}",
                "market-ticker",
                "market-ticker/{instrument_code}",
                "price-ticks/{instrument_code}",
                "trades/{instrument_code}"
            }}
        }},
        {"private", {
            {"GET", {
                "account/balances",
                "account/deposit/crypto/{currency}",
                "account/deposit/fiat/EUR",
                "account/deposits",
                "account/orders",
                "account/orders/{order_id}",
                "account/orders/trades",
                "account/trades",
                "account/trading-volume",
                "account/withdrawals"
            }},
            {"POST", {
                "account/deposit/crypto",
                "account/withdraw/crypto",
                "account/withdraw/fiat",
                "account/orders"
            }},
            {"DELETE", {
                "account/orders",
                "account/orders/{order_id}"
            }}
        }}
    };

    // Initialize timeframes for OHLCV data
    timeframes = {
        {"1m", "1"},
        {"5m", "5"},
        {"15m", "15"},
        {"30m", "30"},
        {"1h", "60"},
        {"4h", "240"},
        {"1d", "1D"},
        {"1w", "1W"},
        {"1M", "1M"}
    };
}

void Bitpanda::sign(Request& request, const std::string& path, const std::string& api,
                   const std::string& method, const json& params, const json& headers,
                   const json& body) {
    if (api == "private") {
        if (this->apiKey.empty()) {
            throw AuthenticationError("Authentication failed: API key required for private endpoints");
        }

        std::string nonce = get_nonce();
        std::string bodyStr = body.dump();
        std::string signature = get_signature(nonce, method, path, bodyStr);

        request.headers["ACCESS-KEY"] = this->apiKey;
        request.headers["ACCESS-TIMESTAMP"] = nonce;
        request.headers["ACCESS-SIGNATURE"] = signature;
        request.headers["Content-Type"] = "application/json";
    }
}

std::string Bitpanda::get_signature(const std::string& timestamp, const std::string& method,
                                  const std::string& path, const std::string& body) {
    std::string message = timestamp + method + path + body;
    unsigned char* digest = HMAC(EVP_sha256(),
                               this->secret.c_str(), this->secret.length(),
                               (unsigned char*)message.c_str(), message.length(),
                               nullptr, nullptr);
    
    std::stringstream ss;
    for(int i = 0; i < 32; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
    }
    return ss.str();
}

std::string Bitpanda::get_nonce() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return std::to_string(ms.count());
}

} // namespace ccxt
