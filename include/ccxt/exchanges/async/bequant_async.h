#ifndef CCXT_EXCHANGES_ASYNC_BEQUANT_ASYNC_H
#define CCXT_EXCHANGES_ASYNC_BEQUANT_ASYNC_H

#include "ccxt/async_base/exchange.h"
#include "ccxt/exchanges/bequant.h"
#include <boost/asio.hpp>
#include <string>
#include <map>
#include <vector>

namespace ccxt {

class BequantAsync : public AsyncExchange, public Bequant {
public:
    explicit BequantAsync(const boost::asio::io_context& context);
    ~BequantAsync() override = default;

    // Market Data
    boost::future<std::vector<Market>> fetch_markets_async() override;
    boost::future<std::vector<Currency>> fetch_currencies_async() override;
    boost::future<OrderBook> fetch_order_book_async(const std::string& symbol, int limit = 0) override;
    boost::future<std::map<std::string, OrderBook>> fetch_order_books_async(const std::vector<std::string>& symbols = {}, int limit = 0) override;
    boost::future<std::map<std::string, Ticker>> fetch_tickers_async() override;
    boost::future<Ticker> fetch_ticker_async(const std::string& symbol) override;
    boost::future<std::vector<Trade>> fetch_trades_async(const std::string& symbol,
                                                       long since = 0,
                                                       int limit = 0) override;
    boost::future<std::vector<OHLCV>> fetch_ohlcv_async(const std::string& symbol,
                                                       const std::string& timeframe = "1m",
                                                       long since = 0,
                                                       int limit = 0) override;
    boost::future<std::vector<OHLCV>> fetch_index_ohlcv_async(const std::string& symbol,
                                                             const std::string& timeframe = "1m",
                                                             long since = 0,
                                                             int limit = 0) override;
    boost::future<std::vector<OHLCV>> fetch_mark_ohlcv_async(const std::string& symbol,
                                                            const std::string& timeframe = "1m",
                                                            long since = 0,
                                                            int limit = 0) override;
    boost::future<double> fetch_open_interest_async(const std::string& symbol) override;

    // Trading
    boost::future<Order> create_order_async(const std::string& symbol,
                                          const std::string& type,
                                          const std::string& side,
                                          double amount,
                                          double price = 0,
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
    boost::future<std::vector<Order>> cancel_all_orders_async(const std::string& symbol = "",
                                                            const std::map<std::string, std::string>& params = {}) override;
    boost::future<Order> fetch_order_async(const std::string& id,
                                         const std::string& symbol = "") override;
    boost::future<Order> fetch_open_order_async(const std::string& id,
                                              const std::string& symbol = "") override;
    boost::future<std::vector<Order>> fetch_open_orders_async(const std::string& symbol = "") override;
    boost::future<std::vector<Order>> fetch_closed_orders_async(const std::string& symbol = "",
                                                              long since = 0,
                                                              int limit = 0) override;
    boost::future<std::vector<Trade>> fetch_my_trades_async(const std::string& symbol = "",
                                                          long since = 0,
                                                          int limit = 0) override;
    boost::future<std::vector<Trade>> fetch_order_trades_async(const std::string& id,
                                                             const std::string& symbol = "",
                                                             long since = 0,
                                                             int limit = 0) override;

    // Account
    boost::future<Balance> fetch_balance_async() override;
    boost::future<MarginModification> add_margin_async(const std::string& symbol,
                                                     double amount,
                                                     const std::map<std::string, std::string>& params = {}) override;
    boost::future<Leverage> fetch_leverage_async(const std::string& symbol,
                                               const std::map<std::string, std::string>& params = {}) override;
    boost::future<std::map<std::string, MarginMode>> fetch_margin_modes_async(const std::vector<std::string>& symbols = {}) override;

    // Funding
    boost::future<DepositAddress> create_deposit_address_async(const std::string& code,
                                                             const std::map<std::string, std::string>& params = {}) override;
    boost::future<DepositAddress> fetch_deposit_address_async(const std::string& code,
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
    boost::future<std::map<std::string, TradingFee>> fetch_trading_fees_async() override;
    boost::future<std::map<std::string, FundingRate>> fetch_funding_rates_async() override;
    boost::future<FundingRate> fetch_funding_rate_async(const std::string& symbol) override;
    boost::future<std::vector<FundingRate>> fetch_funding_rate_history_async(const std::string& symbol = "",
                                                                           long since = 0,
                                                                           int limit = 0,
                                                                           const std::map<std::string, std::string>& params = {}) override;

protected:
    boost::asio::io_context& context_;
};

} // namespace ccxt

#endif // CCXT_EXCHANGES_ASYNC_BEQUANT_ASYNC_H
