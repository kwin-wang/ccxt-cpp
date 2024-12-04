#include "binanceus.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Binanceus::Binanceus() {
    id = "binanceus";
    name = "Binance US";
    version = "v1";
    certified = true;
    pro = false;
    hasPublicAPI = true;
    hasPrivateAPI = true;
    hasFiatAPI = true;
    hasMarginAPI = false;
    hasFuturesAPI = false;
    hasOptionsAPI = false;

    // Initialize URLs
    baseUrl = "https://api.binance.us";
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/65177307-217b7c80-da5f-11e9-876e-0b748ba0a358.jpg"},
        {"api", {
            {"public", "https://api.binance.us/api/v3"},
            {"private", "https://api.binance.us/api/v3"},
            {"web", "https://api.binance.us/wapi/v3"},
            {"sapi", "https://api.binance.us/sapi/v1"}
        }},
        {"www", "https://www.binance.us"},
        {"doc", {
            "https://github.com/binance-us/binance-official-api-docs/blob/master/rest-api.md",
            "https://github.com/binance-us/binance-official-api-docs"
        }},
        {"fees", "https://www.binance.us/en/fee/schedule"}
    };

    initializeApiEndpoints();
    initializeTimeframes();
    initializeMarketTypes();
    initializeOptions();
    initializeErrorCodes();
    initializeFees();
}

void Binanceus::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "ping",
                "time",
                "exchangeInfo",
                "depth",
                "trades",
                "historicalTrades",
                "aggTrades",
                "klines",
                "ticker/24hr",
                "ticker/price",
                "ticker/bookTicker"
            }}
        }},
        {"private", {
            {"GET", {
                "order",
                "openOrders",
                "allOrders",
                "account",
                "myTrades",
                "rateLimit/order"
            }},
            {"POST", {
                "order",
                "order/test"
            }},
            {"DELETE", {
                "order",
                "openOrders"
            }}
        }},
        {"sapi", {
            {"GET", {
                "asset/tradeFee",
                "asset/assetDetail",
                "account/apiRestrictions"
            }}
        }}
    };
}

void Binanceus::initializeTimeframes() {
    timeframes = {
        {"1m", "1m"},
        {"3m", "3m"},
        {"5m", "5m"},
        {"15m", "15m"},
        {"30m", "30m"},
        {"1h", "1h"},
        {"2h", "2h"},
        {"4h", "4h"},
        {"6h", "6h"},
        {"8h", "8h"},
        {"12h", "12h"},
        {"1d", "1d"},
        {"3d", "3d"},
        {"1w", "1w"},
        {"1M", "1M"}
    };
}

json Binanceus::fetchMarkets(const json& params) {
    json response = fetch("/exchangeInfo", "public", "GET", params);
    json markets = response["symbols"];
    json result = json::array();
    
    for (const auto& market : markets) {
        if (market["status"] == "TRADING") {
            String id = market["symbol"].get<String>();
            String baseId = market["baseAsset"].get<String>();
            String quoteId = market["quoteAsset"].get<String>();
            String base = this->safeCurrencyCode(baseId);
            String quote = this->safeCurrencyCode(quoteId);
            String symbol = base + "/" + quote;
            
            json filters = market["filters"];
            json priceFilter = this->safeValue(filters, 0, json::object());
            json lotSizeFilter = this->safeValue(filters, 1, json::object());
            json minNotionalFilter = this->safeValue(filters, 2, json::object());
            
            result.push_back({
                {"id", id},
                {"symbol", symbol},
                {"base", base},
                {"quote", quote},
                {"baseId", baseId},
                {"quoteId", quoteId},
                {"info", market},
                {"type", "spot"},
                {"spot", true},
                {"margin", false},
                {"future", false},
                {"active", true},
                {"precision", {
                    {"price", this->precisionFromString(priceFilter["tickSize"].get<String>())},
                    {"amount", this->precisionFromString(lotSizeFilter["stepSize"].get<String>())}
                }},
                {"limits", {
                    {"amount", {
                        {"min", this->safeFloat(lotSizeFilter, "minQty")},
                        {"max", this->safeFloat(lotSizeFilter, "maxQty")}
                    }},
                    {"price", {
                        {"min", this->safeFloat(priceFilter, "minPrice")},
                        {"max", this->safeFloat(priceFilter, "maxPrice")}
                    }},
                    {"cost", {
                        {"min", this->safeFloat(minNotionalFilter, "minNotional")},
                        {"max", nullptr}
                    }}
                }}
            });
        }
    }
    
    return result;
}

