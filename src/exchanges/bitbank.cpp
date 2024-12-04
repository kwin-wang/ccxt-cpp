#include "bitbank.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Bitbank::Bitbank() {
    id = "bitbank";
    name = "bitbank";
    version = "1";
    rateLimit = 1000;
    certified = true;
    pro = false;

    // Initialize API endpoints
    baseUrl = "https://public.bitbank.cc";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/37808081-b87f2d9c-2e59-11e8-894d-c1900b7584fe.jpg"},
        {"api", {
            {"public", "https://public.bitbank.cc"},
            {"private", "https://api.bitbank.cc/v1"}
        }},
        {"www", "https://bitbank.cc/"},
        {"doc", {
            "https://docs.bitbank.cc/",
            "https://github.com/bitbankinc/bitbank-api-docs"
        }},
        {"fees", "https://bitbank.cc/docs/fees/"}
    };

    timeframes = {
        {"1m", "1min"},
        {"5m", "5min"},
        {"15m", "15min"},
        {"30m", "30min"},
        {"1h", "1hour"},
        {"4h", "4hour"},
        {"8h", "8hour"},
        {"12h", "12hour"},
        {"1d", "1day"},
        {"1w", "1week"}
    };

    options = {
        {"adjustForTimeDifference", true},
        {"fetchOrdersMethod", "private_get_user_orders"}
    };

    errorCodes = {
        {10000, "URL does not exist"},
        {10001, "A system error occurred. Please contact support"},
        {10002, "Invalid JSON format"},
        {10003, "A system error occurred. Please contact support"},
        {10005, "A timeout error occurred. Please wait for a while and try again"},
        {20001, "API authentication failed"},
        {20002, "Illegal API key"},
        {20003, "API key does not exist"},
        {20004, "API Nonce does not exist"},
        {20005, "API signature does not exist"},
        {20011, "Two-step verification failed"},
        {20014, "SMS authentication failed"},
        {30001, "Missing order quantity"},
        {30006, "Price is too low"},
        {30007, "Price is too high"},
        {30009, "Market order amount is too large"},
        {30012, "Insufficient balance"},
        {40001, "Invalid order ID"},
        {40006, "Order ID does not exist"},
        {40007, "Order is not cancellable"},
        {40009, "Order amount is too large"},
        {50001, "Unauthorized IP address"},
        {50002, "Bad API request"},
        {60001, "Market is closed"},
        {60002, "Market is busy"},
        {70001, "System error. Please contact support"}
    };

    initializeApiEndpoints();
}

void Bitbank::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "{pair}/ticker",
                "{pair}/depth",
                "{pair}/transactions",
                "{pair}/candlestick/{candletype}/{yyyymmdd}"
            }}
        }},
        {"private", {
            {"GET", {
                "user/assets",
                "user/spot/order",
                "user/spot/active_orders",
                "user/spot/trade_history",
                "user/withdrawal_account",
                "user/deposit_history",
                "user/withdrawal_history"
            }},
            {"POST", {
                "user/spot/order",
                "user/spot/cancel_order",
                "user/spot/cancel_orders",
                "user/spot/orders",
                "user/request_withdrawal"
            }}
        }}
    };
}

