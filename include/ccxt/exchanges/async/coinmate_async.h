#ifndef CCXT_EXCHANGES_ASYNC_COINMATE_ASYNC_H
#define CCXT_EXCHANGES_ASYNC_COINMATE_ASYNC_H

#include "ccxt/async_base/exchange.h"
#include "ccxt/exchanges/coinmate.h"
#include <boost/asio.hpp>
#include <string>
#include <map>
#include <vector>

namespace ccxt {

class CoinmateAsync : public AsyncExchange, public Coinmate {
public:
    explicit CoinmateAsync(const boost::asio::io_context& context);
    ~CoinmateAsync() override = default;

    // Market Data
    boost::future<std::vector<Market>> fetch_markets_async();
    boost::future<Ticker> fetch_ticker_async(const std::string& symbol);
    boost::future<OrderBook> fetch_order_book_async(const std::string& symbol,
                                                  int limit = 0);
    boost::future<std::vector<Trade>> fetch_trades_async(const std::string& symbol,
                                                       int limit = 0);
    boost::future<Balance> fetch_balance_async();

    // Trading
    boost::future<Order> create_order_async(const std::string& symbol,
                                          const std::string& type,
                                          const std::string& side,
                                          double amount,
                                          double price = 0.0);
    boost::future<Order> cancel_order_async(const std::string& id,
                                          const std::string& symbol = "");
    boost::future<Order> fetch_order_async(const std::string& id,
                                         const std::string& symbol = "");
    boost::future<std::vector<Order>> fetch_orders_async(const std::string& symbol = "",
                                                       int limit = 0);
    boost::future<std::vector<Order>> fetch_open_orders_async(const std::string& symbol = "");
    boost::future<std::vector<Trade>> fetch_my_trades_async(const std::string& symbol = "",
                                                          int limit = 0);

protected:
    boost::asio::io_context& context_;
};

} // namespace ccxt

#endif // CCXT_EXCHANGES_ASYNC_COINMATE_ASYNC_H
