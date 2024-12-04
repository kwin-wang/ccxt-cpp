#include "wazirx.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

WazirX::WazirX() {
    id = "wazirx";
    name = "WazirX";
    version = "v2";
    rateLimit = 100;

    // Initialize API endpoints
    baseUrl = "https://api.wazirx.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/148647666-c109c20b-f8ac-472f-91c3-5f658cb90f49.jpeg"},
        {"api", {
            {"public", "https://api.wazirx.com/sapi/v1"},
            {"private", "https://api.wazirx.com/sapi/v1"}
        }},
        {"www", "https://wazirx.com"},
        {"doc", {
            "https://docs.wazirx.com/#public-rest-api-for-wazirx",
            "https://docs.wazirx.com/"
        }},
        {"fees", "https://wazirx.com/fees"}
    };

    timeframes = {
        {"1m", "1m"},
        {"5m", "5m"},
        {"15m", "15m"},
        {"30m", "30m"},
        {"1h", "1h"},
        {"2h", "2h"},
        {"4h", "4h"},
        {"6h", "6h"},
        {"12h", "12h"},
        {"1d", "1d"},
        {"1w", "1w"}
    };

    options = {
        {"adjustForTimeDifference", true},
        {"recvWindow", "5000"}
    };

    errorCodes = {
        {-1000, "System error"},
        {-1001, "Internal error"},
        {-1002, "Unauthorized"},
        {-1003, "Too many requests"},
        {-1004, "Invalid API key"},
        {-1005, "Invalid signature"},
        {-1006, "Invalid timestamp"},
        {-1007, "Invalid parameters"},
        {-1008, "Order not found"},
        {-1009, "Balance not enough"},
        {-1010, "Order amount too small"},
        {-1011, "Order price out of range"},
        {-1012, "Order has been filled"},
        {-1013, "Order has been cancelled"},
        {-1014, "Order is cancelling"},
        {-1015, "Order type not supported"},
        {-1016, "Invalid order side"}
    };

    initializeApiEndpoints();
}

void WazirX::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "ping",
                "time",
                "systemStatus",
                "exchangeInfo",
                "depth",
                "trades",
                "klines",
                "ticker/24hr",
                "ticker/price",
                "ticker/bookTicker"
            }}
        }},
        {"private", {
            {"GET", {
                "account",
                "allOrders",
                "openOrders",
                "myTrades",
                "funds",
                "deposits",
                "withdrawals",
                "depositAddress"
            }},
            {"POST", {
                "order",
                "order/test"
            }},
            {"DELETE", {
                "order",
                "openOrders"
            }}
        }}
    };
}

json WazirX::fetchMarkets(const json& params) {
    json response = fetch("/exchangeInfo", "public", "GET", params);
    json result = json::array();
    
    for (const auto& market : response["symbols"]) {
        String id = this->safeString(market, "symbol");
        String baseId = this->safeString(market, "baseAsset");
        String quoteId = this->safeString(market, "quoteAsset");
        String base = this->commonCurrencyCode(baseId);
        String quote = this->commonCurrencyCode(quoteId);
        String symbol = base + "/" + quote;
        bool active = market["status"] == "trading";
        
        result.push_back({
            {"id", id},
            {"symbol", symbol},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", active},
            {"type", "spot"},
            {"spot", true},
            {"future", false},
            {"swap", false},
            {"option", false},
            {"contract", false},
            {"precision", {
                {"amount", market["baseAssetPrecision"].get<int>()},
                {"price", market["quotePrecision"].get<int>()}
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

json WazirX::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/account", "private", "GET", params);
    return parseBalance(response);
}

json WazirX::parseBalance(const json& response) {
    json result = {"info", response};
    json balances = response["balances"];
    
    for (const auto& balance : balances) {
        String currencyId = balance["asset"];
        String code = this->commonCurrencyCode(currencyId);
        
        result[code] = {
            {"free", this->safeFloat(balance, "free")},
            {"used", this->safeFloat(balance, "locked")},
            {"total", this->safeFloat(balance, "free") + this->safeFloat(balance, "locked")}
        };
    }
    
    return result;
}

json WazirX::createOrder(const String& symbol, const String& type,
                        const String& side, double amount,
                        double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"symbol", market.id},
        {"side", side.upper()},
        {"type", type.upper()},
        {"quantity", this->amountToPrecision(symbol, amount)}
    };
    
    if (type == "limit") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    json response = fetch("/order", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response, market);
}

String WazirX::sign(const String& path, const String& api,
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
            "timestamp": timestamp,
            "recvWindow": this->options["recvWindow"]
        }, params);
        
        String signature = this->createSignature(timestamp, method, path,
                                               request.dump());
        request["signature"] = signature;
        
        if (method == "GET") {
            url += "?" + this->urlencode(request);
        } else {
            body = request;
            const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/json";
        }
        
        const_cast<std::map<String, String>&>(headers)["X-API-KEY"] = this->apiKey;
    }
    
    return url;
}

String WazirX::createSignature(const String& timestamp, const String& method,
                              const String& path, const String& body) {
    String message = timestamp + method + path + body;
    return this->hmac(message, this->encode(this->secret),
                     "sha256", "hex");
}

json WazirX::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "orderId");
    String timestamp = this->safeString(order, "time");
    String status = this->parseOrderStatus(this->safeString(order, "status"));
    String symbol = market.symbol;
    String type = this->safeStringLower(order, "type");
    String side = this->safeStringLower(order, "side");
    
    return {
        {"id", id},
        {"clientOrderId", this->safeString(order, "clientOrderId")},
        {"timestamp", this->safeInteger(order, "time")},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"status", status},
        {"symbol", symbol},
        {"type", type},
        {"side", side},
        {"price", this->safeFloat(order, "price")},
        {"amount", this->safeFloat(order, "origQty")},
        {"filled", this->safeFloat(order, "executedQty")},
        {"remaining", this->safeFloat(order, "origQty") - this->safeFloat(order, "executedQty")},
        {"cost", this->safeFloat(order, "cummulativeQuoteQty")},
        {"trades", nullptr},
        {"fee", {
            {"currency", market.quote},
            {"cost", this->safeFloat(order, "commission")},
            {"rate", this->safeFloat(order, "commissionRate")}
        }},
        {"info", order}
    };
}

json WazirX::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"NEW", "open"},
        {"PARTIALLY_FILLED", "open"},
        {"FILLED", "closed"},
        {"CANCELED", "canceled"},
        {"PENDING_CANCEL", "canceling"},
        {"REJECTED", "rejected"},
        {"EXPIRED", "expired"}
    };
    
    return statuses.contains(status) ? statuses.at(status) : status;
}

} // namespace ccxt
