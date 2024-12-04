#include "kucoin.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <base64.h>

namespace ccxt {

KuCoin::KuCoin() {
    id = "kucoin";
    name = "KuCoin";
    version = "v2";
    rateLimit = 100;
    
    // Initialize API endpoints
    baseUrl = "https://api.kucoin.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/51840849/87295558-132aaf80-c50e-11ea-9801-a2fb0c57c799.jpg"},
        {"api", {
            {"public", "https://api.kucoin.com"},
            {"private", "https://api.kucoin.com"},
            {"futuresPublic", "https://api-futures.kucoin.com"},
            {"futuresPrivate", "https://api-futures.kucoin.com"}
        }},
        {"www", "https://www.kucoin.com"},
        {"doc", {
            "https://docs.kucoin.com"
        }},
        {"fees", "https://www.kucoin.com/vip/level"}
    };

    timeframes = {
        {"1m", "1min"},
        {"3m", "3min"},
        {"5m", "5min"},
        {"15m", "15min"},
        {"30m", "30min"},
        {"1h", "1hour"},
        {"2h", "2hour"},
        {"4h", "4hour"},
        {"6h", "6hour"},
        {"8h", "8hour"},
        {"12h", "12hour"},
        {"1d", "1day"},
        {"1w", "1week"}
    };

    initializeApiEndpoints();
}

void KuCoin::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "api/v1/market/orderbook/level{level}",
                "api/v1/market/histories",
                "api/v1/market/candles",
                "api/v1/market/stats",
                "api/v1/symbols",
                "api/v1/currencies",
                "api/v1/prices"
            }}
        }},
        {"private", {
            {"GET", {
                "api/v1/accounts",
                "api/v1/orders",
                "api/v1/orders/{orderId}",
                "api/v1/fills"
            }},
            {"POST", {
                "api/v1/orders",
                "api/v1/orders/multi"
            }},
            {"DELETE", {
                "api/v1/orders/{orderId}",
                "api/v1/orders"
            }}
        }}
    };
}

json KuCoin::fetchMarkets(const json& params) {
    json response = fetch("/api/v1/symbols", "public", "GET", params);
    json markets = json::array();
    
    for (const auto& market : response["data"]) {
        markets.push_back({
            {"id", market["symbol"]},
            {"symbol", market["baseCurrency"] + "/" + market["quoteCurrency"]},
            {"base", market["baseCurrency"]},
            {"quote", market["quoteCurrency"]},
            {"baseId", market["baseCurrency"]},
            {"quoteId", market["quoteCurrency"]},
            {"active", market["enableTrading"]},
            {"precision", {
                {"amount", std::stoi(market["baseIncrement"].get<String>())},
                {"price", std::stoi(market["priceIncrement"].get<String>())}
            }},
            {"limits", {
                {"amount", {
                    {"min", std::stod(market["baseMinSize"].get<String>())},
                    {"max", std::stod(market["baseMaxSize"].get<String>())}
                }},
                {"price", {
                    {"min", std::stod(market["priceIncrement"].get<String>())}
                }},
                {"cost", {
                    {"min", std::stod(market["quoteMinSize"].get<String>())},
                    {"max", std::stod(market["quoteMaxSize"].get<String>())}
                }}
            }},
            {"info", market}
        });
    }
    
    return markets;
}

json KuCoin::fetchTicker(const String& symbol, const json& params) {
    Market market = this->market(symbol);
    json response = fetch("/api/v1/market/stats", "public", "GET", {{"symbol", market.id}});
    json ticker = response["data"];
    
    return {
        {"symbol", symbol},
        {"timestamp", nullptr},
        {"datetime", nullptr},
        {"high", std::stod(ticker["high"].get<String>())},
        {"low", std::stod(ticker["low"].get<String>())},
        {"bid", std::stod(ticker["buy"].get<String>())},
        {"bidVolume", nullptr},
        {"ask", std::stod(ticker["sell"].get<String>())},
        {"askVolume", nullptr},
        {"vwap", nullptr},
        {"open", std::stod(ticker["open"].get<String>())},
        {"close", std::stod(ticker["last"].get<String>())},
        {"last", std::stod(ticker["last"].get<String>())},
        {"previousClose", nullptr},
        {"change", std::stod(ticker["changePrice"].get<String>())},
        {"percentage", std::stod(ticker["changeRate"].get<String>()) * 100},
        {"average", nullptr},
        {"baseVolume", std::stod(ticker["vol"].get<String>())},
        {"quoteVolume", std::stod(ticker["volValue"].get<String>())},
        {"info", ticker}
    };
}

json KuCoin::fetchBalance(const json& params) {
    json response = fetch("/api/v1/accounts", "private", "GET", params);
    json result = {
        {"info", response},
        {"timestamp", nullptr},
        {"datetime", nullptr}
    };
    
    for (const auto& balance : response["data"]) {
        String currency = balance["currency"];
        double total = std::stod(balance["balance"].get<String>());
        double used = std::stod(balance["holds"].get<String>());
        double free = total - used;
        
        result[currency] = {
            {"free", free},
            {"used", used},
            {"total", total}
        };
    }
    
    return result;
}

json KuCoin::createOrder(const String& symbol, const String& type,
                        const String& side, double amount,
                        double price, const json& params) {
    Market market = this->market(symbol);
    
    json request = {
        {"clientOid", getTimestamp()},
        {"side", side},
        {"symbol", market.id},
        {"type", type},
        {"size", std::to_string(amount)}
    };
    
    if (type == "limit") {
        if (price == 0) {
            throw InvalidOrder("For limit orders, price cannot be zero");
        }
        request["price"] = std::to_string(price);
    }
    
    return fetch("/api/v1/orders", "private", "POST", request);
}

String KuCoin::sign(const String& path, const String& api,
                    const String& method, const json& params,
                    const std::map<String, String>& headers,
                    const json& body) {
    String endpoint = path;
    String url = baseUrl + endpoint;
    
    if (api == "private") {
        String timestamp = getTimestamp();
        String bodyStr = body.empty() ? "" : body.dump();
        auto authHeaders = getAuthHeaders(method, endpoint, bodyStr);
        
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

String KuCoin::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return std::to_string(ms.count());
}

String KuCoin::createSignature(const String& timestamp, const String& method,
                             const String& endpoint, const String& body) {
    String message = timestamp + method + endpoint + body;
    
    unsigned char* hmac = nullptr;
    unsigned int hmacLen = 0;
    
    HMAC_CTX* ctx = HMAC_CTX_new();
    HMAC_Init_ex(ctx, secret.c_str(), secret.length(), EVP_sha256(), nullptr);
    HMAC_Update(ctx, (unsigned char*)message.c_str(), message.length());
    HMAC_Final(ctx, hmac, &hmacLen);
    HMAC_CTX_free(ctx);
    
    return base64_encode(hmac, hmacLen);
}

std::map<String, String> KuCoin::getAuthHeaders(const String& method,
                                               const String& endpoint,
                                               const String& body) {
    String timestamp = getTimestamp();
    String signature = createSignature(timestamp, method, endpoint, body);
    
    return {
        {"KC-API-KEY", apiKey},
        {"KC-API-SIGN", signature},
        {"KC-API-TIMESTAMP", timestamp},
        {"KC-API-PASSPHRASE", password},
        {"KC-API-KEY-VERSION", "2"}  // API key version 2
    };
}

String KuCoin::getKucoinSymbol(const String& symbol) {
    return symbol;  // KuCoin uses standard symbol format
}

} // namespace ccxt
