#ifndef CCXT_EXCHANGES_ASYNC_GEMINI_ASYNC_H
#define CCXT_EXCHANGES_ASYNC_GEMINI_ASYNC_H

#include "ccxt/async_base/exchange.h"
#include "ccxt/exchanges/gemini.h"
#include <boost/asio.hpp>
#include <string>
#include <map>
#include <vector>
#include <optional>

namespace ccxt {

class GeminiAsync : public AsyncExchange, public Gemini {
public:
    explicit GeminiAsync(const boost::asio::io_context& context);
    ~GeminiAsync() override = default;

    // Market Data API
    boost::future<std::vector<Market>> fetch_markets_async(const json& params = json::object());
    boost::future<Ticker> fetch_ticker_async(const String& symbol, const json& params = json::object());
    boost::future<std::vector<Ticker>> fetch_tickers_async(const std::vector<String>& symbols = {},
                                                         const json& params = json::object());
    boost::future<OrderBook> fetch_order_book_async(const String& symbol,
                                                  const std::optional<int>& limit = std::nullopt,
                                                  const json& params = json::object());
    boost::future<std::vector<Trade>> fetch_trades_async(const String& symbol,
                                                       const std::optional<long long>& since = std::nullopt,
                                                       const std::optional<int>& limit = std::nullopt,
                                                       const json& params = json::object());
    boost::future<std::vector<OHLCV>> fetch_ohlcv_async(const String& symbol,
                                                       const String& timeframe = "1m",
                                                       const std::optional<long long>& since = std::nullopt,
                                                       const std::optional<int>& limit = std::nullopt,
                                                       const json& params = json::object());

    // Trading API
    boost::future<Balance> fetch_balance_async(const json& params = json::object());
    boost::future<Order> create_order_async(const String& symbol,
                                          const String& type,
                                          const String& side,
                                          double amount,
                                          const std::optional<double>& price = std::nullopt,
                                          const json& params = json::object());
    boost::future<Order> cancel_order_async(const String& id,
                                          const String& symbol = "",
                                          const json& params = json::object());
    boost::future<Order> fetch_order_async(const String& id,
                                         const String& symbol = "",
                                         const json& params = json::object());
    boost::future<std::vector<Order>> fetch_orders_async(const String& symbol = "",
                                                       const std::optional<long long>& since = std::nullopt,
                                                       const std::optional<int>& limit = std::nullopt,
                                                       const json& params = json::object());
    boost::future<std::vector<Order>> fetch_open_orders_async(const String& symbol = "",
                                                            const std::optional<long long>& since = std::nullopt,
                                                            const std::optional<int>& limit = std::nullopt,
                                                            const json& params = json::object());
    boost::future<std::vector<Order>> fetch_closed_orders_async(const String& symbol = "",
                                                              const std::optional<long long>& since = std::nullopt,
                                                              const std::optional<int>& limit = std::nullopt,
                                                              const json& params = json::object());

    // Gemini specific async methods
    boost::future<std::vector<Trade>> fetch_my_trades_async(const String& symbol = "",
                                                          const std::optional<long long>& since = std::nullopt,
                                                          const std::optional<int>& limit = std::nullopt,
                                                          const json& params = json::object());
    boost::future<std::vector<Transaction>> fetch_deposits_async(const String& code = "",
                                                               const std::optional<long long>& since = std::nullopt,
                                                               const std::optional<int>& limit = std::nullopt,
                                                               const json& params = json::object());
    boost::future<std::vector<Transaction>> fetch_withdrawals_async(const String& code = "",
                                                                  const std::optional<long long>& since = std::nullopt,
                                                                  const std::optional<int>& limit = std::nullopt,
                                                                  const json& params = json::object());
    boost::future<std::vector<Transfer>> fetch_transfers_async(const String& code = "",
                                                             const std::optional<long long>& since = std::nullopt,
                                                             const std::optional<int>& limit = std::nullopt,
                                                             const json& params = json::object());
    boost::future<Transfer> transfer_async(const String& code,
                                         double amount,
                                         const String& fromAccount,
                                         const String& toAccount,
                                         const json& params = json::object());
    boost::future<DepositAddress> fetch_deposit_address_async(const String& code,
                                                            const json& params = json::object());
    boost::future<DepositAddress> create_deposit_address_async(const String& code,
                                                             const json& params = json::object());
    boost::future<Transaction> withdraw_async(const String& code,
                                            double amount,
                                            const String& address,
                                            const std::optional<String>& tag = std::nullopt,
                                            const json& params = json::object());
    boost::future<std::vector<LedgerEntry>> fetch_ledger_async(const String& code = "",
                                                              const std::optional<long long>& since = std::nullopt,
                                                              const std::optional<int>& limit = std::nullopt,
                                                              const json& params = json::object());
    boost::future<json> fetch_payment_methods_async(const json& params = json::object());

protected:
    boost::asio::io_context& context_;
};

} // namespace ccxt

#endif // CCXT_EXCHANGES_ASYNC_GEMINI_ASYNC_H
