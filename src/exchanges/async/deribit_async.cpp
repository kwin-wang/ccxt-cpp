#include "ccxt/exchanges/async/deribit_async.h"
#include "ccxt/async_base/async_request.h"

namespace ccxt {

DeribitAsync::DeribitAsync(const boost::asio::io_context& context)
    : AsyncExchange(context)
    , Deribit()
    , context_(const_cast<boost::asio::io_context&>(context)) {}

// Market Data API
boost::future<std::vector<Market>> DeribitAsync::fetch_markets_async() {
    return async_request<std::vector<Market>>(context_, [this]() { return fetch_markets(); });
}

boost::future<Ticker> DeribitAsync::fetch_ticker_async(const std::string& symbol) {
    return async_request<Ticker>(context_, [this, symbol]() { return fetch_ticker(symbol); });
}

boost::future<std::map<std::string, Ticker>> DeribitAsync::fetch_tickers_async(const std::vector<std::string>& symbols) {
    return async_request<std::map<std::string, Ticker>>(context_, [this, symbols]() { return fetch_tickers(symbols); });
}

boost::future<OrderBook> DeribitAsync::fetch_order_book_async(const std::string& symbol, const std::optional<int>& limit) {
    return async_request<OrderBook>(context_, [this, symbol, limit]() {
        return fetch_order_book(symbol, limit.value_or(0));
    });
}

boost::future<std::vector<Trade>> DeribitAsync::fetch_trades_async(const std::string& symbol,
                                                                 const std::optional<long long>& since,
                                                                 const std::optional<int>& limit) {
    return async_request<std::vector<Trade>>(context_, [this, symbol, since, limit]() {
        return fetch_trades(symbol, since.value_or(0), limit.value_or(0));
    });
}

boost::future<std::vector<OHLCV>> DeribitAsync::fetch_ohlcv_async(const std::string& symbol,
                                                                 const std::string& timeframe,
                                                                 const std::optional<long long>& since,
                                                                 const std::optional<int>& limit) {
    return async_request<std::vector<OHLCV>>(context_, [this, symbol, timeframe, since, limit]() {
        return fetch_ohlcv(symbol, timeframe, since.value_or(0), limit.value_or(0));
    });
}

// Trading API
boost::future<Balance> DeribitAsync::fetch_balance_async() {
    return async_request<Balance>(context_, [this]() { return fetch_balance(); });
}

boost::future<Order> DeribitAsync::create_order_async(const std::string& symbol,
                                                    const std::string& type,
                                                    const std::string& side,
                                                    double amount,
                                                    const std::optional<double>& price) {
    return async_request<Order>(context_, [this, symbol, type, side, amount, price]() {
        return create_order(symbol, type, side, amount, price.value_or(0));
    });
}

boost::future<Order> DeribitAsync::cancel_order_async(const std::string& id, const std::string& symbol) {
    return async_request<Order>(context_, [this, id, symbol]() { return cancel_order(id, symbol); });
}

boost::future<Order> DeribitAsync::fetch_order_async(const std::string& id, const std::string& symbol) {
    return async_request<Order>(context_, [this, id, symbol]() { return fetch_order(id, symbol); });
}

boost::future<std::vector<Order>> DeribitAsync::fetch_orders_async(const std::string& symbol,
                                                                 const std::optional<long long>& since,
                                                                 const std::optional<int>& limit) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit]() {
        return fetch_orders(symbol, since.value_or(0), limit.value_or(0));
    });
}

boost::future<std::vector<Order>> DeribitAsync::fetch_open_orders_async(const std::string& symbol,
                                                                      const std::optional<long long>& since,
                                                                      const std::optional<int>& limit) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit]() {
        return fetch_open_orders(symbol, since.value_or(0), limit.value_or(0));
    });
}

boost::future<std::vector<Order>> DeribitAsync::fetch_closed_orders_async(const std::string& symbol,
                                                                        const std::optional<long long>& since,
                                                                        const std::optional<int>& limit) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit]() {
        return fetch_closed_orders(symbol, since.value_or(0), limit.value_or(0));
    });
}

// Deribit specific methods
boost::future<std::vector<Position>> DeribitAsync::fetch_positions_async(const std::string& symbol) {
    return async_request<std::vector<Position>>(context_, [this, symbol]() { return fetch_positions(symbol); });
}

boost::future<std::vector<Transaction>> DeribitAsync::fetch_deposits_async(const std::optional<std::string>& code,
                                                                         const std::optional<long long>& since,
                                                                         const std::optional<int>& limit) {
    return async_request<std::vector<Transaction>>(context_, [this, code, since, limit]() {
        return fetch_deposits(code.value_or(""), since.value_or(0), limit.value_or(0));
    });
}

boost::future<std::vector<Transaction>> DeribitAsync::fetch_withdrawals_async(const std::optional<std::string>& code,
                                                                            const std::optional<long long>& since,
                                                                            const std::optional<int>& limit) {
    return async_request<std::vector<Transaction>>(context_, [this, code, since, limit]() {
        return fetch_withdrawals(code.value_or(""), since.value_or(0), limit.value_or(0));
    });
}

boost::future<std::vector<Trade>> DeribitAsync::fetch_my_trades_async(const std::string& symbol,
                                                                    const std::optional<long long>& since,
                                                                    const std::optional<int>& limit) {
    return async_request<std::vector<Trade>>(context_, [this, symbol, since, limit]() {
        return fetch_my_trades(symbol, since.value_or(0), limit.value_or(0));
    });
}

boost::future<std::vector<LedgerEntry>> DeribitAsync::fetch_ledger_async(const std::optional<std::string>& code,
                                                                        const std::optional<long long>& since,
                                                                        const std::optional<int>& limit) {
    return async_request<std::vector<LedgerEntry>>(context_, [this, code, since, limit]() {
        return fetch_ledger(code.value_or(""), since.value_or(0), limit.value_or(0));
    });
}

boost::future<OpenInterest> DeribitAsync::fetch_open_interest_async(const std::string& symbol) {
    return async_request<OpenInterest>(context_, [this, symbol]() { return fetch_open_interest(symbol); });
}

boost::future<std::vector<OptionContract>> DeribitAsync::fetch_option_chain_async(const std::string& underlying) {
    return async_request<std::vector<OptionContract>>(context_, [this, underlying]() { return fetch_option_chain(underlying); });
}

boost::future<std::vector<VolatilityData>> DeribitAsync::fetch_volatility_history_async(const std::string& symbol) {
    return async_request<std::vector<VolatilityData>>(context_, [this, symbol]() { return fetch_volatility_history(symbol); });
}

boost::future<void> DeribitAsync::set_margin_mode_async(const std::string& marginMode) {
    return async_request<void>(context_, [this, marginMode]() { return set_margin_mode(marginMode); });
}

boost::future<void> DeribitAsync::set_leverage_async(const std::string& symbol, double leverage) {
    return async_request<void>(context_, [this, symbol, leverage]() { return set_leverage(symbol, leverage); });
}

} // namespace ccxt
