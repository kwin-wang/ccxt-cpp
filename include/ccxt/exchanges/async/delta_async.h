#ifndef CCXT_EXCHANGES_ASYNC_DELTA_ASYNC_H
#define CCXT_EXCHANGES_ASYNC_DELTA_ASYNC_H

#include "ccxt/async_base/exchange.h"
#include "ccxt/exchanges/delta.h"
#include <boost/asio.hpp>
#include <string>
#include <map>
#include <vector>
#include <optional>

namespace ccxt {

class DeltaAsync : public AsyncExchange, public delta {
public:
    explicit DeltaAsync(const boost::asio::io_context& context);
    ~DeltaAsync() override = default;

    // Market Data
    boost::future<std::vector<Market>> fetch_markets_async();
    boost::future<std::map<std::string, Currency>> fetch_currencies_async();
    boost::future<Ticker> fetch_ticker_async(const std::string& symbol);
    boost::future<std::map<std::string, Ticker>> fetch_tickers_async(const std::vector<std::string>& symbols = {});
    boost::future<OrderBook> fetch_order_book_async(const std::string& symbol,
                                                  const std::optional<int>& limit = std::nullopt);
    boost::future<std::vector<OHLCV>> fetch_ohlcv_async(const std::string& symbol,
                                                       const std::string& timeframe,
                                                       const std::optional<long long>& since = std::nullopt,
                                                       const std::optional<int>& limit = std::nullopt);
    boost::future<std::map<std::string, FundingRate>> fetch_funding_rates_async();
    boost::future<std::map<std::string, Greeks>> fetch_greeks_async();

    // Trading
    boost::future<Order> create_order_async(const std::string& symbol,
                                          const std::string& type,
                                          const std::string& side,
                                          double amount,
                                          const std::optional<double>& price = std::nullopt);
    boost::future<Order> cancel_order_async(const std::string& id,
                                          const std::string& symbol);
    boost::future<Order> fetch_order_async(const std::string& id,
                                         const std::string& symbol);
    boost::future<std::vector<Order>> fetch_open_orders_async(const std::string& symbol = "",
                                                            const std::optional<long long>& since = std::nullopt,
                                                            const std::optional<int>& limit = std::nullopt);
    boost::future<std::vector<Order>> fetch_closed_orders_async(const std::string& symbol = "",
                                                              const std::optional<long long>& since = std::nullopt,
                                                              const std::optional<int>& limit = std::nullopt);
    boost::future<std::vector<Trade>> fetch_my_trades_async(const std::string& symbol = "",
                                                          const std::optional<long long>& since = std::nullopt,
                                                          const std::optional<int>& limit = std::nullopt);

    // Account
    boost::future<Balance> fetch_balance_async();
    boost::future<std::vector<LedgerEntry>> fetch_ledger_async(const std::optional<std::string>& code = std::nullopt,
                                                              const std::optional<long long>& since = std::nullopt,
                                                              const std::optional<int>& limit = std::nullopt);
    boost::future<DepositAddress> fetch_deposit_address_async(const std::string& code,
                                                            const std::optional<std::string>& network = std::nullopt);

protected:
    boost::asio::io_context& context_;
};

} // namespace ccxt

#endif // CCXT_EXCHANGES_ASYNC_DELTA_ASYNC_H
