#ifndef CCXT_POLONIEXFUTURES_H
#define CCXT_POLONIEXFUTURES_H

#include "../exchange.h"
#include <future>

namespace ccxt {

class poloniexfutures : public ExchangeImpl {
public:
    explicit poloniexfutures(const Config& config = Config{});
    void init() override;
    Json describeImpl() const override;

    // Market Data
    Json fetchMarketsImpl() const override;
    Json fetchTimeImpl() const override;
    Json fetchTickerImpl(const std::string& symbol) const override;
    Json fetchTickersImpl(const std::vector<std::string>& symbols = {}) const override;
    Json fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe,
                       const std::optional<long long>& since = std::nullopt,
                       const std::optional<int>& limit = std::nullopt) const override;
    Json fetchTradesImpl(const std::string& symbol, const std::optional<int>& limit = std::nullopt,
                        const std::optional<long long>& since = std::nullopt) const override;

    // Trading
    Json createOrderImpl(const std::string& symbol, const std::string& type,
                        const std::string& side, double amount,
                        const std::optional<double>& price = std::nullopt) override;
    Json createStopOrderImpl(const std::string& symbol, const std::string& type,
                           const std::string& side, double amount,
                           const std::optional<double>& price = std::nullopt,
                           const Json& params = Json::object()) override;
    Json cancelOrderImpl(const std::string& id, const std::string& symbol) override;
    Json cancelAllOrdersImpl(const std::string& symbol = "") override;
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
    Json fetchPositionsImpl(const std::vector<std::string>& symbols = {}) const override;
    Json setMarginModeImpl(const std::string& marginMode,
                          const std::string& symbol = "",
                          const Json& params = Json::object()) override;

    // Funding
    Json fetchFundingRateImpl(const std::string& symbol) const override;
    Json fetchFundingRateHistoryImpl(const std::string& symbol,
                                    const std::optional<long long>& since = std::nullopt,
                                    const std::optional<int>& limit = std::nullopt) const override;
    Json fetchFundingIntervalImpl(const std::string& symbol) const override;

    // Async Market Data
    AsyncPullType fetchMarketsAsync() const;
    AsyncPullType fetchTimeAsync() const;
    AsyncPullType fetchTickerAsync(const std::string& symbol) const;
    AsyncPullType fetchTickersAsync(const std::vector<std::string>& symbols = {}) const;
    AsyncPullType fetchOrderBookAsync(const std::string& symbol,
                                        const std::optional<int>& limit = std::nullopt) const;
    AsyncPullType fetchOHLCVAsync(const std::string& symbol,
                                     const std::string& timeframe,
                                     const std::optional<long long>& since = std::nullopt,
                                     const std::optional<int>& limit = std::nullopt) const;
    AsyncPullType fetchTradesAsync(const std::string& symbol,
                                      const std::optional<int>& limit = std::nullopt,
                                      const std::optional<long long>& since = std::nullopt) const;

    // Async Trading
    AsyncPullType createOrderAsync(const std::string& symbol,
                                     const std::string& type,
                                     const std::string& side,
                                     double amount,
                                     const std::optional<double>& price = std::nullopt);
    AsyncPullType createStopOrderAsync(const std::string& symbol,
                                         const std::string& type,
                                         const std::string& side,
                                         double amount,
                                         const std::optional<double>& price = std::nullopt,
                                         const Json& params = Json::object());
    AsyncPullType cancelOrderAsync(const std::string& id,
                                     const std::string& symbol);
    AsyncPullType cancelAllOrdersAsync(const std::string& symbol = "");
    AsyncPullType fetchOrderAsync(const std::string& id,
                                    const std::string& symbol) const;
    AsyncPullType fetchOrdersAsync(const std::string& symbol = "",
                                     const std::optional<long long>& since = std::nullopt,
                                     const std::optional<int>& limit = std::nullopt) const;
    AsyncPullType fetchOpenOrdersAsync(const std::string& symbol = "",
                                         const std::optional<long long>& since = std::nullopt,
                                         const std::optional<int>& limit = std::nullopt) const;
    AsyncPullType fetchClosedOrdersAsync(const std::string& symbol = "",
                                           const std::optional<long long>& since = std::nullopt,
                                           const std::optional<int>& limit = std::nullopt) const;
    AsyncPullType fetchMyTradesAsync(const std::string& symbol = "",
                                        const std::optional<long long>& since = std::nullopt,
                                        const std::optional<int>& limit = std::nullopt) const;

    // Async Account
    AsyncPullType fetchBalanceAsync() const;
    AsyncPullType fetchPositionsAsync(const std::vector<std::string>& symbols = {}) const;
    AsyncPullType setMarginModeAsync(const std::string& marginMode,
                                        const std::string& symbol = "",
                                        const Json& params = Json::object());

    // Async Funding
    AsyncPullType fetchFundingRateAsync(const std::string& symbol) const;
    AsyncPullType fetchFundingRateHistoryAsync(const std::string& symbol,
                                                  const std::optional<long long>& since = std::nullopt,
                                                  const std::optional<int>& limit = std::nullopt) const;
    AsyncPullType fetchFundingIntervalAsync(const std::string& symbol) const;

private:
    static const std::string defaultBaseURL;
    static const std::string defaultVersion;
    static const int defaultRateLimit;
    static const bool defaultPro;

    // Helper methods
    Json parseTicker(const Json& ticker, const Json& market) const;
    Json parseOrder(const Json& order, const Json& market) const;
    Json parseTrade(const Json& trade, const Json& market) const;
    Json parseOHLCV(const Json& ohlcv) const;
    Json parsePosition(const Json& position, const Json& market = Json::object()) const;
    Json parseFundingRate(const Json& fundingRate, const Json& market = Json::object()) const;
    std::string parseOrderStatus(const std::string& status) const;
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

#endif // CCXT_POLONIEXFUTURES_H
