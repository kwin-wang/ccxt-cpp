#include "ccxt/exchanges/async/coinbase_async.h"
#include "ccxt/async_base/async_utils.h"
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace ccxt {

CoinbaseAsync::CoinbaseAsync(const boost::asio::io_context& context)
    : AsyncExchange(context)
    , Coinbase()
    , context_(const_cast<boost::asio::io_context&>(context)) {}

// Market Data
boost::future<std::vector<Market>> CoinbaseAsync::fetch_markets_async() {
    return async_request<std::vector<Market>>(context_, [this]() { return fetch_markets(); });
}

boost::future<std::vector<Currency>> CoinbaseAsync::fetch_currencies_async() {
    return async_request<std::vector<Currency>>(context_, [this]() { return fetch_currencies(); });
}

boost::future<OrderBook> CoinbaseAsync::fetch_order_book_async(const std::string& symbol, int limit) {
    return async_request<OrderBook>(context_, [this, symbol, limit]() { return fetch_order_book(symbol, limit); });
}

boost::future<std::map<std::string, Ticker>> CoinbaseAsync::fetch_tickers_async() {
    return async_request<std::map<std::string, Ticker>>(context_, [this]() { return fetch_tickers(); });
}

boost::future<Ticker> CoinbaseAsync::fetch_ticker_async(const std::string& symbol) {
    return async_request<Ticker>(context_, [this, symbol]() { return fetch_ticker(symbol); });
}

boost::future<std::vector<Trade>> CoinbaseAsync::fetch_trades_async(const std::string& symbol, long since, int limit) {
    return async_request<std::vector<Trade>>(context_, [this, symbol, since, limit]() { return fetch_trades(symbol, since, limit); });
}

// Trading
boost::future<Order> CoinbaseAsync::create_order_async(const std::string& symbol,
                                                     const std::string& type,
                                                     const std::string& side,
                                                     double amount,
                                                     double price,
                                                     const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, symbol, type, side, amount, price, params]() {
        return create_order(symbol, type, side, amount, price, params);
    });
}

boost::future<Order> CoinbaseAsync::create_limit_buy_order_async(const std::string& symbol,
                                                               double amount,
                                                               double price,
                                                               const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, symbol, amount, price, params]() {
        return create_limit_buy_order(symbol, amount, price, params);
    });
}

boost::future<Order> CoinbaseAsync::create_limit_sell_order_async(const std::string& symbol,
                                                                double amount,
                                                                double price,
                                                                const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, symbol, amount, price, params]() {
        return create_limit_sell_order(symbol, amount, price, params);
    });
}

boost::future<Order> CoinbaseAsync::create_market_buy_order_async(const std::string& symbol,
                                                                double amount,
                                                                const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, symbol, amount, params]() {
        return create_market_buy_order(symbol, amount, params);
    });
}

boost::future<Order> CoinbaseAsync::create_market_sell_order_async(const std::string& symbol,
                                                                 double amount,
                                                                 const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, symbol, amount, params]() {
        return create_market_sell_order(symbol, amount, params);
    });
}

boost::future<Order> CoinbaseAsync::create_stop_order_async(const std::string& symbol,
                                                          const std::string& type,
                                                          const std::string& side,
                                                          double amount,
                                                          double price,
                                                          const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, symbol, type, side, amount, price, params]() {
        return create_stop_order(symbol, type, side, amount, price, params);
    });
}

boost::future<Order> CoinbaseAsync::edit_order_async(const std::string& id,
                                                   const std::string& symbol,
                                                   const std::string& type,
                                                   const std::string& side,
                                                   double amount,
                                                   double price,
                                                   const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, id, symbol, type, side, amount, price, params]() {
        return edit_order(id, symbol, type, side, amount, price, params);
    });
}

boost::future<Order> CoinbaseAsync::cancel_order_async(const std::string& id,
                                                     const std::string& symbol,
                                                     const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, id, symbol, params]() { return cancel_order(id, symbol, params); });
}

boost::future<std::vector<Order>> CoinbaseAsync::cancel_orders_async(const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Order>>(context_, [this, params]() { return cancel_orders(params); });
}

boost::future<Order> CoinbaseAsync::fetch_order_async(const std::string& id, const std::string& symbol) {
    return async_request<Order>(context_, [this, id, symbol]() { return fetch_order(id, symbol); });
}

