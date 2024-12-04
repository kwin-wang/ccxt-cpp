#include "mexc.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Mexc::Mexc() {
    id = "mexc";
    name = "MEXC Global";
    version = "v3";
    rateLimit = 50;
    testnet = false;

    // Initialize API endpoints
    baseUrl = testnet ? "https://api.testnet.mexc.com" : "https://api.mexc.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/137283979-8b2a818d-8633-461b-bfca-de89e8c446b2.jpg"},
        {"api", {
            {"public", baseUrl},
            {"private", baseUrl},
            {"spot", baseUrl + "/api/v3"},
            {"futures", baseUrl + "/api/futures/v1"}
        }},
        {"www", "https://www.mexc.com"},
        {"doc", {
            "https://mxcdevelop.github.io/APIDoc/",
            "https://mxcdevelop.github.io/apidocs/spot_v3_en/"
        }},
        {"fees", "https://www.mexc.com/fee"}
    };

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

    options = {
        {"defaultType", "spot"},
        {"adjustForTimeDifference", true},
        {"recvWindow", "5000"}
    };

    initializeApiEndpoints();
}

void Mexc::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "api/v3/exchangeInfo",
                "api/v3/ticker/24hr",
                "api/v3/ticker/price",
                "api/v3/ticker/bookTicker",
                "api/v3/depth",
                "api/v3/trades",
                "api/v3/historicalTrades",
                "api/v3/klines",
                "api/v3/avgPrice",
                "api/v3/time"
            }}
        }},
        {"private", {
            {"GET", {
                "api/v3/account",
                "api/v3/openOrders",
                "api/v3/allOrders",
                "api/v3/myTrades",
                "api/v3/order",
                "api/v3/capital/config/getall",
                "api/v3/capital/deposit/hisrec",
                "api/v3/capital/withdraw/history",
                "api/v3/capital/deposit/address",
                "api/v3/capital/transfer"
            }},
            {"POST", {
                "api/v3/order",
                "api/v3/order/test",
                "api/v3/capital/withdraw/apply",
                "api/v3/capital/transfer"
            }},
            {"DELETE", {
                "api/v3/order",
                "api/v3/openOrders"
            }}
        }},
        {"futures", {
            {"GET", {
                "api/futures/v1/account/positions",
                "api/futures/v1/account/risk",
                "api/futures/v1/funding/rate",
                "api/futures/v1/funding/history"
            }},
            {"POST", {
                "api/futures/v1/position/leverage",
                "api/futures/v1/position/margin"
            }}
        }}
    };
}

json Mexc::fetchMarkets(const json& params) {
    json response = fetch("/api/v3/exchangeInfo", "public", "GET", params);
    json markets = json::array();
    
    for (const auto& market : response["symbols"]) {
        String id = market["symbol"];
        String baseId = market["baseAsset"];
        String quoteId = market["quoteAsset"];
        String base = this->commonCurrencyCode(baseId);
        String quote = this->commonCurrencyCode(quoteId);
        bool active = market["status"] == "TRADING";
        
        markets.push_back({
            {"id", id},
            {"symbol", base + "/" + quote},
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
            {"linear", false},
            {"inverse", false},
            {"precision", {
                {"amount", this->precisionFromString(market["baseAssetPrecision"])},
                {"price", this->precisionFromString(market["quotePrecision"])}
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
    
    return markets;
}

json Mexc::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/api/v3/account", "private", "GET", params);
    json balances = response["balances"];
    json result = {"info", response};
    
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

json Mexc::createOrder(const String& symbol, const String& type,
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
        request["timeInForce"] = this->safeString(params, "timeInForce", "GTC");
    }
    
    json response = fetch("/api/v3/order", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response, market);
}

String Mexc::sign(const String& path, const String& api,
                  const String& method, const json& params,
                  const std::map<String, String>& headers,
                  const json& body) {
    String url = this->urls["api"][api] + "/" + path;
    String timestamp = std::to_string(this->milliseconds());
    
    if (api == "public") {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    } else {
        this->checkRequiredCredentials();
        
        String query = "timestamp=" + timestamp;
        
        if (!params.empty()) {
            query += "&" + this->urlencode(this->keysort(params));
        }
        
        String signature = this->hmac(query, this->secret, "sha256", "hex");
        url += "?" + query + "&signature=" + signature;
        
        const_cast<std::map<String, String>&>(headers)["X-MEXC-APIKEY"] = this->apiKey;
        
        if (method == "POST") {
            const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/json";
            if (!params.empty()) {
                body = this->json(params);
            }
        }
    }
    
    return url;
}

json Mexc::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "orderId");
    String timestamp = this->safeInteger(order, "time");
    String status = this->parseOrderStatus(this->safeString(order, "status"));
    String symbol = market.symbol;
    String type = this->safeStringLower(order, "type");
    String side = this->safeStringLower(order, "side");
    
    return {
        {"id", id},
        {"clientOrderId", this->safeString(order, "clientOrderId")},
        {"timestamp", timestamp},
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
        {"fee", nullptr},
        {"info", order}
    };
}

json Mexc::parseOrderStatus(const String& status) {
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
