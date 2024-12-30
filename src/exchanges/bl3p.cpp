#include "ccxt/exchanges/bl3p.h"
#include "ccxt/base/json_helper.h"
#include <openssl/hmac.h>
#include <sstream>
#include <iomanip>
#include <boost/bind/bind.hpp>

namespace ccxt {

const std::string bl3p::defaultBaseURL = "https://api.bl3p.eu";
const std::string bl3p::defaultVersion = "1";
const int bl3p::defaultRateLimit = 1000;
const bool bl3p::defaultPro = false;


bl3p::bl3p(const Config& config) : Exchange(config) {
    init();
}

void bl3p::init() {
    
    
    // Set exchange properties
    this->id = "bl3p";
    this->name = "BL3P";
    this->countries = {"NL"};  // Netherlands
    this->version = defaultVersion;
    this->rateLimit = defaultRateLimit;
    this->pro = defaultPro;
    
    if (this->urls.empty()) {
        this->urls = {
            {"logo", "https://github.com/user-attachments/assets/75aeb14e-cd48-43c8-8492-dff002dea0be"},
            {"api", {
                {"rest", defaultBaseURL}
            }},
            {"www", "https://bl3p.eu"},
            {"doc", {
                "https://github.com/BitonicNL/bl3p-api/tree/master/docs",
                "https://bl3p.eu/api",
                "https://bitonic.nl/en/api"
            }}
        };
    }

    // Set capabilities
    this->has = {
        {"CORS", std::nullopt},
        {"spot", true},
        {"margin", false},
        {"swap", false},
        {"future", false},
        {"option", false},
        {"cancelOrder", true},
        {"createOrder", true},
        {"createDepositAddress", true},
        {"fetchBalance", true},
        {"fetchOrderBook", true},
        {"fetchTicker", true},
        {"fetchTrades", true},
        {"fetchTradingFees", true}
    };
}

Json bl3p::describeImpl() const {
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

std::string bl3p::sign(const std::string& path, const std::string& api, const std::string& method,
                      const Json& params, const Json& headers, const Json& body) const {
    std::string url = this->urls["api"]["rest"] + "/" + path;
    
    if (api == "public") {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    } else {
        this->checkRequiredCredentials();
        
        std::string nonce = this->nonce().str();
        std::string auth_body = "";

        if (!params.empty()) {
            auth_body = this->urlencode(this->keysort(params));
        }

        std::string signature = this->hmac(auth_body, this->config_.secret, "SHA256", "hex");
        
        headers["Rest-Key"] = this->config_.apiKey;
        headers["Rest-Sign"] = signature;
    }

    return url;
}

Json bl3p::fetchTickerImpl(const std::string& symbol) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json response = this->publicGetMarketTicker(Json::object({
        {"market", market["id"]}
    }));
    return this->parseTicker(response, market);
}

Json bl3p::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json request = Json::object({
        {"market", this->marketId(symbol)}
    });
    Json response = this->publicGetMarketOrderbook(request);
    return this->parseOrderBook(response, symbol);
}

Json bl3p::fetchTradesImpl(const std::string& symbol, const std::optional<long long>& since,
                          const std::optional<int>& limit) const {
    this->loadMarkets();
    Json request = Json::object({
        {"market", this->marketId(symbol)}
    });
    Json response = this->publicGetMarketTrades(request);
    return this->parseTrades(response["trades"], symbol, since, limit);
}

Json bl3p::fetchTradingFeesImpl() const {
    Json response = this->publicGetFees();
    return this->parseFees(response);
}

Json bl3p::createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side,
                         double amount, const std::optional<double>& price) {
    this->loadMarkets();
    Json market = this->market(symbol);
    
    Json request = Json::object({
        {"market", market["id"]},
        {"type", side},
        {"amount_int", this->amountToPrecision(symbol, amount)},
        {"fee_currency", "EUR"}
    });

    if (type == "limit") {
        if (!price) {
            throw ArgumentsRequired("createOrder() requires a price argument for limit orders");
        }
        request["price_int"] = this->priceToPrecision(symbol, *price);
    }

    Json response = this->privatePostMarketOrder(request);
    return this->parseOrder(response);
}

Json bl3p::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    this->loadMarkets();
    Json request = Json::object({
        {"market", this->marketId(symbol)},
        {"order_id", id}
    });
    return this->privatePostMarketOrderCancel(request);
}

Json bl3p::createDepositAddressImpl(const std::string& code, const std::optional<std::string>& network) {
    this->loadMarkets();
    Json currency = this->currency(code);
    Json request = Json::object({
        {"currency", currency["id"]}
    });
    Json response = this->privatePostGenerateNewAddress(request);
    return this->parseDepositAddress(response);
}

