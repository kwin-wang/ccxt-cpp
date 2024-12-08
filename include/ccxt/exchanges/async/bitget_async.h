#pragma once

#include "ccxt/exchanges/async/exchange_async.h"
#include "ccxt/exchanges/bitget.h"

namespace ccxt {

class BitgetAsync : public ExchangeAsync, public Bitget {
public:
    explicit BitgetAsync(const boost::asio::io_context& context);
    ~BitgetAsync() override = default;

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

    // Bitget specific methods
    boost::future<json> fetchPositionsAsync(const String& symbols = "", const json& params = json::object());
    boost::future<json> fetchPositionRiskAsync(const String& symbols = "", const json& params = json::object());
    boost::future<json> setLeverageAsync(int leverage, const String& symbol, const json& params = json::object());
    boost::future<json> setMarginModeAsync(const String& marginMode, const String& symbol = "", const json& params = json::object());
    boost::future<json> fetchFundingRateAsync(const String& symbol, const json& params = json::object());
    boost::future<json> fetchFundingRatesAsync(const std::vector<String>& symbols = {}, const json& params = json::object());
    boost::future<json> fetchFundingHistoryAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchMyTradesAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchDepositsAsync(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchWithdrawalsAsync(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchDepositAddressAsync(const String& code, const json& params = json::object());
    boost::future<json> withdrawAsync(const String& code, double amount, const String& address,
                                    const String& tag = "", const json& params = json::object());
    boost::future<json> fetchTransactionsAsync(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchLedgerAsync(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchTradingFeesAsync(const json& params = json::object());
    boost::future<json> fetchFundingFeesAsync(const json& params = json::object());

protected:
    boost::future<json> fetchAsync(const String& path, const String& api = "public",
                                 const String& method = "GET", const json& params = json::object(),
                                 const std::map<String, String>& headers = {});
};

} // namespace ccxt
