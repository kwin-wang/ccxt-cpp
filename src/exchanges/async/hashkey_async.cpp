#include "ccxt/exchanges/async/hashkey_async.h"
#include "ccxt/async_base/async_request.h"

namespace ccxt {

HashkeyAsync::HashkeyAsync(const boost::asio::io_context& context)
    : AsyncExchange(context)
    , Hashkey()
    , context_(const_cast<boost::asio::io_context&>(context)) {}

// Market Data API
boost::future<std::vector<Market>> HashkeyAsync::fetch_markets_async() {
    return async_request<std::vector<Market>>(context_, [this]() {
        return fetch_markets();
    });
}

boost::future<Ticker> HashkeyAsync::fetch_ticker_async(const std::string& symbol) {
    return async_request<Ticker>(context_, [this, symbol]() {
        return fetch_ticker(symbol);
    });
}

boost::future<std::vector<Ticker>> HashkeyAsync::fetch_tickers_async(const std::vector<std::string>& symbols) {
    return async_request<std::vector<Ticker>>(context_, [this, symbols]() {
        return fetch_tickers(symbols);
    });
}

boost::future<OrderBook> HashkeyAsync::fetch_order_book_async(const std::string& symbol,
                                                            const std::optional<int>& limit) {
    return async_request<OrderBook>(context_, [this, symbol, limit]() {
        return fetch_order_book(symbol, limit.value_or(0));
    });
}

boost::future<std::vector<Trade>> HashkeyAsync::fetch_trades_async(const std::string& symbol,
                                                                 const std::optional<int>& limit) {
    return async_request<std::vector<Trade>>(context_, [this, symbol, limit]() {
        return fetch_trades(symbol, limit.value_or(0));
    });
}

boost::future<Balance> HashkeyAsync::fetch_balance_async() {
    return async_request<Balance>(context_, [this]() {
        return fetch_balance();
    });
}

boost::future<std::vector<OHLCV>> HashkeyAsync::fetch_ohlcv_async(const std::string& symbol,
                                                                 const std::string& timeframe,
                                                                 const std::optional<long>& since,
                                                                 const std::optional<int>& limit) {
    return async_request<std::vector<OHLCV>>(context_, [this, symbol, timeframe, since, limit]() {
        return fetch_ohlcv(symbol, timeframe, since.value_or(0), limit.value_or(0));
    });
}

// Trading API
boost::future<Order> HashkeyAsync::create_order_async(const std::string& symbol,
                                                    const std::string& type,
                                                    const std::string& side,
                                                    double amount,
                                                    const std::optional<double>& price) {
    return async_request<Order>(context_, [this, symbol, type, side, amount, price]() {
        return create_order(symbol, type, side, amount, price.value_or(0.0));
    });
}

boost::future<Order> HashkeyAsync::cancel_order_async(const std::string& id,
                                                    const std::string& symbol) {
    return async_request<Order>(context_, [this, id, symbol]() {
        return cancel_order(id, symbol);
    });
}

boost::future<std::vector<Order>> HashkeyAsync::cancel_all_orders_async(const std::string& symbol) {
    return async_request<std::vector<Order>>(context_, [this, symbol]() {
        return cancel_all_orders(symbol);
    });
}

boost::future<Order> HashkeyAsync::fetch_order_async(const std::string& id,
                                                   const std::string& symbol) {
    return async_request<Order>(context_, [this, id, symbol]() {
        return fetch_order(id, symbol);
    });
}

boost::future<std::vector<Order>> HashkeyAsync::fetch_orders_async(const std::string& symbol,
                                                                 const std::optional<int>& limit) {
    return async_request<std::vector<Order>>(context_, [this, symbol, limit]() {
        return fetch_orders(symbol, limit.value_or(0));
    });
}

boost::future<std::vector<Order>> HashkeyAsync::fetch_open_orders_async(const std::string& symbol,
                                                                      const std::optional<int>& limit) {
    return async_request<std::vector<Order>>(context_, [this, symbol, limit]() {
        return fetch_open_orders(symbol, limit.value_or(0));
    });
}

boost::future<std::vector<Order>> HashkeyAsync::fetch_closed_orders_async(const std::string& symbol,
                                                                        const std::optional<int>& limit) {
    return async_request<std::vector<Order>>(context_, [this, symbol, limit]() {
        return fetch_closed_orders(symbol, limit.value_or(0));
    });
}

boost::future<std::vector<Trade>> HashkeyAsync::fetch_my_trades_async(const std::string& symbol,
                                                                    const std::optional<int>& since,
                                                                    const std::optional<int>& limit) {
    return async_request<std::vector<Trade>>(context_, [this, symbol, since, limit]() {
        return fetch_my_trades(symbol, since.value_or(0), limit.value_or(0));
    });
}

} // namespace ccxt
