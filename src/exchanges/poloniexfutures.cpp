#include "ccxt/exchanges/poloniexfutures.h"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace ccxt {

const std::string poloniexfutures::defaultBaseURL = "https://futures-api.poloniex.com";
const std::string poloniexfutures::defaultVersion = "v1";
const int poloniexfutures::defaultRateLimit = 33; // 30 requests per second
const bool poloniexfutures::defaultPro = true;

poloniexfutures::poloniexfutures(const Config& config) : Exchange(config) {
    init();
}

void poloniexfutures::init() {
    
    
    id = "poloniexfutures";
    name = "Poloniex Futures";
    countries = {"US"};
    version = defaultVersion;
    rateLimit = defaultRateLimit;
    pro = defaultPro;
    certified = false;
    
    has = {
        {"CORS", nullptr},
        {"spot", false},
        {"margin", true},
        {"swap", true},
        {"future", false},
        {"option", nullptr},
        {"createOrder", true},
        {"createStopOrder", true},
        {"createTriggerOrder", true},
        {"fetchBalance", true},
        {"fetchClosedOrders", true},
        {"fetchCurrencies", false},
        {"fetchDepositAddress", false},
        {"fetchDepositAddresses", false},
        {"fetchDepositAddressesByNetwork", false},
        {"fetchFundingInterval", true},
        {"fetchFundingIntervals", false},
        {"fetchFundingRate", true},
        {"fetchFundingRateHistory", false},
        {"fetchL3OrderBook", true},
        {"fetchMarkets", true},
        {"fetchMyTrades", true},
        {"fetchOHLCV", true},
        {"fetchOpenOrders", true},
        {"fetchOrder", true},
        {"fetchOrderBook", true},
        {"fetchOrdersByStatus", true},
        {"fetchPositions", true},
        {"fetchTicker", true},
        {"fetchTickers", true},
        {"fetchTime", true},
        {"fetchTrades", true},
        {"setMarginMode", true}
    };

    timeframes = {
        {"1m", "1"},
        {"5m", "5"},
        {"15m", "15"},
        {"30m", "30"},
        {"1h", "60"},
        {"2h", "120"},
        {"4h", "480"},
        {"12h", "720"},
        {"1d", "1440"},
        {"1w", "10080"}
    };

    hostname = "poloniex.com";
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/27766817-e9456312-5ee6-11e7-9b3c-b628ca5626a5.jpg"},
        {"api", {
            {"public", "https://futures-api.poloniex.com"},
            {"private", "https://futures-api.poloniex.com"}
        }},
        {"www", "https://www.poloniex.com"},
        {"doc", "https://api-docs.poloniex.com/futures/"},
        {"fees", "https://poloniex.com/fee-schedule"}
    };

    api = {
        {"public", {
            {"get", {
                "contracts/active",
                "contracts/{symbol}",
                "ticker",
                "tickers",
                "level2/snapshot",
                "level2/depth",
                "level3/snapshot",
                "trade/history",
                "kline/query",
                "timestamp"
            }}
        }},
        {"private", {
            {"get", {
                "account-overview",
                "positions",
                "orders",
                "orders/{orderId}",
                "fills",
                "funding-history"
            }},
            {"post", {
                "orders",
                "position/margin/auto-deposit-status",
                "position/risk-limit-level/change",
                "position/margin/deposit-margin"
            }},
            {"delete", {
                "orders/{orderId}",
                "orders",
                "stop-order/{orderId}",
                "stop-orders"
            }}
        }}
    };

    fees = {
        {"trading", {
            {"tierBased", true},
            {"percentage", true},
            {"taker", 0.00075},
            {"maker", 0.0001}
        }}
    };

    requiredCredentials = {
        {"apiKey", true},
        {"secret", true}
    };

    precisionMode = TICK_SIZE;
}

Json poloniexfutures::describeImpl() const {
    return ExchangeImpl::describeImpl();
}

Json poloniexfutures::fetchMarketsImpl() const {
    auto response = request("/contracts/active", "public", "GET");
    return parseMarkets(response);
}

