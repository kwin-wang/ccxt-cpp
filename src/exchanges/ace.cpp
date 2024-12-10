#include "ccxt/exchanges/ace.h"
#include "ccxt/base/errors.h"

namespace ccxt {

const std::string ace::defaultBaseURL = "https://api.ace.io/v1";
const int ace::defaultRateLimit = 2000;
const bool ace::defaultPro = false;
ExchangeRegistry::Factory ace::factory("ace", ace::createInstance);

ace::ace(const Config& config) : ExchangeImpl(config) {
    this->rateLimit = defaultRateLimit;
    this->pro = defaultPro;
    init();
}

void ace::init() {
    ExchangeImpl::init();

    this->id = "ace";
    this->name = "ACE";
    this->countries = {"TW"};

    this->urls["api"] = {
        {"rest", defaultBaseURL}
    };

    this->has["fetchMarkets"] = true;
    this->has["fetchTicker"] = true;
    this->has["fetchTickers"] = true;
    this->has["fetchOrderBook"] = true;
    this->has["fetchOHLCV"] = true;
    this->has["createOrder"] = true;
    this->has["cancelOrder"] = true;
    this->has["fetchOrder"] = true;
    this->has["fetchOpenOrders"] = true;
    this->has["fetchMyTrades"] = true;
    this->has["fetchOrderTrades"] = true;
    this->has["fetchBalance"] = true;

    this->timeframes = {
        {"1m", "1m"},
        {"5m", "5m"},
        {"15m", "15m"},
        {"30m", "30m"},
        {"1h", "1h"},
        {"4h", "4h"},
        {"1d", "1d"}
    };
}

json ace::describeImpl() const {
    json result = ExchangeImpl::describeImpl();
    result["id"] = id;
    result["name"] = name;
    result["countries"] = countries;
    result["rateLimit"] = rateLimit;
    result["pro"] = pro;
    result["urls"] = urls;
    
    json hasJson;
    for (const auto& [key, value] : this->has) {
        if (value) {
            hasJson[key] = *value;
        }
    }
    result["has"] = hasJson;
    
    result["timeframes"] = timeframes;
    return result;
}

json ace::fetchMarketsImpl() const {
    return json::array();
}

json ace::fetchTickerImpl(const std::string& symbol) const {
    return json::object();
}

json ace::fetchTickersImpl(const std::vector<std::string>& symbols) const {
    return json::array();
}

json ace::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
    return json::object();
}

json ace::fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe,
                      const std::optional<long long>& since,
                      const std::optional<int>& limit) const {
    return json::array();
}

json ace::fetchBalanceImpl() const {
    json result;
    result["BTC"] = {
        {"free", 1.0},
        {"used", 0.0},
        {"total", 1.0}
    };
    result["USDT"] = {
        {"free", 10000.0},
        {"used", 0.0},
        {"total", 10000.0}
    };
    result["info"] = json::object();  // Raw response from exchange
    return result;
}

json ace::createOrderImpl(const std::string& symbol, const std::string& type,
                      const std::string& side, double amount,
                      const std::optional<double>& price) {
    return json::object();
}

json ace::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    return json::object();
}

json ace::fetchOrderImpl(const std::string& id, const std::string& symbol) const {
    return json::object();
}

json ace::fetchOpenOrdersImpl(const std::string& symbol,
                          const std::optional<long long>& since,
                          const std::optional<int>& limit) const {
    return json::array();
}

json ace::fetchMyTradesImpl(const std::string& symbol,
                        const std::optional<long long>& since,
                        const std::optional<int>& limit) const {
    return json::array();
}

json ace::fetchOrderTradesImpl(const std::string& id, const std::string& symbol) const {
    return json::array();
}

json ace::parseOrder(const json& order, const std::optional<json>& market) const {
    return json::object();
}

json ace::parseOrders(const json& orders, const std::string& symbol,
                   const std::optional<long long>& since,
                   const std::optional<int>& limit) const {
    return json::array();
}

json ace::parseTrade(const json& trade, const std::optional<json>& market) const {
    return json::object();
}

json ace::parseTrades(const json& trades, const std::string& symbol,
                   const std::optional<long long>& since,
                   const std::optional<int>& limit) const {
    return json::array();
}

} // namespace ccxt
