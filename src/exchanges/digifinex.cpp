#include "digifinex.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Digifinex::Digifinex() {
    id = "digifinex";
    name = "DigiFinex";
    version = "v3";
    rateLimit = 100;

    // Initialize API endpoints
    baseUrl = "https://openapi.digifinex.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/51840849/87443315-01283a00-c5fe-11ea-8628-c2a0feaf07ac.jpg"},
        {"api", {
            {"public", "https://openapi.digifinex.com"},
            {"private", "https://openapi.digifinex.com"}
        }},
        {"www", "https://www.digifinex.com"},
        {"doc", {
            "https://docs.digifinex.com",
            "https://github.com/DigiFinex/api"
        }},
        {"fees", "https://digifinex.zendesk.com/hc/en-us/articles/360000328422-Fee-Structure-on-DigiFinex"}
    };

    timeframes = {
        {"1m", "1"},
        {"5m", "5"},
        {"15m", "15"},
        {"30m", "30"},
        {"1h", "60"},
        {"4h", "240"},
        {"12h", "720"},
        {"1d", "1D"},
        {"1w", "1W"},
        {"1M", "1M"}
    };

    options = {
        {"adjustForTimeDifference", true},
        {"recvWindow", "5000"}
    };

    errorCodes = {
        {10001, "System error"},
        {10002, "Parameter error"},
        {10003, "Invalid signature"},
        {10004, "Invalid API key"},
        {10005, "Invalid timestamp"},
        {10006, "IP not allowed"},
        {10007, "Permission denied"},
        {10008, "Too many requests"},
        {10009, "Insufficient balance"},
        {10010, "Order does not exist"},
        {10011, "Order amount too small"},
        {10012, "Order price out of range"},
        {10013, "Order has been filled"},
        {10014, "Order has been cancelled"},
        {10015, "Order is cancelling"},
        {10016, "Trading pair not supported"},
        {10017, "Trading is disabled"},
        {10018, "Trading pair suspended"}
    };

    initializeApiEndpoints();
}

void Digifinex::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "v3/ping",
                "v3/time",
                "v3/market/symbols",
                "v3/market/ticker",
                "v3/market/depth",
                "v3/market/trades",
                "v3/market/kline",
                "v3/market/pairs",
                "v3/market/coins"
            }}
        }},
        {"private", {
            {"GET", {
                "v3/spot/assets",
                "v3/spot/order",
                "v3/spot/orders",
                "v3/spot/myTrades",
                "v3/margin/assets",
                "v3/margin/order",
                "v3/margin/orders",
                "v3/margin/myTrades",
                "v3/otc/assets",
                "v3/otc/order",
                "v3/otc/orders",
                "v3/otc/myTrades"
            }},
            {"POST", {
                "v3/spot/order/new",
                "v3/spot/order/cancel",
                "v3/margin/order/new",
                "v3/margin/order/cancel",
                "v3/otc/order/new",
                "v3/otc/order/cancel"
            }}
        }}
    };
}

json Digifinex::fetchMarkets(const json& params) {
    json response = fetch("/v3/market/symbols", "public", "GET", params);
    json result = json::array();
    
    for (const auto& market : response["data"]) {
        String id = this->safeString(market, "symbol");
        String baseId = this->safeString(market, "base_currency");
        String quoteId = this->safeString(market, "quote_currency");
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
                {"amount", market["volume_precision"].get<int>()},
                {"price", market["price_precision"].get<int>()}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeFloat(market, "min_volume")},
                    {"max", nullptr}
                }},
                {"price", {
                    {"min", this->safeFloat(market, "min_price")},
                    {"max", this->safeFloat(market, "max_price")}
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

json Digifinex::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/v3/spot/assets", "private", "GET", params);
    return parseBalance(response);
}

json Digifinex::parseBalance(const json& response) {
    json result = {"info", response};
    json balances = response["data"];
    
    for (const auto& balance : balances) {
        String currencyId = balance["currency"];
        String code = this->commonCurrencyCode(currencyId);
        
        result[code] = {
            {"free", this->safeFloat(balance, "free")},
            {"used", this->safeFloat(balance, "frozen")},
            {"total", this->safeFloat(balance, "total")}
        };
    }
    
    return result;
}

json Digifinex::createOrder(const String& symbol, const String& type,
                           const String& side, double amount,
                           double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"symbol", market.id},
        {"type", type.upper()},
        {"side", side.upper()},
        {"amount", this->amountToPrecision(symbol, amount)}
    };
    
    if (type == "limit") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    json response = fetch("/v3/spot/order/new", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response["data"], market);
}

String Digifinex::sign(const String& path, const String& api,
                       const String& method, const json& params,
                       const std::map<String, String>& headers,
                       const json& body) {
    String url = this->urls["api"][api] + path;
    String timestamp = std::to_string(this->milliseconds());
    
    if (api == "public") {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    } else {
        this->checkRequiredCredentials();
        
        json request = this->extend({
            "access_key": this->config_.apiKey,
            "timestamp": timestamp
        }, params);
        
        String signature = this->createSignature(timestamp, method, path,
                                               request.dump());
        request["sign"] = signature;
        
        if (method == "GET") {
            url += "?" + this->urlencode(request);
        } else {
            body = request;
            const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/json";
        }
    }
    
    return url;
}

String Digifinex::createSignature(const String& timestamp, const String& method,
                                 const String& path, const String& body) {
    String message = timestamp + method + path + body;
    return this->hmac(message, this->encode(this->config_.secret),
                     "sha256", "hex");
}

json Digifinex::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "order_id");
    String timestamp = this->safeString(order, "created_date");
    String status = this->parseOrderStatus(this->safeString(order, "status"));
    String symbol = market.symbol;
    String type = this->safeStringLower(order, "type");
    String side = this->safeStringLower(order, "side");
    
    return {
        {"id", id},
        {"clientOrderId", nullptr},
        {"timestamp", this->safeInteger(order, "created_date")},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"status", status},
        {"symbol", symbol},
        {"type", type},
        {"side", side},
        {"price", this->safeFloat(order, "price")},
        {"amount", this->safeFloat(order, "amount")},
        {"filled", this->safeFloat(order, "executed_amount")},
        {"remaining", this->safeFloat(order, "remaining_amount")},
        {"cost", this->safeFloat(order, "executed_amount") * this->safeFloat(order, "price")},
        {"trades", nullptr},
        {"fee", {
            {"currency", market.quote},
            {"cost", this->safeFloat(order, "fee")},
            {"rate", nullptr}
        }},
        {"info", order}
    };
}

json Digifinex::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"0", "open"},
        {"1", "closed"},
        {"2", "canceled"},
        {"3", "canceled"}
    };
    
    return statuses.contains(status) ? statuses.at(status) : status;
}

} // namespace ccxt
