#include "ccxt/exchanges/mixcoin.h"
#include <chrono>
#include <thread>

namespace ccxt {

MixCoin::MixCoin(const Config& config) : Exchange(config) {
    // Exchange specific configurations
    this->id = "mixcoin";
    this->name = "MixCoin";
    this->countries = {"GB", "HK"};
    this->version = "v1";
    this->rateLimit = 1000;
    this->has = {
        {"fetchMarkets", true},
        {"fetchTicker", true},
        {"fetchTickers", true},
        {"fetchOrderBook", true},
        {"fetchTrades", true},
        {"fetchOHLCV", true},
        {"fetchBalance", true},
        {"createOrder", true},
        {"cancelOrder", true},
        {"fetchOrder", true},
        {"fetchOrders", true},
        {"fetchOpenOrders", true},
        {"fetchClosedOrders", true},
        {"fetchMyTrades", true},
        {"fetchAccounts", true},
        {"fetchLedger", true},
        {"fetchTradingFee", true}
    };
    this->urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/30237212-ed29303c-9535-11e7-8af8-fcd381cfa20c.jpg"},
        {"api", {
            {"public", "https://www.mixcoin.com/api/v1/public"},
            {"private", "https://www.mixcoin.com/api/v1/private"}
        }},
        {"www", "https://www.mixcoin.com"},
        {"doc", {
            "https://mixcoin.com/help/api",
            "https://mixcoin.com/help/api/http"
        }}
    };
    this->api = {
        {"public", {
            {"GET", {
                "markets",
                "ticker/{symbol}",
                "tickers",
                "depth/{symbol}",
                "trades/{symbol}",
                "kline/{symbol}"
            }}
        }},
        {"private", {
            {"GET", {
                "accounts",
                "orders/{symbol}",
                "order/{id}",
                "trades/{symbol}",
                "ledger/{currency}",
                "trading_fee/{symbol}"
            }},
            {"POST", {
                "order",
                "order/cancel"
            }}
        }}
    };
}

// Market Data API Implementation
json MixCoin::fetchMarkets(const json& params) {
    auto response = fetch("/markets", "public", "GET", params);
    auto markets = json::array();
    for (const auto& market : response) {
        markets.push_back({
            {"id", market["id"]},
            {"symbol", market["symbol"]},
            {"base", market["base_currency"]},
            {"quote", market["quote_currency"]},
            {"baseId", market["base_currency"]},
            {"quoteId", market["quote_currency"]},
            {"active", market["active"]},
            {"precision", {
                {"amount", market["amount_precision"]},
                {"price", market["price_precision"]}
            }},
            {"limits", {
                {"amount", {
                    {"min", market["min_amount"]},
                    {"max", market["max_amount"]}
                }},
                {"price", {
                    {"min", market["min_price"]},
                    {"max", market["max_price"]}
                }}
            }},
            {"info", market}
        });
    }
    return markets;
}

json MixCoin::fetchTicker(const std::string& symbol, const json& params) {
    auto market = loadMarket(symbol);
    auto response = fetch("/ticker/" + market["id"], "public", "GET", params);
    return parseTicker(response, market);
}

json MixCoin::fetchTickers(const std::vector<std::string>& symbols, const json& params) {
    auto response = fetch("/tickers", "public", "GET", params);
    auto result = json::object();
    for (const auto& ticker : response) {
        auto market = loadMarket(ticker["symbol"]);
        result[market["symbol"]] = parseTicker(ticker, market);
    }
    return result;
}

json MixCoin::fetchOrderBook(const std::string& symbol, int limit, const json& params) {
    auto market = loadMarket(symbol);
    auto request = params;
    if (limit) {
        request["limit"] = limit;
    }
    auto response = fetch("/depth/" + market["id"], "public", "GET", request);
    return {
        {"bids", response["bids"]},
        {"asks", response["asks"]},
        {"timestamp", response["timestamp"]},
        {"datetime", iso8601(response["timestamp"])}
    };
}

