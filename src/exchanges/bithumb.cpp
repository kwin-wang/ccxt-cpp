#include "ccxt/exchanges/bithumb.h"

namespace ccxt {

bithumb::bithumb(const Config& config) : Exchange(config) {
    this->describe({
        {"id", "bithumb"},
        {"name", "Bithumb"},
        {"countries", Json::array({"KR"})},
        {"version", "1"},
        {"rateLimit", 500},
        {"has", {
            {"CORS", true},
            {"spot", true},
            {"margin", false},
            {"swap", false},
            {"future", false},
            {"option", false},
            {"fetchMarkets", true},
            {"fetchTicker", true},
            {"fetchOrderBook", true},
            {"fetchTrades", true},
            {"fetchOHLCV", true},
            {"createOrder", true},
            {"cancelOrder", true},
            {"fetchOrder", true},
            {"fetchOrders", true},
            {"fetchOpenOrders", true},
            {"fetchClosedOrders", true},
            {"fetchBalance", true},
            {"fetchMyTrades", true},
            {"fetchDeposits", true},
            {"fetchWithdrawals", true},
            {"withdraw", true}
        }},
        {"timeframes", {
            {"1m", "1m"},
            {"3m", "3m"},
            {"5m", "5m"},
            {"15m", "15m"},
            {"30m", "30m"},
            {"1h", "1h"},
            {"2h", "2h"},
            {"4h", "4h"},
            {"6h", "6h"},
            {"12h", "12h"},
            {"1d", "24h"},
            {"1w", "1w"},
            {"1M", "1M"}
        }},
        {"urls", {
            {"logo", "https://user-images.githubusercontent.com/1294454/30597177-ea800172-9d5e-11e7-804c-b9d4fa9b56b0.jpg"},
            {"api", {
                {"public", "https://api.bithumb.com/public"},
                {"private", "https://api.bithumb.com"}
            }},
            {"www", "https://www.bithumb.com"},
            {"doc", {
                "https://apidocs.bithumb.com",
                "https://github.com/bithumb-pro/bithumb.pro-official-api-docs"
            }}
        }},
        {"api", {
            {"public", {
                {"get", {
                    "/ticker/{currency}",
                    "/ticker/all",
                    "/orderbook/{currency}",
                    "/orderbook/all",
                    "/transaction_history/{currency}",
                    "/candlestick/{currency}/{interval}"
                }}
            }},
            {"private", {
                {"post", {
                    "/info/balance",
                    "/info/orders",
                    "/info/order_detail",
                    "/trade/place",
                    "/trade/cancel",
                    "/trade/orders",
                    "/trade/order_detail",
                    "/trade/history",
                    "/info/wallet_address",
                    "/info/user_transactions",
                    "/trade/withdraw",
                    "/trade/withdraw_detail"
                }}
            }}
        }}
    });
}

// Market Data Implementation
Json bithumb::fetchMarketsImpl() const {
    return this->fetch("/ticker/all");
}

Json bithumb::fetchTickerImpl(const std::string& symbol) const {
    auto market = this->market(symbol);
    return this->fetch("/ticker/" + market["id"].get<std::string>());
}

Json bithumb::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    return this->fetch("/orderbook/" + market["id"].get<std::string>());
}

Json bithumb::fetchTradesImpl(const std::string& symbol, const std::optional<long long>& since,
                           const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    return this->fetch("/transaction_history/" + market["id"].get<std::string>());
}

Json bithumb::fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe,
                          const std::optional<long long>& since,
                          const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    return this->fetch("/candlestick/" + market["id"].get<std::string>() + "/" + timeframe);
}

// Trading Implementation
Json bithumb::createOrderImpl(const std::string& symbol, const std::string& type,
                          const std::string& side, double amount,
                          const std::optional<double>& price) {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"order_currency", market["id"]},
        {"units", this->amountToPrecision(symbol, amount)},
        {"type", side}
    });
    if (price) {
        request["price"] = this->priceToPrecision(symbol, *price);
    }
    return this->fetch("/trade/place", "private", "POST", request);
}

Json bithumb::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"order_id", id},
        {"currency", market["id"]}
    });
    return this->fetch("/trade/cancel", "private", "POST", request);
}

Json bithumb::fetchOrderImpl(const std::string& id, const std::string& symbol) const {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"order_id", id},
        {"currency", market["id"]}
    });
    return this->fetch("/trade/order_detail", "private", "POST", request);
}

Json bithumb::fetchOrdersImpl(const std::string& symbol, const std::optional<long long>& since,
                          const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({{"currency", market["id"]}});
    return this->fetch("/trade/orders", "private", "POST", request);
}

Json bithumb::fetchOpenOrdersImpl(const std::string& symbol, const std::optional<long long>& since,
                               const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"currency", market["id"]},
        {"status", "open"}
    });
    return this->fetch("/trade/orders", "private", "POST", request);
}

Json bithumb::fetchClosedOrdersImpl(const std::string& symbol, const std::optional<long long>& since,
                                const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"currency", market["id"]},
        {"status", "completed"}
    });
    return this->fetch("/trade/orders", "private", "POST", request);
}

// Account Implementation
Json bithumb::fetchBalanceImpl() const {
    return this->fetch("/info/balance", "private", "POST");
}

Json bithumb::fetchMyTradesImpl(const std::string& symbol, const std::optional<long long>& since,
                             const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({{"currency", market["id"]}});
    return this->fetch("/trade/history", "private", "POST", request);
}

