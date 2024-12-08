#include "ccxt/exchanges/alpaca.h"
#include "../base/json_helper.h"

namespace ccxt {

const std::string alpaca::defaultHostname = "alpaca.markets";
const int alpaca::defaultRateLimit = 333;  // 3 req/s for free tier
const bool alpaca::defaultPro = true;

ExchangeRegistry::Factory alpaca::factory("alpaca", alpaca::createInstance);

alpaca::alpaca(const Config& config) : ExchangeImpl(config) {
    init();
}

void alpaca::init() {
    ExchangeImpl::init();
    
    // Set exchange properties
    this->id = "alpaca";
    this->name = "Alpaca";
    this->countries = {"US"};
    this->hostname = defaultHostname;
    this->rateLimit = defaultRateLimit;
    this->pro = defaultPro;
    
    if (this->urls.empty()) {
        this->urls = {
            {"logo", "https://github.com/user-attachments/assets/e9476df8-a450-4c3e-ab9a-1a7794219e1b"},
            {"www", "https://alpaca.markets"},
            {"api", {
                {"broker", "https://broker-api." + this->hostname},
                {"trader", "https://api." + this->hostname},
                {"market", "https://data." + this->hostname}
            }},
            {"test", {
                {"broker", "https://broker-api.sandbox." + this->hostname},
                {"trader", "https://paper-api." + this->hostname},
                {"market", "https://data.sandbox." + this->hostname}
            }},
            {"doc", "https://alpaca.markets/docs/"},
            {"fees", "https://docs.alpaca.markets/docs/crypto-fees"}
        };
    }

    // Set capabilities
    this->has = {
        {"CORS", false},
        {"spot", true},
        {"margin", false},
        {"swap", false},
        {"future", false},
        {"option", false},
        {"cancelAllOrders", true},
        {"cancelOrder", true},
        {"createOrder", true},
        {"createStopOrder", true},
        {"createTriggerOrder", true},
        {"editOrder", true},
        {"fetchBalance", false},
        {"fetchClosedOrders", true},
        {"fetchDepositAddress", true},
        {"fetchDeposits", true},
        {"fetchDepositsWithdrawals", true},
        {"fetchL1OrderBook", true},
        {"fetchMarkets", true},
        {"fetchMyTrades", true},
        {"fetchOHLCV", true},
        {"fetchOpenOrders", true},
        {"fetchOrder", true},
        {"fetchOrderBook", true},
        {"fetchOrders", true},
        {"fetchTicker", true},
        {"fetchTickers", true},
        {"fetchTime", true},
        {"fetchTrades", true}
    };

    // Set timeframes if supported
    this->timeframes = {
        {"1m", "1Min"},
        {"5m", "5Min"},
        {"15m", "15Min"},
        {"1h", "1Hour"},
        {"1d", "1Day"}
    };
}

Json alpaca::describeImpl() const {
    return Json::object({
        {"id", this->id},
        {"name", this->name},
        {"countries", this->countries},
        {"rateLimit", this->rateLimit},
        {"pro", this->pro},
        {"has", this->has},
        {"timeframes", this->timeframes}
    });
}

Json alpaca::fetchMarketsImpl() const {
    Json response = this->publicGetMarkets();
    return this->parseMarkets(response);
}

Json alpaca::fetchTickerImpl(const std::string& symbol) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json response = this->publicGetTicker(Json::object({
        {"symbol", market["id"]}
    }));
    return this->parseTicker(response, market);
}

Json alpaca::fetchTickersImpl(const std::vector<std::string>& symbols) const {
    this->loadMarkets();
    Json response = this->publicGetTickers();
    return this->parseTickers(response, symbols);
}

Json alpaca::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json request = Json::object({
        {"symbol", this->marketId(symbol)}
    });
    if (limit) {
        request["limit"] = *limit;
    }
    Json response = this->publicGetOrderbook(request);
    return this->parseOrderBook(response, symbol);
}

Json alpaca::fetchTimeImpl() const {
    Json response = this->publicGetTime();
    return this->safeInteger(response, "timestamp");
}

Json alpaca::fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe,
                           const std::optional<long long>& since, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json::object({
        {"symbol", market["id"]},
        {"timeframe", this->timeframes[timeframe]}
    });
    if (since) {
        request["start"] = this->iso8601(*since);
    }
    if (limit) {
        request["limit"] = *limit;
    }
    Json response = this->publicGetBars(request);
    return this->parseOHLCV(response, market, timeframe, since, limit);
}

