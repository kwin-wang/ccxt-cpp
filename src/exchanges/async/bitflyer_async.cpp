#include "ccxt/exchanges/async/bitflyer_async.h"

namespace ccxt {

BitflyerAsync::BitflyerAsync(const boost::asio::io_context& context, const Config& config)
    : ExchangeAsync(context)
    , bitflyer(config) {}

boost::future<Json> BitflyerAsync::fetchAsync(const std::string& path, const std::string& api,
                                           const std::string& method, const Json& params,
                                           const std::map<std::string, std::string>& headers) const {
    return ExchangeAsync::fetchAsync(path, api, method, params, headers);
}

// Market Data Implementation
boost::future<Json> BitflyerAsync::fetchMarketsAsync() const {
    return fetchAsync("/v1/getmarkets");
}

boost::future<Json> BitflyerAsync::fetchTickerAsync(const std::string& symbol) const {
    auto market = this->market(symbol);
    return fetchAsync("/v1/getticker?product_code=" + market["id"].get<std::string>());
}

boost::future<Json> BitflyerAsync::fetchOrderBookAsync(const std::string& symbol, const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = "/v1/getboard?product_code=" + market["id"].get<std::string>();
    return fetchAsync(request);
}

boost::future<Json> BitflyerAsync::fetchTradesAsync(const std::string& symbol,
                                                 const std::optional<long long>& since,
                                                 const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = "/v1/getexecutions?product_code=" + market["id"].get<std::string>();
    if (limit) {
        request += "&count=" + std::to_string(*limit);
    }
    return fetchAsync(request);
}

// Trading Implementation
boost::future<Json> BitflyerAsync::createOrderAsync(const std::string& symbol, const std::string& type,
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
    return fetchAsync("/v1/sendchildorder", "private", "POST", request);
}

boost::future<Json> BitflyerAsync::cancelOrderAsync(const std::string& id, const std::string& symbol) {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"product_code", market["id"]},
        {"child_order_id", id}
    });
    return fetchAsync("/v1/cancelchildorder", "private", "POST", request);
}

boost::future<Json> BitflyerAsync::fetchOrderAsync(const std::string& id, const std::string& symbol) const {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"product_code", market["id"]},
        {"child_order_id", id}
    });
    return fetchAsync("/v1/getchildorders", "private", "GET", request);
}

boost::future<Json> BitflyerAsync::fetchOrdersAsync(const std::string& symbol,
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
    return fetchAsync("/v1/getchildorders", "private", "GET", request);
}

boost::future<Json> BitflyerAsync::fetchOpenOrdersAsync(const std::string& symbol,
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
    return fetchAsync("/v1/getchildorders", "private", "GET", request);
}

boost::future<Json> BitflyerAsync::fetchClosedOrdersAsync(const std::string& symbol,
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
    return fetchAsync("/v1/getchildorders", "private", "GET", request);
}

// Account Implementation
boost::future<Json> BitflyerAsync::fetchBalanceAsync() const {
    return fetchAsync("/v1/getbalance", "private", "GET");
}

boost::future<Json> BitflyerAsync::fetchPositionsAsync(const std::string& symbols,
                                                   const std::optional<long long>& since,
                                                   const std::optional<int>& limit) const {
    return fetchAsync("/v1/getpositions", "private", "GET");
}

boost::future<Json> BitflyerAsync::fetchMyTradesAsync(const std::string& symbol,
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
    return fetchAsync("/v1/getexecutions", "private", "GET", request);
}

boost::future<Json> BitflyerAsync::fetchDepositsAsync(const std::string& code,
                                                  const std::optional<long long>& since,
                                                  const std::optional<int>& limit) const {
    return fetchAsync("/v1/getdeposits", "private", "GET");
}

boost::future<Json> BitflyerAsync::fetchWithdrawalsAsync(const std::string& code,
                                                     const std::optional<long long>& since,
                                                     const std::optional<int>& limit) const {
    return fetchAsync("/v1/getwithdrawals", "private", "GET");
}

boost::future<Json> BitflyerAsync::withdrawAsync(const std::string& code, double amount,
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
    return fetchAsync("/v1/withdraw", "private", "POST", this->extend(request, params));
}

} // namespace ccxt
