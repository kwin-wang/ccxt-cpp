#ifndef CCXT_EXCHANGES_ASYNC_ACE_ASYNC_H
#define CCXT_EXCHANGES_ASYNC_ACE_ASYNC_H

#include "ccxt/async_base/exchange.h"
#include "ccxt/exchanges/ace.h"
#include <boost/asio.hpp>
#include <string>
#include <map>

namespace ccxt {

class AceAsync : public AsyncExchange, public Ace {
public:
    explicit AceAsync(const boost::asio::io_context& context);
    ~AceAsync() override = default;

    // Market Data
    boost::future<OrderBook> fetch_order_book_async(const std::string& symbol, int limit = 0) override;
    boost::future<std::map<std::string, Ticker>> fetch_tickers_async() override;
    boost::future<Ticker> fetch_ticker_async(const std::string& symbol) override;
    boost::future<std::vector<Market>> fetch_markets_async() override;
    boost::future<std::vector<OHLCV>> fetch_ohlcv_async(const std::string& symbol, 
                                                       const std::string& timeframe = "1m",
                                                       long since = 0,
                                                       int limit = 0) override;

    // Trading
    boost::future<Order> create_order_async(const std::string& symbol,
                                          const std::string& type,
                                          const std::string& side,
                                          double amount,
                                          double price = 0) override;
    boost::future<Order> cancel_order_async(const std::string& id,
                                          const std::string& symbol = "") override;
    boost::future<Order> fetch_order_async(const std::string& id,
                                         const std::string& symbol = "") override;
    boost::future<std::vector<Order>> fetch_open_orders_async(const std::string& symbol = "") override;
    boost::future<std::vector<Trade>> fetch_my_trades_async(const std::string& symbol = "",
                                                          long since = 0,
                                                          int limit = 0) override;
    boost::future<std::vector<Trade>> fetch_order_trades_async(const std::string& id,
                                                             const std::string& symbol = "",
                                                             long since = 0,
                                                             int limit = 0) override;

    // Account
    boost::future<Balance> fetch_balance_async() override;

protected:
    boost::asio::io_context& context_;
};

} // namespace ccxt

#endif // CCXT_EXCHANGES_ASYNC_ACE_ASYNC_H
