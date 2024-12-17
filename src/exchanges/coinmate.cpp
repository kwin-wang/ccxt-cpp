#include "ccxt/exchanges/coinmate.h"
#include <openssl/hmac.h>

namespace ccxt {

const std::string coinmate::defaultBaseURL = "https://api.coinmate.io";
const std::string coinmate::defaultVersion = "v1";
const int coinmate::defaultRateLimit = 600;
const bool coinmate::defaultPro = false;

coinmate::coinmate(const Config& config)
    : Exchange(config) {
    init();
}

void coinmate::init() {
    Exchange::init();
    this->id = "coinmate";
    this->name = "CoinMate";
    this->countries = {"GB", "CZ", "EU"};  // UK, Czech Republic
    this->version = defaultVersion;
    this->rateLimit = defaultRateLimit;
    this->pro = defaultPro;

    if (this->urls.empty()) {
        this->urls["api"] = {
            {"public", defaultBaseURL + "/api"},
            {"private", defaultBaseURL + "/api"}
        };
    }

    this->has = {
        {"CORS", true},
        {"spot", true},
        {"margin", false},
        {"swap", false},
        {"future", false},
        {"option", false},
        {"cancelOrder", true},
        {"createOrder", true},
        {"fetchBalance", true},
        {"fetchDepositsWithdrawals", true},
        {"fetchMarkets", true},
        {"fetchMyTrades", true},
        {"fetchOHLCV", true},
        {"fetchOpenOrders", true},
        {"fetchOrder", true},
        {"fetchOrderBook", true},
        {"fetchTicker", true},
        {"fetchTickers", true},
        {"fetchTrades", true}
    };
}

Json coinmate::describeImpl() const {
    return this->deepExtend(Exchange::describeImpl(), {
        "id": "coinmate",
        "name": "CoinMate",
        "countries": {"GB", "CZ", "EU"},
        "version": defaultVersion,
        "rateLimit": defaultRateLimit,
        "pro": defaultPro,
        "has": this->has
    });
}

// Market Data
Json coinmate::fetchMarketsImpl() const {
    Json response = this->publicGetTradingPairs();
    return this->parseMarkets(response["data"]);
}

Json coinmate::fetchTickerImpl(const std::string& symbol) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json response = this->publicGetTicker(Json{
        {"currencyPair", market["id"]}
    });
    return this->parseTicker(response["data"], market);
}

Json coinmate::fetchTickersImpl(const std::vector<std::string>& symbols) const {
    this->loadMarkets();
    Json response = this->publicGetTickers();
    return this->parseTickers(response["data"], symbols);
}

Json coinmate::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json{
        {"currencyPair", market["id"]}
    };
    if (limit.has_value()) {
        request["limit"] = limit.value();
    }
    Json response = this->publicGetOrderBook(request);
    return this->parseOrderBook(response["data"], market);
}

Json coinmate::fetchTradesImpl(const std::string& symbol, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json{
        {"currencyPair", market["id"]}
    };
    if (limit.has_value()) {
        request["limit"] = limit.value();
    }
    Json response = this->publicGetTransactions(request);
    return this->parseTrades(response["data"], market);
}

Json coinmate::fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe, const std::optional<long long>& since, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json{
        {"currencyPair", market["id"]},
        {"period", this->timeframes[timeframe]}
    };
    if (since.has_value()) {
        request["since"] = since.value();
    }
    if (limit.has_value()) {
        request["limit"] = limit.value();
    }
    Json response = this->publicGetOHLCV(request);
    return this->parseOHLCV(response["data"], market, timeframe, since, limit);
}

// Trading
Json coinmate::createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side, double amount, const std::optional<double>& price) {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json{
        {"currencyPair", market["id"]},
        {"amount", this->amountToPrecision(symbol, amount)},
        {"type", type},
        {"orderType", side}
    };
    if (price.has_value()) {
        request["price"] = this->priceToPrecision(symbol, price.value());
    }
    Json response = this->privatePostOrder(request);
    return this->parseOrder(response["data"], market);
}

Json coinmate::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    this->loadMarkets();
    Json request = Json{
        {"orderId", id}
    };
    Json response = this->privatePostCancelOrder(request);
    return this->parseOrder(response["data"]);
}

Json coinmate::fetchOrderImpl(const std::string& id, const std::string& symbol) const {
    this->loadMarkets();
    Json request = Json{
        {"orderId", id}
    };
    Json response = this->privateGetOrder(request);
    return this->parseOrder(response["data"]);
}

Json coinmate::fetchOrdersImpl(const std::string& symbol, const std::optional<long long>& since, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json{
        {"currencyPair", market["id"]}
    };
    if (since.has_value()) {
        request["since"] = since.value();
    }
    if (limit.has_value()) {
        request["limit"] = limit.value();
    }
    Json response = this->privateGetOrders(request);
    return this->parseOrders(response["data"], market);
}

Json coinmate::fetchOpenOrdersImpl(const std::string& symbol, const std::optional<long long>& since, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json{
        {"currencyPair", market["id"]}
    };
    if (since.has_value()) {
        request["since"] = since.value();
    }
    if (limit.has_value()) {
        request["limit"] = limit.value();
    }
    Json response = this->privateGetOpenOrders(request);
    return this->parseOrders(response["data"], market);
}

Json coinmate::fetchClosedOrdersImpl(const std::string& symbol, const std::optional<long long>& since, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json{
        {"currencyPair", market["id"]}
    };
    if (since.has_value()) {
        request["since"] = since.value();
    }
    if (limit.has_value()) {
        request["limit"] = limit.value();
    }
    Json response = this->privateGetClosedOrders(request);
    return this->parseOrders(response["data"], market);
}

