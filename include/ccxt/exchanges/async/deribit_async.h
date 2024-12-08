#ifndef CCXT_EXCHANGES_ASYNC_DERIBIT_ASYNC_H
#define CCXT_EXCHANGES_ASYNC_DERIBIT_ASYNC_H

#include "ccxt/async_base/exchange.h"
#include "ccxt/exchanges/deribit.h"
#include <boost/asio.hpp>
#include <string>
#include <map>
#include <vector>
#include <optional>

namespace ccxt {

class DeribitAsync : public AsyncExchange, public Deribit {
public:
    explicit DeribitAsync(const boost::asio::io_context& context);
    ~DeribitAsync() override = default;

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

    // Deribit specific methods
    boost::future<std::vector<Position>> fetch_positions_async(const std::string& symbol = "");
    boost::future<std::vector<Transaction>> fetch_deposits_async(const std::optional<std::string>& code = std::nullopt,
                                                               const std::optional<long long>& since = std::nullopt,
                                                               const std::optional<int>& limit = std::nullopt);
    boost::future<std::vector<Transaction>> fetch_withdrawals_async(const std::optional<std::string>& code = std::nullopt,
                                                                  const std::optional<long long>& since = std::nullopt,
                                                                  const std::optional<int>& limit = std::nullopt);
    boost::future<std::vector<Trade>> fetch_my_trades_async(const std::string& symbol = "",
                                                          const std::optional<long long>& since = std::nullopt,
                                                          const std::optional<int>& limit = std::nullopt);
    boost::future<std::vector<LedgerEntry>> fetch_ledger_async(const std::optional<std::string>& code = std::nullopt,
                                                              const std::optional<long long>& since = std::nullopt,
                                                              const std::optional<int>& limit = std::nullopt);
    boost::future<OpenInterest> fetch_open_interest_async(const std::string& symbol);
    boost::future<std::vector<OptionContract>> fetch_option_chain_async(const std::string& underlying);
    boost::future<std::vector<VolatilityData>> fetch_volatility_history_async(const std::string& symbol);
    boost::future<void> set_margin_mode_async(const std::string& marginMode);
    boost::future<void> set_leverage_async(const std::string& symbol, double leverage);

protected:
    boost::asio::io_context& context_;
};

} // namespace ccxt

#endif // CCXT_EXCHANGES_ASYNC_DERIBIT_ASYNC_H
