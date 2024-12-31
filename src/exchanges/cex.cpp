#include "ccxt/exchanges/cex.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <openssl/hmac.h>

namespace ccxt {

cex::cex(const Config& config) : Exchange(config) {
    this->describe();
}

void cex::describe() {
    this->id = "cex";
    this->name = "CEX.IO";
    this->countries = {"GB", "EU", "CY", "RU"};
    this->rateLimit = 300;  // 200 req/min
    this->pro = true;
    
    this->has = {
        {"CORS", nullptr},
        {"spot", true},
        {"margin", false},  // has, but not through api
        {"swap", false},
        {"future", false},
        {"option", false},
        {"cancelAllOrders", true},
        {"cancelOrder", true},
        {"createOrder", true},
        {"createStopOrder", true},
        {"createTriggerOrder", true},
        {"fetchAccounts", true},
        {"fetchBalance", true},
        {"fetchClosedOrder", true},
        {"fetchClosedOrders", true},
        {"fetchCurrencies", true},
        {"fetchDepositAddress", true},
        {"fetchDepositsWithdrawals", true},
        {"fetchLedger", true},
        {"fetchMarkets", true},
        {"fetchOHLCV", true},
        {"fetchOpenOrder", true},
        {"fetchOpenOrders", true},
        {"fetchOrderBook", true},
        {"fetchTicker", true},
        {"fetchTickers", true},
        {"fetchTime", true},
        {"fetchTrades", true},
        {"fetchTradingFees", true},
        {"transfer", true}
    };

    this->urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/27766442-8ddc33b0-5ed8-11e7-8b98-f786aef0f3c9.jpg"},
        {"api", {
            {"public", "https://cex.io/api"},
            {"private", "https://cex.io/api"}
        }},
        {"www", "https://cex.io"},
        {"doc", {
            "https://cex.io/cex-api",
            "https://cex.io/websocket-api"
        }},
        {"fees", "https://cex.io/fee-schedule"}
    };

    this->api = {
        {"public", {
            {"get", {
                "currency_profile",
                "currency_limits",
                "ticker/{symbol}",
                "tickers/{symbols}",
                "order_book/{symbol}",
                "trade_history/{symbol}",
                "ohlcv/hd/{yyyymmdd}/{symbol}",
                "last_price/{symbol}",
                "last_prices/{symbols}",
                "convert/{pair}",
                "price_stats/{symbol}"
            }}
        }},
        {"private", {
            {"post", {
                "place_order/{symbol}",
                "cancel_order",
                "cancel_orders/{symbol}",
                "open_orders/{symbol}",
                "open_orders",
                "active_orders_status",
                "archived_orders/{symbol}",
                "get_order",
                "get_order_tx",
                "get_address",
                "get_myfee",
                "balance/",
                "open_position/{symbol}",
                "close_position/{symbol}",
                "get_position",
                "get_positions",
                "get_marginal_fee",
                "cancel_replace_order/{symbol}"
            }}
        }}
    };

    this->fees = {
        {"trading", {
            {"maker", 0.16 / 100},  // 0.16%
            {"taker", 0.25 / 100}   // 0.25%
        }}
    };
}

json cex::fetchMarkets(const json& params) {
    json response = this->fetch("/currency_profile", "public", "GET", params);
    json result = json::array();
    
    for (const auto& market : response["data"]["pairs"]) {
        json entry = {
            {"id", market["symbol1"] + "/" + market["symbol2"]},
            {"symbol", market["symbol1"] + "/" + market["symbol2"]},
            {"base", market["symbol1"]},
            {"quote", market["symbol2"]},
            {"baseId", market["symbol1"]},
            {"quoteId", market["symbol2"]},
            {"active", true},
            {"precision", {
                {"amount", market["scale1"]},
                {"price", market["scale2"]}
            }},
            {"limits", {
                {"amount", {
                    {"min", market["minLotSize"]},
                    {"max", market["maxLotSize"]}
                }},
                {"price", {
                    {"min", market["minPrice"]},
                    {"max", market["maxPrice"]}
                }},
                {"cost", {
                    {"min", market["minLotSizeS2"]},
                    {"max", market["maxLotSizeS2"]}
                }}
            }},
            {"info", market}
        };
        result.push_back(entry);
    }
    return result;
}

