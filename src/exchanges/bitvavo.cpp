#include "bitvavo.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Bitvavo::Bitvavo() {
    id = "bitvavo";
    name = "Bitvavo";
    version = "v2";
    certified = true;
    pro = false;
    hasPublicAPI = true;
    hasPrivateAPI = true;
    hasFiatAPI = true;
    hasMarginAPI = false;
    hasFuturesAPI = false;

    // Initialize URLs
    baseUrl = "https://api.bitvavo.com";
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/83165440-2f1cf200-a116-11ea-9046-a255d09fb2ed.jpg"},
        {"api", {
            {"public", "https://api.bitvavo.com/v2"},
            {"private", "https://api.bitvavo.com/v2"}
        }},
        {"www", "https://bitvavo.com/"},
        {"doc", {
            "https://docs.bitvavo.com/",
            "https://github.com/bitvavo/node-bitvavo-api"
        }},
        {"fees", "https://bitvavo.com/en/fees"}
    };

    initializeApiEndpoints();
    initializeTimeframes();
    initializeMarketTypes();
    initializeOptions();
    initializeErrorCodes();
    initializeFees();
}

void Bitvavo::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "time",
                "markets",
                "assets",
                "book",
                "trades",
                "candles",
                "ticker/24h",
                "ticker/price",
                "ticker/book"
            }}
        }},
        {"private", {
            {"GET", {
                "account",
                "order",
                "orders",
                "ordersOpen",
                "trades",
                "balance",
                "deposit",
                "depositHistory",
                "withdrawalHistory"
            }},
            {"POST", {
                "order",
                "withdrawal"
            }},
            {"DELETE", {
                "order",
                "orders"
            }}
        }}
    };
}

void Bitvavo::initializeTimeframes() {
    timeframes = {
        {"1m", "1m"},
        {"5m", "5m"},
        {"15m", "15m"},
        {"30m", "30m"},
        {"1h", "1h"},
        {"2h", "2h"},
        {"4h", "4h"},
        {"6h", "6h"},
        {"8h", "8h"},
        {"12h", "12h"},
        {"1d", "1d"}
    };
}

json Bitvavo::fetchMarkets(const json& params) {
    json response = fetch("/markets", "public", "GET", params);
    json result = json::array();
    
    for (const auto& market : response) {
        String id = market["market"].get<String>();
        String baseId = market["base"].get<String>();
        String quoteId = market["quote"].get<String>();
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
            {"active", market["status"].get<String>() == "trading"},
            {"precision", {
                {"amount", market["pricePrecision"].get<int>()},
                {"price", market["pricePrecision"].get<int>()}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeFloat(market, "minOrderInBaseAsset")},
                    {"max", this->safeFloat(market, "maxOrderInBaseAsset")}
                }},
                {"price", {
                    {"min", this->safeFloat(market, "minOrderInQuoteAsset")},
                    {"max", this->safeFloat(market, "maxOrderInQuoteAsset")}
                }},
                {"cost", {
                    {"min", this->safeFloat(market, "minOrderInQuoteAsset")},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return result;
}

json Bitvavo::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/balance", "private", "GET", params);
    return parseBalance(response);
}

json Bitvavo::parseBalance(const json& response) {
    json result = {{"info", response}};
    
    for (const auto& balance : response) {
        String currencyId = balance["symbol"].get<String>();
        String code = this->safeCurrencyCode(currencyId);
        String account = {
            {"free", this->safeFloat(balance, "available")},
            {"used", this->safeFloat(balance, "inOrder")},
            {"total", this->safeFloat(balance, "available") + this->safeFloat(balance, "inOrder")}
        };
        result[code] = account;
    }
    
    return result;
}

json Bitvavo::createOrder(const String& symbol, const String& type,
                         const String& side, double amount,
                         double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    String uppercaseType = type.toUpperCase();
    
    json request = {
        {"market", market["id"]},
        {"side", side.toLower()},
        {"orderType", uppercaseType.toLower()},
        {"amount", this->amountToPrecision(symbol, amount)}
    };
    
    if (uppercaseType == "LIMIT") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    json response = fetch("/order", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response, market);
}

String Bitvavo::sign(const String& path, const String& api,
                     const String& method, const json& params,
                     const std::map<String, String>& headers,
                     const json& body) {
    String url = this->urls["api"][api] + path;
    String timestamp = std::to_string(this->milliseconds());
    
    if (api == "private") {
        this->checkRequiredCredentials();
        
        json request = params;
        if (method == "GET") {
            if (!params.empty()) {
                url += "?" + this->urlencode(params);
            }
        } else {
            if (!params.empty()) {
                body = this->json(params);
            }
        }
        
        String payload = timestamp + method + "/v2" + path;
        if (body != nullptr) {
            payload += this->json(body);
        }
        
        String signature = this->hmac(payload, this->encode(this->secret),
                                    "sha256", "hex");
        
        const_cast<std::map<String, String>&>(headers)["BITVAVO-ACCESS-KEY"] = this->apiKey;
        const_cast<std::map<String, String>&>(headers)["BITVAVO-ACCESS-SIGNATURE"] = signature;
        const_cast<std::map<String, String>&>(headers)["BITVAVO-ACCESS-TIMESTAMP"] = timestamp;
        const_cast<std::map<String, String>&>(headers)["BITVAVO-ACCESS-WINDOW"] = "10000";
        
        if (method != "GET") {
            const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/json";
        }
    } else {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    }
    
    return url;
}

String Bitvavo::getNonce() {
    return std::to_string(this->milliseconds());
}

json Bitvavo::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "orderId");
    String timestamp = this->safeString(order, "created");
    String status = this->parseOrderStatus(this->safeString(order, "status"));
    String symbol = nullptr;
    
    if (!market.empty()) {
        symbol = market["symbol"];
    } else {
        String marketId = this->safeString(order, "market");
        if (marketId != nullptr) {
            if (this->markets_by_id.contains(marketId)) {
                market = this->markets_by_id[marketId];
                symbol = market["symbol"];
            } else {
                symbol = marketId;
            }
        }
    }
    
    String type = this->safeStringLower(order, "orderType");
    String side = this->safeStringLower(order, "side");
    
    return {
        {"id", id},
        {"clientOrderId", this->safeString(order, "clientOrderId")},
        {"timestamp", this->parse8601(timestamp)},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"type", type},
        {"timeInForce", this->safeString(order, "timeInForce")},
        {"postOnly", this->safeValue(order, "postOnly")},
        {"status", status},
        {"symbol", symbol},
        {"side", side},
        {"price", this->safeFloat(order, "price")},
        {"amount", this->safeFloat(order, "amount")},
        {"filled", this->safeFloat(order, "filledAmount")},
        {"remaining", this->safeFloat(order, "remainingAmount")},
        {"cost", this->safeFloat(order, "filledAmountQuote")},
        {"trades", nullptr},
        {"fee", {
            {"cost", this->safeFloat(order, "feePaid")},
            {"currency", this->safeString(order, "feeCurrency")}
        }},
        {"info", order}
    };
}

String Bitvavo::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"new", "open"},
        {"canceled", "canceled"},
        {"filled", "closed"},
        {"partial", "open"},
        {"rejected", "rejected"}
    };
    
    return this->safeString(statuses, status, status);
}

} // namespace ccxt