Json poloniexfutures::fetchTimeImpl() const {
    auto response = request("/timestamp", "public", "GET");
    return parseTime(response);
}

Json poloniexfutures::fetchTickerImpl(const std::string& symbol) const {
    auto market = loadMarket(symbol);
    auto request = Json::object();
    request["symbol"] = market["id"];
    
    auto response = request("/ticker", "public", "GET", request);
    return parseTicker(response, market);
}

Json poloniexfutures::fetchTickersImpl(const std::vector<std::string>& symbols) const {
    auto response = request("/tickers", "public", "GET");
    auto tickers = Json::array();
    for (const auto& ticker : response) {
        auto market = loadMarket(ticker["symbol"].get<std::string>());
        tickers.push_back(parseTicker(ticker, market));
    }
    return filterByArray(tickers, "symbol", symbols);
}

Json poloniexfutures::fetchOrderBookImpl(const std::string& symbol,
                                       const std::optional<int>& limit) const {
    auto market = loadMarket(symbol);
    auto request = Json::object();
    request["symbol"] = market["id"];
    if (limit) {
        request["limit"] = *limit;
    }
    
    auto response = request("/level2/snapshot", "public", "GET", request);
    return parseOrderBook(response, symbol);
}

Json poloniexfutures::fetchOHLCVImpl(const std::string& symbol,
                                    const std::string& timeframe,
                                    const std::optional<long long>& since,
                                    const std::optional<int>& limit) const {
    auto market = loadMarket(symbol);
    auto request = Json::object();
    request["symbol"] = market["id"];
    request["type"] = timeframes[timeframe];
    if (since) {
        request["startAt"] = *since;
    }
    if (limit) {
        request["pageSize"] = *limit;
    }
    
    auto response = request("/kline/query", "public", "GET", request);
    return parseOHLCV(response);
}

Json poloniexfutures::fetchTradesImpl(const std::string& symbol,
                                    const std::optional<int>& limit,
                                    const std::optional<long long>& since) const {
    auto market = loadMarket(symbol);
    auto request = Json::object();
    request["symbol"] = market["id"];
    if (limit) {
        request["pageSize"] = *limit;
    }
    if (since) {
        request["startAt"] = *since;
    }
    
    auto response = request("/trade/history", "public", "GET", request);
    return parseTrades(response, market);
}

Json poloniexfutures::createOrderImpl(const std::string& symbol, const std::string& type,
                                    const std::string& side, double amount,
                                    const std::optional<double>& price) {
    auto market = loadMarket(symbol);
    auto request = Json::object({
        {"symbol", market["id"]},
        {"side", side.substr(0, 1).c_str()},  // Convert to uppercase B/S
        {"type", type},
        {"size", amount}
    });

    if (price) {
        request["price"] = *price;
    }

    return request("/orders", "private", "POST", request);
}

Json poloniexfutures::createStopOrderImpl(const std::string& symbol, const std::string& type,
                                        const std::string& side, double amount,
                                        const std::optional<double>& price,
                                        const Json& params) {
    auto market = loadMarket(symbol);
    auto request = Json::object({
        {"symbol", market["id"]},
        {"side", side.substr(0, 1).c_str()},
        {"type", type},
        {"size", amount}
    });

    if (price) {
        request["price"] = *price;
    }

    if (params.contains("stopPrice")) {
        request["stopPrice"] = params["stopPrice"];
    }

    return request("/stop-orders", "private", "POST", request);
}

Json poloniexfutures::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    auto market = loadMarket(symbol);
    auto request = Json::object({
        {"symbol", market["id"]}
    });
    
    return request("/orders/" + id, "private", "DELETE", request);
}

Json poloniexfutures::cancelAllOrdersImpl(const std::string& symbol) {
    auto request = Json::object();
    if (!symbol.empty()) {
        auto market = loadMarket(symbol);
        request["symbol"] = market["id"];
    }
    
    return request("/orders", "private", "DELETE", request);
}

