#include "ascendex.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

AscendEX::AscendEX() {
    id = "ascendex";
    name = "AscendEX";
    version = "v1";
    rateLimit = 500;
    testnet = false;
    accountCategory = "cash";  // default to cash account

    // Initialize API endpoints
    baseUrl = testnet ? "https://api-test.ascendex.com" : "https://ascendex.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/112027508-47984600-8b48-11eb-9097-abf226eb9694.jpg"},
        {"api", {
            {"public", baseUrl},
            {"private", baseUrl}
        }},
        {"www", "https://ascendex.com"},
        {"doc", {
            "https://ascendex.github.io/ascendex-pro-api/#ascendex-pro-api-documentation",
            "https://ascendex-public.s3.amazonaws.com/doc-v2.zip"
        }},
        {"fees", "https://ascendex.com/en/feerate/transactionfee-traderate"},
        {"test", "https://api-test.ascendex.com"}
    };

    timeframes = {
        {"1m", "1"},
        {"5m", "5"},
        {"15m", "15"},
        {"30m", "30"},
        {"1h", "60"},
        {"2h", "120"},
        {"4h", "240"},
        {"6h", "360"},
        {"12h", "720"},
        {"1d", "1D"},
        {"1w", "1W"},
        {"1M", "1M"}
    };

    initializeApiEndpoints();
}

void AscendEX::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "api/pro/v1/assets",
                "api/pro/v1/products",
                "api/pro/v1/ticker",
                "api/pro/v1/barhist",
                "api/pro/v1/depth",
                "api/pro/v1/trades",
                "api/pro/v1/cash/assets",
                "api/pro/v1/margin/assets",
                "api/pro/v1/futures/contracts",
                "api/pro/v1/futures/ref-px",
                "api/pro/v1/futures/market-data",
                "api/pro/v1/futures/funding-rates"
            }}
        }},
        {"private", {
            {"GET", {
                "api/pro/v1/info",
                "api/pro/v1/balance",
                "api/pro/v1/margin/balance",
                "api/pro/v1/futures/position",
                "api/pro/v1/order/open",
                "api/pro/v1/order/status",
                "api/pro/v1/order/hist/current",
                "api/pro/v1/deposit/history",
                "api/pro/v1/withdraw/history",
                "api/pro/v1/transfer/history"
            }},
            {"POST", {
                "api/pro/v1/order",
                "api/pro/v1/margin/order",
                "api/pro/v1/futures/order",
                "api/pro/v1/order/batch",
                "api/pro/v1/order/cancel",
                "api/pro/v1/order/cancel/all",
                "api/pro/v1/transfer"
            }}
        }}
    };
}

