#include "bequant.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Bequant::Bequant() {
    id = "bequant";
    name = "Bequant";
    version = "2";
    certified = true;
    pro = true;
    hasPublicAPI = true;
    hasPrivateAPI = true;
    hasFiatAPI = false;
    hasMarginAPI = true;
    hasFuturesAPI = true;
    hasOptionsAPI = false;
    hasLeveragedAPI = true;

    // Initialize URLs
    baseUrl = "https://api.bequant.io";
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/55248342-a75dfe00-525a-11e9-8aa2-05e9dca943c6.jpg"},
        {"api", {
            {"public", "https://api.bequant.io/api/2"},
            {"private", "https://api.bequant.io/api/2"},
            {"futures", "https://api.bequant.io/api/2/futures"}
        }},
        {"www", "https://bequant.io"},
        {"doc", {
            "https://api.bequant.io/",
            "https://api.bequant.io/api/2/explore/",
            "https://api.bequant.io/api/2/futures/explore/"
        }},
        {"fees", "https://bequant.io/fees-and-limits"}
    };

    initializeApiEndpoints();
    initializeTimeframes();
    initializeMarketTypes();
    initializeOptions();
    initializeErrorCodes();
    initializeFees();
}

void Bequant::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "public/symbol",
                "public/ticker",
                "public/ticker/{symbol}",
                "public/orderbook/{symbol}",
                "public/trades/{symbol}",
                "public/candles/{symbol}",
                "public/currency",
                "public/funding/currencies",
                "public/funding/rates",
                "public/funding/rate_history"
            }}
        }},
        {"private", {
            {"GET", {
                "trading/balance",
                "trading/fee/{symbol}",
                "trading/order",
                "trading/order/{clientOrderId}",
                "trading/order/{orderId}/trades",
                "trading/trade",
                "margin/account",
                "margin/position",
                "margin/position/history",
                "margin/position/fee"
            }},
            {"POST", {
                "trading/order",
                "margin/position/close",
                "margin/position/close/all",
                "margin/position/update",
                "margin/position/update/all"
            }},
            {"DELETE", {
                "trading/order",
                "trading/order/{clientOrderId}"
            }}
        }},
        {"futures", {
            {"GET", {
                "public/futures/info",
                "public/futures/settlement",
                "public/futures/positions",
                "public/futures/funding",
                "public/futures/candles/{symbol}"
            }},
            {"POST", {
                "futures/order",
                "futures/position/close",
                "futures/position/close/all"
            }},
            {"DELETE", {
                "futures/order",
                "futures/order/{clientOrderId}"
            }}
        }}
    };
}

void Bequant::initializeTimeframes() {
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
}

json Bequant::fetchMarkets(const json& params) {
    json response = fetch("/public/symbol", "public", "GET", params);
    json result = json::array();
    
    for (const auto& market : response) {
        String id = market["id"].get<String>();
        String baseId = market["baseCurrency"].get<String>();
        String quoteId = market["quoteCurrency"].get<String>();
        String base = this->safeCurrencyCode(baseId);
        String quote = this->safeCurrencyCode(quoteId);
        String symbol = base + "/" + quote;
        String type = "spot";
        bool spot = true;
        bool margin = market["marginTrading"].get<bool>();
        bool future = false;
        
        if (market.contains("futuresContract") && market["futuresContract"].get<bool>()) {
            type = "future";
            spot = false;
            future = true;
        }
        
        result.push_back({
            {"id", id},
            {"symbol", symbol},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", market["trading"].get<bool>()},
            {"type", type},
            {"spot", spot},
            {"margin", margin},
            {"future", future},
            {"option", false},
            {"contract", future},
            {"precision", {
                {"amount", market["quantityIncrement"].get<int>()},
                {"price", market["tickSize"].get<int>()}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeFloat(market, "minQuantity")},
                    {"max", this->safeFloat(market, "maxQuantity")}
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

json Bequant::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/trading/balance", "private", "GET", params);
    return parseBalance(response);
}

json Bequant::createOrder(const String& symbol, const String& type,
                         const String& side, double amount,
                         double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    String uppercaseType = type.toUpperCase();
    
    json request = {
        {"symbol", market["id"]},
        {"side", side.toUpperCase()},
        {"type", uppercaseType},
        {"quantity", this->amountToPrecision(symbol, amount)}
    };
    
    if (uppercaseType == "LIMIT") {
        request["price"] = this->priceToPrecision(symbol, price);
        request["timeInForce"] = "GTC";
    }
    
    json response = fetch("/trading/order", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response, market);
}

String Bequant::sign(const String& path, const String& api,
                    const String& method, const json& params,
                    const std::map<String, String>& headers,
                    const json& body) {
    String url = this->urls["api"][api] + path;
    
    if (api == "private" || api == "futures") {
        this->checkRequiredCredentials();
        String timestamp = std::to_string(this->milliseconds());
        String auth = timestamp + method + path;
        
        if (method == "POST" || method == "PUT" || method == "DELETE") {
            if (!params.empty()) {
                body = this->json(params);
                auth += body;
            }
        } else {
            if (!params.empty()) {
                String query = this->urlencode(this->keysort(params));
                url += "?" + query;
                auth += "?" + query;
            }
        }
        
        String signature = this->hmac(auth, this->encode(this->secret),
                                    "sha256", "hex");
        
        const_cast<std::map<String, String>&>(headers)["X-API-KEY"] = this->apiKey;
        const_cast<std::map<String, String>&>(headers)["X-API-SIGNATURE"] = signature;
        const_cast<std::map<String, String>&>(headers)["X-API-TIMESTAMP"] = timestamp;
        
        if (method == "POST" || method == "PUT" || method == "DELETE") {
            const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/json";
        }
    } else {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    }
    
    return url;
}

String Bequant::getNonce() {
    return std::to_string(this->milliseconds());
}

json Bequant::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "id");
    String clientOrderId = this->safeString(order, "clientOrderId");
    String timestamp = this->safeString(order, "createdAt");
    String status = this->parseOrderStatus(this->safeString(order, "status"));
    String symbol = nullptr;
    
    if (!market.empty()) {
        symbol = market["symbol"];
    } else {
        String marketId = this->safeString(order, "symbol");
        if (marketId != nullptr) {
            if (this->markets_by_id.contains(marketId)) {
                market = this->markets_by_id[marketId];
                symbol = market["symbol"];
            } else {
                symbol = marketId;
            }
        }
    }
    
    String type = this->safeStringLower(order, "type");
    String side = this->safeStringLower(order, "side");
    
    return {
        {"id", id},
        {"clientOrderId", clientOrderId},
        {"timestamp", this->parse8601(timestamp)},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"type", type},
        {"timeInForce", this->safeString(order, "timeInForce")},
        {"postOnly", nullptr},
        {"status", status},
        {"symbol", symbol},
        {"side", side},
        {"price", this->safeFloat(order, "price")},
        {"amount", this->safeFloat(order, "quantity")},
        {"filled", this->safeFloat(order, "cumQuantity")},
        {"remaining", this->safeFloat(order, "quantity") - this->safeFloat(order, "cumQuantity")},
        {"cost", this->safeFloat(order, "cumQuantity") * this->safeFloat(order, "avgPrice")},
        {"trades", nullptr},
        {"fee", nullptr},
        {"info", order}
    };
}

String Bequant::parseOrderStatus(const String& status) {
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
