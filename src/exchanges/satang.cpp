#include "satang.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Satang::Satang() {
    id = "satang";
    name = "Satang";
    version = "2";
    rateLimit = 1500;
    certified = true;
    pro = false;

    // Initialize API endpoints
    baseUrl = "https://api.satangcorp.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/128690547-2702f96f-1619-4e71-9e40-9c8921c1c485.jpg"},
        {"api", {
            {"public", "https://api.satangcorp.com/api"},
            {"private", "https://api.satangcorp.com/api"}
        }},
        {"www", "https://satangcorp.com"},
        {"doc", {
            "https://docs.satangcorp.com",
            "https://api.satangcorp.com/docs"
        }},
        {"referral", "https://satangcorp.com/exchange/signup?ref=testuser"},
        {"fees", "https://satangcorp.com/fees"}
    };

    timeframes = {
        {"1m", "1min"},
        {"5m", "5min"},
        {"15m", "15min"},
        {"30m", "30min"},
        {"1h", "1hour"},
        {"2h", "2hour"},
        {"4h", "4hour"},
        {"6h", "6hour"},
        {"12h", "12hour"},
        {"1d", "1day"},
        {"1w", "1week"},
        {"1M", "1month"}
    };

    options = {
        {"adjustForTimeDifference", true},
        {"recvWindow", 5000},
        {"timeDifference", 0}
    };

    errorCodes = {
        {1, "Invalid API key"},
        {2, "Invalid signature"},
        {3, "Invalid timestamp"},
        {4, "Invalid recvWindow"},
        {5, "Invalid parameter"},
        {6, "Invalid market"},
        {7, "Invalid side"},
        {8, "Invalid type"},
        {9, "Invalid quantity"},
        {10, "Invalid price"},
        {11, "Insufficient balance"},
        {12, "Market not available"},
        {13, "Order not found"},
        {14, "Order already canceled"},
        {15, "Order already filled"},
        {16, "Internal error"}
    };

    initializeApiEndpoints();
}

void Satang::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "v3/time",
                "v3/exchangeInfo",
                "v3/ticker/24hr",
                "v3/ticker/price",
                "v3/ticker/bookTicker",
                "v3/depth",
                "v3/trades",
                "v3/historicalTrades",
                "v3/klines",
                "v3/ping",
                "v3/status"
            }}
        }},
        {"private", {
            {"GET", {
                "v3/account",
                "v3/allOrders",
                "v3/openOrders",
                "v3/myTrades",
                "v3/depositHistory",
                "v3/withdrawHistory",
                "v3/depositAddress"
            }},
            {"POST", {
                "v3/order",
                "v3/order/test",
                "v3/withdraw"
            }},
            {"DELETE", {
                "v3/order",
                "v3/openOrders"
            }}
        }}
    };
}

json Satang::fetchMarkets(const json& params) {
    json response = fetch("/v3/exchangeInfo", "public", "GET", params);
    json result = json::array();
    
    for (const auto& market : response["symbols"]) {
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
            {"active", this->safeValue(market, "isActive", true)},
            {"type", "spot"},
            {"spot", true},
            {"future", false},
            {"option", false},
            {"margin", false},
            {"contract", false},
            {"precision", {
                {"amount", this->safeInteger(market, "baseAssetPrecision")},
                {"price", this->safeInteger(market, "quotePrecision")}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeFloat(market, "minQty")},
                    {"max", this->safeFloat(market, "maxQty")}
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

json Satang::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/v3/account", "private", "GET", params);
    return parseBalance(response);
}

json Satang::parseBalance(const json& response) {
    json result = {{"info", response}};
    json balances = this->safeValue(response, "balances", json::array());
    
    for (const auto& balance : balances) {
        std::string currencyId = this->safeString(balance, "asset");
        std::string code = this->safeCurrencyCode(currencyId);
        std::string account = {
            {"free", this->safeFloat(balance, "free")},
            {"used", this->safeFloat(balance, "locked")},
            {"total", nullptr}
        };
        account["total"] = this->sum(account["free"], account["used"]);
        result[code] = account;
    }
    
    return result;
}

json Satang::createOrder(const std::string& symbol, const std::string& type,
                        const std::string& side, double amount,
                        double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"symbol", market["id"]},
        {"side", side.toUpperCase()},
        {"type", type.toUpperCase()},
        {"quantity", this->amountToPrecision(symbol, amount)}
    };
    
    if (type == "limit") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    json response = fetch("/v3/order", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response, market);
}

std::string Satang::sign(const std::string& path, const std::string& api,
                   const std::string& method, const json& params,
                   const std::map<std::string, std::string>& headers,
                   const json& body) {
    std::string url = this->urls["api"][api] + "/" + path;
    
    if (api == "public") {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    } else {
        this->checkRequiredCredentials();
        std::string timestamp = std::to_string(this->milliseconds());
        std::string query = "";
        
        if (method == "GET") {
            if (!params.empty()) {
                query = this->urlencode(params);
                url += "?" + query;
            }
        } else {
            if (!params.empty()) {
                body = this->json(params);
                query = body;
            }
        }
        
        std::string auth = timestamp + method + path + query;
        std::string signature = this->hmac(auth, this->encode(this->config_.secret),
                                    "sha256", "hex");
        
        const_cast<std::map<std::string, std::string>&>(headers)["API-Key"] = this->config_.apiKey;
        const_cast<std::map<std::string, std::string>&>(headers)["API-Timestamp"] = timestamp;
        const_cast<std::map<std::string, std::string>&>(headers)["API-Signature"] = signature;
        
        if (method != "GET") {
            const_cast<std::map<std::string, std::string>&>(headers)["Content-Type"] = "application/json";
        }
    }
    
    return url;
}

std::string Satang::createNonce() {
    return std::to_string(this->milliseconds());
}

json Satang::parseOrder(const json& order, const Market& market) {
    std::string timestamp = this->safeString(order, "time");
    std::string status = this->parseOrderStatus(this->safeString(order, "status"));
    std::string symbol = nullptr;
    
    if (!market.empty()) {
        symbol = market["symbol"];
    }
    
    std::string type = this->safeStringLower(order, "type");
    std::string side = this->safeStringLower(order, "side");
    
    return {
        {"id", this->safeString(order, "orderId")},
        {"clientOrderId", this->safeString(order, "clientOrderId")},
        {"datetime", this->iso8601(timestamp)},
        {"timestamp", this->parse8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"type", type},
        {"timeInForce", this->safeString(order, "timeInForce")},
        {"postOnly", nullptr},
        {"status", status},
        {"symbol", symbol},
        {"side", side},
        {"price", this->safeFloat(order, "price")},
        {"stopPrice", this->safeFloat(order, "stopPrice")},
        {"cost", this->safeFloat(order, "cummulativeQuoteQty")},
        {"amount", this->safeFloat(order, "origQty")},
        {"filled", this->safeFloat(order, "executedQty")},
        {"remaining", nullptr},
        {"trades", nullptr},
        {"fee", nullptr},
        {"info", order}
    };
}

std::string Satang::parseOrderStatus(const std::string& status) {
    static const std::map<std::string, std::string> statuses = {
        {"NEW", "open"},
        {"PARTIALLY_FILLED", "open"},
        {"FILLED", "closed"},
        {"CANCELED", "canceled"},
        {"PENDING_CANCEL", "canceling"},
        {"REJECTED", "rejected"},
        {"EXPIRED", "expired"}
    };
    
    return this->safeString(statuses, status, status);
}

} // namespace ccxt
