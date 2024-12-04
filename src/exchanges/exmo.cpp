#include "exmo.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Exmo::Exmo() {
    id = "exmo";
    name = "EXMO";
    version = "1.1";
    rateLimit = 350;
    certified = true;
    pro = false;
    hasPublicAPI = true;
    hasPrivateAPI = true;
    hasFiatAPI = true;
    hasPaymentAPI = true;

    // Initialize API endpoints
    baseUrl = "https://api.exmo.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/27766491-1b0ea956-5eda-11e7-9225-40d67b481b8d.jpg"},
        {"api", {
            {"public", "https://api.exmo.com/v1.1"},
            {"private", "https://api.exmo.com/v1.1"},
            {"web", "https://exmo.me"}
        }},
        {"www", "https://exmo.me"},
        {"doc", {
            "https://exmo.me/en/api_doc",
            "https://github.com/exmo-dev/exmo_api_lib/tree/master/nodejs"
        }},
        {"fees", "https://exmo.com/en/docs/fees"}
    };

    timeframes = {
        {"1m", "1"},
        {"5m", "5"},
        {"15m", "15"},
        {"30m", "30"},
        {"1h", "60"},
        {"2h", "120"},
        {"4h", "240"},
        {"6h", "360"},
        {"12h", "720"},
        {"1d", "1440"},
        {"3d", "4320"},
        {"1w", "10080"}
    };

    options = {
        {"adjustForTimeDifference", true},
        {"recvWindow", 5000},
        {"timeDifference", 0},
        {"defaultType", "spot"}
    };

    errorCodes = {
        {40001, "Authorization has been denied for this request"},
        {40002, "Request not found"},
        {40003, "Signature not valid"},
        {40004, "Invalid parameter"},
        {40005, "Internal server error"},
        {40006, "Method not found"},
        {40007, "Service unavailable"},
        {40008, "Request limit exceeded"},
        {40009, "Non-trading pair"},
        {40010, "Invalid api key"},
        {40011, "User not found"},
        {40012, "Invalid parameter"},
        {40013, "Invalid parameter"},
        {40014, "Invalid parameter"},
        {40015, "Invalid parameter"},
        {40016, "Invalid parameter"},
        {40017, "Invalid parameter"},
        {40018, "Invalid parameter"}
    };

    initializeApiEndpoints();
}

void Exmo::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "currency",
                "currency/list/extended",
                "order_book",
                "pair_settings",
                "ticker",
                "trades",
                "candles_history"
            }}
        }},
        {"private", {
            {"POST", {
                "user_info",
                "order_create",
                "order_cancel",
                "user_open_orders",
                "user_trades",
                "user_cancelled_orders",
                "order_trades",
                "required_amount",
                "deposit_address",
                "withdraw_crypt",
                "withdraw_get_txid",
                "excode_create",
                "excode_load",
                "wallet_history"
            }}
        }}
    };
}

json Exmo::fetchMarkets(const json& params) {
    json response = fetch("/pair_settings", "public", "GET", params);
    json result = json::array();
    
    for (const auto& entry : response.items()) {
        String id = entry.key();
        json market = entry.value();
        
        std::vector<String> parts;
        std::stringstream ss(id);
        String part;
        while (std::getline(ss, part, '_')) {
            parts.push_back(part);
        }
        
        String baseId = parts[0];
        String quoteId = parts[1];
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
                    {"min", market["min_quantity"]},
                    {"max", market["max_quantity"]}
                }},
                {"price", {
                    {"min", market["min_price"]},
                    {"max", market["max_price"]}
                }},
                {"cost", {
                    {"min", market["min_amount"]},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return result;
}

json Exmo::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/user_info", "private", "POST", params);
    return parseBalance(response["balances"]);
}

json Exmo::parseBalance(const json& response) {
    json result = {{"info", response}};
    
    for (const auto& [currency, balance] : response.items()) {
        String code = this->safeCurrencyCode(currency);
        String account = {
            {"free", this->safeFloat(balance, "available")},
            {"used", this->safeFloat(balance, "in_orders")},
            {"total", this->safeFloat(balance, "total")}
        };
        result[code] = account;
    }
    
    return result;
}

json Exmo::createOrder(const String& symbol, const String& type,
                      const String& side, double amount,
                      double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"pair", market["id"]},
        {"quantity", this->amountToPrecision(symbol, amount)},
        {"price", this->priceToPrecision(symbol, price)},
        {"type", side}
    };
    
    json response = fetch("/order_create", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response, market);
}

String Exmo::sign(const String& path, const String& api,
                  const String& method, const json& params,
                  const std::map<String, String>& headers,
                  const json& body) {
    String url = this->urls["api"][api];
    url += "/" + this->implodeParams(path, params);
    
    if (api == "private") {
        this->checkRequiredCredentials();
        String nonce = this->nonce().str();
        json request = this->extend({
            "nonce", nonce
        }, params);
        
        String requestString = this->urlencode(request);
        String signature = this->hmac(requestString, this->encode(this->secret),
                                    "sha512", "hex");
        
        body = requestString;
        const_cast<std::map<String, String>&>(headers)["Key"] = this->apiKey;
        const_cast<std::map<String, String>&>(headers)["Sign"] = signature;
        const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/x-www-form-urlencoded";
    } else if (!params.empty()) {
        url += "?" + this->urlencode(params);
    }
    
    return url;
}

String Exmo::getNonce() {
    return std::to_string(this->milliseconds());
}

json Exmo::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "order_id");
    String timestamp = this->safeString(order, "created");
    String status = this->parseOrderStatus(this->safeString(order, "status"));
    String symbol = nullptr;
    
    if (!market.empty()) {
        symbol = market["symbol"];
    }
    
    String type = this->safeString(order, "type");
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
        {"price", this->safeFloat(order, "price")},
        {"stopPrice", nullptr},
        {"cost", nullptr},
        {"amount", this->safeFloat(order, "quantity")},
        {"filled", this->safeFloat(order, "amount")},
        {"remaining", this->safeFloat(order, "quantity_remaining")},
        {"trades", nullptr},
        {"fee", {
            {"currency", market["quote"]},
            {"cost", this->safeFloat(order, "commission_amount")},
            {"rate", this->safeFloat(order, "commission")}
        }},
        {"info", order}
    };
}

String Exmo::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"0", "open"},
        {"1", "closed"},
        {"2", "canceled"},
        {"3", "canceled"}
    };
    
    return this->safeString(statuses, status, status);
}

} // namespace ccxt
