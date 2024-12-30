#include "upbit.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>
#include <jwt-cpp/jwt.h>

namespace ccxt {

Upbit::Upbit() {
    id = "upbit";
    name = "Upbit";
    version = "v1";
    rateLimit = 1000;
    isJwtAuth = true;

    // Initialize API endpoints
    baseUrl = "https://api.upbit.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/49245610-eeaabe00-f423-11e8-9cba-4b0aed794799.jpg"},
        {"api", {
            {"public", baseUrl},
            {"private", baseUrl}
        }},
        {"www", "https://upbit.com"},
        {"doc", {
            "https://docs.upbit.com/docs",
            "https://github.com/upbit-exchange/open-api-specification"
        }},
        {"fees", "https://upbit.com/service_center/guide"}
    };

    timeframes = {
        {"1m", "minutes/1"},
        {"3m", "minutes/3"},
        {"5m", "minutes/5"},
        {"10m", "minutes/10"},
        {"15m", "minutes/15"},
        {"30m", "minutes/30"},
        {"1h", "minutes/60"},
        {"4h", "minutes/240"},
        {"1d", "days"},
        {"1w", "weeks"},
        {"1M", "months"}
    };

    options = {
        {"recvWindow", "5000"},
        {"adjustForTimeDifference", true}
    };

    initializeApiEndpoints();
}

void Upbit::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "v1/market/all",
                "v1/candles/{timeframe}",
                "v1/ticker",
                "v1/orderbook",
                "v1/trades/ticks"
            }}
        }},
        {"private", {
            {"GET", {
                "v1/accounts",
                "v1/orders/chance",
                "v1/order",
                "v1/orders",
                "v1/withdraws",
                "v1/deposits",
                "v1/deposits/coin_addresses",
                "v1/deposits/coin_address"
            }},
            {"POST", {
                "v1/orders",
                "v1/withdraws/coin",
                "v1/withdraws/krw",
                "v1/deposits/generate_coin_address"
            }},
            {"DELETE", {
                "v1/order"
            }}
        }}
    };
}

json Upbit::fetchMarkets(const json& params) {
    json response = fetch("/v1/market/all", "public", "GET", params);
    json markets = json::array();
    
    for (const auto& market : response) {
        String id = market["market"];
        std::vector<String> parts = this->split(id, "-");
        String quoteId = parts[0];
        String baseId = parts[1];
        String base = this->commonCurrencyCode(baseId);
        String quote = this->commonCurrencyCode(quoteId);
        String symbol = base + "/" + quote;
        bool active = market["state"] == "active";
        
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
                {"amount", 8},
                {"price", 8}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeFloat(market, "min_order_size", 0.0001)},
                    {"max", nullptr}
                }},
                {"price", {
                    {"min", nullptr},
                    {"max", nullptr}
                }},
                {"cost", {
                    {"min", this->safeFloat(market, "min_total", 500)},  // 500 KRW
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return markets;
}

json Upbit::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/v1/accounts", "private", "GET", params);
    json result = {"info", response};
    
    for (const auto& balance : response) {
        String currencyId = balance["currency"];
        String code = this->commonCurrencyCode(currencyId);
        
        result[code] = {
            {"free", this->safeFloat(balance, "balance")},
            {"used", this->safeFloat(balance, "locked")},
            {"total", this->safeFloat(balance, "balance") + this->safeFloat(balance, "locked")}
        };
    }
    
    return result;
}

json Upbit::createOrder(const String& symbol, const String& type,
                       const String& side, double amount,
                       double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"market", market.id},
        {"side", side},
        {"volume", this->amountToPrecision(symbol, amount)}
    };
    
    if (type == "limit") {
        request["ord_type"] = "limit";
        request["price"] = this->priceToPrecision(symbol, price);
    } else {
        request["ord_type"] = "price";
        if (side == "buy") {
            request["price"] = this->priceToPrecision(symbol, amount * price);
            delete request["volume"];
        }
    }
    
    json response = fetch("/v1/orders", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response, market);
}

String Upbit::sign(const String& path, const String& api,
                   const String& method, const json& params,
                   const std::map<String, String>& headers,
                   const json& body) {
    String url = this->urls["api"][api] + "/" + this->implodeParams(path, params);
    String query = this->omit(params, this->extractParams(path));
    
    if (api == "public") {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->checkRequiredCredentials();
        
        if (isJwtAuth) {
            String token = this->createJWT();
            const_cast<std::map<String, String>&>(headers)["Authorization"] = "Bearer " + token;
        }
        
        if (method == "GET" || method == "DELETE") {
            if (!query.empty()) {
                url += "?" + this->urlencode(query);
            }
        } else {
            const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/json";
            if (!query.empty()) {
                body = this->json(query);
            }
        }
    }
    
    return url;
}

String Upbit::createJWT() {
    auto token = jwt::create()
        .set_issuer("upbit")
        .set_type("JWT")
        .set_issued_at(std::chrono::system_clock::now())
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::seconds(60))
        .set_payload_claim("access_key", jwt::claim(std::string(this->config_.apiKey)))
        .sign(jwt::algorithm::hs256(this->config_.secret));
    
    return token;
}

json Upbit::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "uuid");
    String timestamp = this->safeString(order, "created_at");
    String type = this->safeString(order, "ord_type");
    String side = this->parseOrderSide(this->safeString(order, "side"));
    String status = this->parseOrderStatus(this->safeString(order, "state"));
    
    if (type == "price") {
        type = "market";
    }
    
    return {
        {"id", id},
        {"clientOrderId", nullptr},
        {"timestamp", this->parse8601(timestamp)},
        {"datetime", timestamp},
        {"lastTradeTimestamp", nullptr},
        {"status", status},
        {"symbol", market.symbol},
        {"type", type},
        {"side", side},
        {"price", this->safeFloat(order, "price")},
        {"amount", this->safeFloat(order, "volume")},
        {"filled", this->safeFloat(order, "executed_volume")},
        {"remaining", this->safeFloat(order, "remaining_volume")},
        {"cost", this->safeFloat(order, "executed_volume") * this->safeFloat(order, "price")},
        {"trades", nullptr},
        {"fee", {
            {"currency", market.quote},
            {"cost", this->safeFloat(order, "paid_fee")}
        }},
        {"info", order}
    };
}

json Upbit::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"wait", "open"},
        {"done", "closed"},
        {"cancel", "canceled"}
    };
    
    return statuses.contains(status) ? statuses.at(status) : status;
}

json Upbit::parseOrderSide(const String& side) {
    if (side == "ask") {
        return "sell";
    } else if (side == "bid") {
        return "buy";
    }
    return side;
}

} // namespace ccxt
