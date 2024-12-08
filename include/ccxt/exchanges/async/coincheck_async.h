#ifndef CCXT_EXCHANGES_ASYNC_COINCHECK_ASYNC_H
#define CCXT_EXCHANGES_ASYNC_COINCHECK_ASYNC_H

#include "ccxt/async_base/exchange.h"
#include "ccxt/exchanges/coincheck.h"
#include <boost/asio.hpp>
#include <string>
#include <map>
#include <vector>

namespace ccxt {

class CoincheckAsync : public AsyncExchange, public Coincheck {
public:
    explicit CoincheckAsync(const boost::asio::io_context& context);
    ~CoincheckAsync() override = default;

    // Market Data API
    boost::future<std::vector<Market>> fetch_markets_async(const std::map<std::string, std::string>& params = {});
    boost::future<Ticker> fetch_ticker_async(const std::string& symbol, const std::map<std::string, std::string>& params = {});
    boost::future<std::map<std::string, Ticker>> fetch_tickers_async(const std::vector<std::string>& symbols = {}, const std::map<std::string, std::string>& params = {});
    boost::future<OrderBook> fetch_order_book_async(const std::string& symbol, int limit = 0, const std::map<std::string, std::string>& params = {});
    boost::future<std::vector<Trade>> fetch_trades_async(const std::string& symbol, long since = 0, int limit = 0, const std::map<std::string, std::string>& params = {});
    boost::future<std::vector<OHLCV>> fetch_ohlcv_async(const std::string& symbol, const std::string& timeframe = "1m",
                                                       long since = 0, int limit = 0, const std::map<std::string, std::string>& params = {});

    // Trading API
    boost::future<Balance> fetch_balance_async(const std::map<std::string, std::string>& params = {});
    boost::future<Order> create_order_async(const std::string& symbol, const std::string& type, const std::string& side,
                                          double amount, double price = 0, const std::map<std::string, std::string>& params = {});
    boost::future<Order> cancel_order_async(const std::string& id, const std::string& symbol = "", const std::map<std::string, std::string>& params = {});
    boost::future<Order> fetch_order_async(const std::string& id, const std::string& symbol = "", const std::map<std::string, std::string>& params = {});
    boost::future<std::vector<Order>> fetch_orders_async(const std::string& symbol = "", long since = 0, int limit = 0, const std::map<std::string, std::string>& params = {});
    boost::future<std::vector<Order>> fetch_open_orders_async(const std::string& symbol = "", long since = 0, int limit = 0, const std::map<std::string, std::string>& params = {});
    boost::future<std::vector<Order>> fetch_closed_orders_async(const std::string& symbol = "", long since = 0, int limit = 0, const std::map<std::string, std::string>& params = {});

    // Account API
    boost::future<std::vector<Trade>> fetch_my_trades_async(const std::string& symbol = "", long since = 0, int limit = 0, const std::map<std::string, std::string>& params = {});
    boost::future<std::vector<Transaction>> fetch_deposits_async(const std::string& code = "", long since = 0, int limit = 0, const std::map<std::string, std::string>& params = {});
    boost::future<std::vector<Transaction>> fetch_withdrawals_async(const std::string& code = "", long since = 0, int limit = 0, const std::map<std::string, std::string>& params = {});
    boost::future<DepositAddress> fetch_deposit_address_async(const std::string& code, const std::map<std::string, std::string>& params = {});
    boost::future<Transaction> withdraw_async(const std::string& code, double amount, const std::string& address, const std::string& tag = "", const std::map<std::string, std::string>& params = {});

    // Leverage Trading API
    boost::future<Balance> fetch_leverage_balance_async(const std::map<std::string, std::string>& params = {});
    boost::future<Order> create_leverage_order_async(const std::string& symbol, const std::string& type, const std::string& side,
                                                   double amount, double price = 0, const std::map<std::string, std::string>& params = {});
    boost::future<Order> cancel_leverage_order_async(const std::string& id, const std::string& symbol = "", const std::map<std::string, std::string>& params = {});
    boost::future<std::vector<Position>> fetch_leverage_positions_async(const std::map<std::string, std::string>& params = {});
    boost::future<std::vector<Order>> fetch_leverage_orders_async(const std::string& symbol = "", long since = 0, int limit = 0, const std::map<std::string, std::string>& params = {});

protected:
    boost::asio::io_context& context_;
};

} // namespace ccxt

#endif // CCXT_EXCHANGES_ASYNC_COINCHECK_ASYNC_H
