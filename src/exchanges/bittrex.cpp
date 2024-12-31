#include "bittrex.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Bittrex::Bittrex() {
    id = "bittrex";
    name = "Bittrex";
    version = "v3";
    rateLimit = 1500;
    hasPrivateAPI = true;

    // Initialize API endpoints
    baseUrl = "https://api.bittrex.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/27766352-cf0b3c26-5ed5-11e7-82b7-f3826b7a97d8.jpg"},
        {"api", {
            {"public", baseUrl},
            {"private", baseUrl}
        }},
        {"www", "https://bittrex.com"},
        {"doc", {
            "https://bittrex.github.io/api/v3",
            "https://bittrex.github.io/api/v3/swagger"
        }},
        {"fees", "https://bittrex.com/fees"}
    };

    timeframes = {
        {"1m", "MINUTE_1"},
        {"5m", "MINUTE_5"},
        {"1h", "HOUR_1"},
        {"1d", "DAY_1"}
    };

    options = {
        {"adjustForTimeDifference", true},
        {"recvWindow", "5000"},
        {"timeDifference", 0},
        {"parseOrderToPrecision", false}
    };

    initializeApiEndpoints();
}

void Bittrex::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "markets",
                "markets/summaries",
                "markets/{marketSymbol}/summary",
                "markets/{marketSymbol}/orderbook",
                "markets/{marketSymbol}/trades",
                "markets/{marketSymbol}/ticker",
                "markets/{marketSymbol}/candles/{candleInterval}/recent",
                "markets/{marketSymbol}/candles/{candleInterval}/historical/{year}/{month}/{day}",
                "currencies",
                "currencies/{symbol}",
                "ping"
            }}
        }},
        {"private", {
            {"GET", {
                "account",
                "account/volume",
                "addresses",
                "addresses/{currencySymbol}",
                "balances",
                "balances/{currencySymbol}",
                "deposits/open",
                "deposits/closed",
                "deposits/ByTxId/{txId}",
                "orders/open",
                "orders/closed",
                "orders/{orderId}",
                "withdrawals/open",
                "withdrawals/closed",
                "withdrawals/ByTxId/{txId}",
                "conditional-orders/{conditionalOrderId}"
            }},
            {"POST", {
                "addresses",
                "orders",
                "conditional-orders",
                "withdrawals"
            }},
            {"DELETE", {
                "orders/open",
                "orders/{orderId}",
                "conditional-orders/{conditionalOrderId}"
            }}
        }}
    };
}

json Bittrex::fetchMarkets(const json& params) {
    json response = fetch("/markets", "public", "GET", params);
    json markets = json::array();
    
    for (const auto& market : response) {
        std::string id = market["symbol"];
        std::vector<std::string> parts = this->split(id, "-");
        std::string baseId = parts[0];
        std::string quoteId = parts[1];
        std::string base = this->commonCurrencyCode(baseId);
        std::string quote = this->commonCurrencyCode(quoteId);
        std::string symbol = base + "/" + quote;
        bool active = market["status"] == "ONLINE";
        
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
                {"amount", market["precision"].get<int>()},
                {"price", market["precision"].get<int>()}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeFloat(market, "minTradeSize")},
                    {"max", nullptr}
                }},
                {"price", {
                    {"min", nullptr},
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

json Bittrex::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/balances", "private", "GET", params);
    json result = {"info", response};
    
    for (const auto& balance : response) {
        std::string currencyId = balance["currencySymbol"];
        std::string code = this->commonCurrencyCode(currencyId);
        
        result[code] = {
            {"free", this->safeFloat(balance, "available")},
            {"used", this->safeFloat(balance, "total") - this->safeFloat(balance, "available")},
            {"total", this->safeFloat(balance, "total")}
        };
    }
    
    return result;
}

json Bittrex::createOrder(const std::string& symbol, const std::string& type,
                         const std::string& side, double amount,
                         double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"marketSymbol", market.id},
        {"direction", side.upper()},
        {"type", type.upper()},
        {"quantity", this->amountToPrecision(symbol, amount)}
    };
    
    if (type == "LIMIT") {
        request["limit"] = this->priceToPrecision(symbol, price);
    }
    
    json response = fetch("/orders", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response, market);
}

