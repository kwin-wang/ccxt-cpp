#include "ccxt/exchanges/btcmarkets.h"
#include "ccxt/base/json_helper.h"
#include <openssl/hmac.h>
#include <sstream>
#include <iomanip>

namespace ccxt {

const std::string btcmarkets::defaultBaseURL = "https://api.btcmarkets.net";
const std::string btcmarkets::defaultVersion = "v3";
const int btcmarkets::defaultRateLimit = 1000;
const bool btcmarkets::defaultPro = false;

btcmarkets::btcmarkets(const Config& config) : Exchange(config) {
    init();
}

void btcmarkets::init() {
    
    
    // Set exchange properties
    this->id = "btcmarkets";
    this->name = "BTC Markets";
    this->countries = {"AU"};  // Australia
    this->version = defaultVersion;
    this->rateLimit = defaultRateLimit;
    this->pro = defaultPro;
    
    if (this->urls.empty()) {
        this->urls = {
            {"logo", "https://github.com/user-attachments/assets/8c8d6907-3873-4cc4-ad20-e22fba28247e"},
            {"api", {
                {"public", defaultBaseURL},
                {"private", defaultBaseURL}
            }},
            {"www", "https://btcmarkets.net"},
            {"doc", {
                "https://api.btcmarkets.net/doc/v3",
                "https://github.com/BTCMarkets/API"
            }}
        };
    }

    this->has = {
        {"CORS", std::nullopt},
        {"spot", true},
        {"margin", false},
        {"swap", false},
        {"future", false},
        {"option", false},
        {"cancelOrder", true},
        {"cancelOrders", true},
        {"createOrder", true},
        {"createTriggerOrder", true},
        {"fetchBalance", true},
        {"fetchDeposits", true},
        {"fetchMarkets", true},
        {"fetchMyTrades", true},
        {"fetchOHLCV", true},
        {"fetchOpenOrders", true},
        {"fetchOrder", true},
        {"fetchOrderBook", true},
        {"fetchOrders", true},
        {"fetchTicker", true},
        {"fetchTime", true},
        {"fetchTrades", true},
        {"fetchWithdrawals", true},
        {"withdraw", true}
    };
}

std::string btcmarkets::sign(const std::string& path, const std::string& api, const std::string& method,
                            const Json& params, const Json& headers, const Json& body) const {
    std::string url = this->urls["api"][api] + "/" + this->version + "/" + path;
    
    if (api == "public") {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    } else {
        this->checkRequiredCredentials();
        
        long long timestamp = this->milliseconds();
        std::string nonce = std::to_string(timestamp);
        
        std::string auth = path + "\n" + nonce + "\n";
        if (method != "GET") {
            auth += this->json(body) + "\n";
        }
        
        std::string signature = this->hmac(auth, this->base64ToBinary(this->secret), "SHA512", "base64");
        
        headers["BM-AUTH-APIKEY"] = this->apiKey;
        headers["BM-AUTH-TIMESTAMP"] = nonce;
        headers["BM-AUTH-SIGNATURE"] = signature;
        headers["Content-Type"] = "application/json";
    }

    return url;
}

void btcmarkets::handleErrors(const std::string& code, const std::string& reason, const std::string& url,
                             const std::string& method, const Json& headers, const Json& body,
                             const Json& response, const std::string& requestHeaders,
                             const std::string& requestBody) const {
    if (response.is_object() && response.contains("code")) {
        std::string errorCode = this->safeString(response, "code");
        std::string message = this->safeString(response, "message", "Unknown error");
        
        if (errorCode == "InsufficientFund") {
            throw InsufficientFunds(message);
        } else if (errorCode == "InvalidOrder") {
            throw InvalidOrder(message);
        } else if (errorCode == "OrderNotFound") {
            throw OrderNotFound(message);
        } else if (errorCode == "InvalidRequest") {
            throw BadRequest(message);
        }
        
        throw ExchangeError(message);
    }
}

Json btcmarkets::fetchTimeImpl() const {
    Json response = this->publicGetTime();
    return this->safeTimestamp(response, "timestamp");
}

Json btcmarkets::fetchMarketsImpl() const {
    Json response = this->publicGetMarkets();
    return this->parseMarkets(response);
}

Json btcmarkets::fetchTickerImpl(const std::string& symbol) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json response = this->publicGetMarketTicker(Json::object({
        {"marketId", market["id"]}
    }));
    return this->parseTicker(response, market);
}

Json btcmarkets::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json request = Json::object({
        {"marketId", this->marketId(symbol)}
    });
    if (limit) {
        request["level"] = *limit;
    }
    Json response = this->publicGetOrderBook(request);
    return this->parseOrderBook(response, symbol);
}

Json btcmarkets::fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe,
                               const std::optional<long long>& since, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json::object({
        {"marketId", market["id"]},
        {"timeWindow", this->timeframes[timeframe]}
    });
    if (since) {
        request["from"] = this->iso8601(*since);
    }
    if (limit) {
        request["limit"] = *limit;
    }
    Json response = this->publicGetCandles(request);
    return this->parseOHLCVs(response, market, timeframe, since, limit);
}

