#include "korbit.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Korbit::Korbit() {
    id = "korbit";
    name = "Korbit";
    version = "1";
    rateLimit = 500;
    certified = true;
    pro = false;

    // Initialize API endpoints
    baseUrl = "https://api.korbit.co.kr";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/27766836-e9456312-5ee6-11e7-9b3c-b628ca5626a5.jpg"},
        {"api", {
            {"public", "https://api.korbit.co.kr"},
            {"private", "https://api.korbit.co.kr"}
        }},
        {"www", "https://www.korbit.co.kr"},
        {"doc", {
            "https://apidocs.korbit.co.kr"
        }},
        {"referral", "https://www.korbit.co.kr/?ref=testuser"},
        {"fees", "https://www.korbit.co.kr/fees"}
    };

    timeframes = {
        {"1m", "1"},
        {"3m", "3"},
        {"5m", "5"},
        {"15m", "15"},
        {"30m", "30"},
        {"1h", "60"},
        {"2h", "120"},
        {"4h", "240"},
        {"6h", "360"},
        {"12h", "720"},
        {"1d", "1440"},
        {"3d", "4320"},
        {"1w", "10080"}
    };

    options = {
        {"adjustForTimeDifference", true},
        {"recvWindow", 5000},
        {"timeDifference", 0}
    };

    errorCodes = {
        {0, "Success"},
        {1, "Invalid parameters"},
        {2, "Invalid api key"},
        {3, "Invalid signature"},
        {4, "Invalid nonce"},
        {5, "Invalid access token"},
        {6, "Invalid permission"},
        {7, "Insufficient funds"},
        {8, "Rate limit exceeded"},
        {9, "Invalid order"},
        {10, "Order not found"},
        {11, "Order already canceled"}
    };

    currencyIds = {
        {"BTC", "btc"},
        {"ETH", "eth"},
        {"XRP", "xrp"},
        {"BCH", "bch"},
        {"ETC", "etc"},
        {"KRW", "krw"}
    };

    accessToken = "";
    accessTokenExpiry = 0;

    initializeApiEndpoints();
}

void Korbit::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "ticker",
                "ticker/detailed",
                "orderbook",
                "transactions",
                "constants"
            }}
        }},
        {"private", {
            {"GET", {
                "user/transactions",
                "user/orders",
                "user/orders/open",
                "user/balances",
                "user/accounts"
            }},
            {"POST", {
                "user/orders",
                "user/orders/cancel",
                "user/coin/address",
                "user/withdrawal/coin",
                "user/withdrawal/krw"
            }}
        }}
    };
}

json Korbit::fetchMarkets(const json& params) {
    json response = fetch("/constants", "public", "GET", params);
    json result = json::array();
    
    for (const auto& [id, market] : response["exchange"]["markets"].items()) {
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
            {"margin", false},
            {"contract", false},
            {"precision", {
                {"amount", this->safeInteger(market, "baseAssetPrecision")},
                {"price", this->safeInteger(market, "quotePrecision")}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeFloat(market, "minOrderSize")},
                    {"max", this->safeFloat(market, "maxOrderSize")}
                }},
                {"price", {
                    {"min", this->safeFloat(market, "minPrice")},
                    {"max", this->safeFloat(market, "maxPrice")}
                }},
                {"cost", {
                    {"min", this->safeFloat(market, "minTotal")},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return result;
}

json Korbit::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/user/balances", "private", "GET", params);
    return parseBalance(response);
}

json Korbit::parseBalance(const json& response) {
    json result = {{"info", response}};
    
    for (const auto& balance : response) {
        std::string currencyId = this->safeString(balance, "currency");
        std::string code = this->safeCurrencyCode(currencyId);
        std::string account = {
            {"free", this->safeFloat(balance, "available")},
            {"used", this->safeFloat(balance, "trade_in_use")},
            {"total", nullptr}
        };
        account["total"] = this->sum(account["free"], account["used"]);
        result[code] = account;
    }
    
    return result;
}

json Korbit::createOrder(const std::string& symbol, const std::string& type,
                        const std::string& side, double amount,
                        double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"currency_pair", market["id"]},
        {"type", type},
        {"side", side},
        {"amount", this->amountToPrecision(symbol, amount)}
    };
    
    if (type == "limit") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    json response = fetch("/user/orders", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response, market);
}

void Korbit::refreshAccessToken() {
    long long currentTime = this->milliseconds();
    if (currentTime >= accessTokenExpiry) {
        json response = fetch("/oauth2/access_token", "private", "POST", {
            {"client_id", this->config_.apiKey},
            {"client_secret", this->config_.secret},
            {"grant_type", "client_credentials"}
        });
        
        accessToken = this->safeString(response, "access_token");
        int expiresIn = this->safeInteger(response, "expires_in", 3600);
        accessTokenExpiry = currentTime + (expiresIn * 1000);
    }
}

std::string Korbit::sign(const std::string& path, const std::string& api,
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
        this->refreshAccessToken();
        
        std::string nonce = this->nonce().str();
        std::string auth = nonce + method + "/" + path;
        
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
        
        const_cast<std::map<std::string, std::string>&>(headers)["API-Key"] = this->config_.apiKey;
        const_cast<std::map<std::string, std::string>&>(headers)["API-Nonce"] = nonce;
        const_cast<std::map<std::string, std::string>&>(headers)["API-Signature"] = signature;
        const_cast<std::map<std::string, std::string>&>(headers)["Authorization"] = "Bearer " + accessToken;
        
        if (method == "POST") {
            const_cast<std::map<std::string, std::string>&>(headers)["Content-Type"] = "application/json";
        }
    }
    
    return url;
}

std::string Korbit::createNonce() {
    return std::to_string(this->milliseconds());
}

json Korbit::parseOrder(const json& order, const Market& market) {
    std::string id = this->safeString(order, "id");
    std::string timestamp = this->safeString(order, "timestamp");
    std::string status = this->parseOrderStatus(this->safeString(order, "status"));
    std::string symbol = nullptr;
    
    if (!market.empty()) {
        symbol = market["symbol"];
    }
    
    std::string type = this->safeString(order, "type");
    std::string side = this->safeString(order, "side");
    
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
        {"amount", this->safeFloat(order, "amount")},
        {"filled", this->safeFloat(order, "filled_amount")},
        {"remaining", this->safeFloat(order, "remaining_amount")},
        {"trades", nullptr},
        {"fee", {
            {"currency", market["quote"]},
            {"cost", this->safeFloat(order, "fee")},
            {"rate", this->safeFloat(order, "fee_rate")}
        }},
        {"info", order}
    };
}

std::string Korbit::parseOrderStatus(const std::string& status) {
    static const std::map<std::string, std::string> statuses = {
        {"pending", "open"},
        {"unfilled", "open"},
        {"partially_filled", "open"},
        {"filled", "closed"},
        {"cancelled", "canceled"}
    };
    
    return this->safeString(statuses, status, status);
}

} // namespace ccxt
