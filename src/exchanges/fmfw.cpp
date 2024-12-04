#include "fmfw.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

FMFW::FMFW() {
    id = "fmfw";
    name = "FMFW";
    version = "2";
    rateLimit = 100;
    certified = true;
    pro = true;
    hasMultipleOrderTypes = true;
    hasMarginTrading = true;
    hasFuturesTrading = true;

    // Initialize API endpoints
    baseUrl = "https://api.fmfw.io";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/69400764-e7c76d00-0d05-11ea-8c76-077342a74f76.jpg"},
        {"api", {
            {"public", "https://api.fmfw.io/api/3"},
            {"private", "https://api.fmfw.io/api/3"}
        }},
        {"www", "https://fmfw.io"},
        {"doc", {
            "https://api.fmfw.io/api/3/docs",
            "https://github.com/fmfwio/api-docs"
        }},
        {"fees", "https://fmfw.io/fees-and-limits"}
    };

    timeframes = {
        {"1m", "M1"},
        {"3m", "M3"},
        {"5m", "M5"},
        {"15m", "M15"},
        {"30m", "M30"},
        {"1h", "H1"},
        {"4h", "H4"},
        {"1d", "D1"},
        {"1w", "D7"},
        {"1M", "1M"}
    };

    options = {
        {"adjustForTimeDifference", true},
        {"recvWindow", 5000},
        {"timeDifference", 0},
        {"defaultType", "spot"},
        {"accountType", "spot"}
    };

    errorCodes = {
        {400, "Bad Request"},
        {401, "Unauthorized"},
        {403, "Forbidden"},
        {404, "Not Found"},
        {429, "Rate Limit Exceeded"},
        {500, "Internal Server Error"},
        {503, "Service Unavailable"},
        {504, "Gateway Timeout"},
        {20001, "Insufficient funds"},
        {20002, "Order not found"},
        {20003, "Quantity below minimum"},
        {20004, "Quantity above maximum"},
        {20005, "Price below minimum"},
        {20006, "Price above maximum"},
        {20007, "Cost below minimum"},
        {20008, "Cost above maximum"},
        {20009, "Trading suspended"},
        {20010, "Invalid order type"},
        {20011, "Invalid side"},
        {20012, "Invalid timeInForce"},
        {20013, "Invalid postOnly"},
        {20014, "Invalid clientOrderId"},
        {20015, "Invalid stopPrice"},
        {20016, "Invalid reduceOnly"}
    };

    initializeApiEndpoints();
}

void FMFW::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "public/currency",
                "public/symbol",
                "public/ticker",
                "public/ticker/{symbol}",
                "public/orderbook/{symbol}",
                "public/trades/{symbol}",
                "public/candles/{symbol}",
                "public/fee/symbol/{symbol}",
                "public/futures/info",
                "public/futures/mark-price/{symbol}",
                "public/futures/funding-rate/{symbol}"
            }}
        }},
        {"private", {
            {"GET", {
                "spot/balance",
                "spot/order/{clientOrderId}",
                "spot/order",
                "spot/trading/order/{clientOrderId}",
                "spot/trading/order",
                "spot/trading/trade",
                "spot/trading/trade/{clientOrderId}",
                "spot/crypto/address/{currency}",
                "spot/crypto/fee/estimate",
                "spot/crypto/fee/estimate/{currency}",
                "spot/transaction",
                "spot/transaction/{id}",
                "margin/account",
                "margin/account/isolated/{symbol}",
                "margin/position",
                "margin/position/isolated/{symbol}",
                "margin/order",
                "margin/order/{clientOrderId}",
                "futures/account",
                "futures/position",
                "futures/position/{symbol}",
                "futures/order",
                "futures/order/{clientOrderId}"
            }},
            {"POST", {
                "spot/order",
                "spot/order/cancel",
                "spot/order/cancel/{clientOrderId}",
                "margin/order",
                "margin/order/cancel",
                "margin/order/cancel/{clientOrderId}",
                "margin/position/close/{symbol}",
                "futures/order",
                "futures/order/cancel",
                "futures/order/cancel/{clientOrderId}",
                "futures/position/close/{symbol}"
            }}
        }}
    };
}

