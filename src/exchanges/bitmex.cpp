#include "bitmex.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

BitMEX::BitMEX() {
    id = "bitmex";
    name = "BitMEX";
    version = "v1";
    rateLimit = 2000;
    testnet = false;  // Set to true for testnet

    // Initialize API endpoints
    baseUrl = testnet ? "https://testnet.bitmex.com" : "https://www.bitmex.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/27766319-f653c6e6-5ed4-11e7-933d-f0bc3699ae8f.jpg"},
        {"api", {
            {"public", baseUrl},
            {"private", baseUrl}
        }},
        {"www", "https://www.bitmex.com"},
        {"test", "https://testnet.bitmex.com"},
        {"doc", {
            "https://www.bitmex.com/app/apiOverview",
            "https://github.com/BitMEX/api-connectors/tree/master/official-http"
        }},
        {"fees", "https://www.bitmex.com/app/fees"}
    };

    timeframes = {
        {"1m", "1m"},
        {"5m", "5m"},
        {"1h", "1h"},
        {"1d", "1d"}
    };

    initializeApiEndpoints();
}

void BitMEX::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "api/v1/announcement",
                "api/v1/instrument",
                "api/v1/instrument/active",
                "api/v1/instrument/activeAndIndices",
                "api/v1/instrument/activeIntervals",
                "api/v1/instrument/compositeIndex",
                "api/v1/instrument/indices",
                "api/v1/orderBook/L2",
                "api/v1/trade",
                "api/v1/trade/bucketed",
                "api/v1/quote",
                "api/v1/quote/bucketed",
                "api/v1/stats",
                "api/v1/stats/history",
                "api/v1/stats/historyUSD"
            }}
        }},
        {"private", {
            {"GET", {
                "api/v1/position",
                "api/v1/user/margin",
                "api/v1/user/wallet",
                "api/v1/user/walletHistory",
                "api/v1/execution",
                "api/v1/order",
                "api/v1/funding"
            }},
            {"POST", {
                "api/v1/order",
                "api/v1/order/bulk",
                "api/v1/position/leverage",
                "api/v1/position/isolate",
                "api/v1/position/transferMargin"
            }},
            {"PUT", {
                "api/v1/order",
                "api/v1/order/bulk"
            }},
            {"DELETE", {
                "api/v1/order",
                "api/v1/order/all"
            }}
        }}
    };
}

