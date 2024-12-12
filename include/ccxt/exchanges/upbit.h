#ifndef CCXT_UPBIT_H
#define CCXT_UPBIT_H

#include "../exchange.h"
#include <future>

namespace ccxt {

class upbit : public ExchangeImpl {
public:
    explicit upbit(const Config& config = Config{});
    void init() override;
    Json describeImpl() const override;

    // Market Data
    Json fetchMarketsImpl() const override;
    Json fetchCurrenciesImpl() const override;
    Json fetchTickerImpl(const std::string& symbol) const override;
    Json fetchTickersImpl(const std::vector<std::string>& symbols = {}) const override;
    Json fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchTradesImpl(const std::string& symbol, const std::optional<int>& limit = std::nullopt,
                        const std::optional<long long>& since = std::nullopt) const override;
    Json fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe,
                       const std::optional<long long>& since = std::nullopt,
                       const std::optional<int>& limit = std::nullopt) const override;

    // Trading
    Json createOrderImpl(const std::string& symbol, const std::string& type,
                        const std::string& side, double amount,
                        const std::optional<double>& price = std::nullopt) override;
    Json cancelOrderImpl(const std::string& id, const std::string& symbol) override;
    Json fetchOrderImpl(const std::string& id, const std::string& symbol) const override;
    Json fetchOrdersImpl(const std::string& symbol = "",
                        const std::optional<long long>& since = std::nullopt,
                        const std::optional<int>& limit = std::nullopt) const override;
    Json fetchOpenOrdersImpl(const std::string& symbol = "",
                           const std::optional<long long>& since = std::nullopt,
                           const std::optional<int>& limit = std::nullopt) const override;
    Json fetchClosedOrdersImpl(const std::string& symbol = "",
                             const std::optional<long long>& since = std::nullopt,
                             const std::optional<int>& limit = std::nullopt) const override;
    Json fetchMyTradesImpl(const std::string& symbol = "",
                          const std::optional<long long>& since = std::nullopt,
                          const std::optional<int>& limit = std::nullopt) const override;

    // Account
    Json fetchBalanceImpl() const override;
    Json fetchDepositAddressImpl(const std::string& code,
                                const std::optional<std::string>& network = std::nullopt) const override;
    Json fetchDepositsImpl(const std::optional<std::string>& code = std::nullopt,
                          const std::optional<long long>& since = std::nullopt,
                          const std::optional<int>& limit = std::nullopt) const override;
    Json fetchWithdrawalsImpl(const std::optional<std::string>& code = std::nullopt,
                             const std::optional<long long>& since = std::nullopt,
                             const std::optional<int>& limit = std::nullopt) const override;

    // Async Market Data
    std::future<Json> fetchMarketsAsync() const;
    std::future<Json> fetchCurrenciesAsync() const;
    std::future<Json> fetchTickerAsync(const std::string& symbol) const;
    std::future<Json> fetchTickersAsync(const std::vector<std::string>& symbols = {}) const;
    std::future<Json> fetchOrderBookAsync(const std::string& symbol,
                                        const std::optional<int>& limit = std::nullopt) const;
    std::future<Json> fetchTradesAsync(const std::string& symbol,
                                      const std::optional<int>& limit = std::nullopt,
                                      const std::optional<long long>& since = std::nullopt) const;
    std::future<Json> fetchOHLCVAsync(const std::string& symbol,
                                     const std::string& timeframe,
                                     const std::optional<long long>& since = std::nullopt,
                                     const std::optional<int>& limit = std::nullopt) const;

    // Async Trading
    std::future<Json> createOrderAsync(const std::string& symbol,
                                     const std::string& type,
                                     const std::string& side,
                                     double amount,
                                     const std::optional<double>& price = std::nullopt);
    std::future<Json> cancelOrderAsync(const std::string& id,
                                     const std::string& symbol);
    std::future<Json> fetchOrderAsync(const std::string& id,
                                    const std::string& symbol) const;
    std::future<Json> fetchOrdersAsync(const std::string& symbol = "",
                                     const std::optional<long long>& since = std::nullopt,
                                     const std::optional<int>& limit = std::nullopt) const;
    std::future<Json> fetchOpenOrdersAsync(const std::string& symbol = "",
                                         const std::optional<long long>& since = std::nullopt,
                                         const std::optional<int>& limit = std::nullopt) const;
    std::future<Json> fetchClosedOrdersAsync(const std::string& symbol = "",
                                           const std::optional<long long>& since = std::nullopt,
                                           const std::optional<int>& limit = std::nullopt) const;
    std::future<Json> fetchMyTradesAsync(const std::string& symbol = "",
                                        const std::optional<long long>& since = std::nullopt,
                                        const std::optional<int>& limit = std::nullopt) const;

    // Async Account
    std::future<Json> fetchBalanceAsync() const;
    std::future<Json> fetchDepositAddressAsync(const std::string& code,
                                              const std::optional<std::string>& network = std::nullopt) const;
    std::future<Json> fetchDepositsAsync(const std::optional<std::string>& code = std::nullopt,
                                        const std::optional<long long>& since = std::nullopt,
                                        const std::optional<int>& limit = std::nullopt) const;
    std::future<Json> fetchWithdrawalsAsync(const std::optional<std::string>& code = std::nullopt,
                                           const std::optional<long long>& since = std::nullopt,
                                           const std::optional<int>& limit = std::nullopt) const;

private:
    static const std::string defaultBaseURL;
    static const std::string defaultVersion;
    static const int defaultRateLimit;

    // Helper methods
    Json parseTicker(const Json& ticker, const Json& market) const;
    Json parseOrder(const Json& order, const Json& market) const;
    Json parseTrade(const Json& trade, const Json& market) const;
    Json parseOHLCV(const Json& ohlcv) const;
    Json parseBalance(const Json& balance) const;
    Json parseTransaction(const Json& transaction, const std::string& currency) const;
    std::string parseOrderStatus(const std::string& status) const;
    std::string getUpbitSymbol(const std::string& symbol) const;
    std::string getCommonSymbol(const std::string& upbitSymbol) const;

    std::string sign(const std::string& path, const std::string& api,
                    const std::string& method, const Json& params,
                    const Json& headers, const Json& body) const override;
    void handleErrors(const std::string& code, const std::string& reason,
                     const std::string& url, const std::string& method,
                     const Json& headers, const Json& body,
                     const Json& response, const std::string& requestHeaders,
                     const std::string& requestBody) const override;
};

} // namespace ccxt

#endif // CCXT_UPBIT_H
