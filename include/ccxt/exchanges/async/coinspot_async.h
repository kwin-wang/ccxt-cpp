#ifndef CCXT_EXCHANGES_ASYNC_COINSPOT_ASYNC_H
#define CCXT_EXCHANGES_ASYNC_COINSPOT_ASYNC_H

#include "ccxt/async_base/exchange.h"
#include "ccxt/exchanges/coinspot.h"
#include <boost/asio.hpp>
#include <string>
#include <map>
#include <vector>
#include <optional>

namespace ccxt {

class CoinspotAsync : public AsyncExchange, public coinspot {
public:
    explicit CoinspotAsync(const boost::asio::io_context& context);
    ~CoinspotAsync() override = default;

    // Market Data
    boost::future<std::vector<Market>> fetch_markets_async();
    boost::future<Ticker> fetch_ticker_async(const std::string& symbol);
    boost::future<std::map<std::string, Ticker>> fetch_tickers_async(const std::vector<std::string>& symbols = {});
    boost::future<OrderBook> fetch_order_book_async(const std::string& symbol,
                                                  const std::optional<int>& limit = std::nullopt);

    // Trading
    boost::future<Order> create_order_async(const std::string& symbol,
                                          const std::string& type,
                                          const std::string& side,
                                          double amount,
                                          const std::optional<double>& price = std::nullopt);
    boost::future<Order> cancel_order_async(const std::string& id,
                                          const std::string& symbol);
    boost::future<std::vector<Trade>> fetch_my_trades_async(const std::string& symbol = "",
                                                          const std::optional<long long>& since = std::nullopt,
                                                          const std::optional<int>& limit = std::nullopt);

    // Account
    boost::future<Balance> fetch_balance_async();

protected:
    boost::asio::io_context& context_;
};

} // namespace ccxt

#endif // CCXT_EXCHANGES_ASYNC_COINSPOT_ASYNC_H
