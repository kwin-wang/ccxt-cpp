#include "stex.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

STEX::STEX() {
    id = "stex";
    name = "STEX";
    version = "3";
    rateLimit = 500;
    certified = true;
    pro = false;
    hasMultipleWallets = true;
    hasCurrencyGroups = true;

    // Initialize API endpoints
    baseUrl = "https://api3.stex.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/69680782-03fd0b80-10bd-11ea-909e-7f603500e9cc.jpg"},
        {"api", {
            {"public", "https://api3.stex.com/public"},
            {"private", "https://api3.stex.com"}
        }},
        {"www", "https://www.stex.com"},
        {"doc", {
            "https://help.stex.com/en/collections/1593608-api-v3-documentation"
        }},
        {"fees", "https://app.stex.com/en/pairs-specification"}
    };

    timeframes = {
        {"1m", "1"},
        {"5m", "5"},
        {"30m", "30"},
        {"1h", "60"},
        {"4h", "240"},
        {"12h", "720"},
        {"1d", "1D"}
    };

    options = {
        {"adjustForTimeDifference", true},
        {"recvWindow", 5000},
        {"timeDifference", 0},
        {"defaultType", "spot"}
    };

    errorCodes = {
        {1, "Authorization failed"},
        {2, "Authorization header is invalid"},
        {3, "Authorization header is missing"},
        {4, "Nonce is invalid"},
        {5, "Nonce is missing"},
        {6, "Signature is invalid"},
        {7, "Signature is missing"},
        {8, "Timestamp is invalid"},
        {9, "Timestamp is missing"},
        {10, "Request body is missing"},
        {11, "Request body is invalid"},
        {12, "Request parameters are missing"},
        {13, "Request parameters are invalid"},
        {14, "Resource not found"},
        {15, "Resource is forbidden"},
        {16, "Too many requests"},
        {17, "Server error"},
        {18, "Service unavailable"}
    };

    initializeApiEndpoints();
}

void STEX::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "currencies",
                "currencies/{currencyId}",
                "markets",
                "pairs-groups",
                "currency_pairs/list/{code}",
                "currency_pairs/group/{groupId}",
                "ticker",
                "ticker/{currencyPairId}",
                "trades/{currencyPairId}",
                "orderbook/{currencyPairId}",
                "chart/{currencyPairId}/{candlesType}",
                "deposit-statuses",
                "withdrawal-statuses",
                "ping",
                "time"
            }}
        }},
        {"private", {
            {"GET", {
                "profile/info",
                "wallets",
                "wallets/{walletId}",
                "wallets/address/{walletId}",
                "deposits",
                "withdrawals",
                "orders",
                "orders/{orderId}",
                "trades",
                "trades/{orderId}"
            }},
            {"POST", {
                "orders/{currencyPairId}",
                "orders/{orderId}/cancel",
                "withdrawals/{currencyId}"
            }}
        }}
    };
}

json STEX::fetchMarkets(const json& params) {
    json response = fetch("/currency_pairs/list/ALL", "public", "GET", params);
    json result = json::array();
    
    for (const auto& market : response) {
        String id = market["id"];
        String baseId = market["currency_id"];
        String quoteId = market["market_currency_id"];
        String base = this->safeCurrencyCode(market["currency_code"]);
        String quote = this->safeCurrencyCode(market["market_code"]);
        String symbol = base + "/" + quote;
        
        result.push_back({
            {"id", id},
            {"symbol", symbol},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", market["active"]},
            {"type", "spot"},
            {"spot", true},
            {"margin", false},
            {"future", false},
            {"option", false},
            {"contract", false},
            {"precision", {
                {"amount", market["currency_precision"]},
                {"price", market["market_precision"]}
            }},
            {"limits", {
                {"amount", {
                    {"min", market["minimum_amount"]},
                    {"max", market["maximum_amount"]}
                }},
                {"price", {
                    {"min", market["minimum_price"]},
                    {"max", market["maximum_price"]}
                }},
                {"cost", {
                    {"min", market["minimum_value"]},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return result;
}

json STEX::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/wallets", "private", "GET", params);
    return parseBalance(response);
}

json STEX::parseBalance(const json& response) {
    json result = {{"info", response}};
    
    for (const auto& balance : response) {
        String code = this->safeCurrencyCode(balance["currency_code"]);
        String account = {
            {"free", this->safeFloat(balance, "balance")},
            {"used", this->safeFloat(balance, "frozen_balance")},
            {"total", this->safeFloat(balance, "total_balance")}
        };
        result[code] = account;
    }
    
    return result;
}

json STEX::createOrder(const String& symbol, const String& type,
                      const String& side, double amount,
                      double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"type", type},
        {"amount", this->amountToPrecision(symbol, amount)},
        {"side", side}
    };
    
    if (type == "limit") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    json response = fetch("/orders/" + market["id"], "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response, market);
}

String STEX::sign(const String& path, const String& api,
                  const String& method, const json& params,
                  const std::map<String, String>& headers,
                  const json& body) {
    String url = this->urls["api"][api] + "/" + this->implodeParams(path, params);
    json query = this->omit(params, this->extractParams(path));
    
    if (api == "public") {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->checkRequiredCredentials();
        String nonce = this->nonce().str();
        String timestamp = std::to_string(this->milliseconds());
        String payload = timestamp + method + "/" + path;
        
        if (!query.empty()) {
            if (method == "GET") {
                url += "?" + this->urlencode(query);
            } else {
                body = this->json(query);
                payload += body;
            }
        }
        
        String signature = this->hmac(payload, this->encode(this->config_.secret),
                                    "sha256", "hex");
        
        const_cast<std::map<String, String>&>(headers)["APIKEY"] = this->config_.apiKey;
        const_cast<std::map<String, String>&>(headers)["TIMESTAMP"] = timestamp;
        const_cast<std::map<String, String>&>(headers)["SIGNATURE"] = signature;
        const_cast<std::map<String, String>&>(headers)["RECVWINDOW"] = this->options["recvWindow"];
        
        if (method != "GET") {
            const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/json";
        }
    }
    
    return url;
}

String STEX::getNonce() {
    return std::to_string(this->milliseconds());
}

json STEX::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "id");
    String timestamp = this->safeString(order, "timestamp");
    String status = this->parseOrderStatus(this->safeString(order, "status"));
    String symbol = nullptr;
    
    if (!market.empty()) {
        symbol = market["symbol"];
    }
    
    String type = this->safeString(order, "type");
    String side = this->safeString(order, "side");
    
    return {
        {"id", id},
        {"clientOrderId", nullptr},
        {"datetime", this->iso8601(timestamp)},
        {"timestamp", this->parse8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"type", type},
        {"timeInForce", nullptr},
        {"postOnly", nullptr},
        {"status", status},
        {"symbol", symbol},
        {"side", side},
        {"price", this->safeFloat(order, "price")},
        {"stopPrice", nullptr},
        {"cost", this->safeFloat(order, "value")},
        {"amount", this->safeFloat(order, "amount")},
        {"filled", this->safeFloat(order, "filled")},
        {"remaining", this->safeFloat(order, "remaining")},
        {"trades", nullptr},
        {"fee", {
            {"currency", market["quote"]},
            {"cost", this->safeFloat(order, "fee")},
            {"rate", this->safeFloat(order, "fee_rate")}
        }},
        {"info", order}
    };
}

String STEX::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"new", "open"},
        {"processing", "open"},
        {"filled", "closed"},
        {"cancelled", "canceled"},
        {"expired", "expired"}
    };
    
    return this->safeString(statuses, status, status);
}

} // namespace ccxt
