#pragma once

#include "ccxt/base/exchange_impl.h"
#include "ccxt/base/exchange_registry.h"

namespace ccxt {

class ace : public ExchangeImpl<ace> {
public:
    static const std::string defaultBaseURL;
    static const int defaultRateLimit;
    static const bool defaultPro;
    static ExchangeRegistry::Factory factory;

    explicit ace(const Config& config);
    virtual ~ace() {}

protected:
    virtual void init() override;
    virtual json describeImpl() const override;
    virtual json fetchMarketsImpl() const override;
    virtual json fetchTickerImpl(const std::string& symbol) const override;
    virtual json fetchTickersImpl(const std::vector<std::string>& symbols) const override;
    virtual json fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const override;
    virtual json fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe,
                             const std::optional<long long>& since,
                             const std::optional<int>& limit) const override;
    virtual json createOrderImpl(const std::string& symbol, const std::string& type,
                             const std::string& side, double amount,
                             const std::optional<double>& price) override;
    virtual json cancelOrderImpl(const std::string& id, const std::string& symbol) override;
    virtual json fetchOrderImpl(const std::string& id, const std::string& symbol) const override;
    virtual json fetchOpenOrdersImpl(const std::string& symbol,
                                 const std::optional<long long>& since,
                                 const std::optional<int>& limit) const override;
    virtual json fetchMyTradesImpl(const std::string& symbol,
                                const std::optional<long long>& since,
                                const std::optional<int>& limit) const override;
    virtual json fetchOrderTradesImpl(const std::string& id, const std::string& symbol) const override;
    virtual json fetchBalanceImpl() const override;

    virtual json parseOrder(const json& order, const std::optional<json>& market = std::nullopt) const;
    virtual json parseOrders(const json& orders, const std::string& symbol,
                         const std::optional<long long>& since = std::nullopt,
                         const std::optional<int>& limit = std::nullopt) const;
    virtual json parseTrade(const json& trade, const std::optional<json>& market = std::nullopt) const;
    virtual json parseTrades(const json& trades, const std::string& symbol,
                         const std::optional<long long>& since = std::nullopt,
                         const std::optional<int>& limit = std::nullopt) const;

private:
    static Exchange* createInstance(const Config& config) {
        return new ace(config);
    }
};

} // namespace ccxt
