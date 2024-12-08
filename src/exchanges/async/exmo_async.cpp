#include "ccxt/exchanges/async/exmo_async.h"
#include "ccxt/async_base/async_request.h"

namespace ccxt {

ExmoAsync::ExmoAsync(const boost::asio::io_context& context)
    : AsyncExchange(context)
    , Exmo()
    , context_(const_cast<boost::asio::io_context&>(context)) {}

// Market Data API
boost::future<std::vector<Market>> ExmoAsync::fetch_markets_async() {
    return async_request<std::vector<Market>>(context_, [this]() { return fetch_markets(); });
}

boost::future<Ticker> ExmoAsync::fetch_ticker_async(const std::string& symbol) {
    return async_request<Ticker>(context_, [this, symbol]() { return fetch_ticker(symbol); });
}

boost::future<std::map<std::string, Ticker>> ExmoAsync::fetch_tickers_async(const std::vector<std::string>& symbols) {
    return async_request<std::map<std::string, Ticker>>(context_, [this, symbols]() { return fetch_tickers(symbols); });
}

boost::future<OrderBook> ExmoAsync::fetch_order_book_async(const std::string& symbol, const std::optional<int>& limit) {
    return async_request<OrderBook>(context_, [this, symbol, limit]() {
        return fetch_order_book(symbol, limit.value_or(0));
    });
}

boost::future<std::vector<Trade>> ExmoAsync::fetch_trades_async(const std::string& symbol,
                                                              const std::optional<long long>& since,
                                                              const std::optional<int>& limit) {
    return async_request<std::vector<Trade>>(context_, [this, symbol, since, limit]() {
        return fetch_trades(symbol, since.value_or(0), limit.value_or(0));
    });
}

boost::future<std::vector<OHLCV>> ExmoAsync::fetch_ohlcv_async(const std::string& symbol,
                                                              const std::string& timeframe,
                                                              const std::optional<long long>& since,
                                                              const std::optional<int>& limit) {
    return async_request<std::vector<OHLCV>>(context_, [this, symbol, timeframe, since, limit]() {
        return fetch_ohlcv(symbol, timeframe, since.value_or(0), limit.value_or(0));
    });
}

// Trading API
boost::future<Balance> ExmoAsync::fetch_balance_async() {
    return async_request<Balance>(context_, [this]() { return fetch_balance(); });
}

boost::future<Order> ExmoAsync::create_order_async(const std::string& symbol,
                                                 const std::string& type,
                                                 const std::string& side,
                                                 double amount,
                                                 const std::optional<double>& price) {
    return async_request<Order>(context_, [this, symbol, type, side, amount, price]() {
        return create_order(symbol, type, side, amount, price.value_or(0));
    });
}

boost::future<Order> ExmoAsync::cancel_order_async(const std::string& id, const std::string& symbol) {
    return async_request<Order>(context_, [this, id, symbol]() { return cancel_order(id, symbol); });
}

boost::future<Order> ExmoAsync::fetch_order_async(const std::string& id, const std::string& symbol) {
    return async_request<Order>(context_, [this, id, symbol]() { return fetch_order(id, symbol); });
}

boost::future<std::vector<Order>> ExmoAsync::fetch_orders_async(const std::string& symbol,
                                                              const std::optional<long long>& since,
                                                              const std::optional<int>& limit) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit]() {
        return fetch_orders(symbol, since.value_or(0), limit.value_or(0));
    });
}

boost::future<std::vector<Order>> ExmoAsync::fetch_open_orders_async(const std::string& symbol,
                                                                   const std::optional<long long>& since,
                                                                   const std::optional<int>& limit) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit]() {
        return fetch_open_orders(symbol, since.value_or(0), limit.value_or(0));
    });
}

boost::future<std::vector<Order>> ExmoAsync::fetch_closed_orders_async(const std::string& symbol,
                                                                     const std::optional<long long>& since,
                                                                     const std::optional<int>& limit) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit]() {
        return fetch_closed_orders(symbol, since.value_or(0), limit.value_or(0));
    });
}

