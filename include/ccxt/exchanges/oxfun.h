#ifndef CCXT_EXCHANGE_OXFUN_H
#define CCXT_EXCHANGE_OXFUN_H

#include "ccxt/base/exchange.h"


namespace ccxt {

class oxfun : public Exchange {
public:
    oxfun(const Config& config = Config());
    ~oxfun() = default;

    static Exchange* create(const Config& config = Config()) {
        return new oxfun(config);
    }

protected:
    void init() override;
    Json describeImpl() const override;

    // Market Data
    Json fetchMarketsImpl() const override;
    Json fetchCurrenciesImpl() const override;
    Json fetchTickerImpl(const std::string& symbol) const override;
    Json fetchTickersImpl(const std::vector<std::string>& symbols = {}) const override;
    Json fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;

    // Async Market Data
    std::future<Json> fetchMarketsAsync() const;
    std::future<Json> fetchCurrenciesAsync() const;
    std::future<Json> fetchTickerAsync(const std::string& symbol) const;
    std::future<Json> fetchTickersAsync(const std::vector<std::string>& symbols = {}) const;
    std::future<Json> fetchOrderBookAsync(const std::string& symbol, const std::optional<int>& limit = std::nullopt) const;
    std::future<Json> fetchOHLCVAsync(const std::string& symbol, const std::string& timeframe,
                                     const std::optional<long long>& since = std::nullopt,
                                     const std::optional<int>& limit = std::nullopt) const;

    // Trading
    Json createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side, double amount, const std::optional<double>& price = std::nullopt) override;
    Json cancelOrderImpl(const std::string& id, const std::string& symbol) override;
    Json fetchOrderImpl(const std::string& id, const std::string& symbol) const override;
    Json fetchOpenOrdersImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchClosedOrdersImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchMyTradesImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;

    // Async Trading
    std::future<Json> createOrderAsync(const std::string& symbol, const std::string& type,
                                     const std::string& side, double amount,
                                     const std::optional<double>& price = std::nullopt);
    std::future<Json> cancelOrderAsync(const std::string& id, const std::string& symbol);
    std::future<Json> fetchOrderAsync(const std::string& id, const std::string& symbol) const;
    std::future<Json> fetchOpenOrdersAsync(const std::string& symbol = "",
                                         const std::optional<long long>& since = std::nullopt,
                                         const std::optional<int>& limit = std::nullopt) const;
    std::future<Json> fetchClosedOrdersAsync(const std::string& symbol = "",
                                           const std::optional<long long>& since = std::nullopt,
                                           const std::optional<int>& limit = std::nullopt) const;
    std::future<Json> fetchMyTradesAsync(const std::string& symbol = "",
                                        const std::optional<long long>& since = std::nullopt,
                                        const std::optional<int>& limit = std::nullopt) const;

    // Account
    Json fetchBalanceImpl() const override;
    Json fetchDepositAddressImpl(const std::string& code, const std::optional<std::string>& network = std::nullopt) const override;
    Json fetchDepositsImpl(const std::optional<std::string>& code = std::nullopt, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchWithdrawalsImpl(const std::optional<std::string>& code = std::nullopt, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;

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
    static Exchange* createInstance(const Config& config) {
        return new oxfun(config);
    }

    static const std::string defaultBaseURL;
    static const std::string defaultVersion;
    static const int defaultRateLimit;
    static const bool defaultPro;

    

    // Helper methods for parsing responses
    Json parseTicker(const Json& ticker, const Json& market = Json()) const;
    Json parseTrade(const Json& trade, const Json& market = Json()) const;
    Json parseOrder(const Json& order, const Json& market = Json()) const;
    Json parseTransaction(const Json& transaction, const Json& currency = Json()) const;

    // Authentication helpers
    std::string sign(const std::string& path, const std::string& api = "public", const std::string& method = "GET",
                    const Json& params = Json::object(), const Json& headers = Json::object(), const Json& body = Json::object()) const;

    // Error handling
    void handleErrors(const std::string& code, const std::string& reason, const std::string& url, const std::string& method,
                     const Json& headers, const Json& body, const Json& response, const std::string& requestHeaders,
                     const std::string& requestBody) const;
};

} // namespace ccxt

#endif // CCXT_EXCHANGE_OXFUN_H
