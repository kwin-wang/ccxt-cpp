#include "gopax.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Gopax::Gopax() {
    id = "gopax";
    name = "GOPAX";
    version = "1";
    rateLimit = 1000;
    certified = true;
    pro = false;
    hasMultipleAccounts = true;

    // Initialize API endpoints
    baseUrl = "https://api.gopax.co.kr";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/102897212-ae8a5e00-4478-11eb-9bab-91507c643900.jpg"},
        {"api", {
            {"public", "https://api.gopax.co.kr"},
            {"private", "https://api.gopax.co.kr"}
        }},
        {"www", "https://www.gopax.co.kr"},
        {"doc", {
            "https://gopax.github.io/API/index.en.html",
            "https://gopax.github.io/API/index.ko.html"
        }},
        {"referral", "https://www.gopax.co.kr/signup?ref=testuser"},
        {"fees", "https://www.gopax.co.kr/fees"}
    };

    timeframes = {
        {"1m", "1"},
        {"5m", "5"},
        {"15m", "15"},
        {"30m", "30"},
        {"1h", "60"},
        {"4h", "240"},
        {"12h", "720"},
        {"1d", "1D"},
        {"1w", "1W"},
        {"1M", "1M"}
    };

    options = {
        {"adjustForTimeDifference", true},
        {"recvWindow", 5000},
        {"timeDifference", 0},
        {"defaultType", "spot"}
    };

    errorCodes = {
        {100, "Invalid request"},
        {101, "Invalid API key"},
        {102, "Invalid signature"},
        {103, "Invalid nonce"},
        {104, "Invalid scope"},
        {105, "Invalid request"},
        {106, "Rate limit exceeded"},
        {107, "Unauthorized"},
        {200, "No balance"},
        {201, "Invalid order"},
        {202, "Order not found"},
        {203, "Order already closed"},
        {204, "Order amount is too small"},
        {205, "Insufficient balance"},
        {206, "Order price is too low"},
        {207, "Order price is too high"}
    };

    currencyIds = {
        {"BTC", "BTC"},
        {"ETH", "ETH"},
        {"XRP", "XRP"},
        {"BCH", "BCH"},
        {"ETC", "ETC"},
        {"KRW", "KRW"},
        {"USDT", "USDT"}
    };

    initializeApiEndpoints();
}

void Gopax::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "assets",
                "trading-pairs",
                "ticker",
                "orderbook",
                "trades",
                "stats",
                "time",
                "candles"
            }}
        }},
        {"private", {
            {"GET", {
                "balances",
                "orders",
                "orders/open",
                "orders/{order_id}",
                "trades",
                "deposit/address/{asset}",
                "deposit/status",
                "withdrawal/status"
            }},
            {"POST", {
                "orders",
                "withdrawal/crypto",
                "withdrawal/krw"
            }},
            {"DELETE", {
                "orders/{order_id}",
                "orders/cancel"
            }}
        }}
    };
}

json Gopax::fetchMarkets(const json& params) {
    json response = fetch("/trading-pairs", "public", "GET", params);
    json result = json::array();
    
    for (const auto& market : response) {
        std::string id = this->safeString(market, "name");
        std::string baseId = this->safeString(market, "baseAsset");
        std::string quoteId = this->safeString(market, "quoteAsset");
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
            {"margin", this->safeValue(market, "marginEnabled", false)},
            {"contract", false},
            {"precision", {
                {"amount", this->safeInteger(market, "baseAssetPrecision")},
                {"price", this->safeInteger(market, "quotePrecision")}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeFloat(market, "minOrderAmount")},
                    {"max", this->safeFloat(market, "maxOrderAmount")}
                }},
                {"price", {
                    {"min", this->safeFloat(market, "minOrderPrice")},
                    {"max", this->safeFloat(market, "maxOrderPrice")}
                }},
                {"cost", {
                    {"min", this->safeFloat(market, "minOrderValue")},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return result;
}

json Gopax::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/balances", "private", "GET", params);
    return parseBalance(response);
}

json Gopax::parseBalance(const json& response) {
    json result = {{"info", response}};
    
    for (const auto& balance : response) {
        std::string currencyId = this->safeString(balance, "asset");
        std::string code = this->safeCurrencyCode(currencyId);
        std::string account = {
            {"free", this->safeFloat(balance, "avail")},
            {"used", this->safeFloat(balance, "hold")},
            {"total", this->safeFloat(balance, "total")}
        };
        result[code] = account;
    }
    
    return result;
}

json Gopax::createOrder(const std::string& symbol, const std::string& type,
                       const std::string& side, double amount,
                       double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"tradingPair", market["id"]},
        {"side", side.toUpperCase()},
        {"type", type.toUpperCase()},
        {"amount", this->amountToPrecision(symbol, amount)}
    };
    
    if (type == "limit") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    json response = fetch("/orders", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response, market);
}