Json bithumb::fetchDepositsImpl(const std::string& code, const std::optional<long long>& since,
                             const std::optional<int>& limit) const {
    auto request = Json::object({{"currency", code}});
    return this->fetch("/info/user_transactions", "private", "POST", request);
}

Json bithumb::fetchWithdrawalsImpl(const std::string& code, const std::optional<long long>& since,
                                const std::optional<int>& limit) const {
    auto request = Json::object({{"currency", code}});
    return this->fetch("/trade/withdraw_detail", "private", "POST", request);
}

Json bithumb::withdrawImpl(const std::string& code, double amount, const std::string& address,
                        const std::string& tag, const Json& params) {
    auto request = Json::object({
        {"currency", code},
        {"units", amount},
        {"address", address}
    });
    if (!tag.empty()) {
        request["destination"] = tag;
    }
    return this->fetch("/trade/withdraw", "private", "POST", this->extend(request, params));
}

// Async Implementation
boost::future<Json> bithumb::fetchAsync(const std::string& path, const std::string& api,
                                     const std::string& method, const Json& params,
                                     const std::map<std::string, std::string>& headers) const {
    return Exchange::fetchAsync(path, api, method, params, headers);
}

boost::future<Json> bithumb::fetchMarketsAsync() const {
    return this->fetchAsync("/ticker/all");
}

boost::future<Json> bithumb::fetchTickerAsync(const std::string& symbol) const {
    auto market = this->market(symbol);
    return this->fetchAsync("/ticker/" + market["id"].get<std::string>());
}

boost::future<Json> bithumb::fetchOrderBookAsync(const std::string& symbol,
                                              const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    return this->fetchAsync("/orderbook/" + market["id"].get<std::string>());
}

boost::future<Json> bithumb::fetchTradesAsync(const std::string& symbol,
                                           const std::optional<long long>& since,
                                           const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    return this->fetchAsync("/transaction_history/" + market["id"].get<std::string>());
}

boost::future<Json> bithumb::fetchOHLCVAsync(const std::string& symbol,
                                          const std::string& timeframe,
                                          const std::optional<long long>& since,
                                          const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    return this->fetchAsync("/candlestick/" + market["id"].get<std::string>() + "/" + timeframe);
}

boost::future<Json> bithumb::createOrderAsync(const std::string& symbol, const std::string& type,
                                          const std::string& side, double amount,
                                          const std::optional<double>& price) {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"order_currency", market["id"]},
        {"units", this->amountToPrecision(symbol, amount)},
        {"type", side}
    });
    if (price) {
        request["price"] = this->priceToPrecision(symbol, *price);
    }
    return this->fetchAsync("/trade/place", "private", "POST", request);
}

boost::future<Json> bithumb::cancelOrderAsync(const std::string& id, const std::string& symbol) {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"order_id", id},
        {"currency", market["id"]}
    });
    return this->fetchAsync("/trade/cancel", "private", "POST", request);
}

boost::future<Json> bithumb::fetchOrderAsync(const std::string& id, const std::string& symbol) const {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"order_id", id},
        {"currency", market["id"]}
    });
    return this->fetchAsync("/trade/order_detail", "private", "POST", request);
}

boost::future<Json> bithumb::fetchOrdersAsync(const std::string& symbol,
                                          const std::optional<long long>& since,
                                          const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({{"currency", market["id"]}});
    return this->fetchAsync("/trade/orders", "private", "POST", request);
}

boost::future<Json> bithumb::fetchOpenOrdersAsync(const std::string& symbol,
                                              const std::optional<long long>& since,
                                              const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"currency", market["id"]},
        {"status", "open"}
    });
    return this->fetchAsync("/trade/orders", "private", "POST", request);
}

boost::future<Json> bithumb::fetchClosedOrdersAsync(const std::string& symbol,
                                                const std::optional<long long>& since,
                                                const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"currency", market["id"]},
        {"status", "completed"}
    });
    return this->fetchAsync("/trade/orders", "private", "POST", request);
}

boost::future<Json> bithumb::fetchBalanceAsync() const {
    return this->fetchAsync("/info/balance", "private", "POST");
}

boost::future<Json> bithumb::fetchMyTradesAsync(const std::string& symbol,
                                            const std::optional<long long>& since,
                                            const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({{"currency", market["id"]}});
    return this->fetchAsync("/trade/history", "private", "POST", request);
}

boost::future<Json> bithumb::fetchDepositsAsync(const std::string& code,
                                            const std::optional<long long>& since,
                                            const std::optional<int>& limit) const {
    auto request = Json::object({{"currency", code}});
    return this->fetchAsync("/info/user_transactions", "private", "POST", request);
}

boost::future<Json> bithumb::fetchWithdrawalsAsync(const std::string& code,
                                               const std::optional<long long>& since,
                                               const std::optional<int>& limit) const {
    auto request = Json::object({{"currency", code}});
    return this->fetchAsync("/trade/withdraw_detail", "private", "POST", request);
}

boost::future<Json> bithumb::withdrawAsync(const std::string& code, double amount,
                                       const std::string& address, const std::string& tag,
                                       const Json& params) {
    auto request = Json::object({
        {"currency", code},
        {"units", amount},
        {"address", address}
    });
    if (!tag.empty()) {
        request["destination"] = tag;
    }
    return this->fetchAsync("/trade/withdraw", "private", "POST", this->extend(request, params));
}

std::string bithumb::getOrderId() const {
    static int counter = 0;
    return std::to_string(this->milliseconds()) + "_" + std::to_string(++counter);
}

} // namespace ccxt
