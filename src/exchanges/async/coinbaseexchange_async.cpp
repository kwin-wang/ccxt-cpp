#include "ccxt/exchanges/async/coinbaseexchange_async.h"
#include "ccxt/async_base/async_utils.h"
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace ccxt {

CoinbaseExchangeAsync::CoinbaseExchangeAsync(const boost::asio::io_context& context)
    : AsyncExchange(context)
    , CoinbaseExchange()
    , context_(const_cast<boost::asio::io_context&>(context)) {}

// Market Data
boost::future<std::vector<Market>> CoinbaseExchangeAsync::fetch_markets_async() {
    return async_request<std::vector<Market>>(context_, [this]() { return fetch_markets(); });
}

boost::future<std::vector<Currency>> CoinbaseExchangeAsync::fetch_currencies_async() {
    return async_request<std::vector<Currency>>(context_, [this]() { return fetch_currencies(); });
}

boost::future<OrderBook> CoinbaseExchangeAsync::fetch_order_book_async(const std::string& symbol, int limit) {
    return async_request<OrderBook>(context_, [this, symbol, limit]() { return fetch_order_book(symbol, limit); });
}

boost::future<std::map<std::string, Ticker>> CoinbaseExchangeAsync::fetch_tickers_async() {
    return async_request<std::map<std::string, Ticker>>(context_, [this]() { return fetch_tickers(); });
}

boost::future<Ticker> CoinbaseExchangeAsync::fetch_ticker_async(const std::string& symbol) {
    return async_request<Ticker>(context_, [this, symbol]() { return fetch_ticker(symbol); });
}

boost::future<std::vector<Trade>> CoinbaseExchangeAsync::fetch_trades_async(const std::string& symbol, long since, int limit) {
    return async_request<std::vector<Trade>>(context_, [this, symbol, since, limit]() { return fetch_trades(symbol, since, limit); });
}

boost::future<long> CoinbaseExchangeAsync::fetch_time_async() {
    return async_request<long>(context_, [this]() { return fetch_time(); });
}

// Trading
boost::future<Order> CoinbaseExchangeAsync::create_order_async(const std::string& symbol,
                                                             const std::string& type,
                                                             const std::string& side,
                                                             double amount,
                                                             double price,
                                                             const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, symbol, type, side, amount, price, params]() {
        return create_order(symbol, type, side, amount, price, params);
    });
}

boost::future<Order> CoinbaseExchangeAsync::cancel_order_async(const std::string& id,
                                                             const std::string& symbol,
                                                             const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, id, symbol, params]() { return cancel_order(id, symbol, params); });
}

boost::future<std::vector<Order>> CoinbaseExchangeAsync::fetch_orders_async(const std::string& symbol,
                                                                          long since,
                                                                          int limit) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit]() { return fetch_orders(symbol, since, limit); });
}

boost::future<std::vector<Order>> CoinbaseExchangeAsync::fetch_open_orders_async(const std::string& symbol) {
    return async_request<std::vector<Order>>(context_, [this, symbol]() { return fetch_open_orders(symbol); });
}

boost::future<std::vector<Order>> CoinbaseExchangeAsync::fetch_closed_orders_async(const std::string& symbol,
                                                                                 long since,
                                                                                 int limit) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit]() { return fetch_closed_orders(symbol, since, limit); });
}

boost::future<Order> CoinbaseExchangeAsync::fetch_order_async(const std::string& id, const std::string& symbol) {
    return async_request<Order>(context_, [this, id, symbol]() { return fetch_order(id, symbol); });
}

// Account
boost::future<std::vector<Account>> CoinbaseExchangeAsync::fetch_accounts_async() {
    return async_request<std::vector<Account>>(context_, [this]() { return fetch_accounts(); });
}

boost::future<Balance> CoinbaseExchangeAsync::fetch_balance_async() {
    return async_request<Balance>(context_, [this]() { return fetch_balance(); });
}

boost::future<std::map<std::string, TradingFee>> CoinbaseExchangeAsync::fetch_trading_fees_async() {
    return async_request<std::map<std::string, TradingFee>>(context_, [this]() { return fetch_trading_fees(); });
}

boost::future<std::vector<LedgerEntry>> CoinbaseExchangeAsync::fetch_ledger_async(const std::string& code,
                                                                                 long since,
                                                                                 int limit,
                                                                                 const std::map<std::string, std::string>& params) {
    return async_request<std::vector<LedgerEntry>>(context_, [this, code, since, limit, params]() {
        return fetch_ledger(code, since, limit, params);
    });
}

// Funding
boost::future<std::vector<Transaction>> CoinbaseExchangeAsync::fetch_deposits_async(const std::string& code,
                                                                                  long since,
                                                                                  int limit,
                                                                                  const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Transaction>>(context_, [this, code, since, limit, params]() {
        return fetch_deposits(code, since, limit, params);
    });
}

boost::future<std::vector<Transaction>> CoinbaseExchangeAsync::fetch_withdrawals_async(const std::string& code,
                                                                                     long since,
                                                                                     int limit,
                                                                                     const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Transaction>>(context_, [this, code, since, limit, params]() {
        return fetch_withdrawals(code, since, limit, params);
    });
}

boost::future<DepositAddress> CoinbaseExchangeAsync::fetch_deposit_address_async(const std::string& code,
                                                                               const std::map<std::string, std::string>& params) {
    return async_request<DepositAddress>(context_, [this, code, params]() { return fetch_deposit_address(code, params); });
}

// Advanced Trading
boost::future<std::vector<Position>> CoinbaseExchangeAsync::fetch_positions_async(const std::vector<std::string>& symbols,
                                                                                const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Position>>(context_, [this, symbols, params]() { return fetch_positions(symbols, params); });
}

boost::future<MarginMode> CoinbaseExchangeAsync::set_margin_mode_async(const std::string& symbol,
                                                                      const std::string& marginMode,
                                                                      const std::map<std::string, std::string>& params) {
    return async_request<MarginMode>(context_, [this, symbol, marginMode, params]() { return set_margin_mode(symbol, marginMode, params); });
}

} // namespace ccxt