std::string Gopax::sign(const std::string& path, const std::string& api,
                  const std::string& method, const json& params,
                  const std::map<std::string, std::string>& headers,
                  const json& body) {
    std::string url = this->urls["api"][api] + "/" + this->version + "/" + path;
    
    if (api == "public") {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    } else {
        this->checkRequiredCredentials();
        std::string nonce = this->nonce().str();
        std::string timestamp = std::to_string(this->milliseconds());
        std::string auth = timestamp + method + "/" + path;
        
        if (method == "POST") {
            if (!params.empty()) {
                body = this->json(params);
                auth += body;
            }
        } else {
            if (!params.empty()) {
                std::string query = this->urlencode(params);
                url += "?" + query;
                auth += "?" + query;
            }
        }
        
        std::string signature = this->hmac(auth, this->encode(this->config_.secret),
                                    "sha512", "hex");
        
        const_cast<std::map<std::string, std::string>&>(headers)["API-KEY"] = this->config_.apiKey;
        const_cast<std::map<std::string, std::string>&>(headers)["SIGNATURE"] = signature;
        const_cast<std::map<std::string, std::string>&>(headers)["NONCE"] = nonce;
        const_cast<std::map<std::string, std::string>&>(headers)["TIMESTAMP"] = timestamp;
        
        if (method == "POST") {
            const_cast<std::map<std::string, std::string>&>(headers)["Content-Type"] = "application/json";
        }
    }
    
    return url;
}

std::string Gopax::createNonce() {
    return std::to_string(this->milliseconds());
}

json Gopax::parseOrder(const json& order, const Market& market) {
    std::string id = this->safeString(order, "id");
    std::string timestamp = this->safeString(order, "timestamp");
    std::string status = this->parseOrderStatus(this->safeString(order, "status"));
    std::string symbol = nullptr;
    
    if (!market.empty()) {
        symbol = market["symbol"];
    }
    
    std::string type = this->safeStringLower(order, "type");
    std::string side = this->safeStringLower(order, "side");
    
    return {
        {"id", id},
        {"clientOrderId", this->safeString(order, "clientOrderId")},
        {"datetime", this->iso8601(timestamp)},
        {"timestamp", this->parse8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"type", type},
        {"timeInForce", this->safeString(order, "timeInForce")},
        {"postOnly", nullptr},
        {"status", status},
        {"symbol", symbol},
        {"side", side},
        {"price", this->safeFloat(order, "price")},
        {"stopPrice", nullptr},
        {"cost", this->safeFloat(order, "amount") * this->safeFloat(order, "price")},
        {"amount", this->safeFloat(order, "amount")},
        {"filled", this->safeFloat(order, "filledAmount")},
        {"remaining", this->safeFloat(order, "remainingAmount")},
        {"trades", nullptr},
        {"fee", {
            {"currency", market["quote"]},
            {"cost", this->safeFloat(order, "fee")},
            {"rate", this->safeFloat(order, "feeRate")}
        }},
        {"info", order}
    };
}

std::string Gopax::parseOrderStatus(const std::string& status) {
    static const std::map<std::string, std::string> statuses = {
        {"placed", "open"},
        {"cancelled", "canceled"},
        {"completed", "closed"},
        {"rejected", "rejected"},
        {"expired", "expired"}
    };
    
    return this->safeString(statuses, status, status);
}

} // namespace ccxt
