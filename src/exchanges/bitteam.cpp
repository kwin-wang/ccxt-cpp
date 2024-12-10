#include "ccxt/exchanges/bitteam.h"
#include "ccxt/base/json_helper.h"
#include <openssl/hmac.h>
#include <sstream>
#include <iomanip>

namespace ccxt {

const std::string bitteam::defaultBaseURL = "https://bit.team/api";
const std::string bitteam::defaultVersion = "v2.0.6";
const int bitteam::defaultRateLimit = 1;  // no rate limit
const bool bitteam::defaultPro = false;

ExchangeRegistry::Factory bitteam::factory("bitteam", bitteam::createInstance);

bitteam::bitteam(const Config& config) : ExchangeImpl(config) {
    init();
}

void bitteam::init() {
    ExchangeImpl::init();
    
    // Set exchange properties
    this->id = "bitteam";
    this->name = "BIT.TEAM";
    this->countries = {"UK"};
    this->version = defaultVersion;
    this->rateLimit = defaultRateLimit;
    this->pro = defaultPro;
    
    if (this->urls.empty()) {
        this->urls = {
            {"logo", "https://user-images.githubusercontent.com/1294454/156902113-2c1b91a5-726e-4f11-a04e-1c5d8fb9a58a.jpg"},
            {"api", {
                {"rest", defaultBaseURL}
            }},
            {"www", "https://bit.team"},
            {"doc", {"https://bit.team/api-docs"}}
        };
    }

    this->has = {
        {"CORS", std::nullopt},
        {"spot", true},
        {"margin", false},
        {"swap", false},
        {"future", false},
        {"option", false},
        {"cancelAllOrders", true},
        {"cancelOrder", true},
        {"createOrder", true},
        {"fetchBalance", true},
        {"fetchCanceledOrders", true},
        {"fetchClosedOrders", true},
        {"fetchCurrencies", true},
        {"fetchDepositsWithdrawals", true},
        {"fetchMarkets", true},
        {"fetchMyTrades", true},
        {"fetchOHLCV", true},
        {"fetchOpenOrders", true},
        {"fetchOrder", true},
        {"fetchOrderBook", true},
        {"fetchOrders", true},
        {"fetchTicker", true},
        {"fetchTrades", true}
    };
}

Json bitteam::describeImpl() const {
    return Json::object({
        {"id", this->id},
        {"name", this->name},
        {"countries", this->countries},
        {"version", this->version},
        {"rateLimit", this->rateLimit},
        {"pro", this->pro},
        {"has", this->has}
    });
}

std::string bitteam::sign(const std::string& path, const std::string& api, const std::string& method,
                         const Json& params, const Json& headers, const Json& body) const {
    std::string url = this->urls["api"]["rest"] + "/" + this->version + "/" + path;
    
    if (api == "public") {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    } else {
        this->checkRequiredCredentials();
        
        long long timestamp = this->milliseconds();
        std::string nonce = std::to_string(timestamp);
        
        std::string auth = nonce + this->apiKey;
        std::string signature = this->hmac(auth, this->secret, "SHA256", "hex");
        
        headers["Authorization"] = "Bearer " + signature;
        headers["Request-Time"] = nonce;
        headers["Request-Hash"] = this->apiKey;
    }

    return url;
}

void bitteam::handleErrors(const std::string& code, const std::string& reason, const std::string& url, const std::string& method,
                          const Json& headers, const Json& body, const Json& response, const std::string& requestHeaders,
                          const std::string& requestBody) const {
    if (response.is_object() && response.contains("success") && !response["success"].get<bool>()) {
        std::string message = this->safeString(response, "message", "Unknown error");
        std::string errorCode = this->safeString(response, "code", "");
        
        if (errorCode == "4001") {
            throw AuthenticationError(message);
        } else if (errorCode == "4004") {
            throw OrderNotFound(message);
        } else if (errorCode == "4006") {
            throw InsufficientFunds(message);
        } else if (errorCode == "4007") {
            throw BadSymbol(message);
        } else if (errorCode == "4008") {
            throw BadRequest(message);
        } else if (errorCode == "5001") {
            throw ExchangeNotAvailable(message);
        }
        
        throw ExchangeError(message);
    }
}

Json bitteam::fetchMarketsImpl() const {
    Json response = this->publicGetMarkets();
    return this->parseMarkets(response["data"]);
}

Json bitteam::fetchCurrenciesImpl() const {
    Json response = this->publicGetCurrencies();
    return this->parseCurrencies(response["data"]);
}

Json bitteam::fetchTickerImpl(const std::string& symbol) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json response = this->publicGetTicker(Json::object({
        {"market", market["id"]}
    }));
    return this->parseTicker(response["data"], market);
}

Json bitteam::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json request = Json::object({
        {"market", this->marketId(symbol)}
    });
    if (limit) {
        request["limit"] = *limit;
    }
    Json response = this->publicGetOrderbook(request);
    return this->parseOrderBook(response["data"], symbol);
}