boost::future<std::vector<Order>> CoinbaseAsync::fetch_open_orders_async(const std::string& symbol) {
    return async_request<std::vector<Order>>(context_, [this, symbol]() { return fetch_open_orders(symbol); });
}

boost::future<std::vector<Order>> CoinbaseAsync::fetch_closed_orders_async(const std::string& symbol, long since, int limit) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit]() { return fetch_closed_orders(symbol, since, limit); });
}

boost::future<std::vector<Order>> CoinbaseAsync::fetch_canceled_orders_async(const std::string& symbol, long since, int limit) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit]() { return fetch_canceled_orders(symbol, since, limit); });
}

// Account
boost::future<std::vector<Account>> CoinbaseAsync::fetch_accounts_async() {
    return async_request<std::vector<Account>>(context_, [this]() { return fetch_accounts(); });
}

boost::future<Balance> CoinbaseAsync::fetch_balance_async() {
    return async_request<Balance>(context_, [this]() { return fetch_balance(); });
}

boost::future<std::vector<LedgerEntry>> CoinbaseAsync::fetch_ledger_async(const std::string& code,
                                                                         long since,
                                                                         int limit,
                                                                         const std::map<std::string, std::string>& params) {
    return async_request<std::vector<LedgerEntry>>(context_, [this, code, since, limit, params]() {
        return fetch_ledger(code, since, limit, params);
    });
}

// Funding
boost::future<DepositAddress> CoinbaseAsync::create_deposit_address_async(const std::string& code,
                                                                        const std::map<std::string, std::string>& params) {
    return async_request<DepositAddress>(context_, [this, code, params]() { return create_deposit_address(code, params); });
}

boost::future<std::map<std::string, DepositAddress>> CoinbaseAsync::fetch_deposit_addresses_by_network_async(const std::string& code,
                                                                                                           const std::map<std::string, std::string>& params) {
    return async_request<std::map<std::string, DepositAddress>>(context_, [this, code, params]() {
        return fetch_deposit_addresses_by_network(code, params);
    });
}

boost::future<Transaction> CoinbaseAsync::fetch_deposit_async(const std::string& id,
                                                            const std::string& code,
                                                            const std::map<std::string, std::string>& params) {
    return async_request<Transaction>(context_, [this, id, code, params]() { return fetch_deposit(id, code, params); });
}

boost::future<std::vector<Transaction>> CoinbaseAsync::fetch_deposits_async(const std::string& code,
                                                                          long since,
                                                                          int limit,
                                                                          const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Transaction>>(context_, [this, code, since, limit, params]() {
        return fetch_deposits(code, since, limit, params);
    });
}

boost::future<std::vector<Transaction>> CoinbaseAsync::fetch_withdrawals_async(const std::string& code,
                                                                             long since,
                                                                             int limit,
                                                                             const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Transaction>>(context_, [this, code, since, limit, params]() {
        return fetch_withdrawals(code, since, limit, params);
    });
}

boost::future<std::vector<Transaction>> CoinbaseAsync::fetch_deposits_withdrawals_async(const std::string& code,
                                                                                      long since,
                                                                                      int limit,
                                                                                      const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Transaction>>(context_, [this, code, since, limit, params]() {
        return fetch_deposits_withdrawals(code, since, limit, params);
    });
}

// Convert
boost::future<Conversion> CoinbaseAsync::create_convert_trade_async(const std::string& fromCurrency,
                                                                  const std::string& toCurrency,
                                                                  double amount,
                                                                  const std::map<std::string, std::string>& params) {
    return async_request<Conversion>(context_, [this, fromCurrency, toCurrency, amount, params]() {
        return create_convert_trade(fromCurrency, toCurrency, amount, params);
    });
}

boost::future<Conversion> CoinbaseAsync::fetch_convert_trade_async(const std::string& id,
                                                                 const std::map<std::string, std::string>& params) {
    return async_request<Conversion>(context_, [this, id, params]() { return fetch_convert_trade(id, params); });
}

boost::future<Conversion> CoinbaseAsync::fetch_convert_quote_async(const std::string& fromCurrency,
                                                                 const std::string& toCurrency,
                                                                 double amount,
                                                                 const std::map<std::string, std::string>& params) {
    return async_request<Conversion>(context_, [this, fromCurrency, toCurrency, amount, params]() {
        return fetch_convert_quote(fromCurrency, toCurrency, amount, params);
    });
}

} // namespace ccxt
