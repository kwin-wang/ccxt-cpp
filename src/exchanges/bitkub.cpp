#include "bitkub.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Bitkub::Bitkub() {
    id = "bitkub";
    name = "Bitkub";
    version = "2";
    rateLimit = 1000;
    certified = true;
    pro = false;

    // Initialize API endpoints
    baseUrl = "https://api.bitkub.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/51840849/87153926-efbef500-c2c0-11ea-9842-05b63612c4b9.jpg"},
        {"api", {
            {"public", "https://api.bitkub.com/api"},
            {"private", "https://api.bitkub.com/api"}
        }},
        {"www", "https://www.bitkub.com"},
        {"doc", {
            "https://github.com/bitkub/bitkub-official-api-docs",
            "https://api.bitkub.com"
        }},
        {"referral", "https://www.bitkub.com/signup?ref=testuser"},
        {"fees", "https://www.bitkub.com/fee/cryptocurrency"}
    };

    timeframes = {
        {"1m", "1"},
        {"5m", "5"},
        {"15m", "15"},
        {"30m", "30"},
        {"1h", "60"},
        {"4h", "240"},
        {"1d", "1D"},
        {"1w", "1W"}
    };

    options = {
        {"adjustForTimeDifference", true},
        {"recvWindow", 5000},
        {"timeDifference", 0}
    };

    errorCodes = {
        {0, "No error"},
        {1, "Invalid API key"},
        {2, "Invalid signature"},
        {3, "Invalid timestamp"},
        {4, "Invalid user"},
        {5, "Invalid parameter"},
        {6, "Invalid symbol"},
        {7, "Invalid amount"},
        {8, "Invalid rate"},
        {9, "No balance or insufficient balance"},
        {10, "No market matching"},
        {11, "Order id not found"},
        {12, "Invalid order for cancellation"},
        {13, "Invalid side"},
        {14, "Invalid API permission"},
        {15, "Invalid order type"},
        {16, "Insufficient credit balance"},
        {17, "Insufficient ETH balance"},
        {18, "Order book disabled"},
        {19, "Invalid order for lookup"},
        {20, "Rate limit exceeded"}
    };

    initializeApiEndpoints();
}

void Bitkub::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "servertime",
                "market/symbols",
                "market/ticker",
                "market/trades",
                "market/bids",
                "market/asks",
                "market/books",
                "market/trading-view",
                "market/depth",
                "status",
                "market/wstoken"
            }}
        }},
        {"private", {
            {"POST", {
                "market/place-bid",
                "market/place-ask",
                "market/place-bid/test",
                "market/place-ask/test",
                "market/cancel-order",
                "market/my-open-orders",
                "market/my-order-history",
                "market/order-info",
                "crypto/addresses",
                "crypto/withdraw",
                "crypto/deposit-history",
                "crypto/withdraw-history",
                "fiat/accounts",
                "fiat/withdraw",
                "fiat/deposit-history",
                "fiat/withdraw-history",
                "user/limits",
                "user/trading-credits"
            }}
        }}
    };
}

