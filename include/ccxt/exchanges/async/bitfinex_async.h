#pragma once

#include "ccxt/exchanges/async/exchange_async.h"
#include "ccxt/exchanges/bitfinex.h"

namespace ccxt {

class BitfinexAsync : public ExchangeAsync, public Bitfinex {
public:
    explicit BitfinexAsync(const boost::asio::io_context& context);
    ~BitfinexAsync() override = default;

    // Market Data API
    boost::future<json> fetchMarketsAsync(const json& params = json::object());
    boost::future<json> fetchTickerAsync(const String& symbol, const json& params = json::object());
    boost::future<json> fetchTickersAsync(const std::vector<String>& symbols = {}, const json& params = json::object());
    boost::future<json> fetchOrderBookAsync(const String& symbol, int limit = 0, const json& params = json::object());
    boost::future<json> fetchTradesAsync(const String& symbol, int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchOHLCVAsync(const String& symbol, const String& timeframe = "1m",
                                      int since = 0, int limit = 0, const json& params = json::object());

    // Trading API
    boost::future<json> fetchBalanceAsync(const json& params = json::object());
    boost::future<json> createOrderAsync(const String& symbol, const String& type, const String& side,
                                       double amount, double price = 0, const json& params = json::object());
    boost::future<json> cancelOrderAsync(const String& id, const String& symbol = "", const json& params = json::object());
    boost::future<json> fetchOrderAsync(const String& id, const String& symbol = "", const json& params = json::object());
    boost::future<json> fetchOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchOpenOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchClosedOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());

    // Bitfinex specific methods
    boost::future<json> fetchPositionsAsync(const String& symbol = "", const json& params = json::object());
    boost::future<json> fetchMyTradesAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchLedgerAsync(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchFundingRatesAsync(const std::vector<String>& symbols = {}, const json& params = json::object());
    boost::future<json> setLeverageAsync(const String& symbol, double leverage, const json& params = json::object());
    boost::future<json> fetchDepositsAsync(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchWithdrawalsAsync(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchDepositAddressAsync(const String& code, const json& params = json::object());
    boost::future<json> transferAsync(const String& code, double amount, const String& fromAccount, const String& toAccount, const json& params = json::object());

protected:
    boost::future<json> fetchAsync(const String& path, const String& api = "public",
                                 const String& method = "GET", const json& params = json::object(),
                                 const std::map<String, String>& headers = {});
};

} // namespace ccxt
