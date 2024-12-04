#include "coinone.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Coinone::Coinone() {
    id = "coinone";
    name = "CoinOne";
    version = "2";
    rateLimit = 667;
    certified = true;
    pro = false;
    hasV2Api = true;

    // Initialize API endpoints
    baseUrl = "https://api.coinone.co.kr";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/38003300-adc12fba-323f-11e8-8525-725f53c4a659.jpg"},
        {"api", {
            {"public", "https://api.coinone.co.kr"},
            {"private", "https://api.coinone.co.kr"}
        }},
        {"www", "https://coinone.co.kr"},
        {"doc", {
            "https://doc.coinone.co.kr"
        }},
        {"referral", "https://coinone.co.kr?ref=testuser"},
        {"fees", "https://coinone.co.kr/fee"}
    };

    timeframes = {
        {"1m", "1"},
        {"3m", "3"},
        {"5m", "5"},
        {"15m", "15"},
        {"30m", "30"},
        {"1h", "60"},
        {"4h", "240"},
        {"6h", "360"},
        {"12h", "720"},
        {"1d", "1D"},
        {"3d", "3D"},
        {"1w", "1W"}
    };

    options = {
        {"adjustForTimeDifference", true},
        {"recvWindow", 5000},
        {"timeDifference", 0},
        {"defaultType", "spot"},
        {"v2Id", ""}
    };

    errorCodes = {
        {11, "Access token is missing"},
        {12, "Invalid access token"},
        {40, "Invalid API permission"},
        {50, "Authenticate error"},
        {51, "Invalid API key"},
        {52, "Expired API key"},
        {53, "Invalid signature"},
        {100, "Session expired"},
        {101, "Invalid format"},
        {102, "ID is not exist"},
        {103, "Lack of balance"},
        {104, "Order id is not exist"},
        {105, "Price is not correct"},
        {106, "Locking error"},
        {107, "Parameter error"},
        {111, "Order id is not exist"},
        {112, "Cancel failed"},
        {113, "Quantity is too low"},
        {120, "V2 API payload is missing"},
        {121, "V2 API signature is missing"},
        {122, "V2 API nonce is missing"},
        {123, "V2 API signature is not correct"},
        {130, "V2 API Nonce value is not correct"},
        {131, "V2 API Nonce is must be bigger than last nonce"},
        {132, "V2 API body is corrupted"}
    };

    currencyIds = {
        {"BTC", "btc"},
        {"ETH", "eth"},
        {"XRP", "xrp"},
        {"BCH", "bch"},
        {"ETC", "etc"},
        {"KRW", "krw"},
        {"USDT", "usdt"}
    };

    initializeApiEndpoints();
}

void Coinone::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "ticker",
                "ticker/all",
                "orderbook",
                "trades",
                "chart"
            }}
        }},
        {"private", {
            {"POST", {
                "v2/account/balance",
                "v2/account/daily_balance",
                "v2/account/user_info",
                "v2/account/virtual_account",
                "v2/order/limit_buy",
                "v2/order/limit_sell",
                "v2/order/market_buy",
                "v2/order/market_sell",
                "v2/order/cancel",
                "v2/order/complete_orders",
                "v2/order/limit_orders",
                "v2/order/order_info",
                "v2/transaction/auth_number",
                "v2/transaction/history",
                "v2/transaction/krw/history",
                "v2/transaction/coin/history"
            }}
        }}
    };
}

json Coinone::fetchMarkets(const json& params) {
    json response = fetch("/ticker/all", "public", "GET", params);
    json result = json::array();
    
    for (const auto& [id, market] : response.items()) {
        if (id != "result" && id != "errorCode" && id != "timestamp") {
            String baseId = id;
            String quoteId = "krw";
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
                {"margin", false},
                {"contract", false},
                {"precision", {
                    {"amount", 4},
                    {"price", 0}
                }},
                {"limits", {
                    {"amount", {
                        {"min", 0.0001},
                        {"max", nullptr}
                    }},
                    {"price", {
                        {"min", 1},
                        {"max", nullptr}
                    }},
                    {"cost", {
                        {"min", 500},
                        {"max", nullptr}
                    }}
                }},
                {"info", market}
            });
        }
    }
    
    return result;
}

json Coinone::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/v2/account/balance", "private", "POST", params);
    return parseBalance(response);
}

json Coinone::parseBalance(const json& response) {
    json result = {{"info", response}};
    
    for (const auto& [currency, balance] : response.items()) {
        if (currency != "result" && currency != "errorCode") {
            String code = this->safeCurrencyCode(currency);
            String account = {
                {"free", this->safeFloat(balance, "avail")},
                {"used", this->safeFloat(balance, "balance") - this->safeFloat(balance, "avail")},
                {"total", this->safeFloat(balance, "balance")}
            };
            result[code] = account;
        }
    }
    
    return result;
}

json Coinone::createOrder(const String& symbol, const String& type,
                         const String& side, double amount,
                         double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    String method = "v2/order/";
    if (type == "limit") {
        method += "limit_";
    } else {
        method += "market_";
    }
    method += side;
    
    json request = {
        {"currency", market["baseId"].toLower()},
        {"amount", this->amountToPrecision(symbol, amount)}
    };
    
    if (type == "limit") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    json response = fetch(method, "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response, market);
}

String Coinone::sign(const String& path, const String& api,
                    const String& method, const json& params,
                    const std::map<String, String>& headers,
                    const json& body) {
    String url = this->urls["api"][api] + "/" + path;
    
    if (api == "public") {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    } else {
        this->checkRequiredCredentials();
        String nonce = this->nonce().str();
        String auth = nonce + path;
        
        json request = this->extend({
            "access_token", this->apiKey,
            "nonce", nonce
        }, params);
        
        String payload = this->json(request);
        String signature = this->hmac(payload, this->encode(this->secret),
                                    "sha512", "hex");
        
        const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/json";
        const_cast<std::map<String, String>&>(headers)["X-COINONE-PAYLOAD"] = this->base64encode(payload);
        const_cast<std::map<String, String>&>(headers)["X-COINONE-SIGNATURE"] = signature;
        
        body = payload;
    }
    
    return url;
}

String Coinone::createNonce() {
    return std::to_string(this->milliseconds());
}

String Coinone::getV2Id() {
    String v2Id = this->safeString(this->options, "v2Id", "");
    if (v2Id.empty()) {
        json response = fetch("/v2/account/user_info", "private", "POST");
        v2Id = this->safeString(response, "user_id");
        this->options["v2Id"] = v2Id;
    }
    return v2Id;
}

json Coinone::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "orderId");
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
        {"cost", this->safeFloat(order, "total")},
        {"amount", this->safeFloat(order, "qty")},
        {"filled", this->safeFloat(order, "executed_qty")},
        {"remaining", this->safeFloat(order, "remaining_qty")},
        {"trades", nullptr},
        {"fee", {
            {"currency", market["quote"]},
            {"cost", this->safeFloat(order, "fee")},
            {"rate", this->safeFloat(order, "fee_rate")}
        }},
        {"info", order}
    };
}

String Coinone::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"live", "open"},
        {"partially_filled", "open"},
        {"filled", "closed"},
        {"cancelled", "canceled"},
        {"rejected", "rejected"}
    };
    
    return this->safeString(statuses, status, status);
}

} // namespace ccxt
