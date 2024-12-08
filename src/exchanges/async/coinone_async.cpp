#include "ccxt/exchanges/async/coinone_async.h"
#include "ccxt/async_base/async_utils.h"
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace ccxt {

CoinoneAsync::CoinoneAsync(const boost::asio::io_context& context)
    : AsyncExchange(context)
    , Coinone()
    , context_(const_cast<boost::asio::io_context&>(context)) {}

// Market Data API
boost::future<std::vector<Market>> CoinoneAsync::fetch_markets_async(const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Market>>(context_, [this, params]() {
        return fetch_markets(params);
    });
}

boost::future<Ticker> CoinoneAsync::fetch_ticker_async(const std::string& symbol, const std::map<std::string, std::string>& params) {
    return async_request<Ticker>(context_, [this, symbol, params]() {
        return fetch_ticker(symbol, params);
    });
}

boost::future<std::map<std::string, Ticker>> CoinoneAsync::fetch_tickers_async(const std::vector<std::string>& symbols, const std::map<std::string, std::string>& params) {
    return async_request<std::map<std::string, Ticker>>(context_, [this, symbols, params]() {
        return fetch_tickers(symbols, params);
    });
}

boost::future<OrderBook> CoinoneAsync::fetch_order_book_async(const std::string& symbol, int limit, const std::map<std::string, std::string>& params) {
    return async_request<OrderBook>(context_, [this, symbol, limit, params]() {
        return fetch_order_book(symbol, limit, params);
    });
}

boost::future<std::vector<Trade>> CoinoneAsync::fetch_trades_async(const std::string& symbol, long since, int limit, const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Trade>>(context_, [this, symbol, since, limit, params]() {
        return fetch_trades(symbol, since, limit, params);
    });
}

boost::future<std::vector<OHLCV>> CoinoneAsync::fetch_ohlcv_async(const std::string& symbol, const std::string& timeframe,
                                                                 long since, int limit, const std::map<std::string, std::string>& params) {
    return async_request<std::vector<OHLCV>>(context_, [this, symbol, timeframe, since, limit, params]() {
        return fetch_ohlcv(symbol, timeframe, since, limit, params);
    });
}

// Trading API
boost::future<Balance> CoinoneAsync::fetch_balance_async(const std::map<std::string, std::string>& params) {
    return async_request<Balance>(context_, [this, params]() {
        return fetch_balance(params);
    });
}

boost::future<Order> CoinoneAsync::create_order_async(const std::string& symbol, const std::string& type, const std::string& side,
                                                    double amount, double price, const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, symbol, type, side, amount, price, params]() {
        return create_order(symbol, type, side, amount, price, params);
    });
}

boost::future<Order> CoinoneAsync::cancel_order_async(const std::string& id, const std::string& symbol, const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, id, symbol, params]() {
        return cancel_order(id, symbol, params);
    });
}

boost::future<Order> CoinoneAsync::fetch_order_async(const std::string& id, const std::string& symbol, const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, id, symbol, params]() {
        return fetch_order(id, symbol, params);
    });
}

boost::future<std::vector<Order>> CoinoneAsync::fetch_orders_async(const std::string& symbol, long since, int limit, const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit, params]() {
        return fetch_orders(symbol, since, limit, params);
    });
}

boost::future<std::vector<Order>> CoinoneAsync::fetch_open_orders_async(const std::string& symbol, long since, int limit, const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit, params]() {
        return fetch_open_orders(symbol, since, limit, params);
    });
}

boost::future<std::vector<Order>> CoinoneAsync::fetch_closed_orders_async(const std::string& symbol, long since, int limit, const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit, params]() {
        return fetch_closed_orders(symbol, since, limit, params);
    });
}

// Account API
boost::future<std::vector<Trade>> CoinoneAsync::fetch_my_trades_async(const std::string& symbol, long since, int limit, const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Trade>>(context_, [this, symbol, since, limit, params]() {
        return fetch_my_trades(symbol, since, limit, params);
    });
}

boost::future<std::vector<Transaction>> CoinoneAsync::fetch_deposits_async(const std::string& code, long since, int limit, const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Transaction>>(context_, [this, code, since, limit, params]() {
        return fetch_deposits(code, since, limit, params);
    });
}

boost::future<std::vector<Transaction>> CoinoneAsync::fetch_withdrawals_async(const std::string& code, long since, int limit, const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Transaction>>(context_, [this, code, since, limit, params]() {
        return fetch_withdrawals(code, since, limit, params);
    });
}

boost::future<DepositAddress> CoinoneAsync::fetch_deposit_address_async(const std::string& code, const std::map<std::string, std::string>& params) {
    return async_request<DepositAddress>(context_, [this, code, params]() {
        return fetch_deposit_address(code, params);
    });
}

boost::future<Transaction> CoinoneAsync::withdraw_async(const std::string& code, double amount, const std::string& address,
                                                      const std::string& tag, const std::map<std::string, std::string>& params) {
    return async_request<Transaction>(context_, [this, code, amount, address, tag, params]() {
        return withdraw(code, amount, address, tag, params);
    });
}

// Additional Features
boost::future<std::vector<Currency>> CoinoneAsync::fetch_currencies_async(const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Currency>>(context_, [this, params]() {
        return fetch_currencies(params);
    });
}

boost::future<std::map<std::string, Fee>> CoinoneAsync::fetch_trading_fees_async(const std::map<std::string, std::string>& params) {
    return async_request<std::map<std::string, Fee>>(context_, [this, params]() {
        return fetch_trading_fees(params);
    });
}

boost::future<long long> CoinoneAsync::fetch_time_async(const std::map<std::string, std::string>& params) {
    return async_request<long long>(context_, [this, params]() {
        return fetch_time(params);
    });
}

boost::future<ExchangeStatus> CoinoneAsync::fetch_status_async(const std::map<std::string, std::string>& params) {
    return async_request<ExchangeStatus>(context_, [this, params]() {
        return fetch_status(params);
    });
}

boost::future<std::vector<Order>> CoinoneAsync::fetch_limit_orders_async(const std::string& symbol, long since, int limit, const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit, params]() {
        return fetch_limit_orders(symbol, since, limit, params);
    });
}

boost::future<std::vector<Order>> CoinoneAsync::fetch_market_orders_async(const std::string& symbol, long since, int limit, const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit, params]() {
        return fetch_market_orders(symbol, since, limit, params);
    });
}

boost::future<std::vector<Transaction>> CoinoneAsync::fetch_transaction_history_async(const std::string& code, long since, int limit, const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Transaction>>(context_, [this, code, since, limit, params]() {
        return fetch_transaction_history(code, since, limit, params);
    });
}

} // namespace ccxt
