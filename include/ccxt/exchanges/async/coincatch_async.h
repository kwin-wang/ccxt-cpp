#ifndef CCXT_EXCHANGES_ASYNC_COINCATCH_ASYNC_H
#define CCXT_EXCHANGES_ASYNC_COINCATCH_ASYNC_H

#include "ccxt/async_base/exchange.h"
#include "ccxt/exchanges/coincatch.h"
#include <boost/asio.hpp>
#include <string>
#include <map>
#include <vector>

namespace ccxt {

class CoincatchAsync : public AsyncExchange, public coincatch {
public:
    explicit CoincatchAsync(const boost::asio::io_context& context);
    ~CoincatchAsync() override = default;

    // Market Data
    boost::future<std::vector<Market>> fetch_markets_async() override;
    boost::future<std::vector<Currency>> fetch_currencies_async() override;
    boost::future<OrderBook> fetch_order_book_async(const std::string& symbol, int limit = 0) override;
    boost::future<std::map<std::string, Ticker>> fetch_tickers_async() override;
    boost::future<Ticker> fetch_ticker_async(const std::string& symbol) override;
    boost::future<std::vector<Trade>> fetch_trades_async(const std::string& symbol,
                                                       long since = 0,
                                                       int limit = 0) override;
    boost::future<std::vector<OHLCV>> fetch_ohlcv_async(const std::string& symbol,
                                                       const std::string& timeframe = "1m",
                                                       long since = 0,
                                                       int limit = 0) override;
    boost::future<double> fetch_leverage_async(const std::string& symbol,
                                             const std::map<std::string, std::string>& params = {});
    boost::future<double> fetch_funding_rate_async(const std::string& symbol,
                                                 const std::map<std::string, std::string>& params = {});
    boost::future<std::vector<FundingRateHistory>> fetch_funding_rate_history_async(const std::string& symbol,
                                                                                   long since = 0,
                                                                                   int limit = 0);

    // Trading
    boost::future<Order> create_order_async(const std::string& symbol,
                                          const std::string& type,
                                          const std::string& side,
                                          double amount,
                                          double price = 0,
                                          const std::map<std::string, std::string>& params = {}) override;
    boost::future<Order> create_market_order_with_cost_async(const std::string& symbol,
                                                           const std::string& side,
                                                           double cost,
                                                           const std::map<std::string, std::string>& params = {});
    boost::future<Order> create_stop_limit_order_async(const std::string& symbol,
                                                     const std::string& side,
                                                     double amount,
                                                     double price,
                                                     double stopPrice,
                                                     const std::map<std::string, std::string>& params = {});
    boost::future<Order> create_stop_market_order_async(const std::string& symbol,
                                                      const std::string& side,
                                                      double amount,
                                                      double stopPrice,
                                                      const std::map<std::string, std::string>& params = {});
    boost::future<Order> create_take_profit_order_async(const std::string& symbol,
                                                      const std::string& type,
                                                      const std::string& side,
                                                      double amount,
                                                      double price = 0,
                                                      const std::map<std::string, std::string>& params = {});
    boost::future<Order> cancel_order_async(const std::string& id,
                                          const std::string& symbol = "",
                                          const std::map<std::string, std::string>& params = {}) override;
    boost::future<std::vector<Order>> cancel_orders_async(const std::vector<std::string>& ids,
                                                        const std::string& symbol,
                                                        const std::map<std::string, std::string>& params = {});
    boost::future<std::vector<Order>> cancel_all_orders_async(const std::string& symbol = "",
                                                            const std::map<std::string, std::string>& params = {});

    // Account
    boost::future<Balance> fetch_balance_async() override;
    boost::future<std::vector<LedgerEntry>> fetch_ledger_async(const std::string& code = "",
                                                              long since = 0,
                                                              int limit = 0,
                                                              const std::map<std::string, std::string>& params = {}) override;
    boost::future<DepositAddress> fetch_deposit_address_async(const std::string& code,
                                                            const std::string& network = "",
                                                            const std::map<std::string, std::string>& params = {});
    boost::future<std::vector<Transaction>> fetch_deposits_async(const std::string& code = "",
                                                               long since = 0,
                                                               int limit = 0,
                                                               const std::map<std::string, std::string>& params = {}) override;
    boost::future<MarginAddition> add_margin_async(const std::string& symbol,
                                                 double amount,
                                                 const std::map<std::string, std::string>& params = {});

protected:
    boost::asio::io_context& context_;
};

} // namespace ccxt

#endif // CCXT_EXCHANGES_ASYNC_COINCATCH_ASYNC_H