std::string Bittrex::sign(const std::string& path, const std::string& api,
                     const std::string& method, const json& params,
                     const std::map<std::string, std::string>& headers,
                     const json& body) {
    std::string url = this->urls["api"][api] + "/v3";
    std::string endpoint = "/" + this->implodeParams(path, params);
    url += endpoint;
    
    json query = this->omit(params, this->extractParams(path));
    
    if (api == "public") {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->checkRequiredCredentials();
        std::string timestamp = this->getTimestamp();
        std::string contentHash = "";
        
        if (method == "GET" || method == "DELETE") {
            if (!query.empty()) {
                url += "?" + this->urlencode(query);
            }
        } else {
            if (!query.empty()) {
                body = this->json(query);
                contentHash = this->hash(body.dump(), "sha512", "hex");
            }
        }
        
        std::string signature = this->createSignature(timestamp, url, method,
                                               contentHash);
        
        const_cast<std::map<std::string, std::string>&>(headers)["Api-Key"] = this->config_.apiKey;
        const_cast<std::map<std::string, std::string>&>(headers)["Api-Timestamp"] = timestamp;
        const_cast<std::map<std::string, std::string>&>(headers)["Api-Content-Hash"] = contentHash;
        const_cast<std::map<std::string, std::string>&>(headers)["Api-Signature"] = signature;
        
        if (!contentHash.empty()) {
            const_cast<std::map<std::string, std::string>&>(headers)["Content-Type"] = "application/json";
        }
    }
    
    return url;
}

std::string Bittrex::createSignature(const std::string& timestamp, const std::string& uri,
                               const std::string& method, const std::string& body) {
    std::string preSign = timestamp + uri + method + body;
    return this->hmac(preSign, this->base64ToBinary(this->config_.secret),
                     "sha512", "hex");
}

std::string Bittrex::getTimestamp() {
    return std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count());
}

json Bittrex::parseOrder(const json& order, const Market& market) {
    std::string id = this->safeString(order, "id");
    std::string timestamp = this->safeString(order, "createdAt");
    std::string lastTradeTimestamp = this->safeString(order, "closedAt");
    std::string symbol = market.symbol;
    std::string type = this->safeStringLower(order, "type");
    std::string side = this->safeStringLower(order, "direction");
    std::string status = this->parseOrderStatus(this->safeString(order, "status"));
    
    return {
        {"id", id},
        {"clientOrderId", this->safeString(order, "clientOrderId")},
        {"timestamp", this->parse8601(timestamp)},
        {"datetime", timestamp},
        {"lastTradeTimestamp", this->parse8601(lastTradeTimestamp)},
        {"status", status},
        {"symbol", symbol},
        {"type", type},
        {"side", side},
        {"price", this->safeFloat(order, "limit")},
        {"amount", this->safeFloat(order, "quantity")},
        {"filled", this->safeFloat(order, "fillQuantity")},
        {"remaining", this->safeFloat(order, "quantity") - this->safeFloat(order, "fillQuantity")},
        {"cost", this->safeFloat(order, "proceeds")},
        {"trades", nullptr},
        {"fee", {
            {"currency", market.quote},
            {"cost", this->safeFloat(order, "commission")}
        }},
        {"info", order}
    };
}

json Bittrex::parseOrderStatus(const std::string& status) {
    static const std::map<std::string, std::string> statuses = {
        {"OPEN", "open"},
        {"CLOSED", "closed"},
        {"CANCELLED", "canceled"},
        {"PENDING_CANCEL", "canceling"}
    };
    
    return statuses.contains(status) ? statuses.at(status) : status;
}

} // namespace ccxt
