#include "ccxt/exchanges/async/coincatch_async.h"
#include "ccxt/async_base/async_utils.h"
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace ccxt {

CoincatchAsync::CoincatchAsync(const boost::asio::io_context& context)
    : AsyncExchange(context)
    , coincatch()
    , context_(const_cast<boost::asio::io_context&>(context)) {}

// Market Data
boost::future<std::vector<Market>> CoincatchAsync::fetch_markets_async() {
    return async_request<std::vector<Market>>(context_, [this]() { return fetch_markets(); });
}

boost::future<std::vector<Currency>> CoincatchAsync::fetch_currencies_async() {
    return async_request<std::vector<Currency>>(context_, [this]() { return fetch_currencies(); });
}

boost::future<OrderBook> CoincatchAsync::fetch_order_book_async(const std::string& symbol, int limit) {
    return async_request<OrderBook>(context_, [this, symbol, limit]() { return fetch_order_book(symbol, limit); });
}

boost::future<std::map<std::string, Ticker>> CoincatchAsync::fetch_tickers_async() {
    return async_request<std::map<std::string, Ticker>>(context_, [this]() { return fetch_tickers(); });
}

boost::future<Ticker> CoincatchAsync::fetch_ticker_async(const std::string& symbol) {
    return async_request<Ticker>(context_, [this, symbol]() { return fetch_ticker(symbol); });
}

boost::future<std::vector<Trade>> CoincatchAsync::fetch_trades_async(const std::string& symbol, long since, int limit) {
    return async_request<std::vector<Trade>>(context_, [this, symbol, since, limit]() { return fetch_trades(symbol, since, limit); });
}

boost::future<std::vector<OHLCV>> CoincatchAsync::fetch_ohlcv_async(const std::string& symbol,
                                                                   const std::string& timeframe,
                                                                   long since,
                                                                   int limit) {
    return async_request<std::vector<OHLCV>>(context_, [this, symbol, timeframe, since, limit]() {
        return fetch_ohlcv(symbol, timeframe, since, limit);
    });
}

boost::future<double> CoincatchAsync::fetch_leverage_async(const std::string& symbol,
                                                         const std::map<std::string, std::string>& params) {
    return async_request<double>(context_, [this, symbol, params]() { return fetch_leverage(symbol, params); });
}

boost::future<double> CoincatchAsync::fetch_funding_rate_async(const std::string& symbol,
                                                             const std::map<std::string, std::string>& params) {
    return async_request<double>(context_, [this, symbol, params]() { return fetch_funding_rate(symbol, params); });
}

boost::future<std::vector<FundingRateHistory>> CoincatchAsync::fetch_funding_rate_history_async(const std::string& symbol,
                                                                                               long since,
                                                                                               int limit) {
    return async_request<std::vector<FundingRateHistory>>(context_, [this, symbol, since, limit]() {
        return fetch_funding_rate_history(symbol, since, limit);
    });
}

// Trading
boost::future<Order> CoincatchAsync::create_order_async(const std::string& symbol,
                                                      const std::string& type,
                                                      const std::string& side,
                                                      double amount,
                                                      double price,
                                                      const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, symbol, type, side, amount, price, params]() {
        return create_order(symbol, type, side, amount, price, params);
    });
}

boost::future<Order> CoincatchAsync::create_market_order_with_cost_async(const std::string& symbol,
                                                                       const std::string& side,
                                                                       double cost,
                                                                       const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, symbol, side, cost, params]() {
        return create_market_order_with_cost(symbol, side, cost, params);
    });
}

boost::future<Order> CoincatchAsync::create_stop_limit_order_async(const std::string& symbol,
                                                                 const std::string& side,
                                                                 double amount,
                                                                 double price,
                                                                 double stopPrice,
                                                                 const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, symbol, side, amount, price, stopPrice, params]() {
        return create_stop_limit_order(symbol, side, amount, price, stopPrice, params);
    });
}

boost::future<Order> CoincatchAsync::create_stop_market_order_async(const std::string& symbol,
                                                                  const std::string& side,
                                                                  double amount,
                                                                  double stopPrice,
                                                                  const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, symbol, side, amount, stopPrice, params]() {
        return create_stop_market_order(symbol, side, amount, stopPrice, params);
    });
}

boost::future<Order> CoincatchAsync::create_take_profit_order_async(const std::string& symbol,
                                                                  const std::string& type,
                                                                  const std::string& side,
                                                                  double amount,
                                                                  double price,
                                                                  const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, symbol, type, side, amount, price, params]() {
        return create_take_profit_order(symbol, type, side, amount, price, params);
    });
}

boost::future<Order> CoincatchAsync::cancel_order_async(const std::string& id,
                                                      const std::string& symbol,
                                                      const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, id, symbol, params]() { return cancel_order(id, symbol, params); });
}

boost::future<std::vector<Order>> CoincatchAsync::cancel_orders_async(const std::vector<std::string>& ids,
                                                                    const std::string& symbol,
                                                                    const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Order>>(context_, [this, ids, symbol, params]() {
        return cancel_orders(ids, symbol, params);
    });
}

boost::future<std::vector<Order>> CoincatchAsync::cancel_all_orders_async(const std::string& symbol,
                                                                        const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Order>>(context_, [this, symbol, params]() { return cancel_all_orders(symbol, params); });
}

// Account
boost::future<Balance> CoincatchAsync::fetch_balance_async() {
    return async_request<Balance>(context_, [this]() { return fetch_balance(); });
}

boost::future<std::vector<LedgerEntry>> CoincatchAsync::fetch_ledger_async(const std::string& code,
                                                                          long since,
                                                                          int limit,
                                                                          const std::map<std::string, std::string>& params) {
    return async_request<std::vector<LedgerEntry>>(context_, [this, code, since, limit, params]() {
        return fetch_ledger(code, since, limit, params);
    });
}

boost::future<DepositAddress> CoincatchAsync::fetch_deposit_address_async(const std::string& code,
                                                                        const std::string& network,
                                                                        const std::map<std::string, std::string>& params) {
    return async_request<DepositAddress>(context_, [this, code, network, params]() {
        return fetch_deposit_address(code, network, params);
    });
}

boost::future<std::vector<Transaction>> CoincatchAsync::fetch_deposits_async(const std::string& code,
                                                                           long since,
                                                                           int limit,
                                                                           const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Transaction>>(context_, [this, code, since, limit, params]() {
        return fetch_deposits(code, since, limit, params);
    });
}

boost::future<MarginAddition> CoincatchAsync::add_margin_async(const std::string& symbol,
                                                             double amount,
                                                             const std::map<std::string, std::string>& params) {
    return async_request<MarginAddition>(context_, [this, symbol, amount, params]() {
        return add_margin(symbol, amount, params);
    });
}

} // namespace ccxt