json cex::fetchTicker(const std::string& symbol, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {
        {"symbol", market["id"]}
    };
    json response = this->fetch("/ticker/" + market["id"], "public", "GET", this->extend(request, params));
    return this->parseTicker(response, market);
}

json cex::fetchOrderBook(const std::string& symbol, int limit, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {
        {"symbol", market["id"]}
    };
    if (limit != 0) {
        request["depth"] = limit;
    }
    json response = this->fetch("/order_book/" + market["id"], "public", "GET", this->extend(request, params));
    return this->parseOrderBook(response, market["symbol"]);
}

json cex::createOrder(const std::string& symbol, const std::string& type, const std::string& side,
                     double amount, double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"type", type},
        {"side", side},
        {"amount", this->amountToPrecision(symbol, amount)}
    };
    
    if (type == "limit") {
        if (price == 0) {
            throw ExchangeError("createOrder requires price for limit orders");
        }
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    std::string endpoint = "/place_order/" + market["id"];
    json response = this->fetch(endpoint, "private", "POST", this->extend(request, params));
    return this->parseOrder(response, market);
}

json cex::cancelOrder(const std::string& id, const std::string& symbol, const json& params) {
    json request = {
        {"id", id}
    };
    if (!symbol.empty()) {
        this->loadMarkets();
        Market market = this->market(symbol);
        request["symbol"] = market["id"];
    }
    return this->fetch("/cancel_order", "private", "POST", this->extend(request, params));
}

json cex::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = this->fetch("/balance/", "private", "POST", params);
    json result = {
        {"info", response}
    };
    
    for (const auto& balance : response.items()) {
        std::string currencyId = balance.key();
        json account = this->account();
        
        if (balance.value().contains("available")) {
            account["free"] = this->safeString(balance.value(), "available");
        }
        if (balance.value().contains("orders")) {
            account["used"] = this->safeString(balance.value(), "orders");
        }
        
        std::string code = this->safeCurrencyCode(currencyId);
        result[code] = account;
    }
    
    return this->parseBalance(result);
}

json cex::parseTicker(const json& ticker, const json& market) {
    long timestamp = this->safeTimestamp(ticker, "timestamp");
    std::string symbol = this->safeString(market, "symbol");
    
    return {
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"high", this->safeString(ticker, "high")},
        {"low", this->safeString(ticker, "low")},
        {"bid", this->safeString(ticker, "bid")},
        {"bidVolume", nullptr},
        {"ask", this->safeString(ticker, "ask")},
        {"askVolume", nullptr},
        {"vwap", nullptr},
        {"open", this->safeString(ticker, "open")},
        {"close", this->safeString(ticker, "last")},
        {"last", this->safeString(ticker, "last")},
        {"previousClose", nullptr},
        {"change", nullptr},
        {"percentage", nullptr},
        {"average", nullptr},
        {"baseVolume", this->safeString(ticker, "volume")},
        {"quoteVolume", this->safeString(ticker, "volumeQuote")},
        {"info", ticker}
    };
}

std::string cex::sign(const std::string& path, const std::string& api, const std::string& method,
                const json& params, const json& headers, const json& body) {
    std::string url = this->urls["api"][api] + "/" + this->implodeParams(path, params);
    json query = this->omit(params, this->extractParams(path));
    
    if (api == "public") {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->checkRequiredCredentials();
        long nonce = this->nonce();
        std::string auth = std::to_string(nonce) + this->config_.apiKey;
        std::string signature = this->hmac(auth, this->config_.secret, "sha256", "hex");
        
        json request = this->extend({
            {"key", this->config_.apiKey},
            {"signature", signature},
            {"nonce", nonce}
        }, query);
        
        body = this->json(request);
        headers["Content-Type"] = "application/json";
    }
    
    return url;
}