json Binanceus::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/account", "private", "GET", params);
    return parseBalance(response);
}

json Binanceus::parseBalance(const json& response) {
    json result = {{"info", response}};
    json balances = this->safeValue(response, "balances", json::array());
    
    for (const auto& balance : balances) {
        String currencyId = balance["asset"].get<String>();
        String code = this->safeCurrencyCode(currencyId);
        String account = {
            {"free", this->safeFloat(balance, "free")},
            {"used", this->safeFloat(balance, "locked")},
            {"total", this->safeFloat(balance, "free") + this->safeFloat(balance, "locked")}
        };
        result[code] = account;
    }
    
    return result;
}

json Binanceus::createOrder(const String& symbol, const String& type,
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
    
    json response = fetch("/order", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response, market);
}

String Binanceus::sign(const String& path, const String& api,
                      const String& method, const json& params,
                      const std::map<String, String>& headers,
                      const json& body) {
    String url = this->urls["api"][api] + path;
    
    if (api == "private" || api == "sapi") {
        this->checkRequiredCredentials();
        String timestamp = std::to_string(this->milliseconds());
        String query = "";
        
        if (method == "GET") {
            if (!params.empty()) {
                query = this->urlencode(this->keysort(params));
            }
        } else {
            if (!params.empty()) {
                body = this->json(params);
            }
        }
        
        String auth = timestamp;
        if (!query.empty()) {
            auth += "&" + query;
        }
        
        String signature = this->hmac(auth, this->encode(this->secret),
                                    "sha256", "hex");
        
        if (method == "GET") {
            if (!params.empty()) {
                url += "?" + query;
            }
            url += "&timestamp=" + timestamp + "&signature=" + signature;
        } else {
            url += "?timestamp=" + timestamp + "&signature=" + signature;
        }
        
        if (method == "POST" || method == "PUT" || method == "DELETE") {
            const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/x-www-form-urlencoded";
        }
        
        const_cast<std::map<String, String>&>(headers)["X-MBX-APIKEY"] = this->apiKey;
    } else {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    }
    
    return url;
}

String Binanceus::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"NEW", "open"},
        {"PARTIALLY_FILLED", "open"},
        {"FILLED", "closed"},
        {"CANCELED", "canceled"},
        {"PENDING_CANCEL", "canceling"},
        {"REJECTED", "rejected"},
        {"EXPIRED", "expired"}
    };
    
    return this->safeString(statuses, status, status);
}

json Binanceus::parseOrder(const json& order, const Market& market) {
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
    
    double timestamp = this->safeInteger(order, "time");
    double price = this->safeFloat(order, "price");
    double amount = this->safeFloat(order, "origQty");
    double filled = this->safeFloat(order, "executedQty");
    double remaining = amount - filled;
    
    return {
        {"id", this->safeString(order, "orderId")},
        {"clientOrderId", this->safeString(order, "clientOrderId")},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", this->safeInteger(order, "updateTime")},
        {"symbol", symbol},
        {"type", this->safeStringLower(order, "type")},
        {"side", this->safeStringLower(order, "side")},
        {"price", price},
        {"amount", amount},
        {"cost", price * filled},
        {"average", this->safeFloat(order, "avgPrice")},
        {"filled", filled},
        {"remaining", remaining},
        {"status", status},
        {"fee", nullptr},
        {"trades", nullptr},
        {"info", order}
    };
}

} // namespace ccxt
