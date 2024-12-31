#include "kraken.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <base64.h>
#include <future>

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

// Async Market Data Methods
AsyncPullType Kraken::fetchMarketsAsync(const Json& params) const {
    return std::async(std::launch::async, [this, params]() {
        return this->fetchMarketsImpl(params);
    });
}

AsyncPullType Kraken::fetchCurrenciesAsync(const Json& params) const {
    return std::async(std::launch::async, [this, params]() {
        return this->fetchCurrenciesImpl(params);
    });
}

AsyncPullType Kraken::fetchTickerAsync(const std::string& symbol, const Json& params) const {
    return std::async(std::launch::async, [this, symbol, params]() {
        return this->fetchTickerImpl(symbol, params);
    });
}

AsyncPullType Kraken::fetchTickersAsync(const std::vector<std::string>& symbols, const Json& params) const {
    return std::async(std::launch::async, [this, symbols, params]() {
        return this->fetchTickersImpl(symbols, params);
    });
}

AsyncPullType Kraken::fetchOrderBookAsync(const std::string& symbol, const std::optional<int>& limit, const Json& params) const {
    return std::async(std::launch::async, [this, symbol, limit, params]() {
        return this->fetchOrderBookImpl(symbol, limit, params);
    });
}

AsyncPullType Kraken::fetchOHLCVAsync(const std::string& symbol, const std::string& timeframe,
                                        const std::optional<long long>& since, const std::optional<int>& limit,
                                        const Json& params) const {
    return std::async(std::launch::async, [this, symbol, timeframe, since, limit, params]() {
        return this->fetchOHLCVImpl(symbol, timeframe, since, limit, params);
    });
}

AsyncPullType Kraken::fetchTradesAsync(const std::string& symbol, const std::optional<long long>& since,
                                         const std::optional<int>& limit, const Json& params) const {
    return std::async(std::launch::async, [this, symbol, since, limit, params]() {
        return this->fetchTradesImpl(symbol, since, limit, params);
    });
}

// Async Trading Methods
AsyncPullType Kraken::createOrderAsync(const std::string& symbol, const std::string& type,
                                         const std::string& side, double amount,
                                         const std::optional<double>& price, const Json& params) {
    return std::async(std::launch::async, [this, symbol, type, side, amount, price, params]() {
        return this->createOrderImpl(symbol, type, side, amount, price, params);
    });
}

AsyncPullType Kraken::cancelOrderAsync(const std::string& id, const std::string& symbol, const Json& params) {
    return std::async(std::launch::async, [this, id, symbol, params]() {
        return this->cancelOrderImpl(id, symbol, params);
    });
}

AsyncPullType Kraken::fetchOrderAsync(const std::string& id, const std::string& symbol, const Json& params) const {
    return std::async(std::launch::async, [this, id, symbol, params]() {
        return this->fetchOrderImpl(id, symbol, params);
    });
}

AsyncPullType Kraken::fetchOpenOrdersAsync(const std::string& symbol, const std::optional<long long>& since,
                                             const std::optional<int>& limit, const Json& params) const {
    return std::async(std::launch::async, [this, symbol, since, limit, params]() {
        return this->fetchOpenOrdersImpl(symbol, since, limit, params);
    });
}

AsyncPullType Kraken::fetchClosedOrdersAsync(const std::string& symbol, const std::optional<long long>& since,
                                               const std::optional<int>& limit, const Json& params) const {
    return std::async(std::launch::async, [this, symbol, since, limit, params]() {
        return this->fetchClosedOrdersImpl(symbol, since, limit, params);
    });
}

AsyncPullType Kraken::fetchMyTradesAsync(const std::string& symbol, const std::optional<long long>& since,
                                           const std::optional<int>& limit, const Json& params) const {
    return std::async(std::launch::async, [this, symbol, since, limit, params]() {
        return this->fetchMyTradesImpl(symbol, since, limit, params);
    });
}

// Async Account Methods
AsyncPullType Kraken::fetchBalanceAsync(const Json& params) const {
    return std::async(std::launch::async, [this, params]() {
        return this->fetchBalanceImpl(params);
    });
}

AsyncPullType Kraken::fetchDepositAddressAsync(const std::string& code, const std::optional<std::string>& network,
                                                 const Json& params) const {
    return std::async(std::launch::async, [this, code, network, params]() {
        return this->fetchDepositAddressImpl(code, network, params);
    });
}

AsyncPullType Kraken::fetchDepositsAsync(const std::optional<std::string>& code, const std::optional<long long>& since,
                                           const std::optional<int>& limit, const Json& params) const {
    return std::async(std::launch::async, [this, code, since, limit, params]() {
        return this->fetchDepositsImpl(code, since, limit, params);
    });
}

