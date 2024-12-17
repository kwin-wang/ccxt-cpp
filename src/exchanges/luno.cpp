#include "ccxt/exchanges/luno.h"
#include <chrono>
#include <sstream>
#include <iomanip>

namespace ccxt {

Luno::Luno() {
    id = "luno";
    name = "Luno";
    countries = {"GB", "SG", "ZA"};
    rateLimit = 200;
    version = "1";
    certified = true;
    pro = true;
    has = {
        {"CORS", nullptr},
        {"spot", true},
        {"margin", false},
        {"swap", false},
        {"future", false},
        {"option", false},
        {"cancelOrder", true},
        {"createOrder", true},
        {"fetchAccounts", true},
        {"fetchBalance", true},
        {"fetchClosedOrders", true},
        {"fetchLedger", true},
        {"fetchMarkets", true},
        {"fetchMyTrades", true},
        {"fetchOHLCV", true},
        {"fetchOpenOrders", true},
        {"fetchOrder", true},
        {"fetchOrderBook", true},
        {"fetchOrders", true},
        {"fetchTicker", true},
        {"fetchTickers", true},
        {"fetchTrades", true},
        {"fetchTradingFee", true},
        {"fetchTradingFees", false}
    };

    urls = {
        {"referral", "https://www.luno.com/invite/44893A"},
        {"logo", "https://user-images.githubusercontent.com/1294454/27766607-8c1a69d8-5ede-11e7-930c-540b5eb9be24.jpg"},
        {"api", {
            {"public", "https://api.luno.com/api"},
            {"private", "https://api.luno.com/api"},
            {"exchange", "https://api.luno.com/api/exchange"},
            {"exchangePrivate", "https://api.luno.com/api/exchange"}
        }},
        {"www", "https://www.luno.com"},
        {"doc", {
            "https://www.luno.com/en/api",
            "https://npmjs.org/package/bitx",
            "https://github.com/bausmeier/node-bitx"
        }}
    };

    api = {
        {"exchange", {
            {"GET", {
                "markets",
                "tickers",
                "orderbook",
                "trades",
                "candles",
                "accounts",
                "pending",
                "orders",
                "order",
                "trades/market",
                "trades/all"
            }},
            {"POST", {
                "postorder",
                "marketorder",
                "stoporder",
                "order",
                "orders/cancel"
            }}
        }},
        {"exchangePrivate", {
            {"GET", {
                "accounts/{id}/pending",
                "accounts/{id}/transactions",
                "balance",
                "fee_info",
                "funding_address",
                "listorders",
                "listtrades",
                "orders/{id}",
                "quotes/{id}",
                "withdrawals",
                "withdrawals/{id}"
            }},
            {"POST", {
                "accounts",
                "postorder",
                "marketorder",
                "stoporder",
                "funding_address",
                "withdrawals",
                "send",
                "quotes",
                "buy",
                "sell"
            }},
            {"PUT", {
                "quotes/{id}"
            }},
            {"DELETE", {
                "orders/{id}",
                "quotes/{id}"
            }}
        }}
    };

    timeframes = {
        {"1m", "60"},
        {"5m", "300"},
        {"15m", "900"},
        {"30m", "1800"},
        {"1h", "3600"},
        {"4h", "14400"},
        {"1d", "86400"}
    };
}

// Market Data API Implementation
json Luno::fetchMarkets(const json& params) {
    auto response = fetch("/exchange/markets", "exchange", "GET", params);
    auto markets = json::array();
    for (const auto& market : response["markets"]) {
        markets.push_back({
            {"id", market["market_id"]},
            {"symbol", market["trading_pair"]},
            {"base", market["base_currency"]},
            {"quote", market["counter_currency"]},
            {"baseId", market["base_currency"]},
            {"quoteId", market["counter_currency"]},
            {"active", true},
            {"info", market}
        });
    }
    return markets;
}

json Luno::fetchTicker(const String& symbol, const json& params) {
    auto market = loadMarket(symbol);
    auto request = {{"pair", market["id"]}};
    auto response = fetch("/exchange/ticker", "exchange", "GET", extend(request, params));
    return parseTicker(response, market);
}

json Luno::fetchTickers(const std::vector<String>& symbols, const json& params) {
    auto response = fetch("/exchange/tickers", "exchange", "GET", params);
    auto result = json::object();
    for (const auto& ticker : response["tickers"]) {
        auto marketId = ticker["market"].get<String>();
        if (markets_by_id.find(marketId) != markets_by_id.end()) {
            auto market = markets_by_id[marketId];
            auto symbol = market["symbol"].get<String>();
            if (symbols.empty() || std::find(symbols.begin(), symbols.end(), symbol) != symbols.end()) {
                result[symbol] = parseTicker(ticker, market);
            }
        }
    }
    return result;
}

json Luno::fetchOrderBook(const String& symbol, int limit, const json& params) {
    auto market = loadMarket(symbol);
    auto request = {{"pair", market["id"]}};
    if (limit) {
        request["limit"] = limit;
    }
    auto response = fetch("/exchange/orderbook", "exchange", "GET", extend(request, params));
    return parseOrderBook(response, market["symbol"], undefined, "bids", "asks", "price", "volume");
}

json Luno::fetchTrades(const String& symbol, int since, int limit, const json& params) {
    auto market = loadMarket(symbol);
    auto request = {{"pair", market["id"]}};
    if (since) {
        request["since"] = since;
    }
    if (limit) {
        request["limit"] = limit;
    }
    auto response = fetch("/exchange/trades", "exchange", "GET", extend(request, params));
    return parseTrades(response["trades"], market, since, limit);
}

json Luno::fetchOHLCV(const String& symbol, const String& timeframe, int since, int limit, const json& params) {
    auto market = loadMarket(symbol);
    auto request = {
        {"pair", market["id"]},
        {"seconds", timeframes[timeframe]}
    };
    if (since) {
        request["since"] = since;
    }
    if (limit) {
        request["limit"] = limit;
    }
    auto response = fetch("/exchange/candles", "exchange", "GET", extend(request, params));
    return parseOHLCVs(response["candles"], market, timeframe, since, limit);
}

// Async Market Data API Implementation
AsyncPullType Luno::asyncFetchMarkets(const json& params) {
    return std::async(std::launch::async, [this, params]() {
        return fetchMarkets(params);
    });
}

AsyncPullType Luno::asyncFetchTicker(const String& symbol, const json& params) {
    return std::async(std::launch::async, [this, symbol, params]() {
        return fetchTicker(symbol, params);
    });
}

AsyncPullType Luno::asyncFetchTickers(const std::vector<String>& symbols, const json& params) {
    return std::async(std::launch::async, [this, symbols, params]() {
        return fetchTickers(symbols, params);
    });
}

AsyncPullType Luno::asyncFetchOrderBook(const String& symbol, int limit, const json& params) {
    return std::async(std::launch::async, [this, symbol, limit, params]() {
        return fetchOrderBook(symbol, limit, params);
    });
}

AsyncPullType Luno::asyncFetchTrades(const String& symbol, int since, int limit, const json& params) {
    return std::async(std::launch::async, [this, symbol, since, limit, params]() {
        return fetchTrades(symbol, since, limit, params);
    });
}

AsyncPullType Luno::asyncFetchOHLCV(const String& symbol, const String& timeframe, int since, int limit, const json& params) {
    return std::async(std::launch::async, [this, symbol, timeframe, since, limit, params]() {
        return fetchOHLCV(symbol, timeframe, since, limit, params);
    });
}

nlohmann::json Luno::fetch_markets() {
    auto response = this->fetch("exchange/markets", "exchange", "GET");
    return this->parse_markets(response["markets"]);
}

nlohmann::json Luno::fetch_balance() {
    this->check_required_credentials();
    auto response = this->fetch("exchange/balance", "exchangePrivate", "GET");
    return this->parse_balance(response);
}

nlohmann::json Luno::fetch_order_book(const std::string& symbol, int limit) {
    auto market = this->market(symbol);
    auto request = {
        {"pair", market["id"].get<std::string>()}
    };
    auto response = this->fetch("exchange/orderbook", "exchange", "GET", request);
    return this->parse_order_book(response, market["symbol"].get<std::string>());
}

nlohmann::json Luno::fetch_ticker(const std::string& symbol) {
    auto market = this->market(symbol);
    auto request = {
        {"pair", market["id"].get<std::string>()}
    };
    auto response = this->fetch("exchange/ticker", "exchange", "GET", request);
    return this->parse_ticker(response, market);
}

nlohmann::json Luno::create_order(const std::string& symbol, const std::string& type,
                                 const std::string& side, double amount, double price) {
    this->check_required_credentials();
    auto market = this->market(symbol);
    auto method = (type == "market") ? "exchange/marketorder" : "exchange/postorder";
    
    auto request = {
        {"pair", market["id"].get<std::string>()},
        {"volume", this->amount_to_precision(symbol, amount)}
    };

    if (type != "market") {
        request["price"] = this->price_to_precision(symbol, price);
    }

    if (side == "buy") {
        request["type"] = "BID";
    } else {
        request["type"] = "ASK";
    }

    auto response = this->fetch(method, "exchangePrivate", "POST", request);
    return this->parse_order(response);
}

nlohmann::json Luno::cancel_order(const std::string& id, const std::string& symbol) {
    this->check_required_credentials();
    return this->fetch("exchange/orders/" + id, "exchangePrivate", "DELETE");
}

std::string Luno::sign(const std::string& path, const std::string& api,
                      const std::string& method, const nlohmann::json& params,
                      const std::map<std::string, std::string>& headers) {
    auto url = this->urls["api"][api].get<std::string>() + "/" + this->version + "/" + this->implode_params(path, params);
    auto query = this->omit(params, this->extract_params(path));

    if (api == "public") {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->check_required_credentials();
        auto auth = this->apiKey + ":" + this->secret;
        auto auth_base64 = this->encode(auth);
        auto new_headers = headers;
        new_headers["Authorization"] = "Basic " + auth_base64;

        if (method == "GET") {
            if (!query.empty()) {
                url += "?" + this->urlencode(query);
            }
        } else {
            new_headers["Content-Type"] = "application/x-www-form-urlencoded";
        }
    }

    return url;
}

std::string Luno::get_currency_id(const std::string& code) {
    if (code == "BTC") return "XBT";
    return code;
}

// Trading API Implementation
json Luno::fetchBalance(const json& params) {
    checkRequiredCredentials();
    auto response = fetch("/exchange/balance", "exchangePrivate", "GET", params);
    return parseBalance(response);
}

json Luno::createOrder(const String& symbol, const String& type, const String& side,
                      double amount, double price, const json& params) {
    checkRequiredCredentials();
    auto market = loadMarket(symbol);
    auto method = (type == "market") ? "exchange/marketorder" : "exchange/postorder";
    
    auto request = {
        {"pair", market["id"]},
        {"volume", amount}
    };

    if (type == "limit") {
        request["price"] = price;
    }

    if (side == "buy") {
        request["type"] = "BID";
    } else {
        request["type"] = "ASK";
    }

    auto response = fetch(method, "exchangePrivate", "POST", extend(request, params));
    return parseOrder(response);
}

json Luno::cancelOrder(const String& id, const String& symbol, const json& params) {
    checkRequiredCredentials();
    return fetch("/exchange/orders/" + id, "exchangePrivate", "DELETE", params);
}

json Luno::fetchOrder(const String& id, const String& symbol, const json& params) {
    checkRequiredCredentials();
    auto request = {{"id", id}};
    auto response = fetch("/exchange/order", "exchangePrivate", "GET", extend(request, params));
    return parseOrder(response);
}

json Luno::fetchOrders(const String& symbol, int since, int limit, const json& params) {
    checkRequiredCredentials();
    auto market = loadMarket(symbol);
    auto request = {{"pair", market["id"]}};
    if (since) {
        request["since"] = since;
    }
    if (limit) {
        request["limit"] = limit;
    }
    auto response = fetch("/exchange/orders", "exchangePrivate", "GET", extend(request, params));
    return parseOrders(response["orders"], market, since, limit);
}

json Luno::fetchOpenOrders(const String& symbol, int since, int limit, const json& params) {
    auto request = params;
    request["state"] = "PENDING";
    return fetchOrders(symbol, since, limit, request);
}

json Luno::fetchClosedOrders(const String& symbol, int since, int limit, const json& params) {
    auto request = params;
    request["state"] = "COMPLETE";
    return fetchOrders(symbol, since, limit, request);
}

json Luno::fetchMyTrades(const String& symbol, int since, int limit, const json& params) {
    checkRequiredCredentials();
    auto market = loadMarket(symbol);
    auto request = {{"pair", market["id"]}};
    if (since) {
        request["since"] = since;
    }
    if (limit) {
        request["limit"] = limit;
    }
    auto response = fetch("/exchange/trades/all", "exchangePrivate", "GET", extend(request, params));
    return parseTrades(response["trades"], market, since, limit);
}

// Account API Implementation
json Luno::fetchAccounts(const json& params) {
    checkRequiredCredentials();
    auto response = fetch("/exchange/accounts", "exchangePrivate", "GET", params);
    return response["accounts"];
}

json Luno::fetchLedger(const String& code, int since, int limit, const json& params) {
    checkRequiredCredentials();
    auto currency = loadCurrency(code);
    auto accountId = getAccountId("spot", currency["id"]);
    auto request = {{"id", accountId}};
    if (since) {
        request["min_row"] = since;
    }
    if (limit) {
        request["max_row"] = limit;
    }
    auto response = fetch("/exchange/accounts/" + accountId + "/transactions", "exchangePrivate", "GET", extend(request, params));
    return parseLedger(response["transactions"], currency, since, limit);
}

json Luno::fetchTradingFee(const String& symbol, const json& params) {
    checkRequiredCredentials();
    auto market = loadMarket(symbol);
    auto response = fetch("/exchange/fee_info", "exchangePrivate", "GET", params);
    return parseTradingFee(response, market);
}

// Parse Methods Implementation
json Luno::parseTicker(const json& ticker, const Market& market) {
    auto timestamp = safeInteger(ticker, "timestamp");
    return {
        {"symbol", market["symbol"]},
        {"timestamp", timestamp},
        {"datetime", iso8601(timestamp)},
        {"high", safeFloat(ticker, "high")},
        {"low", safeFloat(ticker, "low")},
        {"bid", safeFloat(ticker, "bid")},
        {"bidVolume", safeFloat(ticker, "bid_volume")},
        {"ask", safeFloat(ticker, "ask")},
        {"askVolume", safeFloat(ticker, "ask_volume")},
        {"vwap", safeFloat(ticker, "vwap")},
        {"open", safeFloat(ticker, "open")},
        {"close", safeFloat(ticker, "close")},
        {"last", safeFloat(ticker, "last_trade")},
        {"previousClose", undefined},
        {"change", undefined},
        {"percentage", undefined},
        {"average", undefined},
        {"baseVolume", safeFloat(ticker, "rolling_24_hour_volume")},
        {"quoteVolume", undefined},
        {"info", ticker}
    };
}

json Luno::parseTrade(const json& trade, const Market& market) {
    auto side = safeString(trade, "type");
    if (side == "BID") {
        side = "buy";
    } else if (side == "ASK") {
        side = "sell";
    }
    auto timestamp = safeInteger(trade, "timestamp");
    return {
        {"id", safeString(trade, "order_id")},
        {"info", trade},
        {"timestamp", timestamp},
        {"datetime", iso8601(timestamp)},
        {"symbol", market["symbol"]},
        {"order", safeString(trade, "order_id")},
        {"type", undefined},
        {"side", side},
        {"takerOrMaker", undefined},
        {"price", safeFloat(trade, "price")},
        {"amount", safeFloat(trade, "volume")},
        {"cost", undefined},
        {"fee", undefined}
    };
}

json Luno::parseOrder(const json& order, const Market& market) {
    auto status = safeString(order, "state");
    if (status == "PENDING") {
        status = "open";
    } else if (status == "COMPLETE") {
        status = "closed";
    }
    auto side = safeString(order, "type");
    if (side == "BID") {
        side = "buy";
    } else if (side == "ASK") {
        side = "sell";
    }
    auto timestamp = safeInteger(order, "creation_timestamp");
    auto price = safeFloat(order, "limit_price");
    auto amount = safeFloat(order, "limit_volume");
    auto filled = safeFloat(order, "base");
    auto cost = safeFloat(order, "counter");
    return {
        {"id", safeString(order, "order_id")},
        {"info", order},
        {"timestamp", timestamp},
        {"datetime", iso8601(timestamp)},
        {"lastTradeTimestamp", undefined},
        {"status", status},
        {"symbol", market["symbol"]},
        {"type", price ? "limit" : "market"},
        {"side", side},
        {"price", price},
        {"cost", cost},
        {"amount", amount},
        {"filled", filled},
        {"remaining", amount ? (amount - filled) : undefined},
        {"fee", undefined}
    };
}

json Luno::parseLedgerEntry(const json& item, const Currency& currency) {
    auto timestamp = safeInteger(item, "timestamp");
    auto direction = safeFloat(item, "balance_delta") >= 0 ? "in" : "out";
    return {
        {"id", safeString(item, "row_index")},
        {"info", item},
        {"timestamp", timestamp},
        {"datetime", iso8601(timestamp)},
        {"direction", direction},
        {"account", safeString(item, "account_id")},
        {"referenceId", safeString(item, "tx_id")},
        {"referenceAccount", undefined},
        {"type", safeString(item, "type").toLower()},
        {"currency", currency["code"]},
        {"amount", std::abs(safeFloat(item, "balance_delta"))},
        {"before", safeFloat(item, "balance_before")},
        {"after", safeFloat(item, "balance_after")},
        {"status", "ok"},
        {"fee", undefined}
    };
}

json Luno::parseTradingFee(const json& fee, const Market& market) {
    return {
        {"info", fee},
        {"symbol", market["symbol"]},
        {"maker", safeFloat(fee, "maker_fee")},
        {"taker", safeFloat(fee, "taker_fee")}
    };
}

String Luno::getAccountId(const String& type, const String& currency) {
    auto accounts = fetchAccounts();
    for (const auto& account : accounts) {
        if (account["type"] == type && account["currency"] == currency) {
            return account["id"];
        }
    }
    throw ExchangeError("Account not found");
}

// Async Account API Implementation
AsyncPullType Luno::asyncFetchAccounts(const json& params) {
    return std::async(std::launch::async, [this, params]() {
        return fetchAccounts(params);
    });
}

AsyncPullType Luno::asyncFetchLedger(const String& code, int since, int limit, const json& params) {
    return std::async(std::launch::async, [this, code, since, limit, params]() {
        return fetchLedger(code, since, limit, params);
    });
}

AsyncPullType Luno::asyncFetchTradingFee(const String& symbol, const json& params) {
    return std::async(std::launch::async, [this, symbol, params]() {
        return fetchTradingFee(symbol, params);
    });
}

// Async Trading API Implementation
AsyncPullType Luno::asyncFetchBalance(const json& params) {
    return std::async(std::launch::async, [this, params]() {
        return fetchBalance(params);
    });
}

AsyncPullType Luno::asyncCreateOrder(const String& symbol, const String& type, const String& side,
                                       double amount, double price, const json& params) {
    return std::async(std::launch::async, [this, symbol, type, side, amount, price, params]() {
        return createOrder(symbol, type, side, amount, price, params);
    });
}

AsyncPullType Luno::asyncCancelOrder(const String& id, const String& symbol, const json& params) {
    return std::async(std::launch::async, [this, id, symbol, params]() {
        return cancelOrder(id, symbol, params);
    });
}

AsyncPullType Luno::asyncFetchOrder(const String& id, const String& symbol, const json& params) {
    return std::async(std::launch::async, [this, id, symbol, params]() {
        return fetchOrder(id, symbol, params);
    });
}

AsyncPullType Luno::asyncFetchOrders(const String& symbol, int since, int limit, const json& params) {
    return std::async(std::launch::async, [this, symbol, since, limit, params]() {
        return fetchOrders(symbol, since, limit, params);
    });
}

AsyncPullType Luno::asyncFetchOpenOrders(const String& symbol, int since, int limit, const json& params) {
    return std::async(std::launch::async, [this, symbol, since, limit, params]() {
        return fetchOpenOrders(symbol, since, limit, params);
    });
}

AsyncPullType Luno::asyncFetchClosedOrders(const String& symbol, int since, int limit, const json& params) {
    return std::async(std::launch::async, [this, symbol, since, limit, params]() {
        return fetchClosedOrders(symbol, since, limit, params);
    });
}

AsyncPullType Luno::asyncFetchMyTrades(const String& symbol, int since, int limit, const json& params) {
    return std::async(std::launch::async, [this, symbol, since, limit, params]() {
        return fetchMyTrades(symbol, since, limit, params);
    });
}

} // namespace ccxt
