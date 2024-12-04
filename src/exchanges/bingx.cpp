#include "bingx.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

BingX::BingX() {
    id = "bingx";
    name = "BingX";
    version = "v1";
    rateLimit = 50;
    certified = false;
    pro = true;

    // Initialize API endpoints
    baseUrl = "https://open-api.bingx.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/144690351-e7e7012c-4e67-4ca0-9d7c-c646c4c625b5.jpg"},
        {"api", {
            {"public", "https://open-api.bingx.com/openApi"},
            {"private", "https://open-api.bingx.com/openApi"},
            {"perpetual", "https://open-api.bingx.com/swap-api"},
            {"copyTrading", "https://open-api.bingx.com/copy-trade"}
        }},
        {"www", "https://bingx.com"},
        {"doc", {
            "https://bingx-api.github.io/docs/",
            "https://bingx-api.github.io/docs/swap/",
            "https://bingx-api.github.io/docs/copy-trade/"
        }},
        {"fees", "https://bingx.com/en-us/support/articles/360012666414"}
    };

    timeframes = {
        {"1m", "1m"},
        {"3m", "3m"},
        {"5m", "5m"},
        {"15m", "15m"},
        {"30m", "30m"},
        {"1h", "1h"},
        {"2h", "2h"},
        {"4h", "4h"},
        {"6h", "6h"},
        {"8h", "8h"},
        {"12h", "12h"},
        {"1d", "1d"},
        {"3d", "3d"},
        {"1w", "1w"},
        {"1M", "1M"}
    };

    options = {
        {"adjustForTimeDifference", true},
        {"recvWindow", "5000"},
        {"defaultType", "spot"},
        {"defaultMarginMode", "cross"}
    };

    errorCodes = {
        {10000, "Operation successful"},
        {10001, "System error"},
        {10002, "System is busy"},
        {10003, "Request parameter error"},
        {10004, "Invalid API key"},
        {10005, "Invalid signature"},
        {10006, "IP is not in the whitelist"},
        {10007, "Request timeout"},
        {10008, "System is maintaining"},
        {10009, "Request frequency exceeds the limit"},
        {20001, "Account does not exist"},
        {20002, "Account status is abnormal"},
        {20003, "Account balance is insufficient"},
        {20004, "Account has been frozen"},
        {20005, "Trading pair does not exist"},
        {20006, "Trading pair is not enabled"},
        {20007, "Order does not exist"},
        {20008, "Order has been filled"},
        {20009, "Order has been cancelled"},
        {20010, "Order cannot be cancelled"},
        {20011, "Order price is invalid"},
        {20012, "Order quantity is invalid"},
        {20013, "Order type is invalid"},
        {20014, "Order side is invalid"},
        {20015, "Order has been rejected"},
        {20016, "Position does not exist"},
        {20017, "Position mode is invalid"},
        {20018, "Leverage is invalid"},
        {20019, "Stop price is invalid"},
        {20020, "Take profit price is invalid"},
        {20021, "Stop loss price is invalid"}
    };

    initializeApiEndpoints();
}

void BingX::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "market/time",
                "market/pairs",
                "market/ticker",
                "market/depth",
                "market/trades",
                "market/kline",
                "market/price"
            }}
        }},
        {"private", {
            {"GET", {
                "user/balance",
                "user/orders",
                "user/order",
                "user/trades",
                "user/deposits",
                "user/withdrawals",
                "user/deposit/address"
            }},
            {"POST", {
                "user/order",
                "user/cancel",
                "user/withdraw"
            }}
        }},
        {"perpetual", {
            {"GET", {
                "quote/contracts",
                "quote/depth",
                "quote/trades",
                "quote/klines",
                "quote/ticker",
                "quote/fundingRate",
                "quote/fundingRateHistory",
                "user/balance",
                "user/positions",
                "user/orders",
                "user/historyOrders",
                "user/trades"
            }},
            {"POST", {
                "user/order",
                "user/cancel",
                "user/leverage",
                "user/marginMode"
            }}
        }},
        {"copyTrading", {
            {"GET", {
                "positions",
                "orders",
                "balance"
            }},
            {"POST", {
                "order",
                "cancel"
            }}
        }}
    };
}

