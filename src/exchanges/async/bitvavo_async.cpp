#include "ccxt/exchanges/async/bitvavo_async.h"

namespace ccxt {

BitvavoAsync::BitvavoAsync(const boost::asio::io_context& context, const Config& config)
    : AsyncExchange(context, config)
    , bitvavo(config)
    , context_(const_cast<boost::asio::io_context&>(context)) {}

// Market Data API
boost::future<std::vector<Market>> BitvavoAsync::fetch_markets_async() {
    return async_request<std::vector<Market>>([this]() {
        return this->fetch_markets();
    });
}

boost::future<std::vector<Currency>> BitvavoAsync::fetch_currencies_async() {
    return async_request<std::vector<Currency>>([this]() {
        return this->fetch_currencies();
    });
}

boost::future<Ticker> BitvavoAsync::fetch_ticker_async(const std::string& symbol) {
    return async_request<Ticker>([this, symbol]() {
        return this->fetch_ticker(symbol);
    });
}

boost::future<std::vector<Ticker>> BitvavoAsync::fetch_tickers_async(const std::vector<std::string>& symbols) {
    return async_request<std::vector<Ticker>>([this, symbols]() {
        return this->fetch_tickers(symbols);
    });
}

boost::future<OrderBook> BitvavoAsync::fetch_order_book_async(const std::string& symbol,
                                                          const std::optional<int>& limit) {
    return async_request<OrderBook>([this, symbol, limit]() {
        return this->fetch_order_book(symbol, limit);
    });
}

boost::future<std::vector<Trade>> BitvavoAsync::fetch_trades_async(const std::string& symbol,
                                                               const std::optional<long long>& since,
                                                               const std::optional<int>& limit) {
    return async_request<std::vector<Trade>>([this, symbol, since, limit]() {
        return this->fetch_trades(symbol, since, limit);
    });
}

boost::future<std::vector<OHLCV>> BitvavoAsync::fetch_ohlcv_async(const std::string& symbol,
                                                               const std::string& timeframe,
                                                               const std::optional<long long>& since,
                                                               const std::optional<int>& limit) {
    return async_request<std::vector<OHLCV>>([this, symbol, timeframe, since, limit]() {
        return this->fetch_ohlcv(symbol, timeframe, since, limit);
    });
}

// Trading API
boost::future<Order> BitvavoAsync::create_order_async(const std::string& symbol,
                                                  const std::string& type,
                                                  const std::string& side,
                                                  double amount,
                                                  const std::optional<double>& price) {
    return async_request<Order>([this, symbol, type, side, amount, price]() {
        return this->create_order(symbol, type, side, amount, price);
    });
}

boost::future<Order> BitvavoAsync::cancel_order_async(const std::string& id,
                                                  const std::string& symbol) {
    return async_request<Order>([this, id, symbol]() {
        return this->cancel_order(id, symbol);
    });
}

boost::future<Order> BitvavoAsync::fetch_order_async(const std::string& id,
                                                 const std::string& symbol) {
    return async_request<Order>([this, id, symbol]() {
        return this->fetch_order(id, symbol);
    });
}

boost::future<std::vector<Order>> BitvavoAsync::fetch_open_orders_async(const std::string& symbol,
                                                                    const std::optional<long long>& since,
                                                                    const std::optional<int>& limit) {
    return async_request<std::vector<Order>>([this, symbol, since, limit]() {
        return this->fetch_open_orders(symbol, since, limit);
    });
}

boost::future<std::vector<Order>> BitvavoAsync::fetch_closed_orders_async(const std::string& symbol,
                                                                      const std::optional<long long>& since,
                                                                      const std::optional<int>& limit) {
    return async_request<std::vector<Order>>([this, symbol, since, limit]() {
        return this->fetch_closed_orders(symbol, since, limit);
    });
}

boost::future<std::vector<Trade>> BitvavoAsync::fetch_my_trades_async(const std::string& symbol,
                                                                  const std::optional<long long>& since,
                                                                  const std::optional<int>& limit) {
    return async_request<std::vector<Trade>>([this, symbol, since, limit]() {
        return this->fetch_my_trades(symbol, since, limit);
    });
}

// Account API
boost::future<Balance> BitvavoAsync::fetch_balance_async() {
    return async_request<Balance>([this]() {
        return this->fetch_balance();
    });
}

boost::future<DepositAddress> BitvavoAsync::fetch_deposit_address_async(const std::string& code,
                                                                    const std::optional<std::string>& network) {
    return async_request<DepositAddress>([this, code, network]() {
        return this->fetch_deposit_address(code, network);
    });
}

boost::future<std::vector<Transaction>> BitvavoAsync::fetch_deposits_async(const std::optional<std::string>& code,
                                                                       const std::optional<long long>& since,
                                                                       const std::optional<int>& limit) {
    return async_request<std::vector<Transaction>>([this, code, since, limit]() {
        return this->fetch_deposits(code, since, limit);
    });
}

boost::future<std::vector<Transaction>> BitvavoAsync::fetch_withdrawals_async(const std::optional<std::string>& code,
                                                                          const std::optional<long long>& since,
                                                                          const std::optional<int>& limit) {
    return async_request<std::vector<Transaction>>([this, code, since, limit]() {
        return this->fetch_withdrawals(code, since, limit);
    });
}

// Bitvavo specific methods
boost::future<std::vector<std::string>> BitvavoAsync::fetch_funding_rates_async(const std::vector<std::string>& symbols) {
    return async_request<std::vector<std::string>>([this, symbols]() {
        return this->fetch_funding_rates(symbols);
    });
}

boost::future<std::vector<std::string>> BitvavoAsync::fetch_positions_async(const std::optional<std::string>& symbols,
                                                                        const std::optional<long long>& since,
                                                                        const std::optional<int>& limit) {
    return async_request<std::vector<std::string>>([this, symbols, since, limit]() {
        return this->fetch_positions(symbols, since, limit);
    });
}

} // namespace ccxt