json Bitkub::fetchMarkets(const json& params) {
    json response = fetch("/market/symbols", "public", "GET", params);
    json result = json::array();
    
    for (const auto& market : response["result"]) {
        std::string id = this->safeString(market, "symbol");
        std::string baseId = this->safeString(market, "baseAsset");
        std::string quoteId = this->safeString(market, "quoteAsset");
        std::string base = this->safeCurrencyCode(baseId);
        std::string quote = this->safeCurrencyCode(quoteId);
        std::string symbol = base + "/" + quote;
        
        result.push_back({
            {"id", id},
            {"symbol", symbol},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", true},
            {"type", "spot"},
            {"spot", true},
            {"future", false},
            {"option", false},
            {"margin", false},
            {"contract", false},
            {"precision", {
                {"amount", this->safeInteger(market, "baseAssetPrecision")},
                {"price", this->safeInteger(market, "quoteAssetPrecision")}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeFloat(market, "minAmount")},
                    {"max", this->safeFloat(market, "maxAmount")}
                }},
                {"price", {
                    {"min", this->safeFloat(market, "minPrice")},
                    {"max", this->safeFloat(market, "maxPrice")}
                }},
                {"cost", {
                    {"min", this->safeFloat(market, "minNotional")},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return result;
}

json Bitkub::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/market/wallet", "private", "POST", params);
    return parseBalance(response);
}

json Bitkub::parseBalance(const json& response) {
    json result = {{"info", response}};
    json balances = this->safeValue(response, "result", {});
    
    for (const auto& [currencyId, balance] : balances.items()) {
        std::string code = this->safeCurrencyCode(currencyId);
        std::string account = {
            {"free", this->safeFloat(balance, "available")},
            {"used", this->safeFloat(balance, "reserved")},
            {"total", this->safeFloat(balance, "total")}
        };
        result[code] = account;
    }
    
    return result;
}

json Bitkub::createOrder(const std::string& symbol, const std::string& type,
                        const std::string& side, double amount,
                        double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    std::string endpoint = (side == "buy") ? "/market/place-bid" : "/market/place-ask";
    
    json request = {
        {"sym", market["id"]},
        {"amt", this->amountToPrecision(symbol, amount)},
        {"rat", this->priceToPrecision(symbol, price)},
        {"typ", type}
    };
    
    json response = fetch(endpoint, "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response["result"], market);
}

std::string Bitkub::sign(const std::string& path, const std::string& api,
                   const std::string& method, const json& params,
                   const std::map<std::string, std::string>& headers,
                   const json& body) {
    std::string url = this->urls["api"][api] + "/" + this->version + path;
    
    if (api == "public") {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    } else {
        this->checkRequiredCredentials();
        std::string nonce = this->nonce().str();
        std::string timestamp = std::to_string(this->milliseconds());
        
        json request = this->extend({
            "ts", timestamp,
            "sig", this->config_.apiKey
        }, params);
        
        std::string payload = this->urlencode(request);
        std::string signature = this->hmac(payload, this->encode(this->config_.secret),
                                    "sha256", "hex");
        
        const_cast<std::map<std::string, std::string>&>(headers)["X-BTK-APIKEY"] = this->config_.apiKey;
        const_cast<std::map<std::string, std::string>&>(headers)["X-BTK-SIGN"] = signature;
        const_cast<std::map<std::string, std::string>&>(headers)["X-BTK-TIMESTAMP"] = timestamp;
        
        if (method == "POST") {
            const_cast<std::map<std::string, std::string>&>(headers)["Content-Type"] = "application/x-www-form-urlencoded";
            body = payload;
        } else if (!params.empty()) {
            url += "?" + payload;
        }
    }
    
    return url;
}

std::string Bitkub::createNonce() {
    return std::to_string(this->milliseconds());
}

json Bitkub::parseOrder(const json& order, const Market& market) {
    std::string timestamp = this->safeString(order, "ts");
    std::string status = this->parseOrderStatus(this->safeString(order, "status"));
    std::string symbol = nullptr;
    
    if (!market.empty()) {
        symbol = market["symbol"];
    }
    
    std::string type = this->safeString(order, "type");
    std::string side = this->safeString(order, "side");
    
    return {
        {"id", this->safeString(order, "id")},
        {"clientOrderId", nullptr},
        {"datetime", this->iso8601(timestamp)},
        {"timestamp", this->parse8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"type", type},
        {"timeInForce", nullptr},
        {"postOnly", nullptr},
        {"status", status},
        {"symbol", symbol},
        {"side", side},
        {"price", this->safeFloat(order, "rate")},
        {"stopPrice", nullptr},
        {"cost", nullptr},
        {"amount", this->safeFloat(order, "amount")},
        {"filled", this->safeFloat(order, "filled")},
        {"remaining", this->safeFloat(order, "remaining")},
        {"trades", nullptr},
        {"fee", nullptr},
        {"info", order}
    };
}

std::string Bitkub::parseOrderStatus(const std::string& status) {
    static const std::map<std::string, std::string> statuses = {
        {"filled", "closed"},
        {"partially_filled", "open"},
        {"cancelled", "canceled"},
        {"pending", "open"},
        {"expired", "expired"}
    };
    
    return this->safeString(statuses, status, status);
}

} // namespace ccxt
