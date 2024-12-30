#include "btcex.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Btcex::Btcex() {
    id = "btcex";
    name = "BTCEX";
    version = "v1";
    rateLimit = 50;
    certified = false;
    pro = true;

    // Initialize API endpoints
    baseUrl = "https://api.btcex.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/173489620-d49807a4-55cd-4f4e-aca9-534921298bbf.jpg"},
        {"api", {
            {"public", "https://api.btcex.com/api/v1"},
            {"private", "https://api.btcex.com/api/v1"},
            {"futures", "https://api.btcex.com/api/v1/futures"},
            {"options", "https://api.btcex.com/api/v1/options"}
        }},
        {"www", "https://www.btcex.com"},
        {"doc", {
            "https://docs.btcex.com"
        }},
        {"fees", "https://www.btcex.com/fees"}
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
        {"8h", "480"},
        {"12h", "720"},
        {"1d", "D"},
        {"3d", "3D"},
        {"1w", "W"},
        {"1M", "M"}
    };

    options = {
        {"adjustForTimeDifference", true},
        {"recvWindow", "5000"},
        {"defaultType", "spot"},
        {"defaultMarginMode", "cross"}
    };

    errorCodes = {
        {10000, "Operation successful"},
        {10001, "Internal server error"},
        {10002, "Service unavailable"},
        {10003, "Invalid parameter"},
        {10004, "Invalid API key"},
        {10005, "Invalid signature"},
        {10006, "IP not allowed"},
        {10007, "Request timeout"},
        {10008, "System maintenance"},
        {10009, "Rate limit exceeded"},
        {20001, "Account not found"},
        {20002, "Account suspended"},
        {20003, "Insufficient balance"},
        {20004, "Account frozen"},
        {20005, "Trading pair not found"},
        {20006, "Trading disabled"},
        {20007, "Order not found"},
        {20008, "Order filled"},
        {20009, "Order cancelled"},
        {20010, "Order cannot be cancelled"},
        {20011, "Invalid order price"},
        {20012, "Invalid order quantity"},
        {20013, "Invalid order type"},
        {20014, "Invalid order side"},
        {20015, "Order rejected"},
        {20016, "Position not found"},
        {20017, "Invalid position mode"},
        {20018, "Invalid leverage"},
        {20019, "Invalid stop price"},
        {20020, "Invalid take profit price"},
        {20021, "Invalid stop loss price"},
        {20022, "Invalid margin mode"},
        {20023, "Invalid option type"},
        {20024, "Invalid strike price"},
        {20025, "Invalid expiry time"}
    };

    initializeApiEndpoints();
}

void Btcex::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "time",
                "markets",
                "ticker",
                "depth",
                "trades",
                "klines",
                "currencies"
            }}
        }},
        {"private", {
            {"GET", {
                "account",
                "orders",
                "order",
                "trades",
                "deposits",
                "withdrawals",
                "deposit/address"
            }},
            {"POST", {
                "order",
                "cancel",
                "withdraw"
            }}
        }},
        {"futures", {
            {"GET", {
                "contracts",
                "depth",
                "trades",
                "klines",
                "ticker",
                "fundingRate",
                "fundingRateHistory",
                "balance",
                "positions",
                "orders",
                "historyOrders",
                "trades"
            }},
            {"POST", {
                "order",
                "cancel",
                "leverage",
                "marginMode"
            }}
        }},
        {"options", {
            {"GET", {
                "contracts",
                "depth",
                "trades",
                "klines",
                "ticker",
                "balance",
                "positions",
                "orders",
                "historyOrders",
                "trades"
            }},
            {"POST", {
                "order",
                "cancel"
            }}
        }}
    };
}

json Btcex::fetchMarkets(const json& params) {
    json response = fetch("/markets", "public", "GET", params);
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
            {"margin", market["marginTrading"]},
            {"future", false},
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

json Btcex::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/account", "private", "GET", params);
    return parseBalance(response);
}

json Btcex::parseBalance(const json& response) {
    json balances = response["data"]["balances"];
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

json Btcex::createOrder(const String& symbol, const String& type,
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
    
    json response = fetch("/order", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response["data"], market);
}

String Btcex::sign(const String& path, const String& api,
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
        
        String auth = timestamp + method + endpoint;
        if (!queryString.empty()) {
            auth += "?" + queryString;
        }
        
        String signature = this->hmac(auth, this->encode(this->config_.secret),
                                    "sha256", "hex");
        
        const_cast<std::map<String, String>&>(headers)["BTCEX-ACCESS-KEY"] = this->config_.apiKey;
        const_cast<std::map<String, String>&>(headers)["BTCEX-ACCESS-SIGN"] = signature;
        const_cast<std::map<String, String>&>(headers)["BTCEX-ACCESS-TIMESTAMP"] = timestamp;
        
        if (method == "GET") {
            url += "?" + queryString;
        } else {
            body = this->extend(params, {
                "timestamp": timestamp
            });
            const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/json";
        }
    }
    
    return url;
}

json Btcex::parseOrder(const json& order, const Market& market) {
    String status = this->parseOrderStatus(this->safeString(order, "status"));
    String symbol = market["symbol"];
    String timestamp = this->safeString(order, "time");
    String price = this->safeString(order, "price");
    String amount = this->safeString(order, "quantity");
    String filled = this->safeString(order, "executedQty");
    String cost = this->safeString(order, "cumQuote");
    
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

json Btcex::parseOrderStatus(const String& status) {
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
