#include "kraken.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <base64.h>

namespace ccxt {

Kraken::Kraken() {
    id = "kraken";
    name = "Kraken";
    version = "0";
    rateLimit = 3000;
    
    // Initialize API endpoints
    baseUrl = "https://api.kraken.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/51840849/76173629-fc67fb00-61b1-11ea-84fe-f2de582f58a3.jpg"},
        {"api", {
            {"public", "https://api.kraken.com"},
            {"private", "https://api.kraken.com"},
            {"websockets", "wss://ws.kraken.com"}
        }},
        {"www", "https://www.kraken.com"},
        {"doc", {
            "https://www.kraken.com/features/api",
            "https://support.kraken.com"
        }}
    };

    initializeApiEndpoints();
}

void Kraken::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "0/public/Assets",
                "0/public/AssetPairs",
                "0/public/Ticker",
                "0/public/Depth",
                "0/public/Trades",
                "0/public/OHLC",
                "0/public/Time"
            }}
        }},
        {"private", {
            {"POST", {
                "0/private/Balance",
                "0/private/TradeBalance",
                "0/private/OpenOrders",
                "0/private/ClosedOrders",
                "0/private/QueryOrders",
                "0/private/TradesHistory",
                "0/private/QueryTrades",
                "0/private/AddOrder",
                "0/private/CancelOrder"
            }}
        }}
    };
}

json Kraken::fetchMarkets(const json& params) {
    json response = fetch("/0/public/AssetPairs", "public", "GET", params);
    json markets = json::array();
    
    for (const auto& [id, market] : response["result"].items()) {
        String baseId = market["base"];
        String quoteId = market["quote"];
        String base = getCommonSymbol(baseId);
        String quote = getCommonSymbol(quoteId);
        
        markets.push_back({
            {"id", id},
            {"symbol", base + "/" + quote},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", true},
            {"precision", {
                {"amount", market["lot_decimals"]},
                {"price", market["pair_decimals"]}
            }},
            {"limits", {
                {"amount", {
                    {"min", std::stod(market["ordermin"].get<String>())}
                }}
            }},
            {"info", market}
        });
    }
    
    return markets;
}

json Kraken::fetchTicker(const String& symbol, const json& params) {
    Market market = this->market(symbol);
    json request = {{"pair", market.id}};
    json response = fetch("/0/public/Ticker", "public", "GET", request);
    
    json ticker = response["result"][market.id];
    return {
        {"symbol", symbol},
        {"timestamp", nullptr},
        {"datetime", nullptr},
        {"high", std::stod(ticker["h"][1].get<String>())},
        {"low", std::stod(ticker["l"][1].get<String>())},
        {"bid", std::stod(ticker["b"][0].get<String>())},
        {"bidVolume", std::stod(ticker["b"][2].get<String>())},
        {"ask", std::stod(ticker["a"][0].get<String>())},
        {"askVolume", std::stod(ticker["a"][2].get<String>())},
        {"vwap", std::stod(ticker["p"][1].get<String>())},
        {"open", std::stod(ticker["o"].get<String>())},
        {"close", std::stod(ticker["c"][0].get<String>())},
        {"last", std::stod(ticker["c"][0].get<String>())},
        {"baseVolume", std::stod(ticker["v"][1].get<String>())},
        {"quoteVolume", nullptr},
        {"info", ticker}
    };
}

json Kraken::fetchBalance(const json& params) {
    json response = fetch("/0/private/Balance", "private", "POST", params);
    json result = {
        {"info", response},
        {"timestamp", nullptr},
        {"datetime", nullptr}
    };
    
    for (const auto& [currency, balance] : response["result"].items()) {
        String commonCurrency = getCommonSymbol(currency);
        double total = std::stod(balance.get<String>());
        
        result[commonCurrency] = {
            {"free", total},
            {"used", 0.0},
            {"total", total}
        };
    }
    
    return result;
}

json Kraken::createOrder(const String& symbol, const String& type,
                        const String& side, double amount,
                        double price, const json& params) {
    Market market = this->market(symbol);
    
    json request = {
        {"pair", market.id},
        {"type", side},
        {"ordertype", type},
        {"volume", std::to_string(amount)}
    };
    
    if (type == "limit") {
        if (price == 0) {
            throw InvalidOrder("For limit orders, price cannot be zero");
        }
        request["price"] = std::to_string(price);
    }
    
    return fetch("/0/private/AddOrder", "private", "POST", request);
}

String Kraken::sign(const String& path, const String& api,
                   const String& method, const json& params,
                   const std::map<String, String>& headers,
                   const json& body) {
    String url = baseUrl + path;
    
    if (api == "private") {
        String nonce = getNonce();
        std::stringstream postData;
        postData << "nonce=" << nonce;
        
        for (const auto& [key, value] : params.items()) {
            postData << "&" << key << "=" << value.get<String>();
        }
        
        String signature = createSignature(path, nonce, postData.str());
        
        const_cast<std::map<String, String>&>(headers)["API-Key"] = apiKey;
        const_cast<std::map<String, String>&>(headers)["API-Sign"] = signature;
        
        return url + "?" + postData.str();
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

String Kraken::getNonce() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return std::to_string(ms.count());
}

String Kraken::createSignature(const String& path, const String& nonce,
                             const String& postData) {
    String message = nonce + postData;
    
    unsigned char* sha256 = nullptr;
    unsigned int sha256Len = 0;
    
    SHA256_CTX sha256Ctx;
    SHA256_Init(&sha256Ctx);
    SHA256_Update(&sha256Ctx, message.c_str(), message.length());
    SHA256_Final(sha256, &sha256Ctx);
    
    unsigned char* hmac = nullptr;
    unsigned int hmacLen = 0;
    
    HMAC_CTX* ctx = HMAC_CTX_new();
    HMAC_Init_ex(ctx, secret.c_str(), secret.length(), EVP_sha512(), nullptr);
    HMAC_Update(ctx, (unsigned char*)path.c_str(), path.length());
    HMAC_Update(ctx, sha256, sha256Len);
    HMAC_Final(ctx, hmac, &hmacLen);
    HMAC_CTX_free(ctx);
    
    return base64_encode(hmac, hmacLen);
}

String Kraken::getKrakenSymbol(const String& symbol) {
    // Convert common symbol to Kraken symbol
    // For example: BTC/USD -> XBTUSD
    if (symbol == "BTC/USD") return "XBTUSD";
    if (symbol == "ETH/USD") return "ETHUSD";
    return symbol;
}

String Kraken::getCommonSymbol(const String& krakenSymbol) {
    // Convert Kraken symbol to common symbol
    // For example: XXBT -> BTC
    if (krakenSymbol == "XXBT") return "BTC";
    if (krakenSymbol == "XETH") return "ETH";
    if (krakenSymbol.substr(0, 1) == "X") return krakenSymbol.substr(1);
    return krakenSymbol;
}

} // namespace ccxt
