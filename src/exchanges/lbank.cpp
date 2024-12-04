#include "lbank.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Lbank::Lbank() {
    id = "lbank";
    name = "LBank";
    version = "v2";
    rateLimit = 1000;
    certified = true;
    pro = false;
    hasPublicAPI = true;
    hasPrivateAPI = true;

    // Initialize API endpoints
    baseUrl = "https://api.lbank.info";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/38063602-9605e28a-3302-11e8-81be-64b1e53c4cfb.jpg"},
        {"api", {
            {"public", "https://api.lbank.info/v2"},
            {"private", "https://api.lbank.info/v2"}
        }},
        {"www", "https://www.lbank.info"},
        {"doc", {
            "https://www.lbank.info/en-US/docs/index.html",
            "https://github.com/LBank-exchange/lbank-official-api-docs"
        }},
        {"fees", "https://www.lbank.info/fees.html"}
    };

    timeframes = {
        {"1m", "minute1"},
        {"5m", "minute5"},
        {"15m", "minute15"},
        {"30m", "minute30"},
        {"1h", "hour1"},
        {"2h", "hour2"},
        {"4h", "hour4"},
        {"6h", "hour6"},
        {"8h", "hour8"},
        {"12h", "hour12"},
        {"1d", "day1"},
        {"1w", "week1"},
        {"1M", "month1"}
    };

    options = {
        {"adjustForTimeDifference", true},
        {"recvWindow", 5000},
        {"timeDifference", 0},
        {"defaultType", "spot"}
    };

    errorCodes = {
        {10000, "Internal error"},
        {10001, "The required parameters can not be empty"},
        {10002, "Validation failed"},
        {10003, "Invalid parameter"},
        {10004, "Request too frequent"},
        {10005, "Secret key does not exist"},
        {10006, "User does not exist"},
        {10007, "Invalid signature"},
        {10008, "Invalid Trading Pair"},
        {10009, "Price and/or Amount are required for limit order"},
        {10010, "Price and/or Amount must be more than 0"},
        {10011, "Market order amount is too large"},
        {10012, "PRICE_PRECISION error"},
        {10013, "AMOUNT_PRECISION error"},
        {10014, "AMOUNT_MIN_ERROR"},
        {10015, "AMOUNT_MAX_ERROR"},
        {10016, "PRICE_MIN_ERROR"},
        {10017, "PRICE_MAX_ERROR"}
    };

    initializeApiEndpoints();
}

void Lbank::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "currencyPairs",
                "ticker/24hr",
                "depth",
                "trades",
                "kline",
                "accuracy",
                "usdToCny",
                "timestamp"
            }}
        }},
        {"private", {
            {"POST", {
                "user_info",
                "create_order",
                "cancel_order",
                "orders_info",
                "orders_info_history",
                "order_transaction_detail",
                "transaction_history",
                "withdraw",
                "withdrawCancel",
                "withdraws",
                "withdrawConfigs"
            }}
        }}
    };
}

json Lbank::fetchMarkets(const json& params) {
    json response = fetch("/currencyPairs", "public", "GET", params);
    json result = json::array();
    
    for (const auto& pair : response) {
        String id = pair.get<String>();
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
        
        json accuracy = fetch("/accuracy", "public", "GET", {{"symbol", id}});
        
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
                {"amount", accuracy["quantityAccuracy"]},
                {"price", accuracy["priceAccuracy"]}
            }},
            {"limits", {
                {"amount", {
                    {"min", accuracy["minTranQua"]},
                    {"max", nullptr}
                }},
                {"price", {
                    {"min", nullptr},
                    {"max", nullptr}
                }},
                {"cost", {
                    {"min", accuracy["minTranQua"]},
                    {"max", nullptr}
                }}
            }},
            {"info", accuracy}
        });
    }
    
    return result;
}

json Lbank::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/user_info", "private", "POST", params);
    return parseBalance(response["info"]["funds"]);
}

json Lbank::parseBalance(const json& response) {
    json result = {{"info", response}};
    json free = response["free"];
    json freezed = response["freezed"];
    
    for (const auto& [currency, balance] : free.items()) {
        String code = this->safeCurrencyCode(currency);
        String account = {
            {"free", this->safeFloat(free, currency)},
            {"used", this->safeFloat(freezed, currency)},
            {"total", this->safeFloat(free, currency) + this->safeFloat(freezed, currency)}
        };
        result[code] = account;
    }
    
    return result;
}

json Lbank::createOrder(const String& symbol, const String& type,
                       const String& side, double amount,
                       double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"symbol", market["id"]},
        {"type", type},
        {"side", side},
        {"amount", this->amountToPrecision(symbol, amount)}
    };
    
    if (type == "limit") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    json response = fetch("/create_order", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response, market);
}

String Lbank::sign(const String& path, const String& api,
                   const String& method, const json& params,
                   const std::map<String, String>& headers,
                   const json& body) {
    String url = this->urls["api"][api];
    url += "/" + this->implodeParams(path, params);
    
    if (api == "private") {
        this->checkRequiredCredentials();
        String timestamp = std::to_string(this->milliseconds());
        String query = this->urlencode(this->keysort(params));
        String auth = query + "&secret_key=" + this->secret;
        String signature = this->hash(this->encode(auth), "md5");
        
        if (method == "GET") {
            if (!params.empty()) {
                url += "?" + query;
            }
        } else {
            body = this->extend(params, {
                {"api_key", this->apiKey},
                {"sign", signature},
                {"timestamp", timestamp}
            });
            const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/x-www-form-urlencoded";
        }
    } else if (!params.empty()) {
        url += "?" + this->urlencode(params);
    }
    
    return url;
}

String Lbank::getNonce() {
    return std::to_string(this->milliseconds());
}

json Lbank::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "order_id");
    String timestamp = this->safeString(order, "create_time");
    String status = this->parseOrderStatus(this->safeString(order, "status"));
    String symbol = nullptr;
    
    if (!market.empty()) {
        symbol = market["symbol"];
    }
    
    String type = this->safeString(order, "type");
    String side = this->safeString(order, "side");
    
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
        {"amount", this->safeFloat(order, "amount")},
        {"filled", this->safeFloat(order, "deal_amount")},
        {"remaining", this->safeFloat(order, "remain_amount")},
        {"trades", nullptr},
        {"fee", {
            {"currency", market["quote"]},
            {"cost", this->safeFloat(order, "deal_fee")},
            {"rate", nullptr}
        }},
        {"info", order}
    };
}

String Lbank::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"0", "open"},
        {"1", "closed"},
        {"2", "canceled"},
        {"3", "open"},
        {"4", "canceled"}
    };
    
    return this->safeString(statuses, status, status);
}

} // namespace ccxt
