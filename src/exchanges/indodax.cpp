#include "indodax.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Indodax::Indodax() {
    id = "indodax";
    name = "INDODAX";
    version = "2.0.1";
    rateLimit = 1000;
    certified = true;
    pro = false;

    // Initialize API endpoints
    baseUrl = "https://indodax.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/51840849/87070508-9358c880-c221-11ea-8dc5-5391afbbb422.jpg"},
        {"api", {
            {"public", "https://indodax.com/api"},
            {"private", "https://indodax.com/tapi"}
        }},
        {"www", "https://www.indodax.com"},
        {"doc", {
            "https://github.com/btcid/indodax-official-api-docs",
            "https://indodax.com/downloads/BITCOINCOID-API-DOCUMENTATION.pdf"
        }},
        {"referral", "https://indodax.com/ref/testuser/1"},
        {"fees", "https://help.indodax.com/article/guide-to-idr-market-fees-and-limits"}
    };

    timeframes = {
        {"1m", "1min"},
        {"5m", "5min"},
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
        {"recvWindow", 5000},
        {"timeDifference", 0}
    };

    errorCodes = {
        {1, "Invalid credentials"},
        {2, "Invalid parameter"},
        {3, "Invalid request"},
        {4, "Invalid market"},
        {5, "Invalid currency"},
        {6, "Invalid amount"},
        {7, "Insufficient balance"},
        {8, "Order not found"},
        {9, "Order already canceled"},
        {10, "Order already filled"},
        {11, "Invalid nonce"},
        {12, "Invalid signature"},
        {13, "Invalid timestamp"},
        {14, "IP address not allowed"},
        {15, "Action not allowed"},
        {16, "Server error"}
    };

    initializeApiEndpoints();
}

void Indodax::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "server_time",
                "pairs",
                "{pair}/ticker",
                "{pair}/trades",
                "{pair}/depth",
                "price_increments",
                "summaries",
                "ticker_all"
            }}
        }},
        {"private", {
            {"POST", {
                "getInfo",
                "transHistory",
                "trade",
                "tradeHistory",
                "openOrders",
                "orderHistory",
                "getOrder",
                "cancelOrder",
                "withdrawCoin",
                "withdrawFee",
                "listDownline",
                "listReferral",
                "trade_fee"
            }}
        }}
    };
}

json Indodax::fetchMarkets(const json& params) {
    json response = fetch("/pairs", "public", "GET", params);
    json result = json::array();
    
    for (const auto& market : response) {
        std::string id = this->safeString(market, "id");
        std::string baseId = this->safeString(market, "base_currency");
        std::string quoteId = this->safeString(market, "quote_currency");
        std::string base = this->safeCurrencyCode(baseId);
        std::string quote = this->safeCurrencyCode(quoteId);
        std::string symbol = base + "/" + quote;
        
        result.push_back({
            {"id", id},
            {"symbol", symbol},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", true},
            {"type", "spot"},
            {"spot", true},
            {"future", false},
            {"option", false},
            {"margin", false},
            {"contract", false},
            {"precision", {
                {"amount", this->safeInteger(market, "trade_min_base_currency")},
                {"price", this->safeInteger(market, "trade_min_quote_currency")}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeFloat(market, "trade_min_base_currency")},
                    {"max", this->safeFloat(market, "trade_max_base_currency")}
                }},
                {"price", {
                    {"min", this->safeFloat(market, "trade_min_quote_currency")},
                    {"max", this->safeFloat(market, "trade_max_quote_currency")}
                }},
                {"cost", {
                    {"min", nullptr},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return result;
}

json Indodax::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/getInfo", "private", "POST", params);
    return parseBalance(response);
}

json Indodax::parseBalance(const json& response) {
    json balances = this->safeValue(response, "return", {});
    json balance = this->safeValue(balances, "balance", {});
    json frozen = this->safeValue(balances, "balance_hold", {});
    json result = {{"info", response}};
    
    for (const auto& [currencyId, total] : balance.items()) {
        std::string code = this->safeCurrencyCode(currencyId);
        std::string account = {
            {"free", this->safeFloat(balance, currencyId)},
            {"used", this->safeFloat(frozen, currencyId, 0.0)},
            {"total", nullptr}
        };
        account["total"] = this->sum(account["free"], account["used"]);
        result[code] = account;
    }
    
    return result;
}

json Indodax::createOrder(const std::string& symbol, const std::string& type,
                         const std::string& side, double amount,
                         double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"pair", market["id"]},
        {"type", side},
        {"amount", this->amountToPrecision(symbol, amount)}
    };
    
    if (type == "limit") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    json response = fetch("/trade", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response["return"], market);
}

std::string Indodax::sign(const std::string& path, const std::string& api,
                    const std::string& method, const json& params,
                    const std::map<std::string, std::string>& headers,
                    const json& body) {
    std::string url = this->urls["api"][api];
    std::string query = this->omit(params, this->extractParams(path));
    
    if (api == "public") {
        url += this->implodeParams(path, params);
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->checkRequiredCredentials();
        std::string nonce = this->nonce().str();
        json request = this->extend({
            "method", path,
            "nonce", nonce,
            "timestamp", std::to_string(this->milliseconds())
        }, query);
        
        std::string body = this->urlencode(request);
        std::string signature = this->hmac(body, this->encode(this->config_.secret),
                                    "sha512", "hex");
        
        const_cast<std::map<std::string, std::string>&>(headers)["Key"] = this->config_.apiKey;
        const_cast<std::map<std::string, std::string>&>(headers)["Sign"] = signature;
        
        if (method == "POST") {
            const_cast<std::map<std::string, std::string>&>(headers)["Content-Type"] = "application/x-www-form-urlencoded";
        }
    }
    
    return url;
}

std::string Indodax::createNonce() {
    return std::to_string(this->milliseconds());
}

json Indodax::parseOrder(const json& order, const Market& market) {
    std::string timestamp = this->safeString(order, "submit_time");
    std::string status = this->parseOrderStatus(this->safeString(order, "status"));
    std::string symbol = nullptr;
    
    if (!market.empty()) {
        symbol = market["symbol"];
    }
    
    std::string type = this->safeString(order, "type");
    std::string side = this->safeString(order, "type");
    
    return {
        {"id", this->safeString(order, "order_id")},
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
        {"cost", nullptr},
        {"amount", this->safeFloat(order, "order_rp")},
        {"filled", this->safeFloat(order, "remain_rp")},
        {"remaining", this->safeFloat(order, "remain_rp")},
        {"trades", nullptr},
        {"fee", nullptr},
        {"info", order}
    };
}

std::string Indodax::parseOrderStatus(const std::string& status) {
    static const std::map<std::string, std::string> statuses = {
        {"pending", "open"},
        {"running", "open"},
        {"expired", "expired"},
        {"success", "closed"},
        {"canceled", "canceled"},
        {"failed", "failed"}
    };
    
    return this->safeString(statuses, status, status);
}

} // namespace ccxt
