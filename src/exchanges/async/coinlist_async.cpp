#include "ccxt/exchanges/async/coinlist_async.h"
#include "ccxt/async_base/async_utils.h"
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace ccxt {

CoinlistAsync::CoinlistAsync(const boost::asio::io_context& context)
    : AsyncExchange(context)
    , coinlist()
    , context_(const_cast<boost::asio::io_context&>(context)) {}

// Market Data
boost::future<std::vector<Market>> CoinlistAsync::fetch_markets_async() {
    return async_request<std::vector<Market>>(context_, [this]() {
        return fetch_markets();
    });
}

boost::future<std::vector<Currency>> CoinlistAsync::fetch_currencies_async() {
    return async_request<std::vector<Currency>>(context_, [this]() {
        return fetch_currencies();
    });
}

boost::future<Ticker> CoinlistAsync::fetch_ticker_async(const std::string& symbol) {
    return async_request<Ticker>(context_, [this, symbol]() {
        return fetch_ticker(symbol);
    });
}

boost::future<std::map<std::string, Ticker>> CoinlistAsync::fetch_tickers_async(const std::vector<std::string>& symbols) {
    return async_request<std::map<std::string, Ticker>>(context_, [this, symbols]() {
        return fetch_tickers(symbols);
    });
}

boost::future<OrderBook> CoinlistAsync::fetch_order_book_async(const std::string& symbol,
                                                             const std::optional<int>& limit) {
    return async_request<OrderBook>(context_, [this, symbol, limit]() {
        return fetch_order_book(symbol, limit);
    });
}

boost::future<std::vector<OHLCV>> CoinlistAsync::fetch_ohlcv_async(const std::string& symbol,
                                                                  const std::string& timeframe,
                                                                  const std::optional<long long>& since,
                                                                  const std::optional<int>& limit) {
    return async_request<std::vector<OHLCV>>(context_, [this, symbol, timeframe, since, limit]() {
        return fetch_ohlcv(symbol, timeframe, since, limit);
    });
}

// Trading
boost::future<Order> CoinlistAsync::create_order_async(const std::string& symbol,
                                                     const std::string& type,
                                                     const std::string& side,
                                                     double amount,
                                                     const std::optional<double>& price) {
    return async_request<Order>(context_, [this, symbol, type, side, amount, price]() {
        return create_order(symbol, type, side, amount, price);
    });
}

boost::future<Order> CoinlistAsync::cancel_order_async(const std::string& id,
                                                     const std::string& symbol) {
    return async_request<Order>(context_, [this, id, symbol]() {
        return cancel_order(id, symbol);
    });
}

boost::future<Order> CoinlistAsync::fetch_order_async(const std::string& id,
                                                    const std::string& symbol) {
    return async_request<Order>(context_, [this, id, symbol]() {
        return fetch_order(id, symbol);
    });
}

boost::future<std::vector<Order>> CoinlistAsync::fetch_open_orders_async(const std::string& symbol,
                                                                       const std::optional<long long>& since,
                                                                       const std::optional<int>& limit) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit]() {
        return fetch_open_orders(symbol, since, limit);
    });
}

boost::future<std::vector<Order>> CoinlistAsync::fetch_closed_orders_async(const std::string& symbol,
                                                                         const std::optional<long long>& since,
                                                                         const std::optional<int>& limit) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit]() {
        return fetch_closed_orders(symbol, since, limit);
    });
}

boost::future<std::vector<Trade>> CoinlistAsync::fetch_my_trades_async(const std::string& symbol,
                                                                     const std::optional<long long>& since,
                                                                     const std::optional<int>& limit) {
    return async_request<std::vector<Trade>>(context_, [this, symbol, since, limit]() {
        return fetch_my_trades(symbol, since, limit);
    });
}

// Account
boost::future<Balance> CoinlistAsync::fetch_balance_async() {
    return async_request<Balance>(context_, [this]() {
        return fetch_balance();
    });
}

boost::future<std::vector<LedgerEntry>> CoinlistAsync::fetch_ledger_async(const std::optional<std::string>& code,
                                                                         const std::optional<long long>& since,
                                                                         const std::optional<int>& limit) {
    return async_request<std::vector<LedgerEntry>>(context_, [this, code, since, limit]() {
        return fetch_ledger(code, since, limit);
    });
}

} // namespace ccxt
