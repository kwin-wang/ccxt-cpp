#include "phemex.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Phemex::Phemex() {
    id = "phemex";
    name = "Phemex";
    version = "v1";
    rateLimit = 100;
    testnet = false;
    scale = 10000;  // price scale factor
    defaultType = "swap";

    // Initialize API endpoints
    baseUrl = testnet ? "https://testnet-api.phemex.com" : "https://api.phemex.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/85225056-221eb600-b3d7-11ea-930d-564d2690e3f6.jpg"},
        {"api", {
            {"public", baseUrl},
            {"private", baseUrl}
        }},
        {"www", "https://phemex.com"},
        {"doc", {
            "https://github.com/phemex/phemex-api-docs",
            "https://phemex-docs.github.io"
        }},
        {"fees", "https://phemex.com/fees-conditions"},
        {"test", "https://testnet-api.phemex.com"}
    };

    timeframes = {
        {"1m", "60"},
        {"3m", "180"},
        {"5m", "300"},
        {"15m", "900"},
        {"30m", "1800"},
        {"1h", "3600"},
        {"2h", "7200"},
        {"3h", "10800"},
        {"4h", "14400"},
        {"6h", "21600"},
        {"12h", "43200"},
        {"1d", "86400"},
        {"1w", "604800"},
        {"1M", "2592000"}
    };

    initializeApiEndpoints();
}

void Phemex::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "exchange/public/products",
                "md/orderbook",
                "md/trade",
                "md/ticker/24hr",
                "md/kline",
                "md/v2/ticker/24hr",
                "exchange/public/cfg/v2/products",
                "md/v2/public/kline",
                "md/v2/public/orderbook",
                "md/v2/public/trade",
                "public/products",
                "public/nomics/trades",
                "md/v2/public/ticker/24hr",
                "exchange/public/products"
            }}
        }},
        {"private", {
            {"GET", {
                "accounts/accountPositions",
                "exchange/order/list",
                "exchange/order",
                "exchange/order/trade",
                "phemex-user/users/children",
                "phemex-user/order/list",
                "exchange/margin",
                "exchange/wallet/confirm/withdraw",
                "exchange/margin/transfer",
                "exchange/margin/borrowable",
                "exchange/margin/loan",
                "exchange/margin/interest",
                "assets/convert",
                "assets/quote",
                "assets/confirm/convert"
            }},
            {"POST", {
                "orders",
                "positions/leverage",
                "positions/assign",
                "positions/switch-mode",
                "orders/replace",
                "orders/cancel",
                "orders/cancelAll",
                "phemex-user/order",
                "phemex-user/order/replace",
                "phemex-user/order/cancel",
                "exchange/margin/create",
                "exchange/margin/repay",
                "exchange/margin/borrow"
            }}
        }}
    };
}

