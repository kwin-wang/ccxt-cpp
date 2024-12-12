#include "ccxt/exchanges/coinone.h"
#include <openssl/hmac.h>

namespace ccxt {

const std::string coinone::defaultBaseURL = "https://api.coinone.co.kr";
const std::string coinone::defaultVersion = "v2";
const int coinone::defaultRateLimit = 50;
const bool coinone::defaultPro = false;


coinone::coinone(const Config& config)
    : Exchange(config) {
    init();
}

void coinone::init() {
    Exchange::init();
    this->id = "coinone";
    this->name = "CoinOne";
    this->countries = {"KR"};  // Korea
    this->version = defaultVersion;
    this->rateLimit = defaultRateLimit;
    this->pro = defaultPro;

    if (this->urls.empty()) {
        this->urls["api"] = {
            {"public", defaultBaseURL + "/public"},
            {"private", defaultBaseURL + "/private"}
        };
    }

    this->has = {
        {"CORS", nullptr},
        {"spot", true},
        {"margin", false},
        {"swap", false},
        {"future", false},
        {"option", false},
        {"cancelOrder", true},
        {"createOrder", true},
        {"createMarketOrder", false},
        {"fetchBalance", true},
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

    this->timeframes = {
        {"1m", "1"},
        {"3m", "3"},
        {"5m", "5"},
        {"15m", "15"},
        {"30m", "30"},
        {"1h", "60"},
        {"2h", "120"},
        {"4h", "240"},
        {"6h", "360"},
        {"12h", "720"},
        {"1d", "1D"},
        {"3d", "3D"},
        {"1w", "1W"},
        {"1M", "1M"}
    };
}

Json coinone::describeImpl() const {
    return this->deepExtend(Exchange::describeImpl(), {
        "id": "coinone",
        "name": "CoinOne",
        "countries": {"KR"},
        "version": defaultVersion,
        "rateLimit": defaultRateLimit,
        "pro": defaultPro,
        "has": this->has,
        "timeframes": this->timeframes
    });
}

// Market Data
Json coinone::fetchMarketsImpl() const {
    Json response = this->publicGetMarketAll();
    return this->parseMarkets(response["data"]);
}

Json coinone::fetchTickerImpl(const std::string& symbol) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json response = this->publicGetTicker(Json{
        {"currency", market["id"]}
    });
    return this->parseTicker(response["data"], market);
}

Json coinone::fetchTickersImpl(const std::vector<std::string>& symbols) const {
    this->loadMarkets();
    Json response = this->publicGetTickers();
    return this->parseTickers(response["data"], symbols);
}

Json coinone::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json{
        {"currency", market["id"]}
    };
    if (limit.has_value()) {
        request["limit"] = limit.value();
    }
    Json response = this->publicGetOrderbook(request);
    return this->parseOrderBook(response["data"], market);
}

Json coinone::fetchTradesImpl(const std::string& symbol, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json{
        {"currency", market["id"]}
    };
    if (limit.has_value()) {
        request["limit"] = limit.value();
    }
    Json response = this->publicGetTrades(request);
    return this->parseTrades(response["data"], market);
}

Json coinone::fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe, const std::optional<long long>& since, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json{
        {"currency", market["id"]},
        {"interval", this->timeframes[timeframe]}
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
Json coinone::createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side, double amount, const std::optional<double>& price) {
    if (type == "market") {
        throw ExchangeError("Market orders are not supported by Coinone");
    }
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json{
        {"currency", market["id"]},
        {"qty", this->amountToPrecision(symbol, amount)},
        {"type", side}
    };
    if (price.has_value()) {
        request["price"] = this->priceToPrecision(symbol, price.value());
    }
    Json response = this->privatePostOrder(request);
    return this->parseOrder(response["data"], market);
}

Json coinone::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    this->loadMarkets();
    Json request = Json{
        {"order_id", id}
    };
    Json response = this->privatePostCancelOrder(request);
    return this->parseOrder(response["data"]);
}