Json poloniexfutures::fetchOrderImpl(const std::string& id, const std::string& symbol) const {
    auto market = loadMarket(symbol);
    auto request = Json::object({
        {"symbol", market["id"]}
    });
    
    auto response = request("/orders/" + id, "private", "GET", request);
    return parseOrder(response, market);
}

Json poloniexfutures::fetchOrdersImpl(const std::string& symbol,
                                    const std::optional<long long>& since,
                                    const std::optional<int>& limit) const {
    auto request = Json::object();
    if (!symbol.empty()) {
        auto market = loadMarket(symbol);
        request["symbol"] = market["id"];
    }
    if (since) {
        request["startAt"] = *since;
    }
    if (limit) {
        request["pageSize"] = *limit;
    }
    
    auto response = request("/orders", "private", "GET", request);
    return parseOrders(response, symbol);
}

Json poloniexfutures::fetchOpenOrdersImpl(const std::string& symbol,
                                        const std::optional<long long>& since,
                                        const std::optional<int>& limit) const {
    auto request = Json::object();
    if (!symbol.empty()) {
        auto market = loadMarket(symbol);
        request["symbol"] = market["id"];
    }
    request["status"] = "active";
    if (since) {
        request["startAt"] = *since;
    }
    if (limit) {
        request["pageSize"] = *limit;
    }
    
    auto response = request("/orders", "private", "GET", request);
    return parseOrders(response, symbol);
}

Json poloniexfutures::fetchClosedOrdersImpl(const std::string& symbol,
                                          const std::optional<long long>& since,
                                          const std::optional<int>& limit) const {
    auto request = Json::object();
    if (!symbol.empty()) {
        auto market = loadMarket(symbol);
        request["symbol"] = market["id"];
    }
    request["status"] = "done";
    if (since) {
        request["startAt"] = *since;
    }
    if (limit) {
        request["pageSize"] = *limit;
    }
    
    auto response = request("/orders", "private", "GET", request);
    return parseOrders(response, symbol);
}

Json poloniexfutures::fetchMyTradesImpl(const std::string& symbol,
                                      const std::optional<long long>& since,
                                      const std::optional<int>& limit) const {
    auto request = Json::object();
    if (!symbol.empty()) {
        auto market = loadMarket(symbol);
        request["symbol"] = market["id"];
    }
    if (since) {
        request["startAt"] = *since;
    }
    if (limit) {
        request["pageSize"] = *limit;
    }
    
    auto response = request("/fills", "private", "GET", request);
    return parseTrades(response, symbol);
}

Json poloniexfutures::parseTicker(const Json& ticker, const Json& market) const {
    auto timestamp = safeInteger(ticker, "ts");
    return {
        {"symbol", market["symbol"]},
        {"timestamp", timestamp},
        {"datetime", iso8601(timestamp)},
        {"high", safeString(ticker, "high24h")},
        {"low", safeString(ticker, "low24h")},
        {"bid", safeString(ticker, "bestBid")},
        {"bidVolume", safeString(ticker, "bestBidSize")},
        {"ask", safeString(ticker, "bestAsk")},
        {"askVolume", safeString(ticker, "bestAskSize")},
        {"vwap", nullptr},
        {"open", safeString(ticker, "open24h")},
        {"close", safeString(ticker, "price")},
        {"last", safeString(ticker, "price")},
        {"previousClose", nullptr},
        {"change", safeString(ticker, "change24h")},
        {"percentage", safeString(ticker, "changePercentage24h")},
        {"average", nullptr},
        {"baseVolume", safeString(ticker, "volume24h")},
        {"quoteVolume", safeString(ticker, "turnover24h")},
        {"info", ticker}
    };
}

Json poloniexfutures::parseOHLCV(const Json& ohlcv) const {
    return {
        safeInteger(ohlcv, "time"),
        safeNumber(ohlcv, "open"),
        safeNumber(ohlcv, "high"),
        safeNumber(ohlcv, "low"),
        safeNumber(ohlcv, "close"),
        safeNumber(ohlcv, "volume")
    };
}