json Phemex::fetchMarkets(const json& params) {
    json response = fetch("/exchange/public/products", "public", "GET", params);
    json markets = json::array();
    
    for (const auto& market : response["data"]["products"]) {
        String id = market["symbol"];
        String baseId = market["baseCurrency"];
        String quoteId = market["quoteCurrency"];
        String base = this->commonCurrencyCode(baseId);
        String quote = this->commonCurrencyCode(quoteId);
        String type = market["type"].get<String>();
        bool linear = market["settlementCurrency"] == "USD";
        bool inverse = market["settlementCurrency"] == baseId;
        bool active = market["status"] == "Listed";
        
        markets.push_back({
            {"id", id},
            {"symbol", base + "/" + quote},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", active},
            {"type", type},
            {"spot", type == "Spot"},
            {"future", type == "Perpetual"},
            {"swap", type == "Perpetual"},
            {"option", false},
            {"linear", linear},
            {"inverse", inverse},
            {"contract", type != "Spot"},
            {"contractSize", market["contractSize"]},
            {"precision", {
                {"amount", market["lotSize"]},
                {"price", market["tickSize"]}
            }},
            {"limits", {
                {"amount", {
                    {"min", market["minOrderQty"]},
                    {"max", market["maxOrderQty"]}
                }},
                {"price", {
                    {"min", market["tickSize"]},
                    {"max", nullptr}
                }},
                {"cost", {
                    {"min", nullptr},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return markets;
}

json Phemex::fetchTicker(const String& symbol, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {{"symbol", market.id}};
    json response = fetch("/md/ticker/24hr", "public", "GET",
                         this->extend(request, params));
    json ticker = response["result"];
    
    return {
        {"symbol", symbol},
        {"timestamp", ticker["timestamp"]},
        {"datetime", this->iso8601(ticker["timestamp"])},
        {"high", this->parseNumber(ticker["high24h"])},
        {"low", this->parseNumber(ticker["low24h"])},
        {"bid", this->parseNumber(ticker["bidPrice"])},
        {"bidVolume", this->parseNumber(ticker["bidSize"])},
        {"ask", this->parseNumber(ticker["askPrice"])},
        {"askVolume", this->parseNumber(ticker["askSize"])},
        {"vwap", nullptr},
        {"open", this->parseNumber(ticker["openPrice"])},
        {"close", this->parseNumber(ticker["lastPrice"])},
        {"last", this->parseNumber(ticker["lastPrice"])},
        {"previousClose", nullptr},
        {"change", nullptr},
        {"percentage", nullptr},
        {"average", nullptr},
        {"baseVolume", this->parseNumber(ticker["volume24h"])},
        {"quoteVolume", this->parseNumber(ticker["turnover24h"])},
        {"info", ticker}
    };
}

json Phemex::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/accounts/accountPositions", "private", "GET", params);
    json balances = response["data"]["positions"];
    json result = {"info", response};
    
    for (const auto& balance : balances) {
        String currencyId = balance["currency"];
        String code = this->commonCurrencyCode(currencyId);
        
        result[code] = {
            {"free", this->parseNumber(balance["freeBalance"])},
            {"used", this->parseNumber(balance["usedBalance"])},
            {"total", this->parseNumber(balance["totalBalance"])}
        };
    }
    
    return result;
}

json Phemex::createOrder(const String& symbol, const String& type,
                        const String& side, double amount,
                        double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"symbol", market.id},
        {"side", side.upper()},
        {"orderQty", this->formatNumber(amount)},
        {"ordType", type.upper()}
    };
    
    if (type == "limit") {
        if (price == 0) {
            throw InvalidOrder("For limit orders, price cannot be zero");
        }
        request["priceEp"] = this->formatNumber(price * scale);
    }
    
    json response = fetch("/orders", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response["data"], market);
}

String Phemex::sign(const String& path, const String& api,
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
        
        String auth = timestamp + method + path;
        
        if (method == "GET") {
            if (!params.empty()) {
                String query = this->urlencode(this->keysort(params));
                url += "?" + query;
                auth += "?" + query;
            }
        } else {
            if (!params.empty()) {
                body = this->json(params);
                auth += body.dump();
            }
        }
        
        String signature = this->hmac(auth, this->secret, "sha256", "hex");
        
        const_cast<std::map<String, String>&>(headers)["x-phemex-access-token"] = this->apiKey;
        const_cast<std::map<String, String>&>(headers)["x-phemex-request-signature"] = signature;
        const_cast<std::map<String, String>&>(headers)["x-phemex-request-expiry"] = timestamp;
        
        if (method != "GET") {
            const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/json";
        }
    }
    
    return url;
}

double Phemex::parseNumber(const String& numberString) {
    return std::stod(numberString) / scale;
}

String Phemex::formatNumber(double number) {
    return std::to_string(static_cast<int64_t>(number * scale));
}

json Phemex::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "orderID");
    String timestamp = this->safeInteger(order, "createTime");
    String status = this->parseOrderStatus(this->safeString(order, "ordStatus"));
    
    return {
        {"id", id},
        {"clientOrderId", this->safeString(order, "clOrdID")},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", this->safeInteger(order, "transactTime")},
        {"status", status},
        {"symbol", market.symbol},
        {"type", this->safeStringLower(order, "ordType")},
        {"side", this->safeStringLower(order, "side")},
        {"price", this->parseNumber(order["priceEp"])},
        {"amount", this->parseNumber(order["orderQty"])},
        {"filled", this->parseNumber(order["cumQty"])},
        {"remaining", this->parseNumber(order["leavesQty"])},
        {"cost", this->parseNumber(order["cumValueEv"])},
        {"average", order["avgPriceEp"].empty() ? nullptr : this->parseNumber(order["avgPriceEp"])},
        {"trades", nullptr},
        {"fee", nullptr},
        {"info", order}
    };
}

json Phemex::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"Created", "open"},
        {"Untriggered", "open"},
        {"Deactivated", "closed"},
        {"Triggered", "open"},
        {"Rejected", "rejected"},
        {"New", "open"},
        {"PartiallyFilled", "open"},
        {"Filled", "closed"},
        {"Canceled", "canceled"}
    };
    
    return statuses.contains(status) ? statuses.at(status) : status;
}

} // namespace ccxt
