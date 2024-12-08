#include "ccxt/exchanges/async/coinmate_async.h"
#include "ccxt/async_base/async_utils.h"
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace ccxt {

CoinmateAsync::CoinmateAsync(const boost::asio::io_context& context)
    : AsyncExchange(context)
    , Coinmate()
    , context_(const_cast<boost::asio::io_context&>(context)) {}

// Market Data
boost::future<std::vector<Market>> CoinmateAsync::fetch_markets_async() {
    return async_request<std::vector<Market>>(context_, [this]() {
        return fetch_markets();
    });
}

boost::future<Ticker> CoinmateAsync::fetch_ticker_async(const std::string& symbol) {
    return async_request<Ticker>(context_, [this, symbol]() {
        return fetch_ticker(symbol);
    });
}

boost::future<OrderBook> CoinmateAsync::fetch_order_book_async(const std::string& symbol,
                                                             int limit) {
    return async_request<OrderBook>(context_, [this, symbol, limit]() {
        return fetch_order_book(symbol, limit);
    });
}

boost::future<std::vector<Trade>> CoinmateAsync::fetch_trades_async(const std::string& symbol,
                                                                  int limit) {
    return async_request<std::vector<Trade>>(context_, [this, symbol, limit]() {
        return fetch_trades(symbol, limit);
    });
}

boost::future<Balance> CoinmateAsync::fetch_balance_async() {
    return async_request<Balance>(context_, [this]() {
        return fetch_balance();
    });
}

// Trading
boost::future<Order> CoinmateAsync::create_order_async(const std::string& symbol,
                                                     const std::string& type,
                                                     const std::string& side,
                                                     double amount,
                                                     double price) {
    return async_request<Order>(context_, [this, symbol, type, side, amount, price]() {
        return create_order(symbol, type, side, amount, price);
    });
}

boost::future<Order> CoinmateAsync::cancel_order_async(const std::string& id,
                                                     const std::string& symbol) {
    return async_request<Order>(context_, [this, id, symbol]() {
        return cancel_order(id, symbol);
    });
}

boost::future<Order> CoinmateAsync::fetch_order_async(const std::string& id,
                                                    const std::string& symbol) {
    return async_request<Order>(context_, [this, id, symbol]() {
        return fetch_order(id, symbol);
    });
}

boost::future<std::vector<Order>> CoinmateAsync::fetch_orders_async(const std::string& symbol,
                                                                  int limit) {
    return async_request<std::vector<Order>>(context_, [this, symbol, limit]() {
        return fetch_orders(symbol, limit);
    });
}

boost::future<std::vector<Order>> CoinmateAsync::fetch_open_orders_async(const std::string& symbol) {
    return async_request<std::vector<Order>>(context_, [this, symbol]() {
        return fetch_open_orders(symbol);
    });
}

boost::future<std::vector<Trade>> CoinmateAsync::fetch_my_trades_async(const std::string& symbol,
                                                                     int limit) {
    return async_request<std::vector<Trade>>(context_, [this, symbol, limit]() {
        return fetch_my_trades(symbol, limit);
    });
}

} // namespace ccxt