Json bl3p::fetchBalanceImpl() const {
    this->loadMarkets();
    Json response = this->privateGetBalance();
    return this->parseBalance(response);
}

Json bl3p::parseTicker(const Json& ticker, const Json& market) const {
    long long timestamp = this->milliseconds();
    std::string symbol = this->safeString(market, "symbol");
    return Json::object({
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"high", this->safeString(ticker, "high"),},
        {"low", this->safeString(ticker, "low")},
        {"bid", this->safeString(ticker, "bid")},
        {"ask", this->safeString(ticker, "ask")},
        {"last", this->safeString(ticker, "last")},
        {"volume", this->safeString(ticker, "volume")},
        {"info", ticker}
    });
}

Json bl3p::parseTrade(const Json& trade, const Json& market) const {
    long long timestamp = this->safeTimestamp(trade, "date");
    std::string id = this->safeString(trade, "trade_id");
    std::string price = this->safeString(trade, "price_int");
    std::string amount = this->safeString(trade, "amount_int");
    std::string cost = Precise::stringMul(price, amount);
    
    return Json::object({
        {"id", id},
        {"info", trade},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"symbol", market["symbol"]},
        {"type", nullptr},
        {"side", this->safeString(trade, "type")},
        {"price", this->parseNumber(price)},
        {"amount", this->parseNumber(amount)},
        {"cost", this->parseNumber(cost)}
    });
}

Json bl3p::parseOrder(const Json& order, const Json& market) const {
    std::string id = this->safeString(order, "order_id");
    long long timestamp = this->safeTimestamp(order, "date");
    std::string status = "open";
    
    if (this->safeString(order, "status") == "closed") {
        status = "closed";
    }
    
    return Json::object({
        {"id", id},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"status", status},
        {"symbol", market["symbol"]},
        {"type", this->safeString(order, "type")},
        {"side", this->safeString(order, "type")},
        {"price", this->safeNumber(order, "price")},
        {"amount", this->safeNumber(order, "amount")},
        {"filled", this->safeNumber(order, "amount_executed")},
        {"remaining", this->safeNumber(order, "amount_remaining")},
        {"info", order}
    });
}

Json bl3p::parseFees(const Json& response) const {
    return Json::object({
        {"trading", {
            {"maker", this->safeNumber(response, "maker_fee")},
            {"taker", this->safeNumber(response, "taker_fee")}
        }},
        {"info", response}
    });
}

// Async Market Data implementations
boost::future<Json> bl3p::fetchTickerAsync(const std::string& symbol) const {
    return boost::async(boost::launch::async, [this, symbol]() {
        return this->fetchTickerImpl(symbol);
    });
}

boost::future<Json> bl3p::fetchOrderBookAsync(const std::string& symbol, const std::optional<int>& limit) const {
    return boost::async(boost::launch::async, [this, symbol, limit]() {
        return this->fetchOrderBookImpl(symbol, limit);
    });
}

boost::future<Json> bl3p::fetchTradesAsync(const std::string& symbol, const std::optional<long long>& since,
                                         const std::optional<int>& limit) const {
    return boost::async(boost::launch::async, [this, symbol, since, limit]() {
        return this->fetchTradesImpl(symbol, since, limit);
    });
}

boost::future<Json> bl3p::fetchTradingFeesAsync() const {
    return boost::async(boost::launch::async, [this]() {
        return this->fetchTradingFeesImpl();
    });
}

// Async Trading implementations
boost::future<Json> bl3p::createOrderAsync(const std::string& symbol, const std::string& type,
                                         const std::string& side, double amount,
                                         const std::optional<double>& price) {
    return boost::async(boost::launch::async, [this, symbol, type, side, amount, price]() {
        return this->createOrderImpl(symbol, type, side, amount, price);
    });
}

boost::future<Json> bl3p::cancelOrderAsync(const std::string& id, const std::string& symbol) {
    return boost::async(boost::launch::async, [this, id, symbol]() {
        return this->cancelOrderImpl(id, symbol);
    });
}

boost::future<Json> bl3p::createDepositAddressAsync(const std::string& code,
                                                   const std::optional<std::string>& network) {
    return boost::async(boost::launch::async, [this, code, network]() {
        return this->createDepositAddressImpl(code, network);
    });
}

// Async Account implementations
boost::future<Json> bl3p::fetchBalanceAsync() const {
    return boost::async(boost::launch::async, [this]() {
        return this->fetchBalanceImpl();
    });
}

} // namespace ccxt
