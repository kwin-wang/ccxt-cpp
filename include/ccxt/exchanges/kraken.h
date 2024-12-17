#ifndef CCXT_EXCHANGE_KRAKEN_H
#define CCXT_EXCHANGE_KRAKEN_H

#include "ccxt/base/exchange.h"
#include <future>

namespace ccxt {

class kraken : public Exchange {
public:
    kraken(const Config& config = Config());
    ~kraken() = default;

    static Exchange* create(const Config& config = Config()) {
        return new kraken(config);
    }

    // Sync Market Data
    Json fetchMarkets(const Json& params = Json::object()) override;
    Json fetchCurrencies(const Json& params = Json::object()) override;
    Json fetchTicker(const std::string& symbol, const Json& params = Json::object()) override;
    Json fetchTickers(const std::vector<std::string>& symbols = {}, const Json& params = Json::object()) override;
    Json fetchOrderBook(const std::string& symbol, const std::optional<int>& limit = std::nullopt, const Json& params = Json::object()) override;
    Json fetchOHLCV(const std::string& symbol, const std::string& timeframe, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt, const Json& params = Json::object()) override;
    Json fetchTrades(const std::string& symbol, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt, const Json& params = Json::object()) override;

    // Async Market Data
    AsyncPullType fetchMarketsAsync(const Json& params = Json::object()) const;
    AsyncPullType fetchCurrenciesAsync(const Json& params = Json::object()) const;
    AsyncPullType fetchTickerAsync(const std::string& symbol, const Json& params = Json::object()) const;
    AsyncPullType fetchTickersAsync(const std::vector<std::string>& symbols = {}, const Json& params = Json::object()) const;
    AsyncPullType fetchOrderBookAsync(const std::string& symbol, const std::optional<int>& limit = std::nullopt, const Json& params = Json::object()) const;
    AsyncPullType fetchOHLCVAsync(const std::string& symbol, const std::string& timeframe, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt, const Json& params = Json::object()) const;
    AsyncPullType fetchTradesAsync(const std::string& symbol, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt, const Json& params = Json::object()) const;

    // Sync Trading
    Json createOrder(const std::string& symbol, const std::string& type, const std::string& side, double amount, const std::optional<double>& price = std::nullopt, const Json& params = Json::object()) override;
    Json cancelOrder(const std::string& id, const std::string& symbol, const Json& params = Json::object()) override;
    Json fetchOrder(const std::string& id, const std::string& symbol, const Json& params = Json::object()) const override;
    Json fetchOpenOrders(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt, const Json& params = Json::object()) const override;
    Json fetchClosedOrders(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt, const Json& params = Json::object()) const override;
    Json fetchMyTrades(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt, const Json& params = Json::object()) const override;

    // Async Trading
    AsyncPullType createOrderAsync(const std::string& symbol, const std::string& type, const std::string& side, double amount, const std::optional<double>& price = std::nullopt, const Json& params = Json::object());
    AsyncPullType cancelOrderAsync(const std::string& id, const std::string& symbol, const Json& params = Json::object());
    AsyncPullType fetchOrderAsync(const std::string& id, const std::string& symbol, const Json& params = Json::object()) const;
    AsyncPullType fetchOpenOrdersAsync(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt, const Json& params = Json::object()) const;
    AsyncPullType fetchClosedOrdersAsync(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt, const Json& params = Json::object()) const;
    AsyncPullType fetchMyTradesAsync(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt, const Json& params = Json::object()) const;

    // Sync Account
    Json fetchBalance(const Json& params = Json::object()) override;
    Json fetchDepositAddress(const std::string& code, const std::optional<std::string>& network = std::nullopt, const Json& params = Json::object()) const override;
    Json fetchDeposits(const std::optional<std::string>& code = std::nullopt, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt, const Json& params = Json::object()) const override;
    Json fetchWithdrawals(const std::optional<std::string>& code = std::nullopt, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt, const Json& params = Json::object()) const override;

    // Async Account
    AsyncPullType fetchBalanceAsync(const Json& params = Json::object()) const;
    AsyncPullType fetchDepositAddressAsync(const std::string& code, const std::optional<std::string>& network = std::nullopt, const Json& params = Json::object()) const;
    AsyncPullType fetchDepositsAsync(const std::optional<std::string>& code = std::nullopt, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt, const Json& params = Json::object()) const;
    AsyncPullType fetchWithdrawalsAsync(const std::optional<std::string>& code = std::nullopt, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt, const Json& params = Json::object()) const;

protected:
    void init() override;
    Json describeImpl() const override;

    // Market Data
    Json fetchMarketsImpl(const Json& params = Json::object()) const override;
    Json fetchCurrenciesImpl(const Json& params = Json::object()) const override;
    Json fetchTickerImpl(const std::string& symbol, const Json& params = Json::object()) const override;
    Json fetchTickersImpl(const std::vector<std::string>& symbols = {}, const Json& params = Json::object()) const override;
    Json fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit = std::nullopt, const Json& params = Json::object()) const override;
    Json fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt, const Json& params = Json::object()) const override;
    Json fetchTradesImpl(const std::string& symbol, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt, const Json& params = Json::object()) const override;

    // Trading
    Json createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side, double amount, const std::optional<double>& price = std::nullopt, const Json& params = Json::object()) override;
    Json cancelOrderImpl(const std::string& id, const std::string& symbol, const Json& params = Json::object()) override;
    Json fetchOrderImpl(const std::string& id, const std::string& symbol, const Json& params = Json::object()) const override;
    Json fetchOpenOrdersImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt, const Json& params = Json::object()) const override;
    Json fetchClosedOrdersImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt, const Json& params = Json::object()) const override;
    Json fetchMyTradesImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt, const Json& params = Json::object()) const override;

    // Account
    Json fetchBalanceImpl(const Json& params = Json::object()) const override;
    Json fetchDepositAddressImpl(const std::string& code, const std::optional<std::string>& network = std::nullopt, const Json& params = Json::object()) const override;
    Json fetchDepositsImpl(const std::optional<std::string>& code = std::nullopt, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt, const Json& params = Json::object()) const override;
    Json fetchWithdrawalsImpl(const std::optional<std::string>& code = std::nullopt, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt, const Json& params = Json::object()) const override;

private:
    static Exchange* createInstance(const Config& config) {
        return new kraken(config);
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

    // Kraken specific methods
    std::string getNonce() const;
    std::string getKrakenSymbol(const std::string& symbol) const;
    std::string getCommonSymbol(const std::string& krakenSymbol) const;
};

} // namespace ccxt

#endif // CCXT_EXCHANGE_KRAKEN_H
