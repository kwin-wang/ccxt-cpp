#include "bitrue.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Bitrue::Bitrue() {
    id = "bitrue";
    name = "Bitrue";
    version = "v1";
    rateLimit = 100;
    certified = false;
    pro = false;

    // Initialize API endpoints
    baseUrl = "https://www.bitrue.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/139516488-243a830d-05dd-446b-91c6-c1f18fe30c63.jpg"},
        {"api", {
            {"public", "https://www.bitrue.com/api/v1"},
            {"private", "https://www.bitrue.com/api/v1"},
            {"v2", "https://www.bitrue.com/api/v2"}
        }},
        {"www", "https://www.bitrue.com"},
        {"doc", {
            "https://github.com/Bitrue-exchange/bitrue-official-api-docs"
        }},
        {"fees", "https://bitrue.zendesk.com/hc/en-001/articles/360002043494-Trading-Fees"}
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
        {"timeDifference", 0},
        {"defaultType", "spot"}
    };

    errorCodes = {
        {-1000, "An unknown error occurred while processing the request"},
        {-1001, "Internal error; unable to process your request. Please try again"},
        {-1002, "You are not authorized to execute this request"},
        {-1003, "Too many requests; please use the websocket for live updates"},
        {-1004, "Server is busy, please wait and try again"},
        {-1006, "An unexpected response was received from the message bus"},
        {-1007, "Timeout waiting for response from backend server"},
        {-1014, "Unsupported order combination"},
        {-1015, "Too many new orders"},
        {-1016, "Service shutting down"},
        {-1020, "Unsupported operation"},
        {-1021, "Invalid timestamp"},
        {-1022, "Invalid signature"},
        {-1100, "Illegal characters found in parameter"},
        {-1101, "Too many parameters sent for this endpoint"},
        {-1102, "Mandatory parameter was not sent, was empty/null, or malformed"},
        {-1103, "Unknown parameter sent"},
        {-1104, "Not all sent parameters were read"},
        {-1105, "Parameter empty"},
        {-1106, "Parameter not required"},
        {-1111, "Precision is over the maximum defined for this asset"},
        {-1112, "No orders on book for symbol"},
        {-1114, "TimeInForce parameter sent when not required"},
        {-1115, "Invalid timeInForce"},
        {-1116, "Invalid orderType"},
        {-1117, "Invalid side"},
        {-1118, "New client order ID was empty"},
        {-1119, "Original client order ID was empty"},
        {-1120, "Invalid interval"},
        {-1121, "Invalid symbol"},
        {-1125, "This listenKey does not exist"},
        {-1127, "More than 1000 requests per minute"},
        {-1128, "Request is not valid"},
        {-1130, "Invalid data sent for a parameter"},
        {-2010, "New order rejected"},
        {-2011, "Cancel rejected"},
        {-2013, "No such order"},
        {-2014, "Bad API key format"},
        {-2015, "Invalid API key, IP, or permissions for action"},
        {-2016, "No trading window could be found for the symbol"}
    };

    initializeApiEndpoints();
}

void Bitrue::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "ping",
                "time",
                "exchangeInfo",
                "depth",
                "trades",
                "historicalTrades",
                "aggTrades",
                "klines",
                "ticker/24hr",
                "ticker/price",
                "ticker/bookTicker"
            }}
        }},
        {"private", {
            {"GET", {
                "order",
                "openOrders",
                "allOrders",
                "account",
                "myTrades",
                "depositHistory",
                "withdrawHistory",
                "depositAddress",
                "tradeFee",
                "userAssets"
            }},
            {"POST", {
                "order",
                "order/test",
                "withdraw"
            }},
            {"DELETE", {
                "order"
            }}
        }},
        {"v2", {
            {"GET", {
                "myTrades",
                "capital/config/getall",
                "capital/deposit/address",
                "capital/deposit/history",
                "capital/withdraw/history"
            }}
        }}
    };
}