std::string poloniexfutures::sign(const std::string& path, const std::string& api,
                                 const std::string& method, const Json& params,
                                 const Json& headers, const Json& body) const {
    auto url = urls["api"][api].get<std::string>() + "/" + version + path;
    
    if (api == "public") {
        if (!params.empty()) {
            url += "?" + encodeURIComponent(params);
        }
    } else {
        checkRequiredCredentials();
        
        auto timestamp = std::to_string(milliseconds());
        auto auth = timestamp + method + path;
        
        if (!params.empty()) {
            if (method == "GET" || method == "DELETE") {
                url += "?" + encodeURIComponent(params);
                auth += "?" + encodeURIComponent(params);
            } else {
                auth += encodeURIComponent(params);
            }
        }
        
        auto signature = hmac(auth, secret, "sha256", true);
        headers["KC-API-KEY"] = apiKey;
        headers["KC-API-SIGN"] = signature;
        headers["KC-API-TIMESTAMP"] = timestamp;
        headers["KC-API-PASSPHRASE"] = password;
        
        if (method != "GET" && method != "DELETE") {
            headers["Content-Type"] = "application/json";
            body = params;
        }
    }
    
    return url;
}

void poloniexfutures::handleErrors(const std::string& code, const std::string& reason,
                                 const std::string& url, const std::string& method,
                                 const Json& headers, const Json& body,
                                 const Json& response, const std::string& requestHeaders,
                                 const std::string& requestBody) const {
    if (response.contains("code")) {
        auto errorCode = response["code"].get<int>();
        auto message = response.contains("msg") ? response["msg"].get<std::string>() : "";
        
        if (errorCode != 200000) {
            switch (errorCode) {
                case 400001:
                case 400002:
                case 400003:
                    throw AuthenticationError(message);
                case 400004:
                    throw ArgumentsRequired(message);
                case 400005:
                case 400006:
                case 400007:
                    throw BadRequest(message);
                case 400008:
                    throw InsufficientFunds(message);
                case 400009:
                    throw OrderNotFound(message);
                case 400010:
                    throw InvalidOrder(message);
                case 400011:
                    throw RateLimitExceeded(message);
                case 500000:
                    throw ExchangeNotAvailable(message);
                default:
                    throw ExchangeError(message);
            }
        }
    }
}

// Async implementations for market data
AsyncPullType poloniexfutures::fetchMarketsAsync() const {
    return std::async(std::launch::async, [this]() { return fetchMarketsImpl(); });
}

AsyncPullType poloniexfutures::fetchTimeAsync() const {
    return std::async(std::launch::async, [this]() { return fetchTimeImpl(); });
}

AsyncPullType poloniexfutures::fetchTickerAsync(const std::string& symbol) const {
    return std::async(std::launch::async, [this, symbol]() { return fetchTickerImpl(symbol); });
}

AsyncPullType poloniexfutures::fetchTickersAsync(const std::vector<std::string>& symbols) const {
    return std::async(std::launch::async, [this, symbols]() { return fetchTickersImpl(symbols); });
}

AsyncPullType poloniexfutures::fetchOrderBookAsync(const std::string& symbol,
                                                     const std::optional<int>& limit) const {
    return std::async(std::launch::async,
                     [this, symbol, limit]() { return fetchOrderBookImpl(symbol, limit); });
}

AsyncPullType poloniexfutures::fetchOHLCVAsync(const std::string& symbol,
                                                  const std::string& timeframe,
                                                  const std::optional<long long>& since,
                                                  const std::optional<int>& limit) const {
    return std::async(std::launch::async,
                     [this, symbol, timeframe, since, limit]() {
                         return fetchOHLCVImpl(symbol, timeframe, since, limit);
                     });
}

AsyncPullType poloniexfutures::fetchTradesAsync(const std::string& symbol,
                                                   const std::optional<int>& limit,
                                                   const std::optional<long long>& since) const {
    return std::async(std::launch::async,
                     [this, symbol, limit, since]() {
                         return fetchTradesImpl(symbol, limit, since);
                     });
}

} // namespace ccxt
