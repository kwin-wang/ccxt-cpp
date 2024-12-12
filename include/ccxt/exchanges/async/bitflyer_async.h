#pragma once

#include "ccxt/exchanges/async/exchange_async.h"
#include "ccxt/exchanges/bitflyer.h"

namespace ccxt {

class BitflyerAsync : public ExchangeAsync, public bitflyer {
public:
    explicit BitflyerAsync(const boost::asio::io_context& context, const Config& config = Config());
    ~BitflyerAsync() override = default;

    // Market Data
    boost::future<Json> fetchMarketsAsync() const;
    boost::future<Json> fetchTickerAsync(const std::string& symbol) const;
    boost::future<Json> fetchOrderBookAsync(const std::string& symbol, const std::optional<int>& limit = std::nullopt) const;
    boost::future<Json> fetchTradesAsync(const std::string& symbol, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const;

    // Trading
    boost::future<Json> createOrderAsync(const std::string& symbol, const std::string& type, const std::string& side, double amount, const std::optional<double>& price = std::nullopt);
    boost::future<Json> cancelOrderAsync(const std::string& id, const std::string& symbol);
    boost::future<Json> fetchOrderAsync(const std::string& id, const std::string& symbol) const;
    boost::future<Json> fetchOrdersAsync(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const;
    boost::future<Json> fetchOpenOrdersAsync(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const;
    boost::future<Json> fetchClosedOrdersAsync(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const;
    boost::future<Json> fetchMyTradesAsync(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const;

    // Account
    boost::future<Json> fetchBalanceAsync() const;
    boost::future<Json> fetchPositionsAsync(const std::string& symbols = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const;
    boost::future<Json> fetchDepositsAsync(const std::string& code = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const;
    boost::future<Json> fetchWithdrawalsAsync(const std::string& code = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const;
    boost::future<Json> withdrawAsync(const std::string& code, double amount, const std::string& address, const std::string& tag = "", const Json& params = Json::object());

protected:
    boost::future<Json> fetchAsync(const std::string& path, const std::string& api = "public",
                                 const std::string& method = "GET", const Json& params = Json::object(),
                                 const std::map<std::string, std::string>& headers = {}) const;
};

} // namespace ccxt