// Account API
boost::future<std::vector<Trade>> ExmoAsync::fetch_my_trades_async(const std::string& symbol,
                                                                 const std::optional<long long>& since,
                                                                 const std::optional<int>& limit) {
    return async_request<std::vector<Trade>>(context_, [this, symbol, since, limit]() {
        return fetch_my_trades(symbol, since.value_or(0), limit.value_or(0));
    });
}

boost::future<std::vector<Transaction>> ExmoAsync::fetch_deposits_async(const std::optional<std::string>& code,
                                                                      const std::optional<long long>& since,
                                                                      const std::optional<int>& limit) {
    return async_request<std::vector<Transaction>>(context_, [this, code, since, limit]() {
        return fetch_deposits(code.value_or(""), since.value_or(0), limit.value_or(0));
    });
}

boost::future<std::vector<Transaction>> ExmoAsync::fetch_withdrawals_async(const std::optional<std::string>& code,
                                                                         const std::optional<long long>& since,
                                                                         const std::optional<int>& limit) {
    return async_request<std::vector<Transaction>>(context_, [this, code, since, limit]() {
        return fetch_withdrawals(code.value_or(""), since.value_or(0), limit.value_or(0));
    });
}

boost::future<DepositAddress> ExmoAsync::fetch_deposit_address_async(const std::string& code) {
    return async_request<DepositAddress>(context_, [this, code]() { return fetch_deposit_address(code); });
}

boost::future<Transaction> ExmoAsync::withdraw_async(const std::string& code,
                                                   double amount,
                                                   const std::string& address,
                                                   const std::optional<std::string>& tag) {
    return async_request<Transaction>(context_, [this, code, amount, address, tag]() {
        return withdraw(code, amount, address, tag.value_or(""));
    });
}

// Additional Features
boost::future<std::map<std::string, Currency>> ExmoAsync::fetch_currencies_async() {
    return async_request<std::map<std::string, Currency>>(context_, [this]() { return fetch_currencies(); });
}

boost::future<std::map<std::string, Fee>> ExmoAsync::fetch_trading_fees_async() {
    return async_request<std::map<std::string, Fee>>(context_, [this]() { return fetch_trading_fees(); });
}

boost::future<std::vector<std::string>> ExmoAsync::fetch_required_ids_async() {
    return async_request<std::vector<std::string>>(context_, [this]() { return fetch_required_ids(); });
}

boost::future<std::vector<PaymentMethod>> ExmoAsync::fetch_payment_methods_async() {
    return async_request<std::vector<PaymentMethod>>(context_, [this]() { return fetch_payment_methods(); });
}

boost::future<std::vector<DepositMethod>> ExmoAsync::fetch_deposit_methods_async() {
    return async_request<std::vector<DepositMethod>>(context_, [this]() { return fetch_deposit_methods(); });
}

boost::future<std::vector<WithdrawMethod>> ExmoAsync::fetch_withdraw_methods_async() {
    return async_request<std::vector<WithdrawMethod>>(context_, [this]() { return fetch_withdraw_methods(); });
}

boost::future<std::vector<Transaction>> ExmoAsync::fetch_transactions_async(const std::optional<std::string>& code,
                                                                          const std::optional<long long>& since,
                                                                          const std::optional<int>& limit) {
    return async_request<std::vector<Transaction>>(context_, [this, code, since, limit]() {
        return fetch_transactions(code.value_or(""), since.value_or(0), limit.value_or(0));
    });
}

boost::future<std::vector<Trade>> ExmoAsync::fetch_order_trades_async(const std::string& id,
                                                                    const std::optional<std::string>& symbol) {
    return async_request<std::vector<Trade>>(context_, [this, id, symbol]() {
        return fetch_order_trades(id, symbol.value_or(""));
    });
}

boost::future<std::vector<Trade>> ExmoAsync::fetch_user_trades_async(const std::string& symbol,
                                                                   const std::optional<long long>& since,
                                                                   const std::optional<int>& limit) {
    return async_request<std::vector<Trade>>(context_, [this, symbol, since, limit]() {
        return fetch_user_trades(symbol, since.value_or(0), limit.value_or(0));
    });
}

} // namespace ccxt
