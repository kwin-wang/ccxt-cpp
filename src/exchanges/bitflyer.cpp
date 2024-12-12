#include "ccxt/exchanges/bitflyer.h"

namespace ccxt {

bitflyer::bitflyer(const Config& config) : Exchange(config) {
    this->describe({
        {"id", "bitflyer"},
        {"name", "bitFlyer"},
        {"countries", Json::array({"JP"})},
        {"version", "1"},
        {"rateLimit", 1000},
        {"has", {
            {"fetchMarkets", true},
            {"fetchTicker", true},
            {"fetchOrderBook", true},
            {"fetchTrades", true},
            {"createOrder", true},
            {"cancelOrder", true},
            {"fetchOrder", true},
            {"fetchOrders", true},
            {"fetchOpenOrders", true},
            {"fetchClosedOrders", true},
            {"fetchMyTrades", true},
            {"fetchBalance", true},
            {"fetchPositions", true},
            {"fetchDeposits", true},
            {"fetchWithdrawals", true},
            {"withdraw", true}
        }},
        {"urls", {
            {"logo", "https://user-images.githubusercontent.com/1294454/28051642-56154182-660e-11e7-9b0d-6042d1e6edd8.jpg"},
            {"api", {
                {"rest", "https://api.bitflyer.com"}
            }},
            {"www", "https://bitflyer.com"},
            {"doc", {
                "https://lightning.bitflyer.com/docs?lang=en"
            }}
        }},
        {"api", {
            {"public", {
                {"get", {
                    "/v1/getmarkets",
                    "/v1/getticker",
                    "/v1/getboard",
                    "/v1/getexecutions"
                }}
            }},
            {"private", {
                {"get", {
                    "/v1/me/getbalance",
                    "/v1/me/getpositions",
                    "/v1/me/getchildorders",
                    "/v1/me/getexecutions",
                    "/v1/me/getdeposits",
                    "/v1/me/getwithdrawals"
                }},
                {"post", {
                    "/v1/me/sendchildorder",
                    "/v1/me/cancelchildorder",
                    "/v1/me/withdraw"
                }}
            }}
        }}
    });
}

// Market Data Implementation
Json bitflyer::fetchMarketsImpl() const {
    return this->fetch("/v1/getmarkets");
}

Json bitflyer::fetchTickerImpl(const std::string& symbol) const {
    auto market = this->market(symbol);
    return this->fetch("/v1/getticker?product_code=" + market["id"].get<std::string>());
}

Json bitflyer::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = "/v1/getboard?product_code=" + market["id"].get<std::string>();
    return this->fetch(request);
}

Json bitflyer::fetchTradesImpl(const std::string& symbol, const std::optional<long long>& since,
                            const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = "/v1/getexecutions?product_code=" + market["id"].get<std::string>();
    if (limit) {
        request += "&count=" + std::to_string(*limit);
    }
    return this->fetch(request);
}

// Trading Implementation
Json bitflyer::createOrderImpl(const std::string& symbol, const std::string& type,
                           const std::string& side, double amount,
                           const std::optional<double>& price) {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"product_code", market["id"]},
        {"child_order_type", type},
        {"side", side},
        {"size", this->amountToPrecision(symbol, amount)}
    });
    if (price) {
        request["price"] = this->priceToPrecision(symbol, *price);
    }
    return this->fetch("/v1/me/sendchildorder", "private", "POST", request);
}

Json bitflyer::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"product_code", market["id"]},
        {"child_order_id", id}
    });
    return this->fetch("/v1/me/cancelchildorder", "private", "POST", request);
}

Json bitflyer::fetchOrderImpl(const std::string& id, const std::string& symbol) const {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"product_code", market["id"]},
        {"child_order_id", id}
    });
    return this->fetch("/v1/me/getchildorders", "private", "GET", request);
}

