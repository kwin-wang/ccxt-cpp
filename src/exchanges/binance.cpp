#include "binance.h"
#include <chrono>
#include <sstream>
#include <iomanip>

namespace ccxt {

Binance::Binance() {
    id = "binance";
    name = "Binance";
    version = "v3";
    rateLimit = 50;
    
    // Initialize API endpoints
    baseUrl = "https://api.binance.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/29604020-d5483cdc-87ee-11e7-94c7-d1a8d9169293.jpg"},
        {"api", {
            {"public", "https://api.binance.com"},
            {"private", "https://api.binance.com"},
            {"v3", "https://api.binance.com"},
            {"v1", "https://api.binance.com"}
        }},
        {"www", "https://www.binance.com"},
        {"doc", {
            "https://binance-docs.github.io/apidocs/spot/en"
        }},
        {"fees", "https://www.binance.com/en/fee/schedule"}
    };

    initializeApiEndpoints();
}

void Binance::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "ping",
                "time",
                "exchangeInfo",
                "depth",
                "trades",
                "historicalTrades",
                "aggTrades",
                "klines",
                "ticker/24hr",
                "ticker/price",
                "ticker/bookTicker"
            }}
        }},
        {"private", {
            {"GET", {
                "account",
                "myTrades",
                "openOrders",
                "allOrders",
                "order"
            }},
            {"POST", {
                "order",
                "order/test"
            }},
            {"DELETE", {
                "order"
            }}
        }}
    };
}

json Binance::fetchMarkets(const json& params) {
    json response = fetch("/api/v3/exchangeInfo", "public", "GET", params);
    json markets = json::array();
    
    for (const auto& market : response["symbols"]) {
        if (market["status"] == "TRADING") {
            markets.push_back({
                {"id", market["symbol"]},
                {"symbol", market["baseAsset"] + "/" + market["quoteAsset"]},
                {"base", market["baseAsset"]},
                {"quote", market["quoteAsset"]},
                {"baseId", market["baseAsset"]},
                {"quoteId", market["quoteAsset"]},
                {"active", true},
                {"precision", {
                    {"amount", market["baseAssetPrecision"]},
                    {"price", market["quotePrecision"]}
                }},
                {"limits", {
                    {"amount", {
                        {"min", std::stod(market["filters"][2]["minQty"])},
                        {"max", std::stod(market["filters"][2]["maxQty"])}
                    }},
                    {"price", {
                        {"min", std::stod(market["filters"][0]["minPrice"])},
                        {"max", std::stod(market["filters"][0]["maxPrice"])}
                    }},
                    {"cost", {
                        {"min", std::stod(market["filters"][3]["minNotional"])}
                    }}
                }},
                {"info", market}
            });
        }
    }
    
    return markets;
}

json Binance::fetchTicker(const String& symbol, const json& params) {
    Market market = this->market(symbol);
    json response = fetch("/api/v3/ticker/24hr", "public", "GET", {{"symbol", market.id}});
    
    return {
        {"symbol", symbol},
        {"timestamp", std::stoll(response["closeTime"].get<String>())},
        {"datetime", ""},  // TODO: Convert timestamp to ISO8601
        {"high", std::stod(response["highPrice"].get<String>())},
        {"low", std::stod(response["lowPrice"].get<String>())},
        {"bid", std::stod(response["bidPrice"].get<String>())},
        {"bidVolume", std::stod(response["bidQty"].get<String>())},
        {"ask", std::stod(response["askPrice"].get<String>())},
        {"askVolume", std::stod(response["askQty"].get<String>())},
        {"vwap", std::stod(response["weightedAvgPrice"].get<String>())},
        {"open", std::stod(response["openPrice"].get<String>())},
        {"close", std::stod(response["lastPrice"].get<String>())},
        {"last", std::stod(response["lastPrice"].get<String>())},
        {"previousClose", std::stod(response["prevClosePrice"].get<String>())},
        {"change", std::stod(response["priceChange"].get<String>())},
        {"percentage", std::stod(response["priceChangePercent"].get<String>())},
        {"average", (std::stod(response["openPrice"].get<String>()) + std::stod(response["lastPrice"].get<String>())) / 2},
        {"baseVolume", std::stod(response["volume"].get<String>())},
        {"quoteVolume", std::stod(response["quoteVolume"].get<String>())},
        {"info", response}
    };
}

json Binance::fetchBalance(const json& params) {
    json response = fetch("/api/v3/account", "private", "GET", params);
    json result = {
        {"info", response},
        {"timestamp", nullptr},
        {"datetime", nullptr}
    };
    
    for (const auto& balance : response["balances"]) {
        String currency = balance["asset"].get<String>();
        double free = std::stod(balance["free"].get<String>());
        double used = std::stod(balance["locked"].get<String>());
        double total = free + used;
        
        if (total > 0) {
            result[currency] = {
                {"free", free},
                {"used", used},
                {"total", total}
            };
        }
    }
    
    return result;
}

json Binance::createOrder(const String& symbol, const String& type,
                         const String& side, double amount,
                         double price, const json& params) {
    Market market = this->market(symbol);
    String orderType = type.substr(0, 1) + type.substr(1);  // Capitalize first letter
    
    json order = {
        {"symbol", market.id},
        {"type", orderType},
        {"side", side},
        {"quantity", std::to_string(amount)}
    };
    
    if (type == "limit") {
        if (price == 0) {
            throw InvalidOrder("For limit orders, price cannot be zero");
        }
        order["price"] = std::to_string(price);
        order["timeInForce"] = "GTC";
    }
    
    return fetch("/api/v3/order", "private", "POST", order);
}

String Binance::sign(const String& path, const String& api,
                    const String& method, const json& params,
                    const std::map<String, String>& headers,
                    const json& body) {
    String url = baseUrl + path;
    
    if (api == "private") {
        String timestamp = getTimestamp();
        json newParams = params;
        newParams["timestamp"] = timestamp;
        
        std::stringstream queryString;
        bool first = true;
        for (const auto& [key, value] : newParams.items()) {
            if (!first) queryString << "&";
            queryString << key << "=" << value.get<String>();
            first = false;
        }
        
        String signature = createSignature(queryString.str());
        url += "?" + queryString.str() + "&signature=" + signature;
    } else if (!params.empty()) {
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

String Binance::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return std::to_string(ms.count());
}

String Binance::createSignature(const String& queryString) {
    unsigned char* digest = nullptr;
    unsigned int digestLen = 0;
    
    HMAC_CTX* ctx = HMAC_CTX_new();
    HMAC_Init_ex(ctx, secret.c_str(), secret.length(), EVP_sha256(), nullptr);
    HMAC_Update(ctx, (unsigned char*)queryString.c_str(), queryString.length());
    HMAC_Final(ctx, digest, &digestLen);
    HMAC_CTX_free(ctx);
    
    std::stringstream ss;
    for(unsigned int i = 0; i < digestLen; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
    }
    
    return ss.str();
}

} // namespace ccxt
