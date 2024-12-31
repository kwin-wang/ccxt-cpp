#include "novadax.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Novadax::Novadax() {
    id = "novadax";
    name = "NovaDAX";
    version = "1";
    rateLimit = 1000;
    certified = true;
    pro = false;
    hasPublicAPI = true;
    hasPrivateAPI = true;
    hasFiatAPI = true;

    // Initialize API endpoints
    baseUrl = "https://api.novadax.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/92337550-2b085500-f0b3-11ea-98e7-5794fb07dd3b.jpg"},
        {"api", {
            {"public", "https://api.novadax.com/v1"},
            {"private", "https://api.novadax.com/v1"}
        }},
        {"www", "https://www.novadax.com"},
        {"doc", {
            "https://doc.novadax.com/en-US/",
            "https://doc.novadax.com/pt-BR/"
        }},
        {"fees", "https://www.novadax.com/fees-and-limits"}
    };

    timeframes = {
        {"1m", "ONE_MIN"},
        {"5m", "FIVE_MIN"},
        {"15m", "FIFTEEN_MIN"},
        {"30m", "HALF_HOU"},
        {"1h", "ONE_HOU"},
        {"1d", "ONE_DAY"},
        {"1w", "ONE_WEE"},
        {"1M", "ONE_MON"}
    };

    options = {
        {"adjustForTimeDifference", true},
        {"recvWindow", 5000},
        {"timeDifference", 0},
        {"defaultType", "spot"}
    };

    errorCodes = {
        {1001, "Service unavailable"},
        {1002, "Authorization failed"},
        {1003, "Two-factor authentication failed"},
        {1004, "Invalid parameters"},
        {1005, "Invalid parameter: limit"},
        {1006, "Invalid parameter: offset"},
        {1007, "Invalid parameter: symbol"},
        {1008, "Invalid parameter: side"},
        {1009, "Invalid parameter: amount"},
        {1010, "Invalid parameter: price"},
        {1011, "Invalid parameter: order_id"},
        {1012, "Invalid parameter: order_type"},
        {1013, "Invalid parameter: client_order_id"},
        {1014, "Invalid parameter: trigger_price"},
        {1015, "Invalid parameter: stop_price"},
        {1016, "Invalid parameter: time_in_force"},
        {1017, "Invalid parameter: currency"},
        {1018, "Invalid parameter: address"},
        {1019, "Invalid parameter: tag"},
        {1020, "Invalid parameter: chain"},
        {1021, "Invalid parameter: network"}
    };

    initializeApiEndpoints();
}

void Novadax::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "common/symbols",
                "common/currencies",
                "market/tickers",
                "market/ticker",
                "market/depth",
                "market/trades",
                "market/kline",
                "market/24h",
                "market/hr24",
                "market/latest_trades"
            }}
        }},
        {"private", {
            {"GET", {
                "account/getBalance",
                "account/getDepositAddress",
                "account/getWithdrawConfig",
                "account/getDepositHistory",
                "account/getWithdrawHistory",
                "account/getTransferHistory",
                "orders/list",
                "orders/get",
                "orders/fills",
                "orders/opening",
                "orders/history"
            }},
            {"POST", {
                "orders/create",
                "orders/cancel",
                "orders/cancelAll",
                "account/withdraw"
            }}
        }}
    };
}

json Novadax::fetchMarkets(const json& params) {
    json response = fetch("/common/symbols", "public", "GET", params);
    json result = json::array();
    
    for (const auto& market : response["data"]) {
        std::string id = market["symbol"].get<std::string>();
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
            {"active", market["status"] == "ONLINE"},
            {"type", "spot"},
            {"spot", true},
            {"margin", false},
            {"future", false},
            {"option", false},
            {"contract", false},
            {"precision", {
                {"amount", market["quantityPrecision"].get<int>()},
                {"price", market["pricePrecision"].get<int>()}
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
                    {"min", market["minAmount"].get<double>()},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return result;
}

json Novadax::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/account/getBalance", "private", "GET", params);
    return parseBalance(response["data"]);
}

json Novadax::parseBalance(const json& response) {
    json result = {{"info", response}};
    
    for (const auto& balance : response) {
        std::string currencyId = balance["currency"].get<std::string>();
        std::string code = this->safeCurrencyCode(currencyId);
        std::string account = {
            {"free", this->safeFloat(balance, "available")},
            {"used", this->safeFloat(balance, "hold")},
            {"total", this->safeFloat(balance, "total")}
        };
        result[code] = account;
    }
    
    return result;
}

json Novadax::createOrder(const std::string& symbol, const std::string& type,
                         const std::string& side, double amount,
                         double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"symbol", market["id"]},
        {"type", type.toUpperCase()},
        {"side", side.toUpperCase()},
        {"amount", this->amountToPrecision(symbol, amount)}
    };
    
    if (type == "LIMIT") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    json response = fetch("/orders/create", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response["data"], market);
}

std::string Novadax::sign(const std::string& path, const std::string& api,
                     const std::string& method, const json& params,
                     const std::map<std::string, std::string>& headers,
                     const json& body) {
    std::string url = this->urls["api"][api] + path;
    
    if (api == "private") {
        this->checkRequiredCredentials();
        std::string timestamp = std::to_string(this->milliseconds());
        std::string auth = timestamp + method + path;
        
        if (method == "POST") {
            body = this->json(params);
            auth += body;
        } else {
            if (!params.empty()) {
                std::string query = this->urlencode(this->keysort(params));
                url += "?" + query;
                auth += "?" + query;
            }
        }
        
        std::string signature = this->hmac(auth, this->encode(this->config_.secret),
                                    "sha256", "hex");
        
        const_cast<std::map<std::string, std::string>&>(headers)["X-Nova-Access-Key"] = this->config_.apiKey;
        const_cast<std::map<std::string, std::string>&>(headers)["X-Nova-Signature"] = signature;
        const_cast<std::map<std::string, std::string>&>(headers)["X-Nova-Timestamp"] = timestamp;
        
        if (method == "POST") {
            const_cast<std::map<std::string, std::string>&>(headers)["Content-Type"] = "application/json";
        }
    } else {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    }
    
    return url;
}

std::string Novadax::getNonce() {
    return std::to_string(this->milliseconds());
}

json Novadax::parseOrder(const json& order, const Market& market) {
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
        {"cost", this->safeFloat(order, "amount")},
        {"amount", this->safeFloat(order, "quantity")},
        {"filled", this->safeFloat(order, "filledQuantity")},
        {"remaining", this->safeFloat(order, "remainingQuantity")},
        {"trades", nullptr},
        {"fee", {
            {"currency", market["quote"]},
            {"cost", this->safeFloat(order, "fee")},
            {"rate", this->safeFloat(order, "feeRate")}
        }},
        {"info", order}
    };
}

std::string Novadax::parseOrderStatus(const std::string& status) {
    static const std::map<std::string, std::string> statuses = {
        {"SUBMITTED", "open"},
        {"PROCESSING", "open"},
        {"PARTIAL_FILLED", "open"},
        {"FILLED", "closed"},
        {"CANCELED", "canceled"},
        {"REJECTED", "rejected"}
    };
    
    return this->safeString(statuses, status, status);
}

} // namespace ccxt