Json alpaca::createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side,
                           double amount, const std::optional<double>& price) {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json::object({
        {"symbol", market["id"]},
        {"qty", this->amountToPrecision(symbol, amount)},
        {"side", side},
        {"type", type}
    });
    if (price) {
        request["limit_price"] = this->priceToPrecision(symbol, *price);
    }
    Json response = this->privatePostOrders(request);
    return this->parseOrder(response, market);
}

Json alpaca::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    this->loadMarkets();
    Json request = Json::object({
        {"order_id", id}
    });
    return this->privateDeleteOrders(request);
}

Json alpaca::cancelAllOrdersImpl(const std::string& symbol) {
    this->loadMarkets();
    Json request = Json::object();
    if (!symbol.empty()) {
        Json market = this->market(symbol);
        request["symbol"] = market["id"];
    }
    return this->privateDeleteOrders(request);
}

Json alpaca::fetchOrderImpl(const std::string& id, const std::string& symbol) const {
    this->loadMarkets();
    Json request = Json::object({
        {"order_id", id}
    });
    Json response = this->privateGetOrders(request);
    return this->parseOrder(response);
}

Json alpaca::fetchOrdersImpl(const std::string& symbol, const std::optional<long long>& since,
                           const std::optional<int>& limit) const {
    this->loadMarkets();
    Json request = Json::object();
    if (!symbol.empty()) {
        Json market = this->market(symbol);
        request["symbol"] = market["id"];
    }
    if (since) {
        request["after"] = this->iso8601(*since);
    }
    if (limit) {
        request["limit"] = *limit;
    }
    Json response = this->privateGetOrders(request);
    return this->parseOrders(response, symbol, since, limit);
}

Json alpaca::fetchOpenOrdersImpl(const std::string& symbol, const std::optional<long long>& since,
                               const std::optional<int>& limit) const {
    Json request = Json::object({
        {"status", "open"}
    });
    return this->fetchOrdersImpl(symbol, since, limit);
}

Json alpaca::fetchClosedOrdersImpl(const std::string& symbol, const std::optional<long long>& since,
                                 const std::optional<int>& limit) const {
    Json request = Json::object({
        {"status", "closed"}
    });
    return this->fetchOrdersImpl(symbol, since, limit);
}

Json alpaca::fetchMyTradesImpl(const std::string& symbol, const std::optional<long long>& since,
                             const std::optional<int>& limit) const {
    this->loadMarkets();
    Json request = Json::object();
    if (!symbol.empty()) {
        Json market = this->market(symbol);
        request["symbol"] = market["id"];
    }
    if (since) {
        request["start"] = this->iso8601(*since);
    }
    if (limit) {
        request["limit"] = *limit;
    }
    Json response = this->privateGetTrades(request);
    return this->parseTrades(response, symbol, since, limit);
}

Json alpaca::fetchDepositAddressImpl(const std::string& code,
                                   const std::optional<std::string>& network) const {
    this->loadMarkets();
    Json currency = this->currency(code);
    Json request = Json::object({
        {"asset", currency["id"]}
    });
    if (network) {
        request["network"] = *network;
    }
    Json response = this->privateGetDepositAddress(request);
    return this->parseDepositAddress(response, currency);
}

Json alpaca::fetchDepositsImpl(const std::string& code, const std::optional<long long>& since,
                             const std::optional<int>& limit) const {
    return this->fetchDepositsWithdrawalsImpl(code, since, limit);
}

Json alpaca::fetchDepositsWithdrawalsImpl(const std::string& code,
                                         const std::optional<long long>& since,
                                         const std::optional<int>& limit) const {
    this->loadMarkets();
    Json request = Json::object();
    if (!code.empty()) {
        Json currency = this->currency(code);
        request["asset"] = currency["id"];
    }
    if (since) {
        request["start"] = this->iso8601(*since);
    }
    if (limit) {
        request["limit"] = *limit;
    }
    Json response = this->privateGetTransfers(request);
    return this->parseTransactions(response, code, since, limit);
}

Json alpaca::parseOrder(const Json& order, const std::optional<Json>& market) const {
    // Implementation of order parsing logic
    return Json::object();  // Placeholder
}

Json alpaca::parseOrders(const Json& orders, const std::string& symbol,
                        const std::optional<long long>& since,
                        const std::optional<int>& limit) const {
    // Implementation of orders parsing logic
    return Json::object();  // Placeholder
}

Json alpaca::parseTrade(const Json& trade, const std::optional<Json>& market) const {
    // Implementation of trade parsing logic
    return Json::object();  // Placeholder
}

Json alpaca::parseTrades(const Json& trades, const std::string& symbol,
                        const std::optional<long long>& since,
                        const std::optional<int>& limit) const {
    // Implementation of trades parsing logic
    return Json::object();  // Placeholder
}

} // namespace ccxt
