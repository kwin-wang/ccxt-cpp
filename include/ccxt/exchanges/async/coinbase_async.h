#ifndef CCXT_EXCHANGES_ASYNC_COINBASE_ASYNC_H
#define CCXT_EXCHANGES_ASYNC_COINBASE_ASYNC_H

#include "ccxt/async_base/exchange.h"
#include "ccxt/exchanges/coinbase.h"
#include <boost/asio.hpp>
#include <string>
#include <map>
#include <vector>

namespace ccxt {

class CoinbaseAsync : public AsyncExchange, public Coinbase {
public:
    explicit CoinbaseAsync(const boost::asio::io_context& context);
    ~CoinbaseAsync() override = default;

    // Market Data
    boost::future<std::vector<Market>> fetch_markets_async() override;
    boost::future<std::vector<Currency>> fetch_currencies_async() override;
    boost::future<OrderBook> fetch_order_book_async(const std::string& symbol, int limit = 0) override;
    boost::future<std::map<std::string, Ticker>> fetch_tickers_async() override;
    boost::future<Ticker> fetch_ticker_async(const std::string& symbol) override;
    boost::future<std::vector<Trade>> fetch_trades_async(const std::string& symbol,
                                                       long since = 0,
                                                       int limit = 0) override;

    // Trading
    boost::future<Order> create_order_async(const std::string& symbol,
                                          const std::string& type,
                                          const std::string& side,
                                          double amount,
                                          double price = 0,
                                          const std::map<std::string, std::string>& params = {}) override;
    boost::future<Order> create_limit_buy_order_async(const std::string& symbol,
                                                    double amount,
                                                    double price,
                                                    const std::map<std::string, std::string>& params = {}) override;
    boost::future<Order> create_limit_sell_order_async(const std::string& symbol,
                                                     double amount,
                                                     double price,
                                                     const std::map<std::string, std::string>& params = {}) override;
    boost::future<Order> create_market_buy_order_async(const std::string& symbol,
                                                     double amount,
                                                     const std::map<std::string, std::string>& params = {}) override;
    boost::future<Order> create_market_sell_order_async(const std::string& symbol,
                                                      double amount,
                                                      const std::map<std::string, std::string>& params = {}) override;
    boost::future<Order> create_stop_order_async(const std::string& symbol,
                                               const std::string& type,
                                               const std::string& side,
                                               double amount,
                                               double price = 0,
                                               const std::map<std::string, std::string>& params = {});
    boost::future<Order> edit_order_async(const std::string& id,
                                        const std::string& symbol,
                                        const std::string& type,
                                        const std::string& side,
                                        double amount = 0,
                                        double price = 0,
                                        const std::map<std::string, std::string>& params = {}) override;
    boost::future<Order> cancel_order_async(const std::string& id,
                                          const std::string& symbol = "",
                                          const std::map<std::string, std::string>& params = {}) override;
    boost::future<std::vector<Order>> cancel_orders_async(const std::map<std::string, std::string>& params = {}) override;
    boost::future<Order> fetch_order_async(const std::string& id,
                                         const std::string& symbol = "") override;
    boost::future<std::vector<Order>> fetch_open_orders_async(const std::string& symbol = "") override;
    boost::future<std::vector<Order>> fetch_closed_orders_async(const std::string& symbol = "",
                                                              long since = 0,
                                                              int limit = 0) override;
    boost::future<std::vector<Order>> fetch_canceled_orders_async(const std::string& symbol = "",
                                                                long since = 0,
                                                                int limit = 0) override;

    // Account
    boost::future<std::vector<Account>> fetch_accounts_async() override;
    boost::future<Balance> fetch_balance_async() override;
    boost::future<std::vector<LedgerEntry>> fetch_ledger_async(const std::string& code = "",
                                                              long since = 0,
                                                              int limit = 0,
                                                              const std::map<std::string, std::string>& params = {}) override;

    // Funding
    boost::future<DepositAddress> create_deposit_address_async(const std::string& code,
                                                             const std::map<std::string, std::string>& params = {}) override;
    boost::future<std::map<std::string, DepositAddress>> fetch_deposit_addresses_by_network_async(const std::string& code,
                                                                                                const std::map<std::string, std::string>& params = {}) override;
    boost::future<Transaction> fetch_deposit_async(const std::string& id,
                                                 const std::string& code = "",
                                                 const std::map<std::string, std::string>& params = {}) override;
    boost::future<std::vector<Transaction>> fetch_deposits_async(const std::string& code = "",
                                                               long since = 0,
                                                               int limit = 0,
                                                               const std::map<std::string, std::string>& params = {}) override;
    boost::future<std::vector<Transaction>> fetch_withdrawals_async(const std::string& code = "",
                                                                  long since = 0,
                                                                  int limit = 0,
                                                                  const std::map<std::string, std::string>& params = {}) override;
    boost::future<std::vector<Transaction>> fetch_deposits_withdrawals_async(const std::string& code = "",
                                                                           long since = 0,
                                                                           int limit = 0,
                                                                           const std::map<std::string, std::string>& params = {}) override;

    // Convert
    boost::future<Conversion> create_convert_trade_async(const std::string& fromCurrency,
                                                       const std::string& toCurrency,
                                                       double amount,
                                                       const std::map<std::string, std::string>& params = {}) override;
    boost::future<Conversion> fetch_convert_trade_async(const std::string& id,
                                                      const std::map<std::string, std::string>& params = {}) override;
    boost::future<Conversion> fetch_convert_quote_async(const std::string& fromCurrency,
                                                      const std::string& toCurrency,
                                                      double amount,
                                                      const std::map<std::string, std::string>& params = {}) override;

protected:
    boost::asio::io_context& context_;
};

} // namespace ccxt

#endif // CCXT_EXCHANGES_ASYNC_COINBASE_ASYNC_H
