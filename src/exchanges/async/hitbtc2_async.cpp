#include "ccxt/exchanges/async/hitbtc2_async.h"
#include "ccxt/async_base/async_request.h"

namespace ccxt {

Hitbtc2Async::Hitbtc2Async(const boost::asio::io_context& context)
    : AsyncExchange(context)
    , Hitbtc2()
    , context_(const_cast<boost::asio::io_context&>(context)) {}

// Market Data API
boost::future<std::vector<Market>> Hitbtc2Async::fetch_markets_async(const json& params) {
    return async_request<std::vector<Market>>(context_, [this, params]() {
        return fetch_markets(params);
    });
}

boost::future<Ticker> Hitbtc2Async::fetch_ticker_async(const String& symbol, const json& params) {
    return async_request<Ticker>(context_, [this, symbol, params]() {
        return fetch_ticker(symbol, params);
    });
}

boost::future<std::vector<Ticker>> Hitbtc2Async::fetch_tickers_async(const std::vector<String>& symbols,
                                                                   const json& params) {
    return async_request<std::vector<Ticker>>(context_, [this, symbols, params]() {
        return fetch_tickers(symbols, params);
    });
}

boost::future<OrderBook> Hitbtc2Async::fetch_order_book_async(const String& symbol,
                                                            const std::optional<int>& limit,
                                                            const json& params) {
    return async_request<OrderBook>(context_, [this, symbol, limit, params]() {
        return fetch_order_book(symbol, limit.value_or(0), params);
    });
}

boost::future<std::vector<Trade>> Hitbtc2Async::fetch_trades_async(const String& symbol,
                                                                 const std::optional<long long>& since,
                                                                 const std::optional<int>& limit,
                                                                 const json& params) {
    return async_request<std::vector<Trade>>(context_, [this, symbol, since, limit, params]() {
        return fetch_trades(symbol, since.value_or(0), limit.value_or(0), params);
    });
}

boost::future<std::vector<OHLCV>> Hitbtc2Async::fetch_ohlcv_async(const String& symbol,
                                                                 const String& timeframe,
                                                                 const std::optional<long long>& since,
                                                                 const std::optional<int>& limit,
                                                                 const json& params) {
    return async_request<std::vector<OHLCV>>(context_, [this, symbol, timeframe, since, limit, params]() {
        return fetch_ohlcv(symbol, timeframe, since.value_or(0), limit.value_or(0), params);
    });
}

// Trading API
boost::future<Balance> Hitbtc2Async::fetch_balance_async(const json& params) {
    return async_request<Balance>(context_, [this, params]() {
        return fetch_balance(params);
    });
}

boost::future<Order> Hitbtc2Async::create_order_async(const String& symbol,
                                                    const String& type,
                                                    const String& side,
                                                    double amount,
                                                    const std::optional<double>& price,
                                                    const json& params) {
    return async_request<Order>(context_, [this, symbol, type, side, amount, price, params]() {
        return create_order(symbol, type, side, amount, price.value_or(0), params);
    });
}

boost::future<Order> Hitbtc2Async::cancel_order_async(const String& id,
                                                    const String& symbol,
                                                    const json& params) {
    return async_request<Order>(context_, [this, id, symbol, params]() {
        return cancel_order(id, symbol, params);
    });
}

boost::future<std::vector<Order>> Hitbtc2Async::cancel_all_orders_async(const String& symbol,
                                                                      const json& params) {
    return async_request<std::vector<Order>>(context_, [this, symbol, params]() {
        return cancel_all_orders(symbol, params);
    });
}

boost::future<Order> Hitbtc2Async::fetch_order_async(const String& id,
                                                   const String& symbol,
                                                   const json& params) {
    return async_request<Order>(context_, [this, id, symbol, params]() {
        return fetch_order(id, symbol, params);
    });
}

boost::future<std::vector<Order>> Hitbtc2Async::fetch_orders_async(const String& symbol,
                                                                 const std::optional<long long>& since,
                                                                 const std::optional<int>& limit,
                                                                 const json& params) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit, params]() {
        return fetch_orders(symbol, since.value_or(0), limit.value_or(0), params);
    });
}

boost::future<std::vector<Order>> Hitbtc2Async::fetch_open_orders_async(const String& symbol,
                                                                      const std::optional<long long>& since,
                                                                      const std::optional<int>& limit,
                                                                      const json& params) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit, params]() {
        return fetch_open_orders(symbol, since.value_or(0), limit.value_or(0), params);
    });
}

boost::future<std::vector<Order>> Hitbtc2Async::fetch_closed_orders_async(const String& symbol,
                                                                        const std::optional<long long>& since,
                                                                        const std::optional<int>& limit,
                                                                        const json& params) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit, params]() {
        return fetch_closed_orders(symbol, since.value_or(0), limit.value_or(0), params);
    });
}

boost::future<std::vector<Trade>> Hitbtc2Async::fetch_my_trades_async(const String& symbol,
                                                                    const std::optional<long long>& since,
                                                                    const std::optional<int>& limit,
                                                                    const json& params) {
    return async_request<std::vector<Trade>>(context_, [this, symbol, since, limit, params]() {
        return fetch_my_trades(symbol, since.value_or(0), limit.value_or(0), params);
    });
}

// Account API
boost::future<json> Hitbtc2Async::fetch_deposit_address_async(const String& code,
                                                            const json& params) {
    return async_request<json>(context_, [this, code, params]() {
        return fetch_deposit_address(code, params);
    });
}

boost::future<std::vector<Transaction>> Hitbtc2Async::fetch_deposits_async(const String& code,
                                                                         const std::optional<long long>& since,
                                                                         const std::optional<int>& limit,
                                                                         const json& params) {
    return async_request<std::vector<Transaction>>(context_, [this, code, since, limit, params]() {
        return fetch_deposits(code, since.value_or(0), limit.value_or(0), params);
    });
}

boost::future<std::vector<Transaction>> Hitbtc2Async::fetch_withdrawals_async(const String& code,
                                                                            const std::optional<long long>& since,
                                                                            const std::optional<int>& limit,
                                                                            const json& params) {
    return async_request<std::vector<Transaction>>(context_, [this, code, since, limit, params]() {
        return fetch_withdrawals(code, since.value_or(0), limit.value_or(0), params);
    });
}

boost::future<Transaction> Hitbtc2Async::withdraw_async(const String& code,
                                                      double amount,
                                                      const String& address,
                                                      const std::optional<String>& tag,
                                                      const json& params) {
    return async_request<Transaction>(context_, [this, code, amount, address, tag, params]() {
        return withdraw(code, amount, address, tag.value_or(""), params);
    });
}

} // namespace ccxt
