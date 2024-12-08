#ifndef CCXT_EXCHANGES_ASYNC_HASHKEY_ASYNC_H
#define CCXT_EXCHANGES_ASYNC_HASHKEY_ASYNC_H

#include "ccxt/async_base/exchange.h"
#include "ccxt/exchanges/hashkey.h"
#include <boost/asio.hpp>
#include <string>
#include <map>
#include <vector>
#include <optional>

namespace ccxt {

class HashkeyAsync : public AsyncExchange, public Hashkey {
public:
    explicit HashkeyAsync(const boost::asio::io_context& context);
    ~HashkeyAsync() override = default;

    // Market Data API
    boost::future<std::vector<Market>> fetch_markets_async();
    boost::future<Ticker> fetch_ticker_async(const std::string& symbol);
    boost::future<std::vector<Ticker>> fetch_tickers_async(const std::vector<std::string>& symbols = {});
    boost::future<OrderBook> fetch_order_book_async(const std::string& symbol,
                                                  const std::optional<int>& limit = std::nullopt);
    boost::future<std::vector<Trade>> fetch_trades_async(const std::string& symbol,
                                                       const std::optional<int>& limit = std::nullopt);
    boost::future<Balance> fetch_balance_async();
    boost::future<std::vector<OHLCV>> fetch_ohlcv_async(const std::string& symbol,
                                                       const std::string& timeframe = "1m",
                                                       const std::optional<long>& since = std::nullopt,
                                                       const std::optional<int>& limit = std::nullopt);

    // Trading API
    boost::future<Order> create_order_async(const std::string& symbol,
                                          const std::string& type,
                                          const std::string& side,
                                          double amount,
                                          const std::optional<double>& price = std::nullopt);
    boost::future<Order> cancel_order_async(const std::string& id,
                                          const std::string& symbol = "");
    boost::future<std::vector<Order>> cancel_all_orders_async(const std::string& symbol = "");
    boost::future<Order> fetch_order_async(const std::string& id,
                                         const std::string& symbol = "");
    boost::future<std::vector<Order>> fetch_orders_async(const std::string& symbol = "",
                                                       const std::optional<int>& limit = std::nullopt);
    boost::future<std::vector<Order>> fetch_open_orders_async(const std::string& symbol = "",
                                                            const std::optional<int>& limit = std::nullopt);
    boost::future<std::vector<Order>> fetch_closed_orders_async(const std::string& symbol = "",
                                                              const std::optional<int>& limit = std::nullopt);
    boost::future<std::vector<Trade>> fetch_my_trades_async(const std::string& symbol = "",
                                                          const std::optional<int>& since = std::nullopt,
                                                          const std::optional<int>& limit = std::nullopt);

protected:
    boost::asio::io_context& context_;
};

} // namespace ccxt

#endif // CCXT_EXCHANGES_ASYNC_HASHKEY_ASYNC_H