Json bitteam::fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe,
                            const std::optional<long long>& since, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json request = Json::object({
        {"market", this->marketId(symbol)},
        {"interval", this->timeframes[timeframe]}
    });
    if (since) {
        request["start"] = *since;
    }
    if (limit) {
        request["limit"] = *limit;
    }
    Json response = this->publicGetKlines(request);
    return this->parseOHLCVs(response["data"], market, timeframe, since, limit);
}

Json bitteam::createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side,
                            double amount, const std::optional<double>& price) {
    this->loadMarkets();
    Json market = this->market(symbol);
    
    Json request = Json::object({
        {"market", market["id"]},
        {"side", side},
        {"type", type},
        {"amount", this->amountToPrecision(symbol, amount)}
    });

    if (type == "limit") {
        if (!price) {
            throw ArgumentsRequired("createOrder() requires a price argument for limit orders");
        }
        request["price"] = this->priceToPrecision(symbol, *price);
    }

    Json response = this->privatePostOrders(request);
    return this->parseOrder(response["data"]);
}

Json bitteam::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    this->loadMarkets();
    Json request = Json::object({
        {"orderId", id}
    });
    return this->privateDeleteOrders(request);
}

Json bitteam::cancelAllOrdersImpl(const std::optional<std::string>& symbol) {
    this->loadMarkets();
    Json request = Json::object();
    if (symbol) {
        Json market = this->market(*symbol);
        request["market"] = market["id"];
    }
    return this->privateDeleteOrdersAll(request);
}

Json bitteam::fetchOrderImpl(const std::string& id, const std::string& symbol) const {
    this->loadMarkets();
    Json request = Json::object({
        {"orderId", id}
    });
    Json response = this->privateGetOrders(request);
    return this->parseOrder(response["data"]);
}

Json bitteam::fetchBalanceImpl() const {
    this->loadMarkets();
    Json response = this->privateGetBalance();
    return this->parseBalance(response["data"]);
}

Json bitteam::fetchDepositsWithdrawalsImpl(const std::optional<std::string>& code,
                                          const std::optional<long long>& since,
                                          const std::optional<int>& limit) const {
    this->loadMarkets();
    Json request = Json::object();
    if (code) {
        Json currency = this->currency(*code);
        request["currency"] = currency["id"];
    }
    if (since) {
        request["start"] = *since;
    }
    if (limit) {
        request["limit"] = *limit;
    }
    Json response = this->privateGetHistory(request);
    return this->parseTransactions(response["data"]);
}

Json bitteam::parseTicker(const Json& ticker, const Json& market) const {
    long long timestamp = this->milliseconds();
    std::string symbol = this->safeString(market, "symbol");
    return Json::object({
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"high", this->safeString(ticker, "high")},
        {"low", this->safeString(ticker, "low")},
        {"bid", this->safeString(ticker, "bid")},
        {"ask", this->safeString(ticker, "ask")},
        {"last", this->safeString(ticker, "last")},
        {"baseVolume", this->safeString(ticker, "baseVolume")},
        {"quoteVolume", this->safeString(ticker, "quoteVolume")},
        {"info", ticker}
    });
}

Json bitteam::parseTrade(const Json& trade, const Json& market) const {
    long long timestamp = this->safeTimestamp(trade, "time");
    std::string id = this->safeString(trade, "id");
    std::string side = this->safeString(trade, "side");
    std::string price = this->safeString(trade, "price");
    std::string amount = this->safeString(trade, "amount");
    
    return Json::object({
        {"id", id},
        {"info", trade},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"symbol", market["symbol"]},
        {"type", nullptr},
        {"side", side},
        {"price", this->parseNumber(price)},
        {"amount", this->parseNumber(amount)}
    });
}

Json bitteam::parseOrder(const Json& order, const Json& market) const {
    std::string id = this->safeString(order, "id");
    long long timestamp = this->safeTimestamp(order, "time");
    std::string status = this->parseOrderStatus(this->safeString(order, "status"));
    std::string symbol = this->safeString(market, "symbol");
    std::string type = this->safeString(order, "type");
    std::string side = this->safeString(order, "side");
    
    return Json::object({
        {"id", id},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"status", status},
        {"symbol", symbol},
        {"type", type},
        {"side", side},
        {"price", this->safeNumber(order, "price")},
        {"amount", this->safeNumber(order, "amount")},
        {"filled", this->safeNumber(order, "filled")},
        {"remaining", this->safeNumber(order, "remaining")},
        {"info", order}
    });
}

Json bitteam::parseTransaction(const Json& transaction, const Json& currency) const {
    std::string id = this->safeString(transaction, "id");
    long long timestamp = this->safeTimestamp(transaction, "time");
    std::string type = this->safeString(transaction, "type");
    std::string status = this->parseTransactionStatus(this->safeString(transaction, "status"));
    
    return Json::object({
        {"id", id},
        {"info", transaction},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"currency", currency["code"]},
        {"type", type},
        {"status", status},
        {"amount", this->safeNumber(transaction, "amount")},
        {"fee", this->safeNumber(transaction, "fee")}
    });
}

} // namespace ccxt
