#ifndef CCXT_EXCHANGES_ASYNC_EXMO_ASYNC_H
#define CCXT_EXCHANGES_ASYNC_EXMO_ASYNC_H

#include "ccxt/async_base/exchange.h"
#include "ccxt/exchanges/exmo.h"
#include <boost/asio.hpp>
#include <string>
#include <map>
#include <vector>
#include <optional>

namespace ccxt {

class ExmoAsync : public AsyncExchange, public Exmo {
public:
    explicit ExmoAsync(const boost::asio::io_context& context);
    ~ExmoAsync() override = default;

    // Market Data API
    boost::future<std::vector<Market>> fetch_markets_async();
    boost::future<Ticker> fetch_ticker_async(const std::string& symbol);
    boost::future<std::map<std::string, Ticker>> fetch_tickers_async(const std::vector<std::string>& symbols = {});
    boost::future<OrderBook> fetch_order_book_async(const std::string& symbol,
                                                  const std::optional<int>& limit = std::nullopt);
    boost::future<std::vector<Trade>> fetch_trades_async(const std::string& symbol,
                                                       const std::optional<long long>& since = std::nullopt,
                                                       const std::optional<int>& limit = std::nullopt);
    boost::future<std::vector<OHLCV>> fetch_ohlcv_async(const std::string& symbol,
                                                       const std::string& timeframe = "1m",
                                                       const std::optional<long long>& since = std::nullopt,
                                                       const std::optional<int>& limit = std::nullopt);

    // Trading API
    boost::future<Balance> fetch_balance_async();
    boost::future<Order> create_order_async(const std::string& symbol,
                                          const std::string& type,
                                          const std::string& side,
                                          double amount,
                                          const std::optional<double>& price = std::nullopt);
    boost::future<Order> cancel_order_async(const std::string& id,
                                          const std::string& symbol);
    boost::future<Order> fetch_order_async(const std::string& id,
                                         const std::string& symbol);
    boost::future<std::vector<Order>> fetch_orders_async(const std::string& symbol = "",
                                                       const std::optional<long long>& since = std::nullopt,
                                                       const std::optional<int>& limit = std::nullopt);
    boost::future<std::vector<Order>> fetch_open_orders_async(const std::string& symbol = "",
                                                            const std::optional<long long>& since = std::nullopt,
                                                            const std::optional<int>& limit = std::nullopt);
    boost::future<std::vector<Order>> fetch_closed_orders_async(const std::string& symbol = "",
                                                              const std::optional<long long>& since = std::nullopt,
                                                              const std::optional<int>& limit = std::nullopt);

    // Account API
    boost::future<std::vector<Trade>> fetch_my_trades_async(const std::string& symbol = "",
                                                          const std::optional<long long>& since = std::nullopt,
                                                          const std::optional<int>& limit = std::nullopt);
    boost::future<std::vector<Transaction>> fetch_deposits_async(const std::optional<std::string>& code = std::nullopt,
                                                               const std::optional<long long>& since = std::nullopt,
                                                               const std::optional<int>& limit = std::nullopt);
    boost::future<std::vector<Transaction>> fetch_withdrawals_async(const std::optional<std::string>& code = std::nullopt,
                                                                  const std::optional<long long>& since = std::nullopt,
                                                                  const std::optional<int>& limit = std::nullopt);
    boost::future<DepositAddress> fetch_deposit_address_async(const std::string& code);
    boost::future<Transaction> withdraw_async(const std::string& code,
                                            double amount,
                                            const std::string& address,
                                            const std::optional<std::string>& tag = std::nullopt);

    // Additional Features
    boost::future<std::map<std::string, Currency>> fetch_currencies_async();
    boost::future<std::map<std::string, Fee>> fetch_trading_fees_async();
    boost::future<std::vector<std::string>> fetch_required_ids_async();
    boost::future<std::vector<PaymentMethod>> fetch_payment_methods_async();
    boost::future<std::vector<DepositMethod>> fetch_deposit_methods_async();
    boost::future<std::vector<WithdrawMethod>> fetch_withdraw_methods_async();
    boost::future<std::vector<Transaction>> fetch_transactions_async(const std::optional<std::string>& code = std::nullopt,
                                                                   const std::optional<long long>& since = std::nullopt,
                                                                   const std::optional<int>& limit = std::nullopt);
    boost::future<std::vector<Trade>> fetch_order_trades_async(const std::string& id,
                                                             const std::optional<std::string>& symbol = std::nullopt);
    boost::future<std::vector<Trade>> fetch_user_trades_async(const std::string& symbol = "",
                                                            const std::optional<long long>& since = std::nullopt,
                                                            const std::optional<int>& limit = std::nullopt);

protected:
    boost::asio::io_context& context_;
};

} // namespace ccxt

#endif // CCXT_EXCHANGES_ASYNC_EXMO_ASYNC_H
