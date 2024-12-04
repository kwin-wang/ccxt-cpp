#include "poloniex.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Poloniex::Poloniex() {
    id = "poloniex";
    name = "Poloniex";
    version = "v1";
    rateLimit = 1000;
    hasPrivateAPI = true;
    lastNonce = 0;

    // Initialize API endpoints
    baseUrl = "https://api.poloniex.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/27766817-e9456312-5ee6-11e7-9b3c-b628ca5626a5.jpg"},
        {"api", {
            {"public", baseUrl},
            {"private", baseUrl}
        }},
        {"www", "https://poloniex.com"},
        {"doc", {
            "https://docs.poloniex.com",
            "https://docs.poloniex.com/#http-api",
            "https://docs.poloniex.com/#websocket-api"
        }},
        {"fees", "https://poloniex.com/fees"}
    };

    timeframes = {
        {"1m", "MINUTE_1"},
        {"5m", "MINUTE_5"},
        {"15m", "MINUTE_15"},
        {"30m", "MINUTE_30"},
        {"1h", "HOUR_1"},
        {"2h", "HOUR_2"},
        {"4h", "HOUR_4"},
        {"6h", "HOUR_6"},
        {"12h", "HOUR_12"},
        {"1d", "DAY_1"},
        {"3d", "DAY_3"},
        {"1w", "WEEK_1"},
        {"1M", "MONTH_1"}
    };

    options = {
        {"recvWindow", "10000"},
        {"adjustForTimeDifference", true},
        {"warnOnFetchOpenOrdersWithoutSymbol", true}
    };

    initializeApiEndpoints();
}

void Poloniex::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "markets",
                "markets/{symbol}/price",
                "markets/{symbol}/orderBook",
                "markets/{symbol}/candles",
                "markets/{symbol}/trades",
                "currencies",
                "timestamp"
            }}
        }},
        {"private", {
            {"GET", {
                "accounts",
                "orders",
                "orders/{id}",
                "trades",
                "trades/{id}",
                "wallets/addresses",
                "wallets/activity",
                "wallets/fees"
            }},
            {"POST", {
                "orders",
                "orders/test",
                "wallets/addresses",
                "wallets/withdraw"
            }},
            {"DELETE", {
                "orders",
                "orders/{id}"
            }}
        }}
    };
}

json Poloniex::fetchMarkets(const json& params) {
    json response = fetch("/markets", "public", "GET", params);
    json markets = json::array();
    
    for (const auto& market : response) {
        String id = market["symbol"];
        std::vector<String> parts = this->split(id, "_");
        String baseId = parts[0];
        String quoteId = parts[1];
        String base = this->commonCurrencyCode(baseId);
        String quote = this->commonCurrencyCode(quoteId);
        String symbol = base + "/" + quote;
        bool active = market["state"] == "ONLINE";
        
        markets.push_back({
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
                {"amount", market["baseScale"].get<int>()},
                {"price", market["quoteScale"].get<int>()}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeFloat(market, "minBaseAmount")},
                    {"max", this->safeFloat(market, "maxBaseAmount")}
                }},
                {"price", {
                    {"min", this->safeFloat(market, "minPriceQuote")},
                    {"max", this->safeFloat(market, "maxPriceQuote")}
                }},
                {"cost", {
                    {"min", this->safeFloat(market, "minQuoteAmount")},
                    {"max", this->safeFloat(market, "maxQuoteAmount")}
                }}
            }},
            {"info", market}
        });
    }
    
    return markets;
}

json Poloniex::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/accounts/balances", "private", "GET", params);
    json result = {"info", response};
    
    for (const auto& balance : response) {
        String currencyId = balance["currency"];
        String code = this->commonCurrencyCode(currencyId);
        
        result[code] = {
            {"free", this->safeFloat(balance, "available")},
            {"used", this->safeFloat(balance, "onOrders")},
            {"total", this->safeFloat(balance, "total")}
        };
    }
    
    return result;
}

json Poloniex::createOrder(const String& symbol, const String& type,
                          const String& side, double amount,
                          double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"symbol", market.id},
        {"side", side.upper()},
        {"amount", this->amountToPrecision(symbol, amount)}
    };
    
    if (type == "limit") {
        request["type"] = "LIMIT";
        request["price"] = this->priceToPrecision(symbol, price);
    } else {
        request["type"] = "MARKET";
    }
    
    json response = fetch("/orders", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response, market);
}

String Poloniex::sign(const String& path, const String& api,
                      const String& method, const json& params,
                      const std::map<String, String>& headers,
                      const json& body) {
    String url = this->urls["api"][api];
    String endpoint = "/" + this->version + "/" + this->implodeParams(path, params);
    url += endpoint;
    
    json query = this->omit(params, this->extractParams(path));
    
    if (api == "public") {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->checkRequiredCredentials();
        String nonce = this->getNonce();
        String auth = endpoint + nonce;
        
        if (method == "GET" || method == "DELETE") {
            if (!query.empty()) {
                url += "?" + this->urlencode(query);
                auth += this->urlencode(query);
            }
        } else {
            if (!query.empty()) {
                body = this->json(query);
                auth += body.dump();
            }
        }
        
        String signature = this->hmac(auth, this->base64ToBinary(this->secret),
                                    "sha256", "hex");
        
        const_cast<std::map<String, String>&>(headers)["Key"] = this->apiKey;
        const_cast<std::map<String, String>&>(headers)["Signature"] = signature;
        const_cast<std::map<String, String>&>(headers)["Nonce"] = nonce;
        
        if (body != nullptr) {
            const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/json";
        }
    }
    
    return url;
}

String Poloniex::getNonce() {
    int64_t currentNonce = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    if (currentNonce <= lastNonce) {
        currentNonce = lastNonce + 1;
    }
    lastNonce = currentNonce;
    return std::to_string(currentNonce);
}

json Poloniex::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "id");
    String timestamp = this->safeString(order, "timestamp");
    String status = this->parseOrderStatus(this->safeString(order, "state"));
    String symbol = market.symbol;
    String type = this->safeStringLower(order, "type");
    String side = this->safeStringLower(order, "side");
    
    return {
        {"id", id},
        {"clientOrderId", this->safeString(order, "clientOrderId")},
        {"timestamp", this->parse8601(timestamp)},
        {"datetime", timestamp},
        {"lastTradeTimestamp", nullptr},
        {"status", status},
        {"symbol", symbol},
        {"type", type},
        {"side", side},
        {"price", this->safeFloat(order, "price")},
        {"amount", this->safeFloat(order, "amount")},
        {"filled", this->safeFloat(order, "filledAmount")},
        {"remaining", this->safeFloat(order, "remainingAmount")},
        {"cost", this->safeFloat(order, "cost")},
        {"trades", nullptr},
        {"fee", {
            {"currency", market.quote},
            {"cost", this->safeFloat(order, "fee")}
        }},
        {"info", order}
    };
}

json Poloniex::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"OPEN", "open"},
        {"PENDING", "open"},
        {"FILLED", "closed"},
        {"PARTIALLY_FILLED", "open"},
        {"CANCELLED", "canceled"},
        {"EXPIRED", "expired"}
    };
    
    return statuses.contains(status) ? statuses.at(status) : status;
}

} // namespace ccxt