// Async Market Data Functions
boost::future<Json> cex::fetchMarketsAsync(const Json& params) const {
    return boost::async(boost::launch::async, [this, params]() {
        return this->fetchMarkets(params);
    });
}

boost::future<Json> cex::fetchTickerAsync(const std::string& symbol, const Json& params) const {
    return boost::async(boost::launch::async, [this, symbol, params]() {
        return this->fetchTicker(symbol, params);
    });
}

boost::future<Json> cex::fetchTickersAsync(const std::vector<std::string>& symbols, const Json& params) const {
    return boost::async(boost::launch::async, [this, symbols, params]() {
        return this->fetchTickers(symbols, params);
    });
}

boost::future<Json> cex::fetchOrderBookAsync(const std::string& symbol, const std::optional<int>& limit, const Json& params) const {
    return boost::async(boost::launch::async, [this, symbol, limit, params]() {
        return this->fetchOrderBook(symbol, limit, params);
    });
}

boost::future<Json> cex::fetchTradesAsync(const std::string& symbol, const std::optional<long long>& since, 
                                        const std::optional<int>& limit, const Json& params) const {
    return boost::async(boost::launch::async, [this, symbol, since, limit, params]() {
        return this->fetchTrades(symbol, since, limit, params);
    });
}

boost::future<Json> cex::fetchOHLCVAsync(const std::string& symbol, const std::string& timeframe,
                                       const std::optional<long long>& since, const std::optional<int>& limit,
                                       const Json& params) const {
    return boost::async(boost::launch::async, [this, symbol, timeframe, since, limit, params]() {
        return this->fetchOHLCV(symbol, timeframe, since, limit, params);
    });
}

// Async Trading Functions
boost::future<Json> cex::createOrderAsync(const std::string& symbol, const std::string& type, const std::string& side,
                                       double amount, const std::optional<double>& price, const Json& params) {
    return boost::async(boost::launch::async, [this, symbol, type, side, amount, price, params]() {
        return this->createOrder(symbol, type, side, amount, price, params);
    });
}

boost::future<Json> cex::cancelOrderAsync(const std::string& id, const std::string& symbol, const Json& params) {
    return boost::async(boost::launch::async, [this, id, symbol, params]() {
        return this->cancelOrder(id, symbol, params);
    });
}

boost::future<Json> cex::fetchOrderAsync(const std::string& id, const std::string& symbol, const Json& params) const {
    return boost::async(boost::launch::async, [this, id, symbol, params]() {
        return this->fetchOrder(id, symbol, params);
    });
}

boost::future<Json> cex::fetchOpenOrdersAsync(const std::optional<std::string>& symbol,
                                           const std::optional<long long>& since,
                                           const std::optional<int>& limit,
                                           const Json& params) const {
    return boost::async(boost::launch::async, [this, symbol, since, limit, params]() {
        return this->fetchOpenOrders(symbol, since, limit, params);
    });
}

boost::future<Json> cex::fetchClosedOrdersAsync(const std::optional<std::string>& symbol,
                                             const std::optional<long long>& since,
                                             const std::optional<int>& limit,
                                             const Json& params) const {
    return boost::async(boost::launch::async, [this, symbol, since, limit, params]() {
        return this->fetchClosedOrders(symbol, since, limit, params);
    });
}

// Async Account Functions
boost::future<Json> cex::fetchBalanceAsync(const Json& params) const {
    return boost::async(boost::launch::async, [this, params]() {
        return this->fetchBalance(params);
    });
}

boost::future<Json> cex::fetchDepositAddressAsync(const std::string& code,
                                               const std::optional<std::string>& network,
                                               const Json& params) const {
    return boost::async(boost::launch::async, [this, code, network, params]() {
        return this->fetchDepositAddress(code, network, params);
    });
}

boost::future<Json> cex::fetchTransactionsAsync(const std::optional<std::string>& code,
                                             const std::optional<long long>& since,
                                             const std::optional<int>& limit,
                                             const Json& params) const {
    return boost::async(boost::launch::async, [this, code, since, limit, params]() {
        return this->fetchTransactions(code, since, limit, params);
    });
}

} // namespace ccxt
