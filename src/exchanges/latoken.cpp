#include "latoken.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Latoken::Latoken() {
    id = "latoken";
    name = "Latoken";
    version = "2";
    rateLimit = 1000;
    certified = true;
    pro = false;
    hasPublicAPI = true;
    hasPrivateAPI = true;
    hasFiatAPI = true;

    // Initialize API endpoints
    baseUrl = "https://api.latoken.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/61511972-24c39f00-aa01-11e9-9f7c-471f1d6e5214.jpg"},
        {"api", {
            {"public", "https://api.latoken.com/v2"},
            {"private", "https://api.latoken.com/v2"}
        }},
        {"www", "https://latoken.com"},
        {"doc", {
            "https://api.latoken.com",
            "https://api.latoken.com/doc/v2"
        }},
        {"fees", "https://latoken.com/fees"}
    };

    timeframes = {
        {"1m", "1m"},
        {"5m", "5m"},
        {"15m", "15m"},
        {"30m", "30m"},
        {"1h", "1h"},
        {"4h", "4h"},
        {"1d", "1d"},
        {"1w", "1w"},
        {"1M", "1M"}
    };

    options = {
        {"adjustForTimeDifference", true},
        {"recvWindow", 5000},
        {"timeDifference", 0},
        {"defaultType", "spot"}
    };

    errorCodes = {
        {1, "Invalid request"},
        {2, "Invalid parameters"},
        {3, "Invalid API key"},
        {4, "Invalid signature"},
        {5, "Permission denied"},
        {6, "Internal server error"},
        {7, "Resource not found"},
        {8, "Rate limit exceeded"},
        {9, "Service unavailable"},
        {10, "Insufficient funds"},
        {11, "Order not found"},
        {12, "Market not found"},
        {13, "Invalid order type"},
        {14, "Invalid side"},
        {15, "Invalid timeInForce"},
        {16, "Invalid quantity"},
        {17, "Invalid price"},
        {18, "Invalid stopPrice"},
        {19, "Market closed"},
        {20, "Market limit exceeded"}
    };

    initializeApiEndpoints();
}

void Latoken::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "currency",
                "currency/pair",
                "ticker",
                "book/{currency}/{quote}",
                "trade/history/{currency}/{quote}",
                "chart/history",
                "time"
            }}
        }},
        {"private", {
            {"POST", {
                "auth/account",
                "auth/order/new",
                "auth/order/cancel",
                "auth/order/status",
                "auth/order/active",
                "auth/order/history",
                "auth/trade/history",
                "auth/deposit/address",
                "auth/deposit/history",
                "auth/withdraw",
                "auth/withdraw/history",
                "auth/transaction/history"
            }}
        }}
    };
}

json Latoken::fetchMarkets(const json& params) {
    json response = fetch("/currency/pair", "public", "GET", params);
    json result = json::array();
    
    for (const auto& market : response) {
        std::string id = market["id"].get<std::string>();
        std::string baseId = market["baseCurrency"].get<std::string>();
        std::string quoteId = market["quoteCurrency"].get<std::string>();
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
            {"active", market["active"].get<bool>()},
            {"type", "spot"},
            {"spot", true},
            {"margin", false},
            {"future", false},
            {"option", false},
            {"contract", false},
            {"precision", {
                {"amount", market["quantityScale"].get<int>()},
                {"price", market["priceScale"].get<int>()}
            }},
            {"limits", {
                {"amount", {
                    {"min", market["minQuantity"].get<double>()},
                    {"max", market["maxQuantity"].get<double>()}
                }},
                {"price", {
                    {"min", market["minPrice"].get<double>()},
                    {"max", market["maxPrice"].get<double>()}
                }},
                {"cost", {
                    {"min", market["minCost"].get<double>()},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return result;
}

json Latoken::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/auth/account", "private", "POST", params);
    return parseBalance(response);
}

json Latoken::parseBalance(const json& response) {
    json result = {{"info", response}};
    
    for (const auto& balance : response) {
        std::string currencyId = balance["currency"].get<std::string>();
        std::string code = this->safeCurrencyCode(currencyId);
        std::string account = {
            {"free", this->safeFloat(balance, "available")},
            {"used", this->safeFloat(balance, "frozen")},
            {"total", this->safeFloat(balance, "total")}
        };
        result[code] = account;
    }
    
    return result;
}

json Latoken::createOrder(const std::string& symbol, const std::string& type,
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
    
    if (type == "LIMIT") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    json response = fetch("/auth/order/new", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response, market);
}

std::string Latoken::sign(const std::string& path, const std::string& api,
                     const std::string& method, const json& params,
                     const std::map<std::string, std::string>& headers,
                     const json& body) {
    std::string url = this->urls["api"][api] + path;
    
    if (api == "private") {
        this->checkRequiredCredentials();
        std::string timestamp = std::to_string(this->milliseconds());
        std::string payload = timestamp + method + path;
        
        if (method == "POST") {
            body = this->json(params);
            payload += body;
        } else if (!params.empty()) {
            std::string query = this->urlencode(this->keysort(params));
            url += "?" + query;
            payload += query;
        }
        
        std::string signature = this->hmac(payload, this->encode(this->config_.secret),
                                    "sha512", "hex");
        
        const_cast<std::map<std::string, std::string>&>(headers)["X-LA-APIKEY"] = this->config_.apiKey;
        const_cast<std::map<std::string, std::string>&>(headers)["X-LA-SIGNATURE"] = signature;
        const_cast<std::map<std::string, std::string>&>(headers)["X-LA-DIGEST"] = "HMAC-SHA512";
        const_cast<std::map<std::string, std::string>&>(headers)["X-LA-TIMESTAMP"] = timestamp;
        
        if (method == "POST") {
            const_cast<std::map<std::string, std::string>&>(headers)["Content-Type"] = "application/json";
        }
    } else if (!params.empty()) {
        url += "?" + this->urlencode(params);
    }
    
    return url;
}

std::string Latoken::getNonce() {
    return std::to_string(this->milliseconds());
}

json Latoken::parseOrder(const json& order, const Market& market) {
    std::string id = this->safeString(order, "id");
    std::string timestamp = this->safeString(order, "timestamp");
    std::string status = this->parseOrderStatus(this->safeString(order, "status"));
    std::string symbol = nullptr;
    
    if (!market.empty()) {
        symbol = market["symbol"];
    }
    
    std::string type = this->safeStringLower(order, "type");
    std::string side = this->safeStringLower(order, "side");
    
    return {
        {"id", id},
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
        {"cost", this->safeFloat(order, "cost")},
        {"amount", this->safeFloat(order, "quantity")},
        {"filled", this->safeFloat(order, "filled")},
        {"remaining", this->safeFloat(order, "remaining")},
        {"trades", nullptr},
        {"fee", {
            {"currency", market["quote"]},
            {"cost", this->safeFloat(order, "fee")},
            {"rate", this->safeFloat(order, "feeRate")}
        }},
        {"info", order}
    };
}

std::string Latoken::parseOrderStatus(const std::string& status) {
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
