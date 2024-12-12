#include "ccxt/exchanges/bitvavo.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

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

    rateLimit = 60; // 1000 requests per minute

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

// Synchronous API Methods
json Bitvavo::fetchMarkets(const json& params) {
    auto response = this->publicGetMarkets(params);
    return this->parseMarkets(response);
}

json Bitvavo::fetchBalance(const json& params) {
    auto response = this->privateGetBalance(params);
    return this->parseBalance(response);
}

json Bitvavo::createOrder(const String& symbol, const String& type, const String& side,
                         double amount, double price, const json& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        {"market", market["id"]},
        {"side", side},
        {"orderType", type}
    };
    if (type == "limit") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    request["amount"] = this->amountToPrecision(symbol, amount);
    auto response = this->privatePostOrder(this->extend(request, params));
    return this->parseOrder(response, market);
}

// Async API Methods
boost::future<json> Bitvavo::fetchMarketsAsync(const json& params) {
    return boost::async(boost::launch::async, [this, params]() {
        return this->fetchMarkets(params);
    });
}

boost::future<json> Bitvavo::fetchBalanceAsync(const json& params) {
    return boost::async(boost::launch::async, [this, params]() {
        return this->fetchBalance(params);
    });
}

boost::future<json> Bitvavo::createOrderAsync(const String& symbol, const String& type,
                                            const String& side, double amount,
                                            double price, const json& params) {
    return boost::async(boost::launch::async, [this, symbol, type, side, amount, price, params]() {
        return this->createOrder(symbol, type, side, amount, price, params);
    });
}

// Helper Methods
String Bitvavo::sign(const String& path, const String& api,
                     const String& method, const json& params,
                     const std::map<String, String>& headers,
                     const json& body) {
    auto url = this->urls["api"][api] + "/" + this->implodeParams(path, params);
    auto query = this->omit(params, this->extractParams(path));
    
    if (api == "public") {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->checkRequiredCredentials();
        auto nonce = this->getNonce();
        auto request = "";
        if (method == "GET") {
            if (!query.empty()) {
                url += "?" + this->urlencode(query);
            }
        } else {
            if (!query.empty()) {
                body = query;
                request = this->json(query);
            }
        }
        auto timestamp = std::to_string(nonce);
        auto auth = timestamp + method + "/v2/" + path;
        if (request != "") {
            auth += request;
        }
        auto signature = this->hmac(auth, this->secret, "sha256");
        headers["BITVAVO-ACCESS-KEY"] = this->apiKey;
        headers["BITVAVO-ACCESS-SIGNATURE"] = signature;
        headers["BITVAVO-ACCESS-TIMESTAMP"] = timestamp;
        headers["BITVAVO-ACCESS-WINDOW"] = "10000";
    }
    return url;
}

String Bitvavo::getNonce() {
    return std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count());
}

json Bitvavo::parseOrder(const json& order, const Market& market) {
    auto timestamp = this->safeInteger(order, "created");
    auto status = this->parseOrderStatus(this->safeString(order, "status"));
    auto side = this->safeString(order, "side");
    auto type = this->parseOrderType(this->safeString(order, "orderType"));
    auto id = this->safeString(order, "orderId");
    auto marketId = this->safeString(order, "market");
    auto symbol = this->safeSymbol(marketId, market);
    auto price = this->safeString(order, "price");
    auto amount = this->safeString(order, "amount");
    auto cost = this->safeString(order, "filledAmount");
    auto filled = this->safeString(order, "filledAmount");
    auto remaining = this->safeString(order, "remainingAmount");
    
    return {
        {"id", id},
        {"clientOrderId", nullptr},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"status", status},
        {"symbol", symbol},
        {"type", type},
        {"timeInForce", nullptr},
        {"postOnly", nullptr},
        {"side", side},
        {"price", this->parseNumber(price)},
        {"stopPrice", nullptr},
        {"cost", this->parseNumber(cost)},
        {"amount", this->parseNumber(amount)},
        {"filled", this->parseNumber(filled)},
        {"remaining", this->parseNumber(remaining)},
        {"trades", nullptr},
        {"fee", nullptr},
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

String Bitvavo::parseOrderType(const String& type) {
    static const std::map<String, String> types = {
        {"limit", "limit"},
        {"market", "market"},
        {"stop", "stop"},
        {"stopLimit", "stop_limit"}
    };
    return this->safeString(types, type, type);
}

} // namespace ccxt