json AscendEX::fetchMarkets(const json& params) {
    json response = fetch("/api/pro/v1/products", "public", "GET", params);
    json markets = json::array();
    
    for (const auto& market : response["data"]) {
        String id = market["symbol"];
        String baseId = market["baseAsset"];
        String quoteId = market["quoteAsset"];
        String base = this->commonCurrencyCode(baseId);
        String quote = this->commonCurrencyCode(quoteId);
        String type = market["status"];
        bool active = type == "Normal";
        
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
            {"margin", market["marginTradable"]},
            {"contract", false},
            {"precision", {
                {"amount", market["lotSize"]},
                {"price", market["tickSize"]}
            }},
            {"limits", {
                {"amount", {
                    {"min", market["minQty"]},
                    {"max", market["maxQty"]}
                }},
                {"price", {
                    {"min", market["minNotional"]},
                    {"max", nullptr}
                }},
                {"cost", {
                    {"min", market["minNotional"]},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return markets;
}

json AscendEX::fetchTicker(const String& symbol, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {{"symbol", market.id}};
    json response = fetch("/api/pro/v1/ticker", "public", "GET",
                         this->extend(request, params));
    json ticker = response["data"];
    
    return {
        {"symbol", symbol},
        {"timestamp", ticker["timestamp"]},
        {"datetime", this->iso8601(ticker["timestamp"])},
        {"high", ticker["high"]},
        {"low", ticker["low"]},
        {"bid", ticker["bid"][0]},
        {"bidVolume", ticker["bid"][1]},
        {"ask", ticker["ask"][0]},
        {"askVolume", ticker["ask"][1]},
        {"vwap", nullptr},
        {"open", ticker["open"]},
        {"close", ticker["close"]},
        {"last", ticker["close"]},
        {"previousClose", nullptr},
        {"change", nullptr},
        {"percentage", nullptr},
        {"average", nullptr},
        {"baseVolume", ticker["volume"]},
        {"quoteVolume", ticker["volume"] * ticker["close"]},
        {"info", ticker}
    };
}

json AscendEX::fetchBalance(const json& params) {
    this->loadMarkets();
    String accountCategory = this->getAccountCategory(params);
    
    String path = accountCategory == "futures" ? 
        "/api/pro/v1/futures/position" : "/api/pro/v1/balance";
    
    json response = fetch(path, "private", "GET", params);
    json balances = response["data"];
    json result = {"info", response};
    
    for (const auto& balance : balances) {
        String currencyId = balance["asset"];
        String code = this->commonCurrencyCode(currencyId);
        
        result[code] = {
            {"free", balance["availableBalance"]},
            {"used", balance["totalPosition"] - balance["availableBalance"]},
            {"total", balance["totalPosition"]}
        };
    }
    
    return result;
}

json AscendEX::createOrder(const String& symbol, const String& type,
                          const String& side, double amount,
                          double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    String accountCategory = this->getAccountCategory(params);
    
    json request = {
        {"symbol", market.id},
        {"orderQty", this->amountToPrecision(symbol, amount)},
        {"side", side.upper()},
        {"orderType", type.upper()}
    };
    
    if (type == "limit") {
        if (price == 0) {
            throw InvalidOrder("For limit orders, price cannot be zero");
        }
        request["orderPrice"] = this->priceToPrecision(symbol, price);
    }
    
    String path = accountCategory == "futures" ? 
        "/api/pro/v1/futures/order" : "/api/pro/v1/order";
    
    json response = fetch(path, "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response["data"], market);
}

String AscendEX::sign(const String& path, const String& api,
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
        
        String auth = timestamp + "+" + path;
        
        if (method == "GET") {
            if (!params.empty()) {
                String query = this->urlencode(this->keysort(params));
                url += "?" + query;
                auth += "+" + query;
            }
        } else {
            if (!params.empty()) {
                body = this->json(params);
                auth += "+" + body.dump();
            }
        }
        
        String signature = this->hmac(auth, this->secret, "sha256", "hex");
        
        const_cast<std::map<String, String>&>(headers)["x-auth-key"] = this->apiKey;
        const_cast<std::map<String, String>&>(headers)["x-auth-timestamp"] = timestamp;
        const_cast<std::map<String, String>&>(headers)["x-auth-signature"] = signature;
        
        if (method != "GET") {
            const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/json";
        }
    }
    
    return url;
}

String AscendEX::getAccountCategory(const json& params) {
    return this->safeString(params, "account", accountCategory);
}

json AscendEX::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "orderId");
    String timestamp = this->safeInteger(order, "createTime");
    String status = this->parseOrderStatus(this->safeString(order, "status"));
    
    return {
        {"id", id},
        {"clientOrderId", this->safeString(order, "clientOrderId")},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"status", status},
        {"symbol", market.symbol},
        {"type", this->safeStringLower(order, "orderType")},
        {"side", this->safeStringLower(order, "side")},
        {"price", this->safeFloat(order, "orderPrice")},
        {"amount", this->safeFloat(order, "orderQty")},
        {"filled", this->safeFloat(order, "cumFilledQty")},
        {"remaining", this->safeFloat(order, "orderQty") - this->safeFloat(order, "cumFilledQty")},
        {"cost", this->safeFloat(order, "avgPx") * this->safeFloat(order, "cumFilledQty")},
        {"average", this->safeFloat(order, "avgPx")},
        {"trades", nullptr},
        {"fee", {
            {"cost", this->safeFloat(order, "cumFee")},
            {"currency", market.quote}
        }},
        {"info", order}
    };
}

json AscendEX::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"New", "open"},
        {"PartiallyFilled", "open"},
        {"Filled", "closed"},
        {"Canceled", "canceled"},
        {"Rejected", "rejected"}
    };
    
    return statuses.contains(status) ? statuses.at(status) : status;
}

} // namespace ccxt
