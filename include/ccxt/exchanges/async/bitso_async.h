#ifndef CCXT_BITSO_ASYNC_H
#define CCXT_BITSO_ASYNC_H

#include "ccxt/exchanges/async/exchange_async.h"
#include "ccxt/exchanges/bitso.h"

namespace ccxt {

class BitsoAsync : public ExchangeAsync, public Bitso {
public:
    explicit BitsoAsync(const boost::asio::io_context& context);
    ~BitsoAsync() = default;

    boost::future<json> fetchAsync(const String& path, const String& api,
                                 const String& method, const json& params,
                                 const std::map<String, String>& headers);

    // Market Data
    boost::future<json> fetchTimeAsync(const json& params = json::object());
    boost::future<json> fetchMarketsAsync(const json& params = json::object());
    boost::future<json> fetchCurrenciesAsync(const json& params = json::object());
    boost::future<json> fetchTickerAsync(const String& symbol, const json& params = json::object());
    boost::future<json> fetchTickersAsync(const std::vector<String>& symbols = std::vector<String>(), const json& params = json::object());
    boost::future<json> fetchOrderBookAsync(const String& symbol, int limit = 0, const json& params = json::object());
    boost::future<json> fetchTradesAsync(const String& symbol, int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchOHLCVAsync(const String& symbol, const String& timeframe = "1m",
                                       int since = 0, int limit = 0, const json& params = json::object());

    // Trading
    boost::future<json> fetchBalanceAsync(const json& params = json::object());
    boost::future<json> createOrderAsync(const String& symbol, const String& type,
                                       const String& side, double amount,
                                       double price = 0, const json& params = json::object());
    boost::future<json> cancelOrderAsync(const String& id, const String& symbol = "",
                                       const json& params = json::object());
    boost::future<json> cancelAllOrdersAsync(const String& symbol = "",
                                           const json& params = json::object());
    boost::future<json> fetchOrderAsync(const String& id, const String& symbol = "",
                                      const json& params = json::object());
    boost::future<json> fetchOrdersAsync(const String& symbol = "", int since = 0,
                                       int limit = 0, const json& params = json::object());
    boost::future<json> fetchOpenOrdersAsync(const String& symbol = "", int since = 0,
                                           int limit = 0, const json& params = json::object());
    boost::future<json> fetchClosedOrdersAsync(const String& symbol = "", int since = 0,
                                             int limit = 0, const json& params = json::object());
    boost::future<json> fetchMyTradesAsync(const String& symbol = "", int since = 0,
                                         int limit = 0, const json& params = json::object());

    // Account
    boost::future<json> fetchAccountsAsync(const json& params = json::object());
    boost::future<json> fetchTradingFeesAsync(const json& params = json::object());
    boost::future<json> fetchFundingFeesAsync(const json& params = json::object());

    // Funding
    boost::future<json> fetchDepositAddressAsync(const String& code,
                                               const json& params = json::object());
    boost::future<json> fetchDepositsAsync(const String& code = "", int since = 0,
                                         int limit = 0, const json& params = json::object());
    boost::future<json> fetchWithdrawalsAsync(const String& code = "", int since = 0,
                                           int limit = 0, const json& params = json::object());
    boost::future<json> withdrawAsync(const String& code, double amount,
                                    const String& address, const String& tag = "",
                                    const json& params = json::object());
    boost::future<json> fetchTransactionsAsync(const String& code = "", int since = 0,
                                             int limit = 0, const json& params = json::object());
    boost::future<json> fetchTransferAsync(const String& id,
                                         const json& params = json::object());
    boost::future<json> fetchTransfersAsync(const String& code = "", int since = 0,
                                          int limit = 0, const json& params = json::object());
};

} // namespace ccxt

#endif // CCXT_BITSO_ASYNC_H
