#include "ccxt/exchanges/bitteam.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

namespace ccxt {

const std::string bitteam::defaultBaseURL = "https://bit.team/api";
const std::string bitteam::defaultVersion = "v2.0.6";
const int bitteam::defaultRateLimit = 1;  // no rate limit
const bool bitteam::defaultPro = false;

bitteam::bitteam(const Config& config)
    : Exchange(config)
    , io_context()
    , work_guard(boost::asio::make_work_guard(io_context))
    , io_thread([this]() { io_context.run(); }) {
    init();
}

bitteam::~bitteam() {
    work_guard.reset();
    if (io_thread.joinable()) {
        io_thread.join();
    }
}

void bitteam::init() {
    id = "bitteam";
    name = "BIT.TEAM";
    countries = {"UK"};
    version = defaultVersion;
    rateLimit = defaultRateLimit;
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/156902113-2c1b91a5-726e-4f11-a04e-1c5d8fb9a58a.jpg"},
        {"api", {
            {"rest", defaultBaseURL}
        }},
        {"www", "https://bit.team"},
        {"doc", {"https://bit.team/api-docs"}}
    };

    api = {
        {"public", {
            {"GET", {
                "market/pairs",
                "market/currencies",
                "market/ticker/{symbol}",
                "market/depth/{symbol}",
                "market/history/{symbol}",
                "market/kline/{symbol}"
            }}
        }},
        {"private", {
            {"POST", {
                "account/balances",
                "order/new",
                "order/cancel",
                "order/cancel-all",
                "order/status",
                "order/active",
                "order/history",
                "trade/history"
            }}
        }}
    };
}

// Synchronous REST API Implementation
Json bitteam::fetchMarkets(const json& params) {
    auto response = this->publicGetMarketPairs(params);
    return this->parseMarkets(response);
}

Json bitteam::fetchTicker(const std::string& symbol, const json& params) {
    auto market = this->market(symbol);
    auto request = this->extend(params, {
        "symbol": market["id"]
    });
    auto response = this->publicGetMarketTickerSymbol(request);
    auto ticker = this->parseTicker(response, market);
    ticker["symbol"] = symbol;
    return ticker;
}

Json bitteam::fetchOrderBook(const std::string& symbol, int limit, const json& params) {
    auto market = this->market(symbol);
    auto request = this->extend(params, {
        "symbol": market["id"]
    });
    if (limit) {
        request["limit"] = limit;
    }
    auto response = this->publicGetMarketDepthSymbol(request);
    return this->parseOrderBook(response, symbol);
}

Json bitteam::fetchTrades(const std::string& symbol, int since, int limit, const json& params) {
    auto market = this->market(symbol);
    auto request = this->extend(params, {
        "symbol": market["id"]
    });
    if (limit) {
        request["limit"] = limit;
    }
    auto response = this->publicGetMarketHistorySymbol(request);
    return this->parseTrades(response, market, since, limit);
}

Json bitteam::fetchBalance(const json& params) {
    auto response = this->privatePostAccountBalances(params);
    return this->parseBalance(response);
}

Json bitteam::createOrder(const std::string& symbol, const std::string& type, const std::string& side,
                         double amount, double price, const json& params) {
    auto market = this->market(symbol);
    auto request = {
        "symbol": market["id"],
        "side": side.toLowerCase(),
        "type": type.toLowerCase(),
        "amount": this->amountToPrecision(symbol, amount)
    };
    if (type == "limit") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    auto response = this->privatePostOrderNew(this->extend(request, params));
    return this->parseOrder(response, market);
}

// Asynchronous REST API Implementation
boost::future<Json> bitteam::fetchMarketsAsync(const json& params) {
    return boost::async([this, params]() {
        return this->fetchMarkets(params);
    });
}

boost::future<Json> bitteam::fetchTickerAsync(const std::string& symbol, const json& params) {
    return boost::async([this, symbol, params]() {
        return this->fetchTicker(symbol, params);
    });
}