json BitMEX::fetchMarkets(const json& params) {
    json response = fetch("/api/v1/instrument/active", "public", "GET", params);
    json markets = json::array();
    
    for (const auto& market : response) {
        String id = market["symbol"];
        String baseId = market["underlying"];
        String quoteId = market["quoteCurrency"];
        String base = this->commonCurrencyCode(baseId);
        String quote = this->commonCurrencyCode(quoteId);
        String type = market["isInverse"] ? "inverse" : "linear";
        
        markets.push_back({
            {"id", id},
            {"symbol", base + "/" + quote},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", market["state"] == "Open"},
            {"type", type},
            {"spot", false},
            {"future", market["expiry"].is_null()},
            {"swap", !market["expiry"].is_null()},
            {"prediction", false},
            {"precision", {
                {"amount", market["lotSize"]},
                {"price", market["tickSize"]}
            }},
            {"limits", {
                {"amount", {
                    {"min", market["lotSize"]},
                    {"max", market["maxOrderQty"]}
                }},
                {"price", {
                    {"min", market["tickSize"]},
                    {"max", market["maxPrice"]}
                }},
                {"cost", {
                    {"min", 0},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return markets;
}

json BitMEX::fetchTicker(const String& symbol, const json& params) {
    json markets = this->loadMarkets();
    Market market = this->market(symbol);
    json request = {{"symbol", market.id}, {"binSize", "1d"}, {"partial", true}, {"count", 1}, {"reverse", true}};
    json response = fetch("/api/v1/trade/bucketed", "public", "GET", this->extend(request, params));
    
    json ticker = response[0];
    String timestamp = this->parse8601(ticker["timestamp"].get<String>());
    
    return {
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"high", ticker["high"]},
        {"low", ticker["low"]},
        {"bid", nullptr},
        {"bidVolume", nullptr},
        {"ask", nullptr},
        {"askVolume", nullptr},
        {"vwap", ticker["vwap"]},
        {"open", ticker["open"]},
        {"close", ticker["close"]},
        {"last", ticker["close"]},
        {"previousClose", nullptr},
        {"change", nullptr},
        {"percentage", nullptr},
        {"average", nullptr},
        {"baseVolume", ticker["volume"]},
        {"quoteVolume", ticker["volume"] * ticker["vwap"]},
        {"info", ticker}
    };
}

json BitMEX::fetchBalance(const json& params) {
    json response = fetch("/api/v1/user/margin?currency=all", "private", "GET", params);
    json result = {
        {"info", response},
        {"timestamp", nullptr},
        {"datetime", nullptr}
    };
    
    for (const auto& balance : response) {
        String currency = balance["currency"];
        double free = balance["availableMargin"];
        double total = balance["marginBalance"];
        double used = total - free;
        
        result[currency] = {
            {"free", free},
            {"used", used},
            {"total", total}
        };
    }
    
    return result;
}

json BitMEX::createOrder(const String& symbol, const String& type,
                        const String& side, double amount,
                        double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"symbol", market.id},
        {"side", side},
        {"orderQty", amount},
        {"ordType", type}
    };
    
    if (type == "limit") {
        if (price == 0) {
            throw InvalidOrder("For limit orders, price cannot be zero");
        }
        request["price"] = price;
    }
    
    json response = fetch("/api/v1/order", "private", "POST", this->extend(request, params));
    return this->parseOrder(response);
}

json BitMEX::fetchPositions(const String& symbol, const json& params) {
    this->loadMarkets();
    json request = {};
    
    if (!symbol.empty()) {
        Market market = this->market(symbol);
        request["symbol"] = market.id;
    }
    
    json response = fetch("/api/v1/position", "private", "GET", this->extend(request, params));
    return response;
}

json BitMEX::fetchFundingRate(const String& symbol, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {{"symbol", market.id}};
    
    json response = fetch("/api/v1/instrument", "public", "GET", this->extend(request, params));
    return {
        {"symbol", symbol},
        {"markPrice", response[0]["markPrice"]},
        {"indexPrice", response[0]["indicativeSettlePrice"]},
        {"interestRate", response[0]["indicativeFundingRate"]},
        {"timestamp", this->parse8601(response[0]["timestamp"].get<String>())},
        {"datetime", response[0]["timestamp"]},
        {"info", response[0]}
    };
}

String BitMEX::sign(const String& path, const String& api,
                    const String& method, const json& params,
                    const std::map<String, String>& headers,
                    const json& body) {
    String endpoint = "/" + this->implodeParams(path, params);
    String url = this->urls["api"][api] + endpoint;
    
    if (api == "private") {
        String expires = std::to_string(this->nonce() + 5000);
        String auth = method + endpoint;
        
        if (method == "POST" || method == "PUT" || method == "DELETE") {
            if (!body.empty()) {
                auth += body.dump();
            }
        }
        
        String signature = this->hmac(auth, this->secret, "sha256", "hex");
        
        const_cast<std::map<String, String>&>(headers)["api-expires"] = expires;
        const_cast<std::map<String, String>&>(headers)["api-key"] = this->apiKey;
        const_cast<std::map<String, String>&>(headers)["api-signature"] = signature;
    }
    
    return url;
}

String BitMEX::createSignature(const String& path, const String& method,
                             const String& expires, const String& data) {
    String message = method + path + expires + data;
    
    unsigned char* hmac = nullptr;
    unsigned int hmacLen = 0;
    
    HMAC_CTX* ctx = HMAC_CTX_new();
    HMAC_Init_ex(ctx, secret.c_str(), secret.length(), EVP_sha256(), nullptr);
    HMAC_Update(ctx, (unsigned char*)message.c_str(), message.length());
    HMAC_Final(ctx, hmac, &hmacLen);
    HMAC_CTX_free(ctx);
    
    return this->toHex(hmac, hmacLen);
}

String BitMEX::getBitmexSymbol(const String& symbol) {
    if (symbol.empty()) return symbol;
    Market market = this->market(symbol);
    return market.id;
}

String BitMEX::getCommonSymbol(const String& bitmexSymbol) {
    // Convert BitMEX symbol to common format
    // e.g., XBTUSD -> BTC/USD
    // This is a simplified version, you might want to expand it
    if (bitmexSymbol == "XBTUSD") return "BTC/USD";
    return bitmexSymbol;
}

} // namespace ccxt
