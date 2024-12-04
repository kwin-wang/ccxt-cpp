#include "probit.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Probit::Probit() {
    id = "probit";
    name = "ProBit";
    version = "v1";
    rateLimit = 50;

    // Initialize API endpoints
    baseUrl = "https://api.probit.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/51840849/79268032-c4379480-7ea2-11ea-80b3-dd96bb29fd0d.jpg"},
        {"api", {
            {"public", "https://api.probit.com/api/exchange"},
            {"private", "https://api.probit.com/api/exchange"}
        }},
        {"www", "https://www.probit.com"},
        {"doc", {
            "https://docs-en.probit.com",
            "https://docs-ko.probit.com"
        }},
        {"fees", "https://support.probit.com/hc/en-us/articles/360020968611-Trading-Fees"}
    };

    timeframes = {
        {"1m", "1min"},
        {"3m", "3min"},
        {"5m", "5min"},
        {"10m", "10min"},
        {"15m", "15min"},
        {"30m", "30min"},
        {"1h", "1hour"},
        {"4h", "4hour"},
        {"6h", "6hour"},
        {"12h", "12hour"},
        {"1d", "1day"},
        {"1w", "1week"},
        {"1M", "1month"}
    };

    options = {
        {"adjustForTimeDifference", true},
        {"recvWindow", "5000"}
    };

    errorCodes = {
        {400, "Bad Request"},
        {401, "Unauthorized"},
        {403, "Forbidden"},
        {404, "Not Found"},
        {429, "Too Many Requests"},
        {500, "Internal Server Error"},
        {503, "Service Unavailable"}
    };

    initializeApiEndpoints();
}

void Probit::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "market",
                "currency",
                "ticker",
                "order_book",
                "trade",
                "candle",
                "time"
            }}
        }},
        {"private", {
            {"GET", {
                "balance",
                "order",
                "order_history",
                "trade_history",
                "deposit_address",
                "deposit_history",
                "withdrawal_history",
                "new_order"
            }},
            {"POST", {
                "new_order",
                "cancel_order",
                "withdrawal"
            }}
        }}
    };
}

json Probit::fetchMarkets(const json& params) {
    json response = fetch("/market", "public", "GET", params);
    json result = json::array();
    
    for (const auto& market : response["data"]) {
        String id = this->safeString(market, "id");
        std::vector<String> parts = this->split(id, "-");
        String baseId = parts[0];
        String quoteId = parts[1];
        String base = this->commonCurrencyCode(baseId);
        String quote = this->commonCurrencyCode(quoteId);
        String symbol = base + "/" + quote;
        bool active = market["status"] == "active";
        
        result.push_back({
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
                {"amount", market["amount_precision"].get<int>()},
                {"price", market["price_precision"].get<int>()}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeFloat(market, "min_amount")},
                    {"max", this->safeFloat(market, "max_amount")}
                }},
                {"price", {
                    {"min", this->safeFloat(market, "min_price")},
                    {"max", this->safeFloat(market, "max_price")}
                }},
                {"cost", {
                    {"min", this->safeFloat(market, "min_value")},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return result;
}

json Probit::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/balance", "private", "GET", params);
    return parseBalance(response);
}

json Probit::parseBalance(const json& response) {
    json result = {"info", response};
    json balances = response["data"];
    
    for (const auto& balance : balances) {
        String currencyId = balance["currency_id"];
        String code = this->commonCurrencyCode(currencyId);
        
        result[code] = {
            {"free", this->safeFloat(balance, "available")},
            {"used", this->safeFloat(balance, "locked")},
            {"total", this->safeFloat(balance, "total")}
        };
    }
    
    return result;
}

json Probit::createOrder(const String& symbol, const String& type,
                        const String& side, double amount,
                        double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"market_id", market.id},
        {"type", type.upper()},
        {"side", side.upper()},
        {"quantity", this->amountToPrecision(symbol, amount)}
    };
    
    if (type == "limit") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    json response = fetch("/new_order", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response["data"], market);
}

String Probit::sign(const String& path, const String& api,
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
        
        json request = this->extend({
            "timestamp": timestamp
        }, params);
        
        String signature = this->createSignature(timestamp, method, path,
                                               request.dump());
        
        const_cast<std::map<String, String>&>(headers)["Authorization"] = "Bearer " + this->apiKey;
        const_cast<std::map<String, String>&>(headers)["Signature"] = signature;
        const_cast<std::map<String, String>&>(headers)["Timestamp"] = timestamp;
        
        if (method == "GET") {
            url += "?" + this->urlencode(request);
        } else {
            body = request;
            const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/json";
        }
    }
    
    return url;
}

String Probit::createSignature(const String& timestamp, const String& method,
                              const String& path, const String& body) {
    String message = timestamp + method + path + body;
    return this->hmac(message, this->encode(this->secret),
                     "sha512", "hex");
}

json Probit::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "id");
    String timestamp = this->safeString(order, "time");
    String status = this->parseOrderStatus(this->safeString(order, "status"));
    String symbol = market.symbol;
    String type = this->safeStringLower(order, "type");
    String side = this->safeStringLower(order, "side");
    
    return {
        {"id", id},
        {"clientOrderId", this->safeString(order, "client_order_id")},
        {"timestamp", this->safeInteger(order, "time")},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"status", status},
        {"symbol", symbol},
        {"type", type},
        {"side", side},
        {"price", this->safeFloat(order, "price")},
        {"amount", this->safeFloat(order, "quantity")},
        {"filled", this->safeFloat(order, "filled_quantity")},
        {"remaining", this->safeFloat(order, "open_quantity")},
        {"cost", this->safeFloat(order, "filled_cost")},
        {"trades", nullptr},
        {"fee", {
            {"currency", market.quote},
            {"cost", this->safeFloat(order, "fee")},
            {"rate", this->safeFloat(order, "fee_rate")}
        }},
        {"info", order}
    };
}

json Probit::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"open", "open"},
        {"filled", "closed"},
        {"cancelled", "canceled"},
        {"partially_filled", "open"}
    };
    
    return statuses.contains(status) ? statuses.at(status) : status;
}

} // namespace ccxt
