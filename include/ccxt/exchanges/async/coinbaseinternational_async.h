#ifndef CCXT_EXCHANGES_ASYNC_COINBASEINTERNATIONAL_ASYNC_H
#define CCXT_EXCHANGES_ASYNC_COINBASEINTERNATIONAL_ASYNC_H

#include "ccxt/async_base/exchange.h"
#include "ccxt/exchanges/coinbase.h"
#include <boost/asio.hpp>
#include <string>
#include <map>
#include <vector>

namespace ccxt {

class CoinbaseInternationalAsync : public AsyncExchange, public Coinbase {
public:
    explicit CoinbaseInternationalAsync(const boost::asio::io_context& context);
    ~CoinbaseInternationalAsync() override = default;

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

    // Trading
    boost::future<Order> create_order_async(const std::string& symbol,
                                          const std::string& type,
                                          const std::string& side,
                                          double amount,
                                          double price = 0,
                                          const std::map<std::string, std::string>& params = {}) override;
    boost::future<Order> cancel_order_async(const std::string& id,
                                          const std::string& symbol = "",
                                          const std::map<std::string, std::string>& params = {}) override;
    boost::future<std::vector<Order>> fetch_orders_async(const std::string& symbol = "",
                                                       long since = 0,
                                                       int limit = 0) override;
    boost::future<std::vector<Order>> fetch_open_orders_async(const std::string& symbol = "") override;
    boost::future<std::vector<Order>> fetch_closed_orders_async(const std::string& symbol = "",
                                                              long since = 0,
                                                              int limit = 0) override;
    boost::future<Order> fetch_order_async(const std::string& id,
                                         const std::string& symbol = "") override;

    // Account
    boost::future<Balance> fetch_balance_async() override;
    boost::future<std::vector<Account>> fetch_accounts_async() override;
    boost::future<std::map<std::string, TradingFee>> fetch_trading_fees_async() override;
    boost::future<std::vector<LedgerEntry>> fetch_ledger_async(const std::string& code = "",
                                                              long since = 0,
                                                              int limit = 0,
                                                              const std::map<std::string, std::string>& params = {}) override;

    // Funding
    boost::future<std::vector<Transaction>> fetch_deposits_async(const std::string& code = "",
                                                               long since = 0,
                                                               int limit = 0,
                                                               const std::map<std::string, std::string>& params = {}) override;
    boost::future<std::vector<Transaction>> fetch_withdrawals_async(const std::string& code = "",
                                                                  long since = 0,
                                                                  int limit = 0,
                                                                  const std::map<std::string, std::string>& params = {}) override;
    boost::future<DepositAddress> fetch_deposit_address_async(const std::string& code,
                                                            const std::map<std::string, std::string>& params = {}) override;

    // Advanced Trading
    boost::future<std::vector<Position>> fetch_positions_async(const std::vector<std::string>& symbols = std::vector<std::string>(),
                                                             const std::map<std::string, std::string>& params = {}) override;
    boost::future<MarginMode> set_margin_mode_async(const std::string& symbol,
                                                   const std::string& marginMode,
                                                   const std::map<std::string, std::string>& params = {}) override;

protected:
    boost::asio::io_context& context_;
};

} // namespace ccxt

#endif // CCXT_EXCHANGES_ASYNC_COINBASEINTERNATIONAL_ASYNC_H