json BingX::fetchMarkets(const json& params) {
    json response = fetch("/market/pairs", "public", "GET", params);
    json markets = response["data"];
    json result = json::array();
    
    for (const auto& market : markets) {
        String id = market["symbol"];
        String baseId = market["baseAsset"];
        String quoteId = market["quoteAsset"];
        String base = this->commonCurrencyCode(baseId);
        String quote = this->commonCurrencyCode(quoteId);
        String symbol = base + "/" + quote;
        
        result.push_back({
            {"id", id},
            {"symbol", symbol},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", market["status"] == "TRADING"},
            {"type", "spot"},
            {"spot", true},
            {"future", false},
            {"swap", false},
            {"option", false},
            {"contract", false},
            {"precision", {
                {"amount", market["quantityPrecision"].get<int>()},
                {"price", market["pricePrecision"].get<int>()}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeFloat(market, "minQuantity")},
                    {"max", this->safeFloat(market, "maxQuantity")}
                }},
                {"price", {
                    {"min", this->safeFloat(market, "minPrice")},
                    {"max", this->safeFloat(market, "maxPrice")}
                }},
                {"cost", {
                    {"min", this->safeFloat(market, "minNotional")},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return result;
}

json BingX::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/user/balance", "private", "GET", params);
    return parseBalance(response);
}

json BingX::parseBalance(const json& response) {
    json balances = response["data"];
    json result = {{"info", response}};
    
    for (const auto& balance : balances) {
        String currencyId = balance["asset"];
        String code = this->commonCurrencyCode(currencyId);
        String account = {
            {"free", this->safeFloat(balance, "free")},
            {"used", this->safeFloat(balance, "locked")},
            {"total", this->safeFloat(balance, "total")}
        };
        result[code] = account;
    }
    
    return result;
}

json BingX::createOrder(const String& symbol, const String& type,
                       const String& side, double amount,
                       double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"symbol", market["id"]},
        {"side", side.upper()},
        {"type", type.upper()},
        {"quantity", this->amountToPrecision(symbol, amount)}
    };
    
    if (type == "LIMIT") {
        request["price"] = this->priceToPrecision(symbol, price);
        request["timeInForce"] = "GTC";
    }
    
    json response = fetch("/user/order", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response["data"], market);
}

String BingX::sign(const String& path, const String& api,
                   const String& method, const json& params,
                   const std::map<String, String>& headers,
                   const json& body) {
    String endpoint = "/" + this->version + path;
    String url = this->urls["api"][api] + endpoint;
    
    if (api == "public") {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    } else {
        this->checkRequiredCredentials();
        
        String timestamp = std::to_string(this->milliseconds());
        String queryString = this->urlencode(this->extend({
            "timestamp": timestamp,
            "recvWindow": this->options["recvWindow"]
        }, params));
        
        String auth = queryString + "|" + this->secret;
        String signature = this->hmac(auth, this->encode(this->secret),
                                    "sha256", "hex");
        
        queryString += "&signature=" + signature;
        
        const_cast<std::map<String, String>&>(headers)["X-BX-APIKEY"] = this->apiKey;
        
        if (method == "GET") {
            url += "?" + queryString;
        } else {
            body = this->extend(params, {
                "timestamp": timestamp,
                "signature": signature
            });
            const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/json";
        }
    }
    
    return url;
}

json BingX::parseOrder(const json& order, const Market& market) {
    String status = this->parseOrderStatus(this->safeString(order, "status"));
    String symbol = market["symbol"];
    String timestamp = this->safeString(order, "time");
    String price = this->safeString(order, "price");
    String amount = this->safeString(order, "origQty");
    String filled = this->safeString(order, "executedQty");
    String cost = this->safeString(order, "cummulativeQuoteQty");
    
    return {
        {"id", this->safeString(order, "orderId")},
        {"clientOrderId", this->safeString(order, "clientOrderId")},
        {"timestamp", this->safeInteger(order, "time")},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"symbol", symbol},
        {"type", this->safeStringLower(order, "type")},
        {"side", this->safeStringLower(order, "side")},
        {"price", price},
        {"amount", amount},
        {"cost", cost},
        {"average", filled == "0" ? nullptr : this->safeString(order, "avgPrice")},
        {"filled", filled},
        {"remaining", this->safeString(order, "remainingQty")},
        {"status", status},
        {"fee", {
            {"cost", this->safeString(order, "fee")},
            {"currency", market["quote"]}
        }},
        {"trades", nullptr},
        {"info", order}
    };
}

json BingX::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"NEW", "open"},
        {"PARTIALLY_FILLED", "open"},
        {"FILLED", "closed"},
        {"CANCELED", "canceled"},
        {"PENDING_CANCEL", "canceling"},
        {"REJECTED", "rejected"},
        {"EXPIRED", "expired"}
    };
    
    return statuses.contains(status) ? statuses.at(status) : status;
}

} // namespace ccxt
