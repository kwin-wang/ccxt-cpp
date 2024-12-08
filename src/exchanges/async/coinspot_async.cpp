#include "ccxt/exchanges/async/coinspot_async.h"
#include "ccxt/async_base/async_request.h"

namespace ccxt {

CoinspotAsync::CoinspotAsync(const boost::asio::io_context& context)
    : AsyncExchange(context)
    , coinspot()
    , context_(const_cast<boost::asio::io_context&>(context)) {}

// Market Data
boost::future<std::vector<Market>> CoinspotAsync::fetch_markets_async() {
    return async_request<std::vector<Market>>(context_, [this]() { return fetch_markets(); });
}

boost::future<Ticker> CoinspotAsync::fetch_ticker_async(const std::string& symbol) {
    return async_request<Ticker>(context_, [this, symbol]() { return fetch_ticker(symbol); });
}

boost::future<std::map<std::string, Ticker>> CoinspotAsync::fetch_tickers_async(const std::vector<std::string>& symbols) {
    return async_request<std::map<std::string, Ticker>>(context_, [this, symbols]() { return fetch_tickers(symbols); });
}

boost::future<OrderBook> CoinspotAsync::fetch_order_book_async(const std::string& symbol, const std::optional<int>& limit) {
    return async_request<OrderBook>(context_, [this, symbol, limit]() { return fetch_order_book(symbol, limit); });
}

// Trading
boost::future<Order> CoinspotAsync::create_order_async(const std::string& symbol,
                                                     const std::string& type,
                                                     const std::string& side,
                                                     double amount,
                                                     const std::optional<double>& price) {
    return async_request<Order>(context_, [this, symbol, type, side, amount, price]() {
        return create_order(symbol, type, side, amount, price);
    });
}

boost::future<Order> CoinspotAsync::cancel_order_async(const std::string& id, const std::string& symbol) {
    return async_request<Order>(context_, [this, id, symbol]() { return cancel_order(id, symbol); });
}

boost::future<std::vector<Trade>> CoinspotAsync::fetch_my_trades_async(const std::string& symbol,
                                                                     const std::optional<long long>& since,
                                                                     const std::optional<int>& limit) {
    return async_request<std::vector<Trade>>(context_, [this, symbol, since, limit]() {
        return fetch_my_trades(symbol, since, limit);
    });
}

// Account
boost::future<Balance> CoinspotAsync::fetch_balance_async() {
    return async_request<Balance>(context_, [this]() { return fetch_balance(); });
}

} // namespace ccxt