json MixCoin::fetchTrades(const std::string& symbol, int since, int limit, const json& params) {
    auto market = loadMarket(symbol);
    auto request = params;
    if (since) {
        request["since"] = since;
    }
    if (limit) {
        request["limit"] = limit;
    }
    auto response = fetch("/trades/" + market["id"], "public", "GET", request);
    return parseTrades(response, market, since, limit);
}

json MixCoin::fetchOHLCV(const std::string& symbol, const std::string& timeframe, int since, int limit, const json& params) {
    auto market = loadMarket(symbol);
    auto request = params;
    request["period"] = timeframe;
    if (since) {
        request["since"] = since;
    }
    if (limit) {
        request["limit"] = limit;
    }
    auto response = fetch("/kline/" + market["id"], "public", "GET", request);
    return parseOHLCV(response, market, timeframe, since, limit);
}

// Trading API Implementation
json MixCoin::fetchBalance(const json& params) {
    checkRequiredCredentials();
    auto response = fetch("/accounts", "private", "GET", params);
    auto result = {"info", response};
    for (const auto& balance : response) {
        auto currencyId = balance["currency"];
        auto code = getCurrencyId(currencyId);
        result[code] = {
            {"free", balance["available"]},
            {"used", balance["frozen"]},
            {"total", balance["balance"]}
        };
    }
    return result;
}

json MixCoin::createOrder(const std::string& symbol, const std::string& type, const std::string& side,
                         double amount, double price, const json& params) {
    checkRequiredCredentials();
    auto market = loadMarket(symbol);
    auto request = {
        {"market", market["id"]},
        {"side", side},
        {"volume", amount},
        {"ord_type", type}
    };
    if (type == "limit") {
        request["price"] = price;
    }
    auto response = fetch("/order", "private", "POST", extend(request, params));
    return parseOrder(response);
}

json MixCoin::cancelOrder(const std::string& id, const std::string& symbol, const json& params) {
    checkRequiredCredentials();
    auto request = {{"id", id}};
    return fetch("/order/cancel", "private", "POST", extend(request, params));
}

json MixCoin::fetchOrder(const std::string& id, const std::string& symbol, const json& params) {
    checkRequiredCredentials();
    auto market = loadMarket(symbol);
    auto response = fetch("/order/" + id, "private", "GET", params);
    return parseOrder(response, market);
}

json MixCoin::fetchOrders(const std::string& symbol, int since, int limit, const json& params) {
    checkRequiredCredentials();
    auto market = loadMarket(symbol);
    auto request = params;
    if (since) {
        request["since"] = since;
    }
    if (limit) {
        request["limit"] = limit;
    }
    auto response = fetch("/orders/" + market["id"], "private", "GET", request);
    return parseOrders(response, market, since, limit);
}

json MixCoin::fetchOpenOrders(const std::string& symbol, int since, int limit, const json& params) {
    auto request = extend(params, {{"state", "wait"}});
    return fetchOrders(symbol, since, limit, request);
}

json MixCoin::fetchClosedOrders(const std::string& symbol, int since, int limit, const json& params) {
    auto request = extend(params, {{"state", "done"}});
    return fetchOrders(symbol, since, limit, request);
}

json MixCoin::fetchMyTrades(const std::string& symbol, int since, int limit, const json& params) {
    checkRequiredCredentials();
    auto market = loadMarket(symbol);
    auto request = params;
    if (since) {
        request["since"] = since;
    }
    if (limit) {
        request["limit"] = limit;
    }
    auto response = fetch("/trades/" + market["id"], "private", "GET", request);
    return parseTrades(response, market, since, limit);
}

// Account API Implementation
json MixCoin::fetchAccounts(const json& params) {
    checkRequiredCredentials();
    auto response = fetch("/accounts", "private", "GET", params);
    return response["accounts"];
}

