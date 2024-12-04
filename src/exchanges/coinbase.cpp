#include "coinbase.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <base64.h>

namespace ccxt {

Coinbase::Coinbase() {
    id = "coinbase";
    name = "Coinbase Exchange";
    version = "v2";
    rateLimit = 100;
    
    // Initialize API endpoints
    baseUrl = "https://api.exchange.coinbase.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/41764625-63b7ffde-760a-11e8-996d-a6328fa9347a.jpg"},
        {"api", {
            {"public", "https://api.exchange.coinbase.com"},
            {"private", "https://api.exchange.coinbase.com"}
        }},
        {"www", "https://pro.coinbase.com/"},
        {"doc", {
            "https://docs.cloud.coinbase.com/exchange/docs"
        }},
        {"fees", "https://pro.coinbase.com/fees"}
    };

    initializeApiEndpoints();
}

void Coinbase::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "/products",
                "/products/{id}/ticker",
                "/products/{id}/trades",
                "/products/{id}/book",
                "/products/{id}/candles",
                "/currencies",
                "/time"
            }}
        }},
        {"private", {
            {"GET", {
                "/accounts",
                "/orders",
                "/orders/{id}",
                "/fills"
            }},
            {"POST", {
                "/orders"
            }},
            {"DELETE", {
                "/orders",
                "/orders/{id}"
            }}
        }}
    };
}

json Coinbase::fetchMarkets(const json& params) {
    json response = fetch("/products", "public", "GET", params);
    json markets = json::array();
    
    for (const auto& market : response) {
        markets.push_back({
            {"id", market["id"]},
            {"symbol", market["base_currency"] + "/" + market["quote_currency"]},
            {"base", market["base_currency"]},
            {"quote", market["quote_currency"]},
            {"baseId", market["base_currency"]},
            {"quoteId", market["quote_currency"]},
            {"active", market["status"] == "online"},
            {"precision", {
                {"amount", market["base_increment"].get<String>().find('1') - 1},
                {"price", market["quote_increment"].get<String>().find('1') - 1}
            }},
            {"limits", {
                {"amount", {
                    {"min", std::stod(market["base_min_size"].get<String>())},
                    {"max", std::stod(market["base_max_size"].get<String>())}
                }},
                {"price", {
                    {"min", std::stod(market["quote_increment"].get<String>())}
                }},
                {"cost", {
                    {"min", std::stod(market["min_market_funds"].get<String>())},
                    {"max", std::stod(market["max_market_funds"].get<String>())}
                }}
            }},
            {"info", market}
        });
    }
    
    return markets;
}

json Coinbase::fetchBalance(const json& params) {
    json response = fetch("/accounts", "private", "GET", params);
    json result = {
        {"info", response},
        {"timestamp", nullptr},
        {"datetime", nullptr}
    };
    
    for (const auto& balance : response) {
        String currency = balance["currency"];
        double available = std::stod(balance["available"].get<String>());
        double hold = std::stod(balance["hold"].get<String>());
        double total = available + hold;
        
        if (total > 0) {
            result[currency] = {
                {"free", available},
                {"used", hold},
                {"total", total}
            };
        }
    }
    
    return result;
}

json Coinbase::createOrder(const String& symbol, const String& type,
                         const String& side, double amount,
                         double price, const json& params) {
    Market market = this->market(symbol);
    
    json order = {
        {"product_id", market.id},
        {"side", side},
        {"type", type},
        {"size", std::to_string(amount)}
    };
    
    if (type == "limit") {
        if (price == 0) {
            throw InvalidOrder("For limit orders, price cannot be zero");
        }
        order["price"] = std::to_string(price);
    }
    
    return fetch("/orders", "private", "POST", order);
}

String Coinbase::sign(const String& path, const String& api,
                     const String& method, const json& params,
                     const std::map<String, String>& headers,
                     const json& body) {
    String url = baseUrl + path;
    
    if (api == "private") {
        String timestamp = getTimestamp();
        String bodyStr = body.empty() ? "" : body.dump();
        auto authHeaders = getAuthHeaders(method, path, bodyStr);
        
        for (const auto& [key, value] : authHeaders) {
            const_cast<std::map<String, String>&>(headers)[key] = value;
        }
    }
    
    if (!params.empty()) {
        std::stringstream queryString;
        bool first = true;
        for (const auto& [key, value] : params.items()) {
            if (!first) queryString << "&";
            queryString << key << "=" << value.get<String>();
            first = false;
        }
        url += "?" + queryString.str();
    }
    
    return url;
}

String Coinbase::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    return std::to_string(std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());
}

String Coinbase::createSignature(const String& timestamp, const String& method,
                               const String& requestPath, const String& body) {
    String message = timestamp + method + requestPath + body;
    
    unsigned char* hmac = nullptr;
    unsigned int hmacLen = 0;
    
    HMAC_CTX* ctx = HMAC_CTX_new();
    HMAC_Init_ex(ctx, base64_decode(secret).c_str(), base64_decode(secret).length(), EVP_sha256(), nullptr);
    HMAC_Update(ctx, (unsigned char*)message.c_str(), message.length());
    HMAC_Final(ctx, hmac, &hmacLen);
    HMAC_CTX_free(ctx);
    
    return base64_encode(hmac, hmacLen);
}

std::map<String, String> Coinbase::getAuthHeaders(const String& method,
                                                const String& requestPath,
                                                const String& body) {
    String timestamp = getTimestamp();
    String signature = createSignature(timestamp, method, requestPath, body);
    
    return {
        {"CB-ACCESS-KEY", apiKey},
        {"CB-ACCESS-SIGN", signature},
        {"CB-ACCESS-TIMESTAMP", timestamp},
        {"CB-ACCESS-PASSPHRASE", password}
    };
}

} // namespace ccxt