AsyncPullType Kraken::fetchWithdrawalsAsync(const std::optional<std::string>& code, const std::optional<long long>& since,
                                             const std::optional<int>& limit, const Json& params) const {
    return std::async(std::launch::async, [this, code, since, limit, params]() {
        return this->fetchWithdrawalsImpl(code, since, limit, params);
    });
}

json Kraken::fetchMarkets(const json& params) {
    json response = fetch("/0/public/AssetPairs", "public", "GET", params);
    json markets = json::array();
    
    for (const auto& [id, market] : response["result"].items()) {
        std::string baseId = market["base"];
        std::string quoteId = market["quote"];
        std::string base = getCommonSymbol(baseId);
        std::string quote = getCommonSymbol(quoteId);
        
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
                    {"min", std::stod(market["ordermin"].get<std::string>())}
                }}
            }},
            {"info", market}
        });
    }
    
    return markets;
}

json Kraken::fetchTicker(const std::string& symbol, const json& params) {
    Market market = this->market(symbol);
    json request = {{"pair", market.id}};
    json response = fetch("/0/public/Ticker", "public", "GET", request);
    
    json ticker = response["result"][market.id];
    return {
        {"symbol", symbol},
        {"timestamp", nullptr},
        {"datetime", nullptr},
        {"high", std::stod(ticker["h"][1].get<std::string>())},
        {"low", std::stod(ticker["l"][1].get<std::string>())},
        {"bid", std::stod(ticker["b"][0].get<std::string>())},
        {"bidVolume", std::stod(ticker["b"][2].get<std::string>())},
        {"ask", std::stod(ticker["a"][0].get<std::string>())},
        {"askVolume", std::stod(ticker["a"][2].get<std::string>())},
        {"vwap", std::stod(ticker["p"][1].get<std::string>())},
        {"open", std::stod(ticker["o"].get<std::string>())},
        {"close", std::stod(ticker["c"][0].get<std::string>())},
        {"last", std::stod(ticker["c"][0].get<std::string>())},
        {"baseVolume", std::stod(ticker["v"][1].get<std::string>())},
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
        std::string commonCurrency = getCommonSymbol(currency);
        double total = std::stod(balance.get<std::string>());
        
        result[commonCurrency] = {
            {"free", total},
            {"used", 0.0},
            {"total", total}
        };
    }
    
    return result;
}

json Kraken::createOrder(const std::string& symbol, const std::string& type,
                        const std::string& side, double amount,
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

std::string Kraken::sign(const std::string& path, const std::string& api,
                   const std::string& method, const json& params,
                   const std::map<std::string, std::string>& headers,
                   const json& body) {
    std::string url = baseUrl + path;
    
    if (api == "private") {
        std::string nonce = getNonce();
        std::stringstream postData;
        postData << "nonce=" << nonce;
        
        for (const auto& [key, value] : params.items()) {
            postData << "&" << key << "=" << value.get<std::string>();
        }
        
        std::string signature = createSignature(path, nonce, postData.str());
        
        const_cast<std::map<std::string, std::string>&>(headers)["API-Key"] = apiKey;
        const_cast<std::map<std::string, std::string>&>(headers)["API-Sign"] = signature;
        
        return url + "?" + postData.str();
    } else if (!params.empty()) {
        std::stringstream querystd::string;
        bool first = true;
        for (const auto& [key, value] : params.items()) {
            if (!first) querystd::string << "&";
            querystd::string << key << "=" << value.get<std::string>();
            first = false;
        }
        url += "?" + querystd::string.str();
    }
    
    return url;
}

std::string Kraken::getNonce() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return std::to_string(ms.count());
}

std::string Kraken::createSignature(const std::string& path, const std::string& nonce,
                             const std::string& postData) {
    std::string message = nonce + postData;
    
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

std::string Kraken::getKrakenSymbol(const std::string& symbol) {
    // Convert common symbol to Kraken symbol
    // For example: BTC/USD -> XBTUSD
    if (symbol == "BTC/USD") return "XBTUSD";
    if (symbol == "ETH/USD") return "ETHUSD";
    return symbol;
}

std::string Kraken::getCommonSymbol(const std::string& krakenSymbol) {
    // Convert Kraken symbol to common symbol
    // For example: XXBT -> BTC
    if (krakenSymbol == "XXBT") return "BTC";
    if (krakenSymbol == "XETH") return "ETH";
    if (krakenSymbol.substr(0, 1) == "X") return krakenSymbol.substr(1);
    return krakenSymbol;
}

} // namespace ccxt
