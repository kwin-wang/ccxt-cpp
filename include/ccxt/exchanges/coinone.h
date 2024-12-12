#ifndef CCXT_EXCHANGE_COINONE_H
#define CCXT_EXCHANGE_COINONE_H

#include "ccxt/base/exchange.h"
#include <future>

namespace ccxt {

class coinone : public Exchange {
public:
    coinone(const Config& config = Config());
    ~coinone() = default;

    static Exchange* create(const Config& config = Config()) {
        return new coinone(config);
    }

protected:
    void init() override;
    Json describeImpl() const override;

    // Market Data
    Json fetchMarketsImpl() const override;
    Json fetchTickerImpl(const std::string& symbol) const override;
    Json fetchTickersImpl(const std::vector<std::string>& symbols = {}) const override;
    Json fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchTradesImpl(const std::string& symbol, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;

    // Trading
    Json createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side, double amount, const std::optional<double>& price = std::nullopt) override;
    Json cancelOrderImpl(const std::string& id, const std::string& symbol) override;
    Json fetchOrderImpl(const std::string& id, const std::string& symbol) const override;
    Json fetchOrdersImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchOpenOrdersImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchClosedOrdersImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchMyTradesImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;

    // Account
    Json fetchBalanceImpl() const override;
    Json fetchDepositAddressImpl(const std::string& code, const std::optional<std::string>& chain = std::nullopt) const override;
    Json fetchDepositsImpl(const std::optional<std::string>& code = std::nullopt, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchWithdrawalsImpl(const std::optional<std::string>& code = std::nullopt, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json withdrawImpl(const std::string& code, double amount, const std::string& address, const std::optional<std::string>& tag = std::nullopt) override;

    // Async Methods
    // Market Data
    std::future<Json> fetchMarketsAsync() const;
    std::future<Json> fetchTickerAsync(const std::string& symbol) const;
    std::future<Json> fetchTickersAsync(const std::vector<std::string>& symbols = {}) const;
    std::future<Json> fetchOrderBookAsync(const std::string& symbol, const std::optional<int>& limit = std::nullopt) const;
    std::future<Json> fetchTradesAsync(const std::string& symbol, const std::optional<int>& limit = std::nullopt) const;
    std::future<Json> fetchOHLCVAsync(const std::string& symbol, const std::string& timeframe, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const;

    // Trading
    std::future<Json> createOrderAsync(const std::string& symbol, const std::string& type, const std::string& side, double amount, const std::optional<double>& price = std::nullopt);
    std::future<Json> cancelOrderAsync(const std::string& id, const std::string& symbol);
    std::future<Json> fetchOrderAsync(const std::string& id, const std::string& symbol) const;
    std::future<Json> fetchOrdersAsync(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const;
    std::future<Json> fetchOpenOrdersAsync(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const;
    std::future<Json> fetchClosedOrdersAsync(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const;
    std::future<Json> fetchMyTradesAsync(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const;

    // Account
    std::future<Json> fetchBalanceAsync() const;
    std::future<Json> fetchDepositAddressAsync(const std::string& code, const std::optional<std::string>& chain = std::nullopt) const;
    std::future<Json> fetchDepositsAsync(const std::optional<std::string>& code = std::nullopt, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const;
    std::future<Json> fetchWithdrawalsAsync(const std::optional<std::string>& code = std::nullopt, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const;
    std::future<Json> withdrawAsync(const std::string& code, double amount, const std::string& address, const std::optional<std::string>& tag = std::nullopt);

private:
    static Exchange* createInstance(const Config& config) {
        return new coinone(config);
    }

    static const std::string defaultBaseURL;
    static const std::string defaultVersion;
    static const int defaultRateLimit;
    static const bool defaultPro;

    Json parseTicker(const Json& ticker, const Json& market = Json()) const;
    Json parseTrade(const Json& trade, const Json& market = Json()) const;
    Json parseOrder(const Json& order, const Json& market = Json()) const;
    std::string getCurrencyId(const std::string& code) const;
};

} // namespace ccxt

#endif // CCXT_EXCHANGE_COINONE_H
