#ifndef CCXT_ASYNC_BITRUE_ASYNC_H
#define CCXT_ASYNC_BITRUE_ASYNC_H

#include "../async_base/async_exchange.h"
#include "../bitrue.h"

namespace ccxt {

class BitrueAsync : public AsyncExchange, public bitrue {
public:
    BitrueAsync(const boost::asio::io_context& context, const Config& config);

    // Market Data API
    boost::future<std::vector<Market>> fetch_markets_async() override;
    boost::future<std::vector<Currency>> fetch_currencies_async() override;
    boost::future<Ticker> fetch_ticker_async(const std::string& symbol) override;
    boost::future<std::vector<Ticker>> fetch_tickers_async(const std::vector<std::string>& symbols = {}) override;
    boost::future<OrderBook> fetch_order_book_async(const std::string& symbol,
                                                  const std::optional<int>& limit = std::nullopt) override;
    boost::future<std::vector<Trade>> fetch_trades_async(const std::string& symbol,
                                                       const std::optional<long long>& since = std::nullopt,
                                                       const std::optional<int>& limit = std::nullopt) override;
    boost::future<std::vector<OHLCV>> fetch_ohlcv_async(const std::string& symbol,
                                                       const std::string& timeframe = "1m",
                                                       const std::optional<long long>& since = std::nullopt,
                                                       const std::optional<int>& limit = std::nullopt) override;

    // Trading API
    boost::future<Order> create_order_async(const std::string& symbol,
                                          const std::string& type,
                                          const std::string& side,
                                          double amount,
                                          const std::optional<double>& price = std::nullopt) override;
    boost::future<Order> cancel_order_async(const std::string& id,
                                          const std::string& symbol) override;
    boost::future<Order> fetch_order_async(const std::string& id,
                                         const std::string& symbol) override;
    boost::future<std::vector<Order>> fetch_open_orders_async(const std::string& symbol,
                                                            const std::optional<long long>& since = std::nullopt,
                                                            const std::optional<int>& limit = std::nullopt) override;
    boost::future<std::vector<Order>> fetch_closed_orders_async(const std::string& symbol,
                                                              const std::optional<long long>& since = std::nullopt,
                                                              const std::optional<int>& limit = std::nullopt) override;
    boost::future<std::vector<Trade>> fetch_my_trades_async(const std::string& symbol,
                                                          const std::optional<long long>& since = std::nullopt,
                                                          const std::optional<int>& limit = std::nullopt) override;

    // Account API
    boost::future<Balance> fetch_balance_async() override;
    boost::future<DepositAddress> fetch_deposit_address_async(const std::string& code,
                                                            const std::optional<std::string>& network = std::nullopt) override;
    boost::future<std::vector<Transaction>> fetch_deposits_async(const std::optional<std::string>& code = std::nullopt,
                                                               const std::optional<long long>& since = std::nullopt,
                                                               const std::optional<int>& limit = std::nullopt) override;
    boost::future<std::vector<Transaction>> fetch_withdrawals_async(const std::optional<std::string>& code = std::nullopt,
                                                                  const std::optional<long long>& since = std::nullopt,
                                                                  const std::optional<int>& limit = std::nullopt) override;

    // Bitrue specific methods
    boost::future<std::vector<std::string>> fetch_funding_rates_async(const std::vector<std::string>& symbols = {}) override;
    boost::future<std::vector<std::string>> fetch_positions_async(const std::optional<std::string>& symbols = std::nullopt,
                                                                const std::optional<long long>& since = std::nullopt,
                                                                const std::optional<int>& limit = std::nullopt) override;
    boost::future<std::vector<std::string>> fetch_leverage_tiers_async(const std::vector<std::string>& symbols = {}) override;

private:
    boost::asio::io_context& context_;
};

} // namespace ccxt

#endif // CCXT_ASYNC_BITRUE_ASYNC_H