json MixCoin::fetchLedger(const std::string& code, int since, int limit, const json& params) {
    checkRequiredCredentials();
    auto currency = loadCurrency(code);
    auto request = params;
    if (since) {
        request["since"] = since;
    }
    if (limit) {
        request["limit"] = limit;
    }
    auto response = fetch("/ledger/" + currency["id"], "private", "GET", request);
    return parseLedger(response["transactions"], currency, since, limit);
}

json MixCoin::fetchTradingFee(const std::string& symbol, const json& params) {
    checkRequiredCredentials();
    auto market = loadMarket(symbol);
    auto response = fetch("/trading_fee/" + market["id"], "private", "GET", params);
    return parseTradingFee(response, market);
}

// Parse Methods Implementation
json MixCoin::parseTicker(const json& ticker, const Market& market) {
    auto timestamp = safeInteger(ticker, "timestamp");
    return {
        {"symbol", market["symbol"]},
        {"timestamp", timestamp},
        {"datetime", iso8601(timestamp)},
        {"high", safeFloat(ticker, "high")},
        {"low", safeFloat(ticker, "low")},
        {"bid", safeFloat(ticker, "buy")},
        {"ask", safeFloat(ticker, "sell")},
        {"last", safeFloat(ticker, "last")},
        {"close", safeFloat(ticker, "last")},
        {"baseVolume", safeFloat(ticker, "vol")},
        {"info", ticker}
    };
}

json MixCoin::parseTrade(const json& trade, const Market& market) {
    auto timestamp = safeInteger(trade, "date");
    return {
        {"id", safeString(trade, "tid")},
        {"timestamp", timestamp},
        {"datetime", iso8601(timestamp)},
        {"symbol", market["symbol"]},
        {"type", undefined},
        {"side", safeString(trade, "type")},
        {"price", safeFloat(trade, "price")},
        {"amount", safeFloat(trade, "amount")},
        {"info", trade}
    };
}

json MixCoin::parseOrder(const json& order, const Market& market) {
    auto status = safeString(order, "state");
    if (status == "wait") {
        status = "open";
    } else if (status == "done") {
        status = "closed";
    } else if (status == "cancel") {
        status = "canceled";
    }
    auto timestamp = safeInteger(order, "created_at");
    return {
        {"id", safeString(order, "id")},
        {"timestamp", timestamp},
        {"datetime", iso8601(timestamp)},
        {"lastTradeTimestamp", undefined},
        {"status", status},
        {"symbol", market["symbol"]},
        {"type", safeString(order, "ord_type")},
        {"side", safeString(order, "side")},
        {"price", safeFloat(order, "price")},
        {"amount", safeFloat(order, "volume")},
        {"filled", safeFloat(order, "executed_volume")},
        {"remaining", safeFloat(order, "remaining_volume")},
        {"trades", undefined},
        {"fee", undefined},
        {"info", order}
    };
}

json MixCoin::parseLedgerEntry(const json& item, const Currency& currency) {
    auto timestamp = safeInteger(item, "created_at");
    auto direction = safeFloat(item, "amount") >= 0 ? "in" : "out";
    return {
        {"id", safeString(item, "id")},
        {"timestamp", timestamp},
        {"datetime", iso8601(timestamp)},
        {"direction", direction},
        {"account", safeString(item, "account_id")},
        {"referenceId", safeString(item, "ref_id")},
        {"type", safeString(item, "type")},
        {"currency", currency["code"]},
        {"amount", std::abs(safeFloat(item, "amount"))},
        {"status", "ok"},
        {"info", item}
    };
}

json MixCoin::parseTradingFee(const json& fee, const Market& market) {
    return {
        {"info", fee},
        {"symbol", market["symbol"]},
        {"maker", safeFloat(fee, "maker_fee")},
        {"taker", safeFloat(fee, "taker_fee")}
    };
}