json FMFW::fetchMarkets(const json& params) {
    json response = fetch("/public/symbol", "public", "GET", params);
    json result = json::array();
    
    for (const auto& market : response) {
        String id = market["id"];
        String baseId = market["baseCurrency"];
        String quoteId = market["quoteCurrency"];
        String base = this->safeCurrencyCode(baseId);
        String quote = this->safeCurrencyCode(quoteId);
        String symbol = base + "/" + quote;
        String type = market["type"];
        bool active = market["active"];
        
        result.push_back({
            {"id", id},
            {"symbol", symbol},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", active},
            {"type", type},
            {"spot", type == "spot"},
            {"margin", type == "margin"},
            {"future", type == "futures"},
            {"option", false},
            {"contract", type == "futures"},
            {"precision", {
                {"amount", market["quantityPrecision"]},
                {"price", market["pricePrecision"]}
            }},
            {"limits", {
                {"amount", {
                    {"min", market["minQuantity"]},
                    {"max", market["maxQuantity"]}
                }},
                {"price", {
                    {"min", market["minPrice"]},
                    {"max", market["maxPrice"]}
                }},
                {"cost", {
                    {"min", market["minNotional"]},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return result;
}

json FMFW::fetchBalance(const json& params) {
    this->loadMarkets();
    String type = this->safeString(params, "type", this->options["defaultType"]);
    String accountType = this->safeString(this->options, "accountType", type);
    String method = accountType + "/balance";
    json response = fetch("/" + method, "private", "GET", this->omit(params, "type"));
    return parseBalance(response);
}

json FMFW::parseBalance(const json& response) {
    json result = {{"info", response}};
    
    for (const auto& balance : response) {
        String code = this->safeCurrencyCode(balance["currency"]);
        String account = {
            {"free", this->safeFloat(balance, "available")},
            {"used", this->safeFloat(balance, "reserved")},
            {"total", this->safeFloat(balance, "total")}
        };
        result[code] = account;
    }
    
    return result;
}

json FMFW::createOrder(const String& symbol, const String& type,
                      const String& side, double amount,
                      double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    String accountType = this->safeString(this->options, "accountType", market["type"]);
    String method = accountType + "/order";
    
    json request = {
        {"symbol", market["id"]},
        {"side", side},
        {"quantity", this->amountToPrecision(symbol, amount)},
        {"type", type}
    };
    
    if (type == "limit") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    json response = fetch("/" + method, "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response, market);
}

String FMFW::sign(const String& path, const String& api,
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
        String timestamp = std::to_string(this->milliseconds());
        String payload = timestamp + method + "/" + path;
        
        if (!query.empty()) {
            if (method == "GET") {
                url += "?" + this->urlencode(query);
            } else {
                body = this->json(query);
                payload += body;
            }
        }
        
        String signature = this->hmac(payload, this->encode(this->secret),
                                    "sha256", "hex");
        
        const_cast<std::map<String, String>&>(headers)["API-KEY"] = this->apiKey;
        const_cast<std::map<String, String>&>(headers)["API-TIMESTAMP"] = timestamp;
        const_cast<std::map<String, String>&>(headers)["API-SIGNATURE"] = signature;
        
        if (method != "GET") {
            const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/json";
        }
    }
    
    return url;
}

String FMFW::getNonce() {
    return std::to_string(this->milliseconds());
}

json FMFW::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "id");
    String clientOrderId = this->safeString(order, "clientOrderId");
    String timestamp = this->safeString(order, "createdAt");
    String status = this->parseOrderStatus(this->safeString(order, "status"));
    String symbol = nullptr;
    
    if (!market.empty()) {
        symbol = market["symbol"];
    }
    
    String type = this->safeString(order, "type");
    String side = this->safeString(order, "side");
    
    return {
        {"id", id},
        {"clientOrderId", clientOrderId},
        {"datetime", this->iso8601(timestamp)},
        {"timestamp", this->parse8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"type", type},
        {"timeInForce", this->safeString(order, "timeInForce")},
        {"postOnly", this->safeValue(order, "postOnly")},
        {"status", status},
        {"symbol", symbol},
        {"side", side},
        {"price", this->safeFloat(order, "price")},
        {"stopPrice", this->safeFloat(order, "stopPrice")},
        {"cost", this->safeFloat(order, "cost")},
        {"amount", this->safeFloat(order, "quantity")},
        {"filled", this->safeFloat(order, "cumQuantity")},
        {"remaining", this->safeFloat(order, "quantity") - this->safeFloat(order, "cumQuantity")},
        {"trades", nullptr},
        {"fee", {
            {"currency", market["quote"]},
            {"cost", this->safeFloat(order, "fee")},
            {"rate", this->safeFloat(order, "feeRate")}
        }},
        {"info", order}
    };
}

String FMFW::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"new", "open"},
        {"suspended", "open"},
        {"partiallyFilled", "open"},
        {"filled", "closed"},
        {"canceled", "canceled"},
        {"expired", "expired"}
    };
    
    return this->safeString(statuses, status, status);
}

} // namespace ccxt
