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
        String id = this->safeString(market, "name");
        String baseId = this->safeString(market, "baseAsset");
        String quoteId = this->safeString(market, "quoteAsset");
        String base = this->safeCurrencyCode(baseId);
        String quote = this->safeCurrencyCode(quoteId);
        String symbol = base + "/" + quote;
        
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
        String currencyId = this->safeString(balance, "asset");
        String code = this->safeCurrencyCode(currencyId);
        String account = {
            {"free", this->safeFloat(balance, "avail")},
            {"used", this->safeFloat(balance, "hold")},
            {"total", this->safeFloat(balance, "total")}
        };
        result[code] = account;
    }
    
    return result;
}

json Gopax::createOrder(const String& symbol, const String& type,
                       const String& side, double amount,
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

String Gopax::sign(const String& path, const String& api,
                  const String& method, const json& params,
                  const std::map<String, String>& headers,
                  const json& body) {
    String url = this->urls["api"][api] + "/" + this->version + "/" + path;
    
    if (api == "public") {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    } else {
        this->checkRequiredCredentials();
        String nonce = this->nonce().str();
        String timestamp = std::to_string(this->milliseconds());
        String auth = timestamp + method + "/" + path;
        
        if (method == "POST") {
            if (!params.empty()) {
                body = this->json(params);
                auth += body;
            }
        } else {
            if (!params.empty()) {
                String query = this->urlencode(params);
                url += "?" + query;
                auth += "?" + query;
            }
        }
        
        String signature = this->hmac(auth, this->encode(this->secret),
                                    "sha512", "hex");
        
        const_cast<std::map<String, String>&>(headers)["API-KEY"] = this->apiKey;
        const_cast<std::map<String, String>&>(headers)["SIGNATURE"] = signature;
        const_cast<std::map<String, String>&>(headers)["NONCE"] = nonce;
        const_cast<std::map<String, String>&>(headers)["TIMESTAMP"] = timestamp;
        
        if (method == "POST") {
            const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/json";
        }
    }
    
    return url;
}

String Gopax::createNonce() {
    return std::to_string(this->milliseconds());
}

json Gopax::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "id");
    String timestamp = this->safeString(order, "timestamp");
    String status = this->parseOrderStatus(this->safeString(order, "status"));
    String symbol = nullptr;
    
    if (!market.empty()) {
        symbol = market["symbol"];
    }
    
    String type = this->safeStringLower(order, "type");
    String side = this->safeStringLower(order, "side");
    
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

String Gopax::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"placed", "open"},
        {"cancelled", "canceled"},
        {"completed", "closed"},
        {"rejected", "rejected"},
        {"expired", "expired"}
    };
    
    return this->safeString(statuses, status, status);
}

} // namespace ccxt
