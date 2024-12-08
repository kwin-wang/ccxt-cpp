#include "ccxt/exchanges/ace.h"
#include "../base/json_helper.h"

namespace ccxt {

const std::string ace::defaultBaseURL = "https://ace.io/api";
const std::string ace::defaultVersion = "v2";
const int ace::defaultRateLimit = 100;
const bool ace::defaultPro = false;

ExchangeRegistry::Factory ace::factory("ace", ace::createInstance);

ace::ace(const Config& config) : ExchangeImpl(config) {
    init();
}

void ace::init() {
    ExchangeImpl::init();
    
    // Set exchange properties
    this->id = "ace";
    this->name = "ACE";
    this->countries = {"TW"}; // Taiwan
    this->version = defaultVersion;
    this->rateLimit = defaultRateLimit;
    this->pro = defaultPro;
    
    if (this->urls.empty()) {
        this->urls["api"] = {
            {"rest", defaultBaseURL}
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
        {"fetchBalance", true},
        {"fetchMarkets", true},
        {"fetchMyTrades", true},
        {"fetchOHLCV", true},
        {"fetchOpenOrders", true},
        {"fetchOrder", true},
        {"fetchOrderBook", true},
        {"fetchOrderTrades", true},
        {"fetchTicker", true},
        {"fetchTickers", true}
    };

    // Set timeframes
    this->timeframes = {
        {"1m", 1},
        {"5m", 5},
        {"10m", 10},
        {"30m", 30},
        {"1h", 60}
    };
}

Json ace::describeImpl() const {
    return Json::object({
        {"id", this->id},
        {"name", this->name},
        {"countries", this->countries},
        {"version", this->version},
        {"rateLimit", this->rateLimit},
        {"pro", this->pro},
        {"has", this->has},
        {"timeframes", this->timeframes}
    });
}

Json ace::fetchMarketsImpl() const {
    Json response = this->publicGetMarkets();
    return this->parseMarkets(response);
}

Json ace::fetchTickerImpl(const std::string& symbol) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json response = this->publicGetTicker(Json::object({
        {"market", market["id"]}
    }));
    return this->parseTicker(response, market);
}

Json ace::fetchTickersImpl(const std::vector<std::string>& symbols) const {
    this->loadMarkets();
    Json response = this->publicGetTickers();
    return this->parseTickers(response, symbols);
}

Json ace::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json request = Json::object({
        {"market", this->marketId(symbol)}
    });
    if (limit) {
        request["limit"] = *limit;
    }
    Json response = this->publicGetOrderBook(request);
    return this->parseOrderBook(response, symbol);
}

Json ace::fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe, 
                        const std::optional<long long>& since, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json request = Json::object({
        {"market", this->marketId(symbol)},
        {"period", this->timeframes[timeframe]}
    });
    if (since) {
        request["since"] = *since;
    }
    if (limit) {
        request["limit"] = *limit;
    }
    Json response = this->publicGetKlines(request);
    return this->parseOHLCV(response, symbol, timeframe, since, limit);
}

Json ace::createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side,
                        double amount, const std::optional<double>& price) const {
    this->loadMarkets();
    Json request = Json::object({
        {"market", this->marketId(symbol)},
        {"side", side},
        {"volume", this->amountToPrecision(symbol, amount)},
        {"ord_type", type}
    });
    if (price) {
        request["price"] = this->priceToPrecision(symbol, *price);
    }
    Json response = this->privatePostOrders(request);
    return this->parseOrder(response);
}

Json ace::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    this->loadMarkets();
    Json request = Json::object({
        {"id", id}
    });
    Json response = this->privatePostOrderDelete(request);
    return this->parseOrder(response);
}

Json ace::fetchOrderImpl(const std::string& id, const std::string& symbol) const {
    this->loadMarkets();
    Json request = Json::object({
        {"id", id}
    });
    Json response = this->privateGetOrder(request);
    return this->parseOrder(response);
}

Json ace::fetchOpenOrdersImpl(const std::string& symbol, const std::optional<long long>& since,
                            const std::optional<int>& limit) const {
    this->loadMarkets();
    Json request = Json::object();
    if (!symbol.empty()) {
        request["market"] = this->marketId(symbol);
    }
    if (limit) {
        request["limit"] = *limit;
    }
    Json response = this->privateGetOrders(request);
    return this->parseOrders(response, symbol, since, limit);
}

Json ace::fetchMyTradesImpl(const std::string& symbol, const std::optional<long long>& since,
                          const std::optional<int>& limit) const {
    this->loadMarkets();
    Json request = Json::object();
    if (!symbol.empty()) {
        request["market"] = this->marketId(symbol);
    }
    if (since) {
        request["since"] = *since;
    }
    if (limit) {
        request["limit"] = *limit;
    }
    Json response = this->privateGetTrades(request);
    return this->parseTrades(response, symbol, since, limit);
}

Json ace::fetchOrderTradesImpl(const std::string& id, const std::string& symbol) const {
    this->loadMarkets();
    Json request = Json::object({
        {"id", id}
    });
    Json response = this->privateGetOrderTrades(request);
    return this->parseTrades(response, symbol);
}

Json ace::fetchBalanceImpl() const {
    this->loadMarkets();
    Json response = this->privateGetBalances();
    return this->parseBalance(response);
}

} // namespace ccxt