Json bitflyer::fetchOrdersImpl(const std::string& symbol, const std::optional<long long>& since,
                           const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({{"product_code", market["id"]}});
    if (since) {
        request["since"] = *since;
    }
    if (limit) {
        request["count"] = *limit;
    }
    return this->fetch("/v1/me/getchildorders", "private", "GET", request);
}

Json bitflyer::fetchOpenOrdersImpl(const std::string& symbol, const std::optional<long long>& since,
                                const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"product_code", market["id"]},
        {"child_order_state", "ACTIVE"}
    });
    if (since) {
        request["since"] = *since;
    }
    if (limit) {
        request["count"] = *limit;
    }
    return this->fetch("/v1/me/getchildorders", "private", "GET", request);
}

Json bitflyer::fetchClosedOrdersImpl(const std::string& symbol, const std::optional<long long>& since,
                                  const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"product_code", market["id"]},
        {"child_order_state", "COMPLETED"}
    });
    if (since) {
        request["since"] = *since;
    }
    if (limit) {
        request["count"] = *limit;
    }
    return this->fetch("/v1/me/getchildorders", "private", "GET", request);
}

// Account Implementation
Json bitflyer::fetchBalanceImpl() const {
    return this->fetch("/v1/me/getbalance", "private", "GET");
}

Json bitflyer::fetchPositionsImpl(const std::string& symbols, const std::optional<long long>& since,
                               const std::optional<int>& limit) const {
    return this->fetch("/v1/me/getpositions", "private", "GET");
}

Json bitflyer::fetchMyTradesImpl(const std::string& symbol, const std::optional<long long>& since,
                              const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({{"product_code", market["id"]}});
    if (since) {
        request["since"] = *since;
    }
    if (limit) {
        request["count"] = *limit;
    }
    return this->fetch("/v1/me/getexecutions", "private", "GET", request);
}

Json bitflyer::fetchDepositsImpl(const std::string& code, const std::optional<long long>& since,
                              const std::optional<int>& limit) const {
    return this->fetch("/v1/me/getdeposits", "private", "GET");
}

Json bitflyer::fetchWithdrawalsImpl(const std::string& code, const std::optional<long long>& since,
                                 const std::optional<int>& limit) const {
    return this->fetch("/v1/me/getwithdrawals", "private", "GET");
}

Json bitflyer::withdrawImpl(const std::string& code, double amount, const std::string& address,
                         const std::string& tag, const Json& params) {
    auto request = Json::object({
        {"currency_code", code},
        {"amount", amount},
        {"address", address}
    });
    if (!tag.empty()) {
        request["payment_id"] = tag;
    }
    return this->fetch("/v1/me/withdraw", "private", "POST", this->extend(request, params));
}

// Async Implementation
boost::future<Json> bitflyer::fetchAsync(const std::string& path, const std::string& api,
                                      const std::string& method, const Json& params,
                                      const std::map<std::string, std::string>& headers) const {
    return Exchange::fetchAsync(path, api, method, params, headers);
}

boost::future<Json> bitflyer::fetchMarketsAsync() const {
    return this->fetchAsync("/v1/getmarkets");
}

boost::future<Json> bitflyer::fetchTickerAsync(const std::string& symbol) const {
    auto market = this->market(symbol);
    return this->fetchAsync("/v1/getticker?product_code=" + market["id"].get<std::string>());
}

boost::future<Json> bitflyer::fetchOrderBookAsync(const std::string& symbol,
                                               const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = "/v1/getboard?product_code=" + market["id"].get<std::string>();
    return this->fetchAsync(request);
}

boost::future<Json> bitflyer::fetchTradesAsync(const std::string& symbol,
                                            const std::optional<long long>& since,
                                            const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = "/v1/getexecutions?product_code=" + market["id"].get<std::string>();
    if (limit) {
        request += "&count=" + std::to_string(*limit);
    }
    return this->fetchAsync(request);
}