Json coinmate::fetchMyTradesImpl(const std::string& symbol, const std::optional<long long>& since, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json{
        {"currencyPair", market["id"]}
    };
    if (since.has_value()) {
        request["since"] = since.value();
    }
    if (limit.has_value()) {
        request["limit"] = limit.value();
    }
    Json response = this->privateGetTradeHistory(request);
    return this->parseTrades(response["data"], market);
}

// Account
Json coinmate::fetchBalanceImpl() const {
    this->loadMarkets();
    Json response = this->privateGetBalances();
    return this->parseBalance(response["data"]);
}

Json coinmate::fetchDepositsWithdrawalsImpl(const std::optional<std::string>& code, const std::optional<long long>& since, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json request = Json::object();
    if (code.has_value()) {
        request["currency"] = this->getCurrencyId(code.value());
    }
    if (since.has_value()) {
        request["since"] = since.value();
    }
    if (limit.has_value()) {
        request["limit"] = limit.value();
    }
    Json response = this->privateGetTransferHistory(request);
    return this->parseTransactions(response["data"]);
}

// Async Methods
// Market Data
AsyncPullType coinmate::fetchMarketsAsync() const {
    return std::async(std::launch::async, [this]() {
        return this->fetchMarketsImpl();
    });
}

AsyncPullType coinmate::fetchTickerAsync(const std::string& symbol) const {
    return std::async(std::launch::async, [this, symbol]() {
        return this->fetchTickerImpl(symbol);
    });
}

AsyncPullType coinmate::fetchTickersAsync(const std::vector<std::string>& symbols) const {
    return std::async(std::launch::async, [this, symbols]() {
        return this->fetchTickersImpl(symbols);
    });
}

AsyncPullType coinmate::fetchOrderBookAsync(const std::string& symbol, const std::optional<int>& limit) const {
    return std::async(std::launch::async, [this, symbol, limit]() {
        return this->fetchOrderBookImpl(symbol, limit);
    });
}

AsyncPullType coinmate::fetchTradesAsync(const std::string& symbol, const std::optional<int>& limit) const {
    return std::async(std::launch::async, [this, symbol, limit]() {
        return this->fetchTradesImpl(symbol, limit);
    });
}

AsyncPullType coinmate::fetchOHLCVAsync(const std::string& symbol, const std::string& timeframe, const std::optional<long long>& since, const std::optional<int>& limit) const {
    return std::async(std::launch::async, [this, symbol, timeframe, since, limit]() {
        return this->fetchOHLCVImpl(symbol, timeframe, since, limit);
    });
}

// Trading
AsyncPullType coinmate::createOrderAsync(const std::string& symbol, const std::string& type, const std::string& side, double amount, const std::optional<double>& price) {
    return std::async(std::launch::async, [this, symbol, type, side, amount, price]() {
        return this->createOrderImpl(symbol, type, side, amount, price);
    });
}

AsyncPullType coinmate::cancelOrderAsync(const std::string& id, const std::string& symbol) {
    return std::async(std::launch::async, [this, id, symbol]() {
        return this->cancelOrderImpl(id, symbol);
    });
}

AsyncPullType coinmate::fetchOrderAsync(const std::string& id, const std::string& symbol) const {
    return std::async(std::launch::async, [this, id, symbol]() {
        return this->fetchOrderImpl(id, symbol);
    });
}

AsyncPullType coinmate::fetchOrdersAsync(const std::string& symbol, const std::optional<long long>& since, const std::optional<int>& limit) const {
    return std::async(std::launch::async, [this, symbol, since, limit]() {
        return this->fetchOrdersImpl(symbol, since, limit);
    });
}

AsyncPullType coinmate::fetchOpenOrdersAsync(const std::string& symbol, const std::optional<long long>& since, const std::optional<int>& limit) const {
    return std::async(std::launch::async, [this, symbol, since, limit]() {
        return this->fetchOpenOrdersImpl(symbol, since, limit);
    });
}

AsyncPullType coinmate::fetchClosedOrdersAsync(const std::string& symbol, const std::optional<long long>& since, const std::optional<int>& limit) const {
    return std::async(std::launch::async, [this, symbol, since, limit]() {
        return this->fetchClosedOrdersImpl(symbol, since, limit);
    });
}

AsyncPullType coinmate::fetchMyTradesAsync(const std::string& symbol, const std::optional<long long>& since, const std::optional<int>& limit) const {
    return std::async(std::launch::async, [this, symbol, since, limit]() {
        return this->fetchMyTradesImpl(symbol, since, limit);
    });
}

// Account
AsyncPullType coinmate::fetchBalanceAsync() const {
    return std::async(std::launch::async, [this]() {
        return this->fetchBalanceImpl();
    });
}

AsyncPullType coinmate::fetchDepositsWithdrawalsAsync(const std::optional<std::string>& code, const std::optional<long long>& since, const std::optional<int>& limit) const {
    return std::async(std::launch::async, [this, code, since, limit]() {
        return this->fetchDepositsWithdrawalsImpl(code, since, limit);
    });
}

// Helper Functions
Json coinmate::parseTicker(const Json& ticker, const Json& market) const {
    // Implementation of parseTicker
    return Json();
}

Json coinmate::parseTrade(const Json& trade, const Json& market) const {
    // Implementation of parseTrade
    return Json();
}

Json coinmate::parseOrder(const Json& order, const Json& market) const {
    // Implementation of parseOrder
    return Json();
}

std::string coinmate::getCurrencyId(const std::string& code) const {
    // Implementation of getCurrencyId
    return code;
}

} // namespace ccxt
