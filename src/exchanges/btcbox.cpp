#include "btcbox.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

BTCBOX::BTCBOX() {
    id = "btcbox";
    name = "BtcBox";
    version = "1";
    rateLimit = 1000;
    certified = false;
    pro = false;

    // Initialize API endpoints
    baseUrl = "https://www.btcbox.co.jp/api/v1";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/51840849/87327317-98c55400-c53c-11ea-9a11-81f7d951cc74.jpg"},
        {"api", {
            {"public", "https://www.btcbox.co.jp/api/v1"},
            {"private", "https://www.btcbox.co.jp/api/v1"}
        }},
        {"www", "https://www.btcbox.co.jp/"},
        {"doc", {
            "https://blog.btcbox.jp/en/archives/8762",
            "https://blog.btcbox.jp/en/archives/8766"
        }},
        {"fees", "https://www.btcbox.co.jp/help/fees"}
    };

    timeframes = {
        {"1m", "1min"},
        {"5m", "5min"},
        {"15m", "15min"},
        {"30m", "30min"},
        {"1h", "1hour"},
        {"4h", "4hour"},
        {"1d", "1day"},
        {"1w", "1week"}
    };

    options = {
        {"adjustForTimeDifference", true},
        {"recvWindow", 5000},
        {"timeDifference", 0}
    };

    errorCodes = {
        {10000, "URL does not exist"},
        {10001, "A system error occurred. Please contact support"},
        {10002, "API authentication failed"},
        {10003, "API key does not exist"},
        {10004, "API key has been disabled"},
        {10005, "Invalid nonce parameter"},
        {10006, "Invalid signature"},
        {10007, "Invalid IP address"},
        {10008, "Required parameters are missing"},
        {10009, "Invalid parameters"},
        {10010, "Order does not exist"},
        {10011, "Insufficient balance"},
        {10012, "Order quantity is too small"},
        {10013, "Order price is invalid"},
        {10014, "Rate limit exceeded"}
    };

    currencyIds = {
        {"BTC", "btc"},
        {"ETH", "eth"},
        {"LTC", "ltc"},
        {"BCH", "bch"},
        {"JPY", "jpy"}
    };

    initializeApiEndpoints();
}

void BTCBOX::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "ticker",
                "depth",
                "trades",
                "candlestick"
            }}
        }},
        {"private", {
            {"POST", {
                "balance",
                "trade_add",
                "trade_cancel",
                "trade_view",
                "trade_list",
                "trade_history",
                "deposit_history",
                "withdraw_history",
                "withdraw_coin"
            }}
        }}
    };
}

json BTCBOX::fetchMarkets(const json& params) {
    json response = fetch("/spot/markets", "public", "GET", params);
    json result = json::array();
    
    for (const auto& market : response) {
        String id = market["id"];
        String baseId = market["base_currency"];
        String quoteId = market["quote_currency"];
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
            {"future", false},
            {"option", false},
            {"margin", false},
            {"contract", false},
            {"precision", {
                {"amount", market["amount_precision"]},
                {"price", market["price_precision"]}
            }},
            {"limits", {
                {"amount", {
                    {"min", market["min_order_amount"]},
                    {"max", market["max_order_amount"]}
                }},
                {"price", {
                    {"min", market["min_order_price"]},
                    {"max", market["max_order_price"]}
                }},
                {"cost", {
                    {"min", market["min_order_value"]},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return result;
}

json BTCBOX::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/balance", "private", "POST", params);
    return parseBalance(response);
}

json BTCBOX::parseBalance(const json& response) {
    json result = {{"info", response}};
    
    for (const auto& [currency, balance] : response.items()) {
        if (currency != "success" && currency != "result") {
            String code = this->safeCurrencyCode(currency);
            String account = {
                {"free", this->safeFloat(balance, "available")},
                {"used", this->safeFloat(balance, "in_use")},
                {"total", this->safeFloat(balance, "total")}
            };
            result[code] = account;
        }
    }
    
    return result;
}

json BTCBOX::createOrder(const String& symbol, const String& type,
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
    
    json response = fetch("/trade_add", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response, market);
}

String BTCBOX::sign(const String& path, const String& api,
                   const String& method, const json& params,
                   const std::map<String, String>& headers,
                   const json& body) {
    String url = this->urls["api"][api] + "/" + this->implodeParams(path, params);
    json query = this->omit(params, this->extractParams(path));
    
    if (api == "public") {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->checkRequiredCredentials();
        String nonce = this->nonce().str();
        json auth = this->extend({
            "key", this->apiKey,
            "nonce", nonce
        }, query);
        
        String queryString = this->urlencode(this->keysort(auth));
        String signature = this->hmac(queryString, this->encode(this->secret),
                                    "sha256", "hex");
        
        auth["signature"] = signature;
        body = this->json(auth);
        const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/json";
    }
    
    return url;
}

String BTCBOX::getNonce() {
    return std::to_string(this->milliseconds());
}

json BTCBOX::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "id");
    String timestamp = this->safeString(order, "datetime");
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
        {"cost", this->safeFloat(order, "total")},
        {"amount", this->safeFloat(order, "amount")},
        {"filled", this->safeFloat(order, "filled_amount")},
        {"remaining", this->safeFloat(order, "remaining_amount")},
        {"trades", nullptr},
        {"fee", {
            {"currency", market["quote"]},
            {"cost", this->safeFloat(order, "fee")},
            {"rate", this->safeFloat(order, "fee_rate")}
        }},
        {"info", order}
    };
}

String BTCBOX::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"open", "open"},
        {"closed", "closed"},
        {"canceled", "canceled"},
        {"expired", "expired"}
    };
    
    return this->safeString(statuses, status, status);
}

} // namespace ccxt
