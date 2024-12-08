#include "ccxt/exchanges/async/fmfwio_async.h"
#include "ccxt/async_base/async_request.h"

namespace ccxt {

FmfwioAsync::FmfwioAsync(const boost::asio::io_context& context)
    : AsyncExchange(context)
    , Fmfwio()
    , context_(const_cast<boost::asio::io_context&>(context)) {}

// Market Data
boost::future<std::vector<Market>> FmfwioAsync::fetch_markets_async() {
    return async_request<std::vector<Market>>(context_, [this]() { return fetch_markets(); });
}

boost::future<Ticker> FmfwioAsync::fetch_ticker_async(const std::string& symbol) {
    return async_request<Ticker>(context_, [this, symbol]() { return fetch_ticker(symbol); });
}

boost::future<OrderBook> FmfwioAsync::fetch_order_book_async(const std::string& symbol, const std::optional<int>& limit) {
    return async_request<OrderBook>(context_, [this, symbol, limit]() {
        return fetch_order_book(symbol, limit.value_or(0));
    });
}

boost::future<std::vector<Trade>> FmfwioAsync::fetch_trades_async(const std::string& symbol,
                                                                const std::optional<long long>& since,
                                                                const std::optional<int>& limit) {
    return async_request<std::vector<Trade>>(context_, [this, symbol, since, limit]() {
        return fetch_trades(symbol, since.value_or(0), limit.value_or(0));
    });
}

boost::future<std::vector<OHLCV>> FmfwioAsync::fetch_ohlcv_async(const std::string& symbol,
                                                                const std::string& timeframe,
                                                                const std::optional<long long>& since,
                                                                const std::optional<int>& limit) {
    return async_request<std::vector<OHLCV>>(context_, [this, symbol, timeframe, since, limit]() {
        return fetch_ohlcv(symbol, timeframe, since.value_or(0), limit.value_or(0));
    });
}

// Trading
boost::future<Order> FmfwioAsync::create_order_async(const std::string& symbol,
                                                   const std::string& type,
                                                   const std::string& side,
                                                   double amount,
                                                   const std::optional<double>& price) {
    return async_request<Order>(context_, [this, symbol, type, side, amount, price]() {
        return create_order(symbol, type, side, amount, price.value_or(0));
    });
}

boost::future<Order> FmfwioAsync::cancel_order_async(const std::string& id, const std::string& symbol) {
    return async_request<Order>(context_, [this, id, symbol]() { return cancel_order(id, symbol); });
}

boost::future<std::vector<Order>> FmfwioAsync::cancel_all_orders_async(const std::string& symbol) {
    return async_request<std::vector<Order>>(context_, [this, symbol]() { return cancel_all_orders(symbol); });
}

boost::future<Order> FmfwioAsync::fetch_order_async(const std::string& id, const std::string& symbol) {
    return async_request<Order>(context_, [this, id, symbol]() { return fetch_order(id, symbol); });
}

boost::future<std::vector<Order>> FmfwioAsync::fetch_orders_async(const std::string& symbol,
                                                                const std::optional<long long>& since,
                                                                const std::optional<int>& limit) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit]() {
        return fetch_orders(symbol, since.value_or(0), limit.value_or(0));
    });
}

boost::future<std::vector<Order>> FmfwioAsync::fetch_open_orders_async(const std::string& symbol,
                                                                     const std::optional<long long>& since,
                                                                     const std::optional<int>& limit) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit]() {
        return fetch_open_orders(symbol, since.value_or(0), limit.value_or(0));
    });
}

boost::future<std::vector<Order>> FmfwioAsync::fetch_closed_orders_async(const std::string& symbol,
                                                                       const std::optional<long long>& since,
                                                                       const std::optional<int>& limit) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit]() {
        return fetch_closed_orders(symbol, since.value_or(0), limit.value_or(0));
    });
}

boost::future<std::vector<Trade>> FmfwioAsync::fetch_my_trades_async(const std::string& symbol,
                                                                   const std::optional<long long>& since,
                                                                   const std::optional<int>& limit) {
    return async_request<std::vector<Trade>>(context_, [this, symbol, since, limit]() {
        return fetch_my_trades(symbol, since.value_or(0), limit.value_or(0));
    });
}

// Account
boost::future<Balance> FmfwioAsync::fetch_balance_async() {
    return async_request<Balance>(context_, [this]() { return fetch_balance(); });
}

boost::future<DepositAddress> FmfwioAsync::fetch_deposit_address_async(const std::string& code) {
    return async_request<DepositAddress>(context_, [this, code]() { return fetch_deposit_address(code); });
}

boost::future<std::vector<Transaction>> FmfwioAsync::fetch_deposits_async(const std::optional<std::string>& code,
                                                                        const std::optional<long long>& since,
                                                                        const std::optional<int>& limit) {
    return async_request<std::vector<Transaction>>(context_, [this, code, since, limit]() {
        return fetch_deposits(code.value_or(""), since.value_or(0), limit.value_or(0));
    });
}

boost::future<std::vector<Transaction>> FmfwioAsync::fetch_withdrawals_async(const std::optional<std::string>& code,
                                                                           const std::optional<long long>& since,
                                                                           const std::optional<int>& limit) {
    return async_request<std::vector<Transaction>>(context_, [this, code, since, limit]() {
        return fetch_withdrawals(code.value_or(""), since.value_or(0), limit.value_or(0));
    });
}

boost::future<Transaction> FmfwioAsync::withdraw_async(const std::string& code,
                                                     double amount,
                                                     const std::string& address,
                                                     const std::optional<std::string>& tag) {
    return async_request<Transaction>(context_, [this, code, amount, address, tag]() {
        return withdraw(code, amount, address, tag.value_or(""));
    });
}

} // namespace ccxt
