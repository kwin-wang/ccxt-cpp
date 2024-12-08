#include "ccxt/exchanges/async/coinex_async.h"
#include "ccxt/async_base/async_utils.h"
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace ccxt {

CoinExAsync::CoinExAsync(const boost::asio::io_context& context)
    : AsyncExchange(context)
    , CoinEx()
    , context_(const_cast<boost::asio::io_context&>(context)) {}

// Market Data
boost::future<std::vector<Market>> CoinExAsync::fetch_markets_async() {
    return async_request<std::vector<Market>>(context_, [this]() { return fetch_markets(); });
}

boost::future<Ticker> CoinExAsync::fetch_ticker_async(const std::string& symbol) {
    return async_request<Ticker>(context_, [this, symbol]() { return fetch_ticker(symbol); });
}

boost::future<OrderBook> CoinExAsync::fetch_order_book_async(const std::string& symbol, int limit) {
    return async_request<OrderBook>(context_, [this, symbol, limit]() { return fetch_order_book(symbol, limit); });
}

boost::future<std::vector<Trade>> CoinExAsync::fetch_trades_async(const std::string& symbol, long since, int limit) {
    return async_request<std::vector<Trade>>(context_, [this, symbol, since, limit]() {
        return fetch_trades(symbol, since, limit);
    });
}

boost::future<std::vector<OHLCV>> CoinExAsync::fetch_ohlcv_async(const std::string& symbol,
                                                               const std::string& timeframe,
                                                               long since,
                                                               int limit) {
    return async_request<std::vector<OHLCV>>(context_, [this, symbol, timeframe, since, limit]() {
        return fetch_ohlcv(symbol, timeframe, since, limit);
    });
}

// Trading
boost::future<Order> CoinExAsync::create_order_async(const std::string& symbol,
                                                   const std::string& type,
                                                   const std::string& side,
                                                   double amount,
                                                   double price) {
    return async_request<Order>(context_, [this, symbol, type, side, amount, price]() {
        return create_order(symbol, type, side, amount, price);
    });
}

boost::future<Order> CoinExAsync::cancel_order_async(const std::string& id, const std::string& symbol) {
    return async_request<Order>(context_, [this, id, symbol]() { return cancel_order(id, symbol); });
}

boost::future<std::vector<Order>> CoinExAsync::cancel_all_orders_async(const std::string& symbol) {
    return async_request<std::vector<Order>>(context_, [this, symbol]() { return cancel_all_orders(symbol); });
}

boost::future<Order> CoinExAsync::fetch_order_async(const std::string& id, const std::string& symbol) {
    return async_request<Order>(context_, [this, id, symbol]() { return fetch_order(id, symbol); });
}

boost::future<std::vector<Order>> CoinExAsync::fetch_orders_async(const std::string& symbol, long since, int limit) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit]() {
        return fetch_orders(symbol, since, limit);
    });
}

boost::future<std::vector<Order>> CoinExAsync::fetch_open_orders_async(const std::string& symbol, long since, int limit) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit]() {
        return fetch_open_orders(symbol, since, limit);
    });
}

boost::future<std::vector<Order>> CoinExAsync::fetch_closed_orders_async(const std::string& symbol, long since, int limit) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit]() {
        return fetch_closed_orders(symbol, since, limit);
    });
}

boost::future<std::vector<Trade>> CoinExAsync::fetch_my_trades_async(const std::string& symbol, long since, int limit) {
    return async_request<std::vector<Trade>>(context_, [this, symbol, since, limit]() {
        return fetch_my_trades(symbol, since, limit);
    });
}

// Account
boost::future<Balance> CoinExAsync::fetch_balance_async() {
    return async_request<Balance>(context_, [this]() { return fetch_balance(); });
}

boost::future<DepositAddress> CoinExAsync::fetch_deposit_address_async(const std::string& code) {
    return async_request<DepositAddress>(context_, [this, code]() { return fetch_deposit_address(code); });
}

boost::future<std::vector<Transaction>> CoinExAsync::fetch_deposits_async(const std::string& code, long since, int limit) {
    return async_request<std::vector<Transaction>>(context_, [this, code, since, limit]() {
        return fetch_deposits(code, since, limit);
    });
}

boost::future<std::vector<Transaction>> CoinExAsync::fetch_withdrawals_async(const std::string& code, long since, int limit) {
    return async_request<std::vector<Transaction>>(context_, [this, code, since, limit]() {
        return fetch_withdrawals(code, since, limit);
    });
}

boost::future<Transaction> CoinExAsync::withdraw_async(const std::string& code,
                                                    double amount,
                                                    const std::string& address,
                                                    const std::string& tag,
                                                    const std::map<std::string, std::string>& params) {
    return async_request<Transaction>(context_, [this, code, amount, address, tag, params]() {
        return withdraw(code, amount, address, tag, params);
    });
}

// Margin Trading
boost::future<Balance> CoinExAsync::fetch_margin_balance_async() {
    return async_request<Balance>(context_, [this]() { return fetch_margin_balance(); });
}

boost::future<Order> CoinExAsync::create_margin_order_async(const std::string& symbol,
                                                         const std::string& type,
                                                         const std::string& side,
                                                         double amount,
                                                         double price) {
    return async_request<Order>(context_, [this, symbol, type, side, amount, price]() {
        return create_margin_order(symbol, type, side, amount, price);
    });
}

boost::future<MarginLoan> CoinExAsync::borrow_margin_async(const std::string& code,
                                                         double amount,
                                                         const std::string& symbol) {
    return async_request<MarginLoan>(context_, [this, code, amount, symbol]() {
        return borrow_margin(code, amount, symbol);
    });
}

boost::future<MarginLoan> CoinExAsync::repay_margin_async(const std::string& code,
                                                        double amount,
                                                        const std::string& symbol) {
    return async_request<MarginLoan>(context_, [this, code, amount, symbol]() {
        return repay_margin(code, amount, symbol);
    });
}

} // namespace ccxt