json Bitrue::fetchMarkets(const json& params) {
    json response = fetch("/exchangeInfo", "public", "GET", params);
    json markets = response["symbols"];
    json result = json::array();
    
    for (const auto& market : markets) {
        if (market["status"] != "TRADING") {
            continue;
        }
        
        std::string id = market["symbol"];
        std::string baseId = market["baseAsset"];
        std::string quoteId = market["quoteAsset"];
        std::string base = this->commonCurrencyCode(baseId);
        std::string quote = this->commonCurrencyCode(quoteId);
        std::string symbol = base + "/" + quote;
        
        json filters = this->indexBy(market["filters"], "filterType");
        
        json priceFilter = filters["PRICE_FILTER"];
        json lotSize = filters["LOT_SIZE"];
        
        int precision = {
            {"amount", this->precisionFromstd::string(this->safeString(lotSize, "minQty"))},
            {"price", this->precisionFromstd::string(this->safeString(priceFilter, "minPrice"))}
        };
        
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
            {"margin", this->safeValue(market, "isMarginTradingAllowed", false)},
            {"precision", precision},
            {"limits", {
                {"amount", {
                    {"min", this->safeFloat(lotSize, "minQty")},
                    {"max", this->safeFloat(lotSize, "maxQty")}
                }},
                {"price", {
                    {"min", this->safeFloat(priceFilter, "minPrice")},
                    {"max", this->safeFloat(priceFilter, "maxPrice")}
                }},
                {"cost", {
                    {"min", this->safeFloat(market, "minNotional", 0)},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return result;
}

json Bitrue::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/account", "private", "GET", params);
    return parseBalance(response);
}

json Bitrue::parseBalance(const json& response) {
    json balances = response["balances"];
    json result = {{"info", response}};
    
    for (const auto& balance : balances) {
        std::string currencyId = balance["asset"];
        std::string code = this->commonCurrencyCode(currencyId);
        std::string account = {
            {"free", this->safeFloat(balance, "free")},
            {"used", this->safeFloat(balance, "locked")},
            {"total", nullptr}
        };
        account["total"] = this->sum(account["free"], account["used"]);
        result[code] = account;
    }
    
    return result;
}

json Bitrue::createOrder(const std::string& symbol, const std::string& type,
                        const std::string& side, double amount,
                        double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    std::string uppercaseType = type.upper();
    
    json request = {
        {"symbol", market["id"]},
        {"side", side.upper()},
        {"type", uppercaseType},
        {"quantity", this->amountToPrecision(symbol, amount)}
    };
    
    if (uppercaseType == "LIMIT") {
        request["price"] = this->priceToPrecision(symbol, price);
        request["timeInForce"] = "GTC";
    }
    
    json response = fetch("/order", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response, market);
}

std::string Bitrue::sign(const std::string& path, const std::string& api,
                    const std::string& method, const json& params,
                    const std::map<std::string, std::string>& headers,
                    const json& body) {
    std::string url = this->urls["api"][api] + path;
    std::string timestamp = std::to_string(this->milliseconds());
    
    if (api == "public") {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    } else {
        this->checkRequiredCredentials();
        
        json request = this->extend({
            "timestamp": timestamp,
            "recvWindow": this->options["recvWindow"]
        }, params);
        
        std::string querystd::string = this->urlencode(request);
        std::string signature = this->hmac(querystd::string, this->encode(this->config_.secret),
                                    "sha256", "hex");
        querystd::string += "&signature=" + signature;
        
        if (method == "GET") {
            url += "?" + querystd::string;
        } else {
            body = request;
            body["signature"] = signature;
            const_cast<std::map<std::string, std::string>&>(headers)["Content-Type"] = "application/x-www-form-urlencoded";
        }
        
        const_cast<std::map<std::string, std::string>&>(headers)["X-MBX-APIKEY"] = this->config_.apiKey;
    }
    
    return url;
}

json Bitrue::parseOrder(const json& order, const Market& market) {
    std::string status = this->parseOrderStatus(this->safeString(order, "status"));
    std::string symbol = market["symbol"];
    std::string timestamp = this->safeString(order, "time");
    std::string price = this->safeString(order, "price");
    std::string amount = this->safeString(order, "origQty");
    std::string filled = this->safeString(order, "executedQty");
    std::string remaining = nullptr;
    
    if (amount != nullptr && filled != nullptr) {
        remaining = std::to_string(std::stod(amount) - std::stod(filled));
    }
    
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
        {"cost", this->safeString(order, "cummulativeQuoteQty")},
        {"average", nullptr},
        {"filled", filled},
        {"remaining", remaining},
        {"status", status},
        {"fee", nullptr},
        {"trades", nullptr},
        {"info", order}
    };
}

json Bitrue::parseOrderStatus(const std::string& status) {
    static const std::map<std::string, std::string> statuses = {
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
