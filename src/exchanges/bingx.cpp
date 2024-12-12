#include "ccxt/exchanges/bingx.h"
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/spawn.hpp>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <openssl/hmac.h>
#include <openssl/sha.h>

namespace ccxt {

BingX::BingX() : Exchange() {
    id = "bingx";
    name = "BingX";
    version = "v1";
    rateLimit = 100;
    certified = true;
    pro = true;
    countries = {"SG"};
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/158227251-3a92a220-9222-453c-9277-977c6677fe71.jpg"},
        {"api", {
            {"public", "https://api-swap-rest.bingx.com"},
            {"private", "https://api-swap-rest.bingx.com"}
        }},
        {"www", "https://bingx.com"},
        {"doc", {
            "https://bingx-api.github.io/docs/",
            "https://bingx-api.github.io/docs/swap/intro"
        }},
        {"fees", "https://bingx.com/en-us/support/articles/360012929114"}
    };
    api = {
        {"public", {
            {"GET", {
                "market/tickers",
                "market/ticker",
                "market/depth",
                "market/trades",
                "market/kline",
                "market/time",
                "market/contracts",
                "market/fundingRate"
            }}
        }},
        {"private", {
            {"GET", {
                "user/balance",
                "user/orders",
                "user/order",
                "user/openOrders",
                "user/positions",
                "user/position",
                "user/trades",
                "user/deposits",
                "user/withdrawals",
                "user/depositAddress"
            }},
            {"POST", {
                "user/order",
                "user/cancel",
                "user/leverage",
                "user/marginType",
                "user/positionMode",
                "user/withdraw",
                "user/transfer"
            }}
        }}
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
        {"defaultType", "swap"},
        {"marginMode", "isolated"},
        {"defaultMarginMode", "isolated"}
    };
    errorCodes = {
        {10001, "System error"},
        {10002, "Invalid request parameters"},
        {10003, "Invalid request"},
        {10004, "Rate limit exceeded"},
        {10005, "Permission denied"},
        {10006, "Too many requests"},
        {10007, "Invalid signature"},
        {10008, "Invalid timestamp"},
        {10009, "Invalid api key"},
        {10010, "Invalid api secret"},
        {10011, "Invalid api permission"},
        {10012, "Invalid ip"},
        {10013, "Invalid order id"},
        {10014, "Invalid symbol"},
        {10015, "Invalid order type"},
        {10016, "Invalid side"},
        {10017, "Invalid quantity"},
        {10018, "Invalid price"},
        {10019, "Invalid timestamp"},
        {10020, "Invalid leverage"},
        {10021, "Invalid margin mode"},
        {10022, "Invalid position mode"},
        {10023, "Invalid order status"},
        {10024, "Invalid time in force"},
        {10025, "Invalid stop price"},
        {10026, "Invalid stop type"},
        {10027, "Invalid currency"},
        {10028, "Invalid network"},
        {10029, "Invalid address"},
        {10030, "Invalid amount"},
        {10031, "Invalid fee"},
        {10032, "Invalid tag"},
        {10033, "Invalid transfer type"},
        {10034, "Invalid from account"},
        {10035, "Invalid to account"}
    };
    initializeApiEndpoints();
}

void BingX::initializeApiEndpoints() {
    // Initialize API endpoints here
}

// Synchronous implementations
json BingX::fetchMarkets(const json& params) {
    auto response = request("market/contracts", "public", "GET", params);
    return parseMarkets(response);
}

// Example of an async implementation using boost::future
boost::future<json> BingX::fetchMarketsAsync(const json& params) {
    return makeAsyncRequest("market/contracts", "public", "GET", params)
        .then([this](json response) {
            return parseMarkets(response);
        });
}

// Helper function to make async requests
boost::future<json> BingX::makeAsyncRequest(const String& path, const String& api,
                                          const String& method, const json& params,
                                          const std::map<String, String>& headers,
                                          const json& body) {
    return boost::async([=]() {
        return request(path, api, method, params, headers, body);
    });
}

String BingX::sign(const String& path, const String& api,
                  const String& method, const json& params,
                  const std::map<String, String>& headers,
                  const json& body) {
    auto timestamp = std::to_string(Exchange::milliseconds());
    auto queryString = buildQueryString(params);
    
    if (api == "private") {
        auto signature = createSignature(timestamp, method, path, queryString);
        queryString += "&timestamp=" + timestamp + "&signature=" + signature;
    }
    
    return path + (queryString.empty() ? "" : "?" + queryString);
}

String BingX::createSignature(const String& timestamp, const String& method,
                            const String& path, const String& queryString) {
    auto message = timestamp + method + path + queryString;
    return hmac(message, secret, "sha256");
}

// Implement other methods similarly...

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

json BingX::fetchTime(const json& params) {
    json response = request("server/time", "spot/v1/public", "GET", params);
    return {
        {"timestamp", response["data"]["serverTime"].get<int64_t>()}
    };
}

json BingX::fetchTradingFee(const String& symbol, const json& params) {
    if (symbol.empty()) {
        throw ArgumentsRequired("fetchTradingFee requires a symbol argument");
    }
    loadMarkets();
    Market market = getMarket(symbol);
    json response = request("user/commissionRate", "spot/v1/private", "GET", {
        {"symbol", market.id}
    });
    json data = response["data"];
    return {
        {"info", response},
        {"symbol", symbol},
        {"maker", data["makerCommissionRate"].get<double>()},
        {"taker", data["takerCommissionRate"].get<double>()}
    };
}

json BingX::transfer(const String& code, double amount, const String& fromAccount, const String& toAccount, const json& params) {
    loadMarkets();
    Currency currency = getCurrency(code);
    json request = {
        {"asset", currency.id},
        {"amount", amount},
        {"fromAccountType", fromAccount},
        {"toAccountType", toAccount}
    };
    json response = request("asset/transfer", "spot/v3/private", "POST", extend(request, params));
    return parseTransfer(response);
}

json BingX::setLeverage(int leverage, const String& symbol, const json& params) {
    loadMarkets();
    Market market = getMarket(symbol);
    json request = {
        {"symbol", market.id},
        {"leverage", leverage}
    };
    return request("contract/leverage", "swap/v1/private", "POST", extend(request, params));
}

json BingX::setMarginMode(const String& marginMode, const String& symbol, const json& params) {
    loadMarkets();
    Market market = getMarket(symbol);
    String mode = marginMode.to_upper();
    if (mode != "ISOLATED" && mode != "CROSS") {
        throw BadRequest("marginMode must be either 'ISOLATED' or 'CROSS'");
    }
    json request = {
        {"symbol", market.id},
        {"marginMode", mode}
    };
    return request("contract/marginMode", "swap/v1/private", "POST", extend(request, params));
}

json BingX::setPositionMode(const String& hedged, const String& symbol, const json& params) {
    loadMarkets();
    Market market = getMarket(symbol);
    json request = {
        {"symbol", market.id},
        {"positionMode", hedged}
    };
    return request("contract/positionMode", "swap/v1/private", "POST", extend(request, params));
}

} // namespace ccxt