Json coinone::fetchOrderImpl(const std::string& id, const std::string& symbol) const {
    this->loadMarkets();
    Json request = Json{
        {"order_id", id}
    };
    Json response = this->privateGetOrder(request);
    return this->parseOrder(response["data"]);
}

Json coinone::fetchOrdersImpl(const std::string& symbol, const std::optional<long long>& since, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json{
        {"currency", market["id"]}
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

Json coinone::fetchOpenOrdersImpl(const std::string& symbol, const std::optional<long long>& since, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json{
        {"currency", market["id"]}
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

Json coinone::fetchClosedOrdersImpl(const std::string& symbol, const std::optional<long long>& since, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json{
        {"currency", market["id"]}
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

Json coinone::fetchMyTradesImpl(const std::string& symbol, const std::optional<long long>& since, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json{
        {"currency", market["id"]}
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
Json coinone::fetchBalanceImpl() const {
    this->loadMarkets();
    Json response = this->privateGetBalance();
    return this->parseBalance(response["data"]);
}

Json coinone::fetchDepositAddressImpl(const std::string& code, const std::optional<std::string>& chain) const {
    this->loadMarkets();
    Json request = Json{
        {"currency", this->getCurrencyId(code)}
    };
    if (chain.has_value()) {
        request["chain"] = chain.value();
    }
    Json response = this->privateGetDepositAddress(request);
    return this->parseDepositAddress(response["data"]);
}

Json coinone::fetchDepositsImpl(const std::optional<std::string>& code, const std::optional<long long>& since, const std::optional<int>& limit) const {
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
    Json response = this->privateGetDeposits(request);
    return this->parseTransactions(response["data"]);
}

Json coinone::fetchWithdrawalsImpl(const std::optional<std::string>& code, const std::optional<long long>& since, const std::optional<int>& limit) const {
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
    Json response = this->privateGetWithdrawals(request);
    return this->parseTransactions(response["data"]);
}

Json coinone::withdrawImpl(const std::string& code, double amount, const std::string& address, const std::optional<std::string>& tag) {
    this->loadMarkets();
    Json request = Json{
        {"currency", this->getCurrencyId(code)},
        {"amount", this->amountToPrecision(code, amount)},
        {"address", address}
    };
    if (tag.has_value()) {
        request["tag"] = tag.value();
    }
    Json response = this->privatePostWithdraw(request);
    return this->parseTransaction(response["data"]);
}

// Async Methods
// Market Data
std::future<Json> coinone::fetchMarketsAsync() const {
    return std::async(std::launch::async, [this]() {
        return this->fetchMarketsImpl();
    });
}

std::future<Json> coinone::fetchTickerAsync(const std::string& symbol) const {
    return std::async(std::launch::async, [this, symbol]() {
        return this->fetchTickerImpl(symbol);
    });
}

std::future<Json> coinone::fetchTickersAsync(const std::vector<std::string>& symbols) const {
    return std::async(std::launch::async, [this, symbols]() {
        return this->fetchTickersImpl(symbols);
    });
}

std::future<Json> coinone::fetchOrderBookAsync(const std::string& symbol, const std::optional<int>& limit) const {
    return std::async(std::launch::async, [this, symbol, limit]() {
        return this->fetchOrderBookImpl(symbol, limit);
    });
}

std::future<Json> coinone::fetchTradesAsync(const std::string& symbol, const std::optional<int>& limit) const {
    return std::async(std::launch::async, [this, symbol, limit]() {
        return this->fetchTradesImpl(symbol, limit);
    });
}

std::future<Json> coinone::fetchOHLCVAsync(const std::string& symbol, const std::string& timeframe, const std::optional<long long>& since, const std::optional<int>& limit) const {
    return std::async(std::launch::async, [this, symbol, timeframe, since, limit]() {
        return this->fetchOHLCVImpl(symbol, timeframe, since, limit);
    });
}

// Trading
std::future<Json> coinone::createOrderAsync(const std::string& symbol, const std::string& type, const std::string& side, double amount, const std::optional<double>& price) {
    return std::async(std::launch::async, [this, symbol, type, side, amount, price]() {
        return this->createOrderImpl(symbol, type, side, amount, price);
    });
}

std::future<Json> coinone::cancelOrderAsync(const std::string& id, const std::string& symbol) {
    return std::async(std::launch::async, [this, id, symbol]() {
        return this->cancelOrderImpl(id, symbol);
    });
}

std::future<Json> coinone::fetchOrderAsync(const std::string& id, const std::string& symbol) const {
    return std::async(std::launch::async, [this, id, symbol]() {
        return this->fetchOrderImpl(id, symbol);
    });
}

std::future<Json> coinone::fetchOrdersAsync(const std::string& symbol, const std::optional<long long>& since, const std::optional<int>& limit) const {
    return std::async(std::launch::async, [this, symbol, since, limit]() {
        return this->fetchOrdersImpl(symbol, since, limit);
    });
}

std::future<Json> coinone::fetchOpenOrdersAsync(const std::string& symbol, const std::optional<long long>& since, const std::optional<int>& limit) const {
    return std::async(std::launch::async, [this, symbol, since, limit]() {
        return this->fetchOpenOrdersImpl(symbol, since, limit);
    });
}

std::future<Json> coinone::fetchClosedOrdersAsync(const std::string& symbol, const std::optional<long long>& since, const std::optional<int>& limit) const {
    return std::async(std::launch::async, [this, symbol, since, limit]() {
        return this->fetchClosedOrdersImpl(symbol, since, limit);
    });
}

std::future<Json> coinone::fetchMyTradesAsync(const std::string& symbol, const std::optional<long long>& since, const std::optional<int>& limit) const {
    return std::async(std::launch::async, [this, symbol, since, limit]() {
        return this->fetchMyTradesImpl(symbol, since, limit);
    });
}

// Account
std::future<Json> coinone::fetchBalanceAsync() const {
    return std::async(std::launch::async, [this]() {
        return this->fetchBalanceImpl();
    });
}

std::future<Json> coinone::fetchDepositAddressAsync(const std::string& code, const std::optional<std::string>& chain) const {
    return std::async(std::launch::async, [this, code, chain]() {
        return this->fetchDepositAddressImpl(code, chain);
    });
}

std::future<Json> coinone::fetchDepositsAsync(const std::optional<std::string>& code, const std::optional<long long>& since, const std::optional<int>& limit) const {
    return std::async(std::launch::async, [this, code, since, limit]() {
        return this->fetchDepositsImpl(code, since, limit);
    });
}

std::future<Json> coinone::fetchWithdrawalsAsync(const std::optional<std::string>& code, const std::optional<long long>& since, const std::optional<int>& limit) const {
    return std::async(std::launch::async, [this, code, since, limit]() {
        return this->fetchWithdrawalsImpl(code, since, limit);
    });
}

std::future<Json> coinone::withdrawAsync(const std::string& code, double amount, const std::string& address, const std::optional<std::string>& tag) {
    return std::async(std::launch::async, [this, code, amount, address, tag]() {
        return this->withdrawImpl(code, amount, address, tag);
    });
}

// Helper Functions
Json coinone::parseTicker(const Json& ticker, const Json& market) const {
    // Implementation of parseTicker
    return Json();
}

Json coinone::parseTrade(const Json& trade, const Json& market) const {
    // Implementation of parseTrade
    return Json();
}

Json coinone::parseOrder(const Json& order, const Json& market) const {
    // Implementation of parseOrder
    return Json();
}

std::string coinone::getCurrencyId(const std::string& code) const {
    // Implementation of getCurrencyId
    return code;
}

} // namespace ccxt