Json btcmarkets::createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side,
                               double amount, const std::optional<double>& price) {
    this->loadMarkets();
    Json market = this->market(symbol);
    
    Json request = Json::object({
        {"marketId", market["id"]},
        {"side", side.upper()},
        {"type", type.upper()},
        {"amount", this->amountToPrecision(symbol, amount)}
    });

    if (type == "limit") {
        if (!price) {
            throw ArgumentsRequired("createOrder() requires a price argument for limit orders");
        }
        request["price"] = this->priceToPrecision(symbol, *price);
    }

    Json response = this->privatePostOrders(request);
    return this->parseOrder(response);
}

Json btcmarkets::createTriggerOrderImpl(const std::string& symbol, const std::string& type, const std::string& side,
                                      double amount, double price, const std::optional<Json>& params) {
    this->loadMarkets();
    Json market = this->market(symbol);
    
    Json request = Json::object({
        {"marketId", market["id"]},
        {"side", side.upper()},
        {"type", type.upper()},
        {"amount", this->amountToPrecision(symbol, amount)},
        {"triggerPrice", this->priceToPrecision(symbol, price)}
    });

    if (type == "limit") {
        if (!params || !params->contains("price")) {
            throw ArgumentsRequired("createTriggerOrder() requires a price parameter for trigger limit orders");
        }
        request["price"] = this->priceToPrecision(symbol, (*params)["price"].get<double>());
    }

    Json response = this->privatePostOrders(request);
    return this->parseOrder(response);
}

Json btcmarkets::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    this->loadMarkets();
    Json request = Json::object({
        {"orderId", id}
    });
    return this->privateDeleteOrders(request);
}

Json btcmarkets::cancelOrdersImpl(const std::vector<std::string>& ids, const std::optional<std::string>& symbol) {
    this->loadMarkets();
    Json request = Json::object({
        {"orderIds", ids}
    });
    return this->privateDeleteBatchOrders(request);
}

Json btcmarkets::fetchBalanceImpl() const {
    this->loadMarkets();
    Json response = this->privateGetAccounts();
    return this->parseBalance(response);
}

Json btcmarkets::withdrawImpl(const std::string& code, double amount, const std::string& address,
                             const std::optional<std::string>& tag) {
    this->checkAddress(address);
    this->loadMarkets();
    Json currency = this->currency(code);
    
    Json request = Json::object({
        {"assetName", currency["id"]},
        {"amount", this->currencyToPrecision(code, amount)},
        {"toAddress", address}
    });

    if (tag) {
        request["paymentDetail"] = *tag;
    }

    Json response = this->privatePostWithdrawals(request);
    return this->parseTransaction(response, currency);
}

Json btcmarkets::parseTicker(const Json& ticker, const Json& market) const {
    long long timestamp = this->safeTimestamp(ticker, "timestamp");
    std::string symbol = this->safeString(market, "symbol");
    return Json::object({
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"high", this->safeString(ticker, "high24h")},
        {"low", this->safeString(ticker, "low24h")},
        {"bid", this->safeString(ticker, "bestBid")},
        {"ask", this->safeString(ticker, "bestAsk")},
        {"last", this->safeString(ticker, "lastPrice")},
        {"volume", this->safeString(ticker, "volume24h")},
        {"info", ticker}
    });
}

Json btcmarkets::parseOrder(const Json& order, const Json& market) const {
    std::string id = this->safeString(order, "orderId");
    std::string timestamp = this->safeString(order, "creationTime");
    std::string status = this->parseOrderStatus(this->safeString(order, "status"));
    std::string symbol = this->safeString(market, "symbol");
    std::string type = this->safeStringLower(order, "type");
    std::string side = this->safeStringLower(order, "side");
    
    return Json::object({
        {"id", id},
        {"timestamp", this->parse8601(timestamp)},
        {"datetime", timestamp},
        {"status", status},
        {"symbol", symbol},
        {"type", type},
        {"side", side},
        {"price", this->safeNumber(order, "price")},
        {"amount", this->safeNumber(order, "amount")},
        {"filled", this->safeNumber(order, "filledAmount")},
        {"remaining", this->safeNumber(order, "openAmount")},
        {"info", order}
    });
}

Json btcmarkets::parseTransaction(const Json& transaction, const Json& currency) const {
    std::string id = this->safeString(transaction, "id");
    long long timestamp = this->safeTimestamp(transaction, "creationTime");
    std::string status = this->parseTransactionStatus(this->safeString(transaction, "status"));
    
    return Json::object({
        {"id", id},
        {"info", transaction},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"currency", currency["code"]},
        {"amount", this->safeNumber(transaction, "amount")},
        {"address", this->safeString(transaction, "toAddress")},
        {"tag", this->safeString(transaction, "paymentDetail")},
        {"status", status},
        {"fee", this->safeNumber(transaction, "fee")}
    });
}

Json btcmarkets::parseOHLCV(const Json& ohlcv, const Json& market) const {
    return Json::array({
        this->parse8601(this->safeString(ohlcv, "timestamp")),
        this->safeNumber(ohlcv, "open"),
        this->safeNumber(ohlcv, "high"),
        this->safeNumber(ohlcv, "low"),
        this->safeNumber(ohlcv, "close"),
        this->safeNumber(ohlcv, "volume")
    });
}

} // namespace ccxt
