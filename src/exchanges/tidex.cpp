#include "tidex.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Tidex::Tidex() {
    id = "tidex";
    name = "Tidex";
    version = "3";
    rateLimit = 2000;
    certified = true;
    pro = false;
    hasPublicAPI = true;
    hasPrivateAPI = true;

    // Initialize API endpoints
    baseUrl = "https://api.tidex.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/30781780-03149dc4-a12e-11e7-82bb-313b269d24d4.jpg"},
        {"api", {
            {"public", "https://api.tidex.com/api/3"},
            {"private", "https://api.tidex.com/tapi"}
        }},
        {"www", "https://tidex.com"},
        {"doc", {
            "https://tidex.com/exchange/public-api",
            "https://tidex.com/exchange/trading-api"
        }},
        {"fees", "https://tidex.com/exchange/fees-and-limits"}
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
        {1, "Invalid parameter"},
        {2, "Invalid parameter"},
        {3, "Invalid parameter"},
        {4, "Invalid parameter"},
        {5, "Invalid parameter"},
        {6, "Invalid parameter"},
        {7, "Invalid parameter"},
        {8, "Invalid parameter"},
        {9, "Invalid parameter"},
        {10, "Invalid parameter"},
        {11, "Invalid parameter"},
        {12, "Invalid parameter"},
        {13, "Invalid parameter"},
        {14, "Invalid parameter"},
        {15, "Invalid parameter"},
        {16, "Invalid parameter"},
        {17, "Invalid parameter"},
        {18, "Invalid parameter"},
        {19, "Invalid parameter"},
        {20, "Invalid parameter"},
        {21, "Invalid parameter"},
        {22, "Invalid parameter"},
        {23, "Invalid parameter"},
        {24, "Invalid parameter"},
        {25, "Invalid parameter"},
        {26, "Invalid parameter"},
        {27, "Invalid parameter"},
        {28, "Invalid parameter"},
        {29, "Invalid parameter"},
        {30, "Invalid parameter"},
        {31, "Invalid parameter"},
        {32, "Invalid parameter"},
        {33, "Invalid parameter"},
        {34, "Invalid parameter"},
        {35, "Invalid parameter"},
        {36, "Invalid parameter"}
    };

    initializeApiEndpoints();
}

void Tidex::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "info",
                "ticker/{pair}",
                "depth/{pair}",
                "trades/{pair}",
                "candles/{pair}"
            }}
        }},
        {"private", {
            {"POST", {
                "getInfo",
                "Trade",
                "ActiveOrders",
                "OrderInfo",
                "CancelOrder",
                "TradeHistory",
                "TransHistory",
                "CoinDepositAddress",
                "WithdrawCoin",
                "CreateCoupon",
                "RedeemCoupon"
            }}
        }}
    };
}

json Tidex::fetchMarkets(const json& params) {
    json response = fetch("/info", "public", "GET", params);
    json pairs = response["pairs"];
    json result = json::array();
    
    for (const auto& entry : pairs.items()) {
        String id = entry.key();
        json market = entry.value();
        String baseId = id.substr(0, 3);
        String quoteId = id.substr(3);
        String base = this->safeCurrencyCode(baseId);
        String quote = this->safeCurrencyCode(quoteId);
        String symbol = base + "/" + quote;
        
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
            {"margin", false},
            {"future", false},
            {"option", false},
            {"contract", false},
            {"precision", {
                {"amount", market["decimal_places"]},
                {"price", market["decimal_places"]}
            }},
            {"limits", {
                {"amount", {
                    {"min", market["min_amount"]},
                    {"max", market["max_amount"]}
                }},
                {"price", {
                    {"min", market["min_price"]},
                    {"max", market["max_price"]}
                }},
                {"cost", {
                    {"min", market["min_total"]},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return result;
}

json Tidex::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/getInfo", "private", "POST", params);
    return parseBalance(response["return"]);
}

json Tidex::parseBalance(const json& response) {
    json result = {{"info", response}};
    json funds = response["funds"];
    
    for (const auto& [currency, balance] : funds.items()) {
        String code = this->safeCurrencyCode(currency);
        String account = {
            {"free", this->safeFloat(balance, "value")},
            {"used", this->safeFloat(balance, "inOrders")},
            {"total", this->safeFloat(balance, "value") + this->safeFloat(balance, "inOrders")}
        };
        result[code] = account;
    }
    
    return result;
}

json Tidex::createOrder(const String& symbol, const String& type,
                       const String& side, double amount,
                       double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"pair", market["id"]},
        {"type", side},
        {"amount", this->amountToPrecision(symbol, amount)},
        {"rate", this->priceToPrecision(symbol, price)}
    };
    
    json response = fetch("/Trade", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response["return"], market);
}

String Tidex::sign(const String& path, const String& api,
                   const String& method, const json& params,
                   const std::map<String, String>& headers,
                   const json& body) {
    String url = this->urls["api"][api];
    
    if (api == "public") {
        url += "/" + this->implodeParams(path, params);
        json query = this->omit(params, this->extractParams(path));
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->checkRequiredCredentials();
        String nonce = this->nonce().str();
        json request = this->extend({
            "method", path,
            "nonce", nonce
        }, params);
        
        String requestString = this->urlencode(request);
        String signature = this->hmac(requestString, this->encode(this->config_.secret),
                                    "sha512", "hex");
        
        body = requestString;
        const_cast<std::map<String, String>&>(headers)["Key"] = this->config_.apiKey;
        const_cast<std::map<String, String>&>(headers)["Sign"] = signature;
        const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/x-www-form-urlencoded";
    }
    
    return url;
}

String Tidex::getNonce() {
    return std::to_string(this->milliseconds());
}

json Tidex::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "order_id");
    String timestamp = this->safeString(order, "timestamp");
    String status = this->parseOrderStatus(this->safeString(order, "status"));
    String symbol = nullptr;
    
    if (!market.empty()) {
        symbol = market["symbol"];
    }
    
    String type = "limit";
    String side = this->safeString(order, "type");
    
    return {
        {"id", id},
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
        {"filled", this->safeFloat(order, "received")},
        {"remaining", this->safeFloat(order, "remains")},
        {"trades", nullptr},
        {"fee", {
            {"currency", market["quote"]},
            {"cost", this->safeFloat(order, "fee")},
            {"rate", nullptr}
        }},
        {"info", order}
    };
}

String Tidex::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"0", "open"},
        {"1", "closed"},
        {"2", "canceled"},
        {"3", "canceled"}
    };
    
    return this->safeString(statuses, status, status);
}

} // namespace ccxt
