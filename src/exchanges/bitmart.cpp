#include "bitmart.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Bitmart::Bitmart() {
    id = "bitmart";
    name = "BitMart";
    version = "v2";
    rateLimit = 250;

    // Initialize API endpoints
    baseUrl = "https://api-cloud.bitmart.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/129991357-8f47464b-d0f4-41d6-8a82-34122f0d1398.jpg"},
        {"api", {
            {"public", "https://api-cloud.bitmart.com"},
            {"private", "https://api-cloud.bitmart.com"}
        }},
        {"www", "https://www.bitmart.com"},
        {"doc", {
            "https://developer-pro.bitmart.com",
            "https://github.com/bitmartexchange/bitmart-official-api-docs"
        }},
        {"fees", "https://www.bitmart.com/fee/en"}
    };

    timeframes = {
        {"1m", "1"},
        {"3m", "3"},
        {"5m", "5"},
        {"15m", "15"},
        {"30m", "30"},
        {"45m", "45"},
        {"1h", "60"},
        {"2h", "120"},
        {"3h", "180"},
        {"4h", "240"},
        {"1d", "1D"},
        {"1w", "1W"},
        {"1M", "1M"}
    };

    options = {
        {"adjustForTimeDifference", true},
        {"recvWindow", "5000"}
    };

    errorCodes = {
        {50000, "System error"},
        {50001, "Parameter error"},
        {50002, "Signature error"},
        {50004, "API key not found"},
        {50005, "API key expired"},
        {50006, "IP not allowed"},
        {50007, "Invalid timestamp"},
        {50008, "Invalid signature version"},
        {50009, "Request too frequent"},
        {50010, "Account suspended"},
        {50011, "Order count over limit"},
        {50012, "Order amount over limit"},
        {50013, "Order price over limit"},
        {50014, "Insufficient balance"},
        {50015, "Order does not exist"},
        {50016, "Order already cancelled"},
        {50017, "Order already filled"},
        {50018, "Order partially filled"},
        {50019, "Order amount too small"},
        {50020, "Order price too low"},
        {50021, "Order price too high"},
        {50022, "Invalid order type"},
        {50023, "Invalid side"},
        {50024, "Invalid symbol"}
    };

    initializeApiEndpoints();
}

void Bitmart::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "spot/v1/currencies",
                "spot/v1/symbols",
                "spot/v1/symbols/details",
                "spot/v1/ticker",
                "spot/v1/steps",
                "spot/v1/symbols/kline",
                "spot/v1/symbols/book",
                "spot/v1/symbols/trades",
                "spot/v2/ticker"
            }}
        }},
        {"private", {
            {"GET", {
                "spot/v1/wallet",
                "spot/v1/orders",
                "spot/v2/orders",
                "spot/v1/trades",
                "spot/v2/trades",
                "spot/v1/orders/detail"
            }},
            {"POST", {
                "spot/v1/submit_order",
                "spot/v2/submit_order",
                "spot/v1/batch_orders",
                "spot/v2/batch_orders",
                "spot/v1/cancel_order",
                "spot/v2/cancel_order",
                "spot/v1/cancel_orders"
            }}
        }}
    };
}

