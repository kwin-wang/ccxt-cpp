#ifndef CCXT_EXCHANGE_COINMATE_H
#define CCXT_EXCHANGE_COINMATE_H

#include "ccxt/base/exchange.h"
#include <future>

namespace ccxt {

class coinmate : public Exchange {
public:
    coinmate(const Config& config = Config());
    ~coinmate() = default;

    static Exchange* create(const Config& config = Config()) {
        return new coinmate(config);
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
    Json fetchDepositsWithdrawalsImpl(const std::optional<std::string>& code = std::nullopt, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;

    // Async Methods
    // Market Data
    AsyncPullType fetchMarketsAsync() const;
    AsyncPullType fetchTickerAsync(const std::string& symbol) const;
    AsyncPullType fetchTickersAsync(const std::vector<std::string>& symbols = {}) const;
    AsyncPullType fetchOrderBookAsync(const std::string& symbol, const std::optional<int>& limit = std::nullopt) const;
    AsyncPullType fetchTradesAsync(const std::string& symbol, const std::optional<int>& limit = std::nullopt) const;
    AsyncPullType fetchOHLCVAsync(const std::string& symbol, const std::string& timeframe, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const;

    // Trading
    AsyncPullType createOrderAsync(const std::string& symbol, const std::string& type, const std::string& side, double amount, const std::optional<double>& price = std::nullopt);
    AsyncPullType cancelOrderAsync(const std::string& id, const std::string& symbol);
    AsyncPullType fetchOrderAsync(const std::string& id, const std::string& symbol) const;
    AsyncPullType fetchOrdersAsync(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const;
    AsyncPullType fetchOpenOrdersAsync(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const;
    AsyncPullType fetchClosedOrdersAsync(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const;
    AsyncPullType fetchMyTradesAsync(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const;

    // Account
    AsyncPullType fetchBalanceAsync() const;
    AsyncPullType fetchDepositsWithdrawalsAsync(const std::optional<std::string>& code = std::nullopt, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const;

private:
    static Exchange* createInstance(const Config& config) {
        return new coinmate(config);
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

#endif // CCXT_EXCHANGE_COINMATE_H
