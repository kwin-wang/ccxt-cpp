#ifndef CCXT_EXCHANGE_ALPACA_H
#define CCXT_EXCHANGE_ALPACA_H

#include "../exchange.h"
#include "../exchange_impl.h"

namespace ccxt {

class alpaca : public ExchangeImpl {
public:
    alpaca(const Config& config = Config());
    ~alpaca() = default;

    static Exchange* create(const Config& config = Config()) {
        return new alpaca(config);
    }

protected:
    void init() override;
    Json describeImpl() const override;

    // Market Data
    Json fetchMarketsImpl() const override;
    Json fetchTickerImpl(const std::string& symbol) const override;
    Json fetchTickersImpl(const std::vector<std::string>& symbols = {}) const override;
    Json fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchTradesImpl(const std::string& symbol, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchTimeImpl() const override;

    // Trading
    Json createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side, double amount, const std::optional<double>& price = std::nullopt) override;
    Json cancelOrderImpl(const std::string& id, const std::string& symbol) override;
    Json cancelAllOrdersImpl(const std::string& symbol = "") override;
    Json fetchOrderImpl(const std::string& id, const std::string& symbol) const override;
    Json fetchOrdersImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchOpenOrdersImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchClosedOrdersImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchMyTradesImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;

    // Account
    Json fetchDepositAddressImpl(const std::string& code, const std::optional<std::string>& network = std::nullopt) const override;
    Json fetchDepositsImpl(const std::string& code = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchDepositsWithdrawalsImpl(const std::string& code = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;

private:
    static Exchange* createInstance(const Config& config) {
        return new alpaca(config);
    }

    static const std::string defaultHostname;
    static const std::string defaultBaseURL;
    static const int defaultRateLimit;
    static const bool defaultPro;

    static ExchangeRegistry::Factory factory;

    Json parseOrder(const Json& order, const std::optional<Json>& market = std::nullopt) const;
    Json parseOrders(const Json& orders, const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const;
    Json parseTrade(const Json& trade, const std::optional<Json>& market = std::nullopt) const;
    Json parseTrades(const Json& trades, const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const;
};

} // namespace ccxt

#endif // CCXT_EXCHANGE_ALPACA_H