json Bitmart::fetchMarkets(const json& params) {
    json response = fetch("/spot/v1/symbols/details", "public", "GET", params);
    json result = json::array();
    
    for (const auto& market : response["data"]["symbols"]) {
        String id = this->safeString(market, "symbol");
        std::vector<String> parts = this->split(id, "_");
        String baseId = parts[0];
        String quoteId = parts[1];
        String base = this->commonCurrencyCode(baseId);
        String quote = this->commonCurrencyCode(quoteId);
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
            {"swap", false},
            {"option", false},
            {"contract", false},
            {"precision", {
                {"amount", market["price_precision"].get<int>()},
                {"price", market["price_precision"].get<int>()}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeFloat(market, "min_buy_amount")},
                    {"max", this->safeFloat(market, "max_buy_amount")}
                }},
                {"price", {
                    {"min", this->safeFloat(market, "min_buy_price")},
                    {"max", this->safeFloat(market, "max_buy_price")}
                }},
                {"cost", {
                    {"min", this->safeFloat(market, "min_buy_amount") * this->safeFloat(market, "min_buy_price")},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return result;
}

json Bitmart::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/spot/v1/wallet", "private", "GET", params);
    return parseBalance(response);
}

json Bitmart::parseBalance(const json& response) {
    json result = {"info", response};
    json balances = response["data"]["wallet"];
    
    for (const auto& balance : balances) {
        String currencyId = balance["id"];
        String code = this->commonCurrencyCode(currencyId);
        
        result[code] = {
            {"free", this->safeFloat(balance, "available")},
            {"used", this->safeFloat(balance, "frozen")},
            {"total", this->safeFloat(balance, "available") + this->safeFloat(balance, "frozen")}
        };
    }
    
    return result;
}

json Bitmart::createOrder(const String& symbol, const String& type,
                         const String& side, double amount,
                         double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"symbol", market.id},
        {"side", side.upper()},
        {"type", type.upper()},
        {"size", this->amountToPrecision(symbol, amount)}
    };
    
    if (type == "limit") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    json response = fetch("/spot/v2/submit_order", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response["data"], market);
}

String Bitmart::sign(const String& path, const String& api,
                     const String& method, const json& params,
                     const std::map<String, String>& headers,
                     const json& body) {
    String url = this->urls["api"][api] + "/" + this->version + path;
    String timestamp = std::to_string(this->milliseconds());
    
    if (api == "public") {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    } else {
        this->checkRequiredCredentials();
        
        if (method == "GET") {
            if (!params.empty()) {
                url += "?" + this->urlencode(params);
            }
        } else {
            body = this->json(params);
        }
        
        String signature = this->createSignature(timestamp, method, path,
                                               body.empty() ? "" : body.dump());
        
        const_cast<std::map<String, String>&>(headers)["X-BM-KEY"] = this->apiKey;
        const_cast<std::map<String, String>&>(headers)["X-BM-SIGN"] = signature;
        const_cast<std::map<String, String>&>(headers)["X-BM-TIMESTAMP"] = timestamp;
        
        if (!body.empty()) {
            const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/json";
        }
    }
    
    return url;
}

String Bitmart::createSignature(const String& timestamp, const String& method,
                               const String& path, const String& body) {
    String message = timestamp + "#" + this->apiKey + "#" + method + "#" +
                    path + "#" + body;
    return this->hmac(message, this->encode(this->secret),
                     "sha256", "hex");
}

json Bitmart::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "order_id");
    String timestamp = this->safeString(order, "create_time");
    String status = this->parseOrderStatus(this->safeString(order, "status"));
    String symbol = market.symbol;
    String type = this->safeStringLower(order, "type");
    String side = this->safeStringLower(order, "side");
    
    return {
        {"id", id},
        {"clientOrderId", nullptr},
        {"timestamp", this->safeInteger(order, "create_time")},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"status", status},
        {"symbol", symbol},
        {"type", type},
        {"side", side},
        {"price", this->safeFloat(order, "price")},
        {"amount", this->safeFloat(order, "size")},
        {"filled", this->safeFloat(order, "filled_size")},
        {"remaining", this->safeFloat(order, "size") - this->safeFloat(order, "filled_size")},
        {"cost", this->safeFloat(order, "filled_size") * this->safeFloat(order, "price")},
        {"trades", nullptr},
        {"fee", {
            {"currency", market.quote},
            {"cost", this->safeFloat(order, "fee")},
            {"rate", nullptr}
        }},
        {"info", order}
    };
}

json Bitmart::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"1", "open"},
        {"2", "filled"},
        {"3", "canceled"},
        {"4", "canceled"},
        {"5", "partially_filled"}
    };
    
    return statuses.contains(status) ? statuses.at(status) : status;
}

} // namespace ccxt