boost::future<Json> bitteam::fetchOrderBookAsync(const std::string& symbol, int limit, const json& params) {
    return boost::async([this, symbol, limit, params]() {
        return this->fetchOrderBook(symbol, limit, params);
    });
}

boost::future<Json> bitteam::fetchTradesAsync(const std::string& symbol, int since, int limit, const json& params) {
    return boost::async([this, symbol, since, limit, params]() {
        return this->fetchTrades(symbol, since, limit, params);
    });
}

boost::future<Json> bitteam::fetchBalanceAsync(const json& params) {
    return boost::async([this, params]() {
        return this->fetchBalance(params);
    });
}

boost::future<Json> bitteam::createOrderAsync(const std::string& symbol, const std::string& type,
                                            const std::string& side, double amount,
                                            double price, const json& params) {
    return boost::async([this, symbol, type, side, amount, price, params]() {
        return this->createOrder(symbol, type, side, amount, price, params);
    });
}

// Helper methods
std::string bitteam::sign(const std::string& path, const std::string& api, const std::string& method,
                    const json& params, const std::map<std::string, std::string>& headers,
                    const json& body) {
    auto url = this->urls["api"]["rest"] + "/" + this->version + "/" + path;
    
    if (api == "public") {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    } else {
        this->checkRequiredCredentials();
        auto nonce = this->nonce();
        auto auth = nonce + this->config_.apiKey;
        auto signature = this->hmac(auth, this->config_.secret, "sha256");
        
        json request = this->extend({
            "key": this->config_.apiKey,
            "signature": signature,
            "nonce": nonce
        }, params);
        
        body = this->json(request);
        headers["Content-Type"] = "application/json";
    }
    
    return url;
}

void bitteam::handleErrors(const std::string& httpCode, const std::string& reason, const std::string& url,
                          const std::string& method, const json& headers, const json& body,
                          const json& response, const json& requestHeaders,
                          const json& requestBody) {
    if (!response.contains("success")) {
        return;
    }
    
    auto success = this->safeBool(response, "success");
    if (success) {
        return;
    }
    
    auto message = this->safeString(response, "message", "Unknown error");
    auto feedback = this->id + " " + message;
    
    if (message.find("Invalid signature") != std::string::npos) {
        throw AuthenticationError(feedback);
    } else if (message.find("Insufficient funds") != std::string::npos) {
        throw InsufficientFunds(feedback);
    } else if (message.find("Order not found") != std::string::npos) {
        throw OrderNotFound(feedback);
    }
    
    throw ExchangeError(feedback);
}

Json bitteam::parseTicker(const Json& ticker, const Json& market) {
    auto timestamp = this->safeTimestamp(ticker, "timestamp");
    auto symbol = market ? this->safeString(market, "symbol") : undefined;
    
    return {
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"high", this->safeString(ticker, "high")},
        {"low", this->safeString(ticker, "low")},
        {"bid", this->safeString(ticker, "bid")},
        {"ask", this->safeString(ticker, "ask")},
        {"last", this->safeString(ticker, "last")},
        {"close", this->safeString(ticker, "close")},
        {"baseVolume", this->safeString(ticker, "baseVolume")},
        {"quoteVolume", this->safeString(ticker, "quoteVolume")},
        {"info", ticker}
    };
}

Json bitteam::parseOrder(const Json& order, const Json& market) {
    auto id = this->safeString(order, "id");
    auto timestamp = this->safeTimestamp(order, "timestamp");
    auto symbol = market ? this->safeString(market, "symbol") : undefined;
    auto type = this->safeStringLower(order, "type");
    auto side = this->safeStringLower(order, "side");
    
    return {
        {"id", id},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"symbol", symbol},
        {"type", type},
        {"side", side},
        {"price", this->safeString(order, "price")},
        {"amount", this->safeString(order, "amount")},
        {"cost", this->safeString(order, "cost")},
        {"filled", this->safeString(order, "filled")},
        {"remaining", this->safeString(order, "remaining")},
        {"status", this->parseOrderStatus(this->safeString(order, "status"))},
        {"info", order}
    };
}

} // namespace ccxt