std::string MixCoin::getAccountId(const std::string& type, const std::string& currency) {
    auto accounts = fetchAccounts();
    for (const auto& account : accounts) {
        if (account["type"] == type && account["currency"] == currency) {
            return account["id"];
        }
    }
    throw ExchangeError("Account not found");
}

// Sign Implementation
std::string MixCoin::sign(const std::string& path, const std::string& api,
                     const std::string& method, const json& params,
                     const std::map<std::string, std::string>& headers,
                     const json& body) {
    auto endpoint = "/" + this->version + "/" + path;
    auto url = this->urls["api"][api] + endpoint;

    if (api == "public") {
        if (params.size()) {
            url += "?" + this->urlencode(params);
        }
    } else {
        checkRequiredCredentials();
        auto nonce = std::to_string(this->nonce());
        auto auth = nonce + method + endpoint;
        if (method == "GET") {
            if (params.size()) {
                url += "?" + this->urlencode(params);
                auth += "?" + this->urlencode(params);
            }
        } else {
            if (params.size()) {
                body = params;
                auth += this->json(params);
            }
        }
        auto signature = this->hmac(auth, this->config_.secret);
        auto newHeaders = headers;
        newHeaders["ACCESS-KEY"] = this->config_.apiKey;
        newHeaders["ACCESS-TIMESTAMP"] = nonce;
        newHeaders["ACCESS-SIGN"] = signature;
        if (body.size()) {
            newHeaders["Content-Type"] = "application/json";
        }
        return {url, newHeaders, body};
    }
    return {url, headers, body};
}

// Async Methods Implementation
#define IMPLEMENT_ASYNC_METHOD(name, ...) \
    AsyncPullType MixCoin::async##name(__VA_ARGS__) { \
        return std::async(std::launch::async, [this](auto... params) { \
            return this->name(params...); \
        }, __VA_ARGS__); \
    }

IMPLEMENT_ASYNC_METHOD(FetchMarkets, const json& params)
IMPLEMENT_ASYNC_METHOD(FetchTicker, const std::string& symbol, const json& params)
IMPLEMENT_ASYNC_METHOD(FetchTickers, const std::vector<std::string>& symbols, const json& params)
IMPLEMENT_ASYNC_METHOD(FetchOrderBook, const std::string& symbol, int limit, const json& params)
IMPLEMENT_ASYNC_METHOD(FetchTrades, const std::string& symbol, int since, int limit, const json& params)
IMPLEMENT_ASYNC_METHOD(FetchOHLCV, const std::string& symbol, const std::string& timeframe, int since, int limit, const json& params)
IMPLEMENT_ASYNC_METHOD(FetchBalance, const json& params)
IMPLEMENT_ASYNC_METHOD(CreateOrder, const std::string& symbol, const std::string& type, const std::string& side, double amount, double price, const json& params)
IMPLEMENT_ASYNC_METHOD(CancelOrder, const std::string& id, const std::string& symbol, const json& params)
IMPLEMENT_ASYNC_METHOD(FetchOrder, const std::string& id, const std::string& symbol, const json& params)
IMPLEMENT_ASYNC_METHOD(FetchOrders, const std::string& symbol, int since, int limit, const json& params)
IMPLEMENT_ASYNC_METHOD(FetchOpenOrders, const std::string& symbol, int since, int limit, const json& params)
IMPLEMENT_ASYNC_METHOD(FetchClosedOrders, const std::string& symbol, int since, int limit, const json& params)
IMPLEMENT_ASYNC_METHOD(FetchMyTrades, const std::string& symbol, int since, int limit, const json& params)
IMPLEMENT_ASYNC_METHOD(FetchAccounts, const json& params)
IMPLEMENT_ASYNC_METHOD(FetchLedger, const std::string& code, int since, int limit, const json& params)
IMPLEMENT_ASYNC_METHOD(FetchTradingFee, const std::string& symbol, const json& params)

} // namespace ccxt