json Bitbank::fetchMarkets(const json& params) {
    json response = fetch("/spot/pairs", "public", "GET", params);
    json data = this->safeValue(response, "data", json::array());
    json result = json::array();
    
    for (const auto& market : data) {
        String id = this->safeString(market, "name");
        String baseId = this->safeString(market, "base_asset");
        String quoteId = this->safeString(market, "quote_asset");
        String base = this->safeCurrencyCode(baseId);
        String quote = this->safeCurrencyCode(quoteId);
        
        result.push_back({
            {"id", id},
            {"symbol", base + "/" + quote},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", true},
            {"type", "spot"},
            {"spot", true},
            {"future", false},
            {"option", false},
            {"margin", true},
            {"contract", false},
            {"precision", {
                {"amount", this->safeInteger(market, "amount_digits")},
                {"price", this->safeInteger(market, "price_digits")}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeFloat(market, "min_order_amount")},
                    {"max", this->safeFloat(market, "max_order_amount")}
                }},
                {"price", {
                    {"min", this->safeFloat(market, "min_order_price")},
                    {"max", this->safeFloat(market, "max_order_price")}
                }},
                {"cost", {
                    {"min", nullptr},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return result;
}

json Bitbank::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/user/assets", "private", "GET", params);
    return parseBalance(response);
}

json Bitbank::parseBalance(const json& response) {
    json data = this->safeValue(response, "data", json::array());
    json result = {{"info", response}};
    
    for (const auto& balance : data) {
        String currencyId = this->safeString(balance, "asset");
        String code = this->safeCurrencyCode(currencyId);
        String account = {
            {"free", this->safeFloat(balance, "free_amount")},
            {"used", this->safeFloat(balance, "locked_amount")},
            {"total", this->safeFloat(balance, "onhand_amount")}
        };
        result[code] = account;
    }
    
    return result;
}

json Bitbank::createOrder(const String& symbol, const String& type,
                         const String& side, double amount,
                         double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"pair", market["id"]},
        {"amount", this->amountToPrecision(symbol, amount)},
        {"side", side},
        {"type", type}
    };
    
    if (type == "limit") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    json response = fetch("/user/spot/order", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response["data"], market);
}

String Bitbank::sign(const String& path, const String& api,
                    const String& method, const json& params,
                    const std::map<String, String>& headers,
                    const json& body) {
    String url = this->urls["api"][api];
    String query = this->omit(params, this->extractParams(path));
    
    if (api == "public") {
        url += this->implodeParams(path, params);
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->checkRequiredCredentials();
        String nonce = this->nonce().str();
        String auth = nonce + url + path;
        
        if (method == "POST") {
            body = this->json(this->extend(params, query));
            auth += body;
        } else {
            if (!query.empty()) {
                url += "?" + this->urlencode(query);
            }
        }
        
        String signature = this->hmac(auth, this->encode(this->secret),
                                    "sha256", "hex");
        
        const_cast<std::map<String, String>&>(headers)["ACCESS-KEY"] = this->apiKey;
        const_cast<std::map<String, String>&>(headers)["ACCESS-NONCE"] = nonce;
        const_cast<std::map<String, String>&>(headers)["ACCESS-SIGNATURE"] = signature;
        
        if (method == "POST") {
            const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/json";
        }
    }
    
    return url;
}

String Bitbank::createNonce() {
    return std::to_string(this->milliseconds());
}

json Bitbank::parseOrder(const json& order, const Market& market) {
    String timestamp = this->safeString(order, "ordered_at");
    String status = this->parseOrderStatus(this->safeString(order, "status"));
    String symbol = nullptr;
    
    if (!market.empty()) {
        symbol = market["symbol"];
    }
    
    String type = this->safeString(order, "type");
    String side = this->safeString(order, "side");
    
    return {
        {"id", this->safeString(order, "order_id")},
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
        {"cost", this->safeFloat(order, "executed_amount")},
        {"amount", this->safeFloat(order, "start_amount")},
        {"filled", this->safeFloat(order, "executed_amount")},
        {"remaining", this->safeFloat(order, "remaining_amount")},
        {"trades", nullptr},
        {"fee", nullptr},
        {"info", order}
    };
}

String Bitbank::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"UNFILLED", "open"},
        {"PARTIALLY_FILLED", "open"},
        {"FULLY_FILLED", "closed"},
        {"CANCELED_UNFILLED", "canceled"},
        {"CANCELED_PARTIALLY_FILLED", "canceled"}
    };
    
    return this->safeString(statuses, status, status);
}

} // namespace ccxt
