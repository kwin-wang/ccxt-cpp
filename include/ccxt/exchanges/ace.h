#ifndef CCXT_EXCHANGE_ACE_H
#define CCXT_EXCHANGE_ACE_H

#include "../exchange.h"
#include "../exchange_impl.h"

namespace ccxt {

class ace : public ExchangeImpl {
public:
    ace(const Config& config = Config());
    ~ace() = default;

    static Exchange* create(const Config& config = Config()) {
        return new ace(config);
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

    // Trading
    Json createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side, double amount, const std::optional<double>& price = std::nullopt) override;
    Json cancelOrderImpl(const std::string& id, const std::string& symbol) override;
    Json fetchOrderImpl(const std::string& id, const std::string& symbol) const override;
    Json fetchOpenOrdersImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchMyTradesImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchOrderTradesImpl(const std::string& id, const std::string& symbol) const override;

    // Account
    Json fetchBalanceImpl() const override;

private:
    static Exchange* createInstance(const Config& config) {
        return new ace(config);
    }

    static const std::string defaultBaseURL;
    static const std::string defaultVersion;
    static const int defaultRateLimit;
    static const bool defaultPro;

    static ExchangeRegistry::Factory factory;
};

} // namespace ccxt

#endif // CCXT_EXCHANGE_ACE_H
