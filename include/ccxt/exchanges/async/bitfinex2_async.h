#pragma once

#include "ccxt/exchanges/async/exchange_async.h"
#include "ccxt/exchanges/bitfinex2.h"

namespace ccxt {

class Bitfinex2Async : public ExchangeAsync, public bitfinex2 {
public:
    explicit Bitfinex2Async(const boost::asio::io_context& context, const Config& config = Config());
    ~Bitfinex2Async() override = default;

    // Market Data
    boost::future<Json> fetchMarketsAsync() const;
    boost::future<Json> fetchCurrenciesAsync() const;
    boost::future<Json> fetchTickerAsync(const std::string& symbol) const;
    boost::future<Json> fetchTickersAsync(const std::vector<std::string>& symbols = {}) const;
    boost::future<Json> fetchOrderBookAsync(const std::string& symbol, const std::optional<int>& limit = std::nullopt) const;
    boost::future<Json> fetchOHLCVAsync(const std::string& symbol, const std::string& timeframe,
                                      const std::optional<long long>& since = std::nullopt,
                                      const std::optional<int>& limit = std::nullopt) const;
    boost::future<Json> fetchTradesAsync(const std::string& symbol,
                                       const std::optional<long long>& since = std::nullopt,
                                       const std::optional<int>& limit = std::nullopt) const;

    // Trading
    boost::future<Json> createOrderAsync(const std::string& symbol, const std::string& type,
                                       const std::string& side, double amount,
                                       const std::optional<double>& price = std::nullopt);
    boost::future<Json> editOrderAsync(const std::string& id, const std::string& symbol,
                                     const std::string& type, const std::string& side,
                                     const std::optional<double>& amount = std::nullopt,
                                     const std::optional<double>& price = std::nullopt);
    boost::future<Json> cancelOrderAsync(const std::string& id, const std::string& symbol);
    boost::future<Json> cancelOrdersAsync(const std::vector<std::string>& ids, const std::string& symbol = "");
    boost::future<Json> cancelAllOrdersAsync(const std::string& symbol = "");
    boost::future<Json> fetchOrderAsync(const std::string& id, const std::string& symbol) const;
    boost::future<Json> fetchOpenOrdersAsync(const std::string& symbol = "",
                                           const std::optional<long long>& since = std::nullopt,
                                           const std::optional<int>& limit = std::nullopt) const;
    boost::future<Json> fetchClosedOrdersAsync(const std::string& symbol = "",
                                             const std::optional<long long>& since = std::nullopt,
                                             const std::optional<int>& limit = std::nullopt) const;
    boost::future<Json> fetchMyTradesAsync(const std::string& symbol = "",
                                         const std::optional<long long>& since = std::nullopt,
                                         const std::optional<int>& limit = std::nullopt) const;

    // Account
    boost::future<Json> fetchBalanceAsync() const;
    boost::future<Json> fetchPositionsAsync(const std::string& symbols = "") const;
    boost::future<Json> fetchLedgerAsync(const std::string& code = "",
                                       const std::optional<long long>& since = std::nullopt,
                                       const std::optional<int>& limit = std::nullopt) const;
    boost::future<Json> fetchOrderBookSnapshotAsync(const std::string& symbol,
                                                  const std::optional<int>& limit = std::nullopt) const;
    boost::future<Json> fetchFundingRatesAsync(const std::vector<std::string>& symbols = {}) const;
    boost::future<Json> setLeverageAsync(const std::string& symbol, double leverage);
    boost::future<Json> fetchDepositsAsync(const std::string& code = "",
                                         const std::optional<long long>& since = std::nullopt,
                                         const std::optional<int>& limit = std::nullopt) const;
    boost::future<Json> fetchWithdrawalsAsync(const std::string& code = "",
                                           const std::optional<long long>& since = std::nullopt,
                                           const std::optional<int>& limit = std::nullopt) const;
    boost::future<Json> fetchDepositAddressAsync(const std::string& code) const;
    boost::future<Json> transferAsync(const std::string& code, double amount,
                                    const std::string& fromAccount,
                                    const std::string& toAccount);

protected:
    boost::future<Json> fetchAsync(const std::string& path, const std::string& api = "public",
                                 const std::string& method = "GET", const Json& params = Json::object(),
                                 const std::map<std::string, std::string>& headers = {}) const;
};

} // namespace ccxt