boost::future<Json> bitflyer::createOrderAsync(const std::string& symbol, const std::string& type,
                                           const std::string& side, double amount,
                                           const std::optional<double>& price) {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"product_code", market["id"]},
        {"child_order_type", type},
        {"side", side},
        {"size", this->amountToPrecision(symbol, amount)}
    });
    if (price) {
        request["price"] = this->priceToPrecision(symbol, *price);
    }
    return this->fetchAsync("/v1/me/sendchildorder", "private", "POST", request);
}

boost::future<Json> bitflyer::cancelOrderAsync(const std::string& id, const std::string& symbol) {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"product_code", market["id"]},
        {"child_order_id", id}
    });
    return this->fetchAsync("/v1/me/cancelchildorder", "private", "POST", request);
}

boost::future<Json> bitflyer::fetchOrderAsync(const std::string& id, const std::string& symbol) const {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"product_code", market["id"]},
        {"child_order_id", id}
    });
    return this->fetchAsync("/v1/me/getchildorders", "private", "GET", request);
}

boost::future<Json> bitflyer::fetchOrdersAsync(const std::string& symbol,
                                           const std::optional<long long>& since,
                                           const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({{"product_code", market["id"]}});
    if (since) {
        request["since"] = *since;
    }
    if (limit) {
        request["count"] = *limit;
    }
    return this->fetchAsync("/v1/me/getchildorders", "private", "GET", request);
}

boost::future<Json> bitflyer::fetchOpenOrdersAsync(const std::string& symbol,
                                               const std::optional<long long>& since,
                                               const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"product_code", market["id"]},
        {"child_order_state", "ACTIVE"}
    });
    if (since) {
        request["since"] = *since;
    }
    if (limit) {
        request["count"] = *limit;
    }
    return this->fetchAsync("/v1/me/getchildorders", "private", "GET", request);
}

boost::future<Json> bitflyer::fetchClosedOrdersAsync(const std::string& symbol,
                                                 const std::optional<long long>& since,
                                                 const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"product_code", market["id"]},
        {"child_order_state", "COMPLETED"}
    });
    if (since) {
        request["since"] = *since;
    }
    if (limit) {
        request["count"] = *limit;
    }
    return this->fetchAsync("/v1/me/getchildorders", "private", "GET", request);
}

boost::future<Json> bitflyer::fetchBalanceAsync() const {
    return this->fetchAsync("/v1/me/getbalance", "private", "GET");
}

boost::future<Json> bitflyer::fetchPositionsAsync(const std::string& symbols,
                                              const std::optional<long long>& since,
                                              const std::optional<int>& limit) const {
    return this->fetchAsync("/v1/me/getpositions", "private", "GET");
}

boost::future<Json> bitflyer::fetchMyTradesAsync(const std::string& symbol,
                                             const std::optional<long long>& since,
                                             const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({{"product_code", market["id"]}});
    if (since) {
        request["since"] = *since;
    }
    if (limit) {
        request["count"] = *limit;
    }
    return this->fetchAsync("/v1/me/getexecutions", "private", "GET", request);
}

boost::future<Json> bitflyer::fetchDepositsAsync(const std::string& code,
                                             const std::optional<long long>& since,
                                             const std::optional<int>& limit) const {
    return this->fetchAsync("/v1/me/getdeposits", "private", "GET");
}

boost::future<Json> bitflyer::fetchWithdrawalsAsync(const std::string& code,
                                                const std::optional<long long>& since,
                                                const std::optional<int>& limit) const {
    return this->fetchAsync("/v1/me/getwithdrawals", "private", "GET");
}

boost::future<Json> bitflyer::withdrawAsync(const std::string& code, double amount,
                                        const std::string& address, const std::string& tag,
                                        const Json& params) {
    auto request = Json::object({
        {"currency_code", code},
        {"amount", amount},
        {"address", address}
    });
    if (!tag.empty()) {
        request["payment_id"] = tag;
    }
    return this->fetchAsync("/v1/me/withdraw", "private", "POST", this->extend(request, params));
}

} // namespace ccxt
