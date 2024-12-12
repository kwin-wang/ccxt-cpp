#pragma once

#include "hitbtc.h"

namespace ccxt {

class Bequant : public HitBTC {
public:
    static const std::string defaultHostname;
    static const int defaultRateLimit;
    static const bool defaultPro;
    

    explicit Bequant(const Config& config = Config());
    virtual ~Bequant() = default;

    void init() override;

protected:
    // Market Data API
    json fetchMarketsImpl() const override;
    json fetchTickerImpl(const std::string& symbol) const override;
    json fetchTickersImpl(const std::vector<std::string>& symbols = {}) const override;
    json fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit = std::nullopt) const override;
    json fetchTradesImpl(const std::string& symbol, const std::optional<long long>& since = std::nullopt,
                      const std::optional<int>& limit = std::nullopt) const override;
    json fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe = "1m",
                     const std::optional<long long>& since = std::nullopt,
                     const std::optional<int>& limit = std::nullopt) const override;
    json fetchTimeImpl() const override;
    json fetchCurrenciesImpl() const override;
    json fetchTradingFeesImpl() const override;
    json fetchBalanceImpl() const override;
    json fetchDepositAddressImpl(const std::string& code, const json& params = json::object()) const override;
    json fetchDepositsImpl(const std::string& code = "", const std::optional<long long>& since = std::nullopt,
                        const std::optional<int>& limit = std::nullopt) const override;
    json fetchWithdrawalsImpl(const std::string& code = "", const std::optional<long long>& since = std::nullopt,
                          const std::optional<int>& limit = std::nullopt) const override;
    json fetchDepositsWithdrawalsImpl(const std::string& code = "", const std::optional<long long>& since = std::nullopt,
                                   const std::optional<int>& limit = std::nullopt) const override;
    json fetchDepositWithdrawFeesImpl() const override;
    json fetchFundingRatesImpl(const std::vector<std::string>& symbols = {}) const override;
    json fetchFundingRateHistoryImpl(const std::string& symbol, const std::optional<long long>& since = std::nullopt,
                                  const std::optional<int>& limit = std::nullopt) const override;
    json fetchLeverageImpl(const std::string& symbol) const override;
    json fetchMarginModesImpl(const std::vector<std::string>& symbols = {}) const override;
    json fetchPositionsImpl(const std::vector<std::string>& symbols = {}) const override;

    // Trading API
    json createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side,
                      double amount, const std::optional<double>& price = std::nullopt) override;
    json cancelOrderImpl(const std::string& id, const std::string& symbol = "") override;
    json cancelAllOrdersImpl(const std::string& symbol = "") override;
    json fetchOrderImpl(const std::string& id, const std::string& symbol = "") const override;
    json fetchOrdersImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt,
                      const std::optional<int>& limit = std::nullopt) const override;
    json fetchOpenOrdersImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt,
                         const std::optional<int>& limit = std::nullopt) const override;
    json fetchClosedOrdersImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt,
                           const std::optional<int>& limit = std::nullopt) const override;
    json setLeverageImpl(int leverage, const std::string& symbol = "") override;
    json setMarginModeImpl(const std::string& marginMode, const std::string& symbol = "") override;
    json addMarginImpl(const std::string& symbol, double amount) override;
    json reduceMarginImpl(const std::string& symbol, double amount) override;
    json transferImpl(const std::string& code, double amount, const std::string& fromAccount,
                   const std::string& toAccount) override;

private:
    // Helper methods
    std::string sign(const std::string& path, const std::string& api = "public",
                  const std::string& method = "GET", const json& params = json::object(),
                  const std::map<std::string, std::string>& headers = {},
                  const json& body = nullptr) const override;
};

} // namespace ccxt
