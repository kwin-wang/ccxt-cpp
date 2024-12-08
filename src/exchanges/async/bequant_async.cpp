#include "ccxt/exchanges/async/bequant_async.h"
#include "ccxt/async_base/async_utils.h"
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace ccxt {

BequantAsync::BequantAsync(const boost::asio::io_context& context)
    : AsyncExchange(context)
    , Bequant()
    , context_(const_cast<boost::asio::io_context&>(context)) {}

// Market Data
boost::future<std::vector<Market>> BequantAsync::fetch_markets_async() {
    return async_request<std::vector<Market>>(context_, [this]() { return fetch_markets(); });
}

boost::future<std::vector<Currency>> BequantAsync::fetch_currencies_async() {
    return async_request<std::vector<Currency>>(context_, [this]() { return fetch_currencies(); });
}

boost::future<OrderBook> BequantAsync::fetch_order_book_async(const std::string& symbol, int limit) {
    return async_request<OrderBook>(context_, [this, symbol, limit]() { return fetch_order_book(symbol, limit); });
}

boost::future<std::map<std::string, OrderBook>> BequantAsync::fetch_order_books_async(const std::vector<std::string>& symbols, int limit) {
    return async_request<std::map<std::string, OrderBook>>(context_, [this, symbols, limit]() { return fetch_order_books(symbols, limit); });
}

boost::future<std::map<std::string, Ticker>> BequantAsync::fetch_tickers_async() {
    return async_request<std::map<std::string, Ticker>>(context_, [this]() { return fetch_tickers(); });
}

boost::future<Ticker> BequantAsync::fetch_ticker_async(const std::string& symbol) {
    return async_request<Ticker>(context_, [this, symbol]() { return fetch_ticker(symbol); });
}

boost::future<std::vector<Trade>> BequantAsync::fetch_trades_async(const std::string& symbol, long since, int limit) {
    return async_request<std::vector<Trade>>(context_, [this, symbol, since, limit]() { return fetch_trades(symbol, since, limit); });
}

boost::future<std::vector<OHLCV>> BequantAsync::fetch_ohlcv_async(const std::string& symbol, const std::string& timeframe, long since, int limit) {
    return async_request<std::vector<OHLCV>>(context_, [this, symbol, timeframe, since, limit]() { return fetch_ohlcv(symbol, timeframe, since, limit); });
}

boost::future<std::vector<OHLCV>> BequantAsync::fetch_index_ohlcv_async(const std::string& symbol, const std::string& timeframe, long since, int limit) {
    return async_request<std::vector<OHLCV>>(context_, [this, symbol, timeframe, since, limit]() { return fetch_index_ohlcv(symbol, timeframe, since, limit); });
}

boost::future<std::vector<OHLCV>> BequantAsync::fetch_mark_ohlcv_async(const std::string& symbol, const std::string& timeframe, long since, int limit) {
    return async_request<std::vector<OHLCV>>(context_, [this, symbol, timeframe, since, limit]() { return fetch_mark_ohlcv(symbol, timeframe, since, limit); });
}

boost::future<double> BequantAsync::fetch_open_interest_async(const std::string& symbol) {
    return async_request<double>(context_, [this, symbol]() { return fetch_open_interest(symbol); });
}

// Trading
boost::future<Order> BequantAsync::create_order_async(const std::string& symbol,
                                                    const std::string& type,
                                                    const std::string& side,
                                                    double amount,
                                                    double price,
                                                    const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, symbol, type, side, amount, price, params]() {
        return create_order(symbol, type, side, amount, price, params);
    });
}

boost::future<Order> BequantAsync::create_stop_order_async(const std::string& symbol,
                                                         const std::string& type,
                                                         const std::string& side,
                                                         double amount,
                                                         double price,
                                                         const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, symbol, type, side, amount, price, params]() {
        return create_stop_order(symbol, type, side, amount, price, params);
    });
}

boost::future<Order> BequantAsync::edit_order_async(const std::string& id,
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

boost::future<Order> BequantAsync::cancel_order_async(const std::string& id,
                                                    const std::string& symbol,
                                                    const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, id, symbol, params]() { return cancel_order(id, symbol, params); });
}

boost::future<std::vector<Order>> BequantAsync::cancel_all_orders_async(const std::string& symbol,
                                                                      const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Order>>(context_, [this, symbol, params]() { return cancel_all_orders(symbol, params); });
}

boost::future<Order> BequantAsync::fetch_order_async(const std::string& id, const std::string& symbol) {
    return async_request<Order>(context_, [this, id, symbol]() { return fetch_order(id, symbol); });
}

boost::future<Order> BequantAsync::fetch_open_order_async(const std::string& id, const std::string& symbol) {
    return async_request<Order>(context_, [this, id, symbol]() { return fetch_open_order(id, symbol); });
}

boost::future<std::vector<Order>> BequantAsync::fetch_open_orders_async(const std::string& symbol) {
    return async_request<std::vector<Order>>(context_, [this, symbol]() { return fetch_open_orders(symbol); });
}

boost::future<std::vector<Order>> BequantAsync::fetch_closed_orders_async(const std::string& symbol, long since, int limit) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit]() { return fetch_closed_orders(symbol, since, limit); });
}

boost::future<std::vector<Trade>> BequantAsync::fetch_my_trades_async(const std::string& symbol, long since, int limit) {
    return async_request<std::vector<Trade>>(context_, [this, symbol, since, limit]() { return fetch_my_trades(symbol, since, limit); });
}

boost::future<std::vector<Trade>> BequantAsync::fetch_order_trades_async(const std::string& id,
                                                                       const std::string& symbol,
                                                                       long since,
                                                                       int limit) {
    return async_request<std::vector<Trade>>(context_, [this, id, symbol, since, limit]() { return fetch_order_trades(id, symbol, since, limit); });
}

// Account
boost::future<Balance> BequantAsync::fetch_balance_async() {
    return async_request<Balance>(context_, [this]() { return fetch_balance(); });
}

boost::future<MarginModification> BequantAsync::add_margin_async(const std::string& symbol,
                                                               double amount,
                                                               const std::map<std::string, std::string>& params) {
    return async_request<MarginModification>(context_, [this, symbol, amount, params]() { return add_margin(symbol, amount, params); });
}

boost::future<Leverage> BequantAsync::fetch_leverage_async(const std::string& symbol,
                                                         const std::map<std::string, std::string>& params) {
    return async_request<Leverage>(context_, [this, symbol, params]() { return fetch_leverage(symbol, params); });
}

boost::future<std::map<std::string, MarginMode>> BequantAsync::fetch_margin_modes_async(const std::vector<std::string>& symbols) {
    return async_request<std::map<std::string, MarginMode>>(context_, [this, symbols]() { return fetch_margin_modes(symbols); });
}

// Funding
boost::future<DepositAddress> BequantAsync::create_deposit_address_async(const std::string& code,
                                                                       const std::map<std::string, std::string>& params) {
    return async_request<DepositAddress>(context_, [this, code, params]() { return create_deposit_address(code, params); });
}

boost::future<DepositAddress> BequantAsync::fetch_deposit_address_async(const std::string& code,
                                                                      const std::map<std::string, std::string>& params) {
    return async_request<DepositAddress>(context_, [this, code, params]() { return fetch_deposit_address(code, params); });
}

boost::future<std::vector<Transaction>> BequantAsync::fetch_deposits_async(const std::string& code,
                                                                         long since,
                                                                         int limit,
                                                                         const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Transaction>>(context_, [this, code, since, limit, params]() {
        return fetch_deposits(code, since, limit, params);
    });
}

boost::future<std::vector<Transaction>> BequantAsync::fetch_withdrawals_async(const std::string& code,
                                                                            long since,
                                                                            int limit,
                                                                            const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Transaction>>(context_, [this, code, since, limit, params]() {
        return fetch_withdrawals(code, since, limit, params);
    });
}

boost::future<std::vector<Transaction>> BequantAsync::fetch_deposits_withdrawals_async(const std::string& code,
                                                                                     long since,
                                                                                     int limit,
                                                                                     const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Transaction>>(context_, [this, code, since, limit, params]() {
        return fetch_deposits_withdrawals(code, since, limit, params);
    });
}

boost::future<std::map<std::string, TradingFee>> BequantAsync::fetch_trading_fees_async() {
    return async_request<std::map<std::string, TradingFee>>(context_, [this]() { return fetch_trading_fees(); });
}

boost::future<std::map<std::string, FundingRate>> BequantAsync::fetch_funding_rates_async() {
    return async_request<std::map<std::string, FundingRate>>(context_, [this]() { return fetch_funding_rates(); });
}

boost::future<FundingRate> BequantAsync::fetch_funding_rate_async(const std::string& symbol) {
    return async_request<FundingRate>(context_, [this, symbol]() { return fetch_funding_rate(symbol); });
}

boost::future<std::vector<FundingRate>> BequantAsync::fetch_funding_rate_history_async(const std::string& symbol,
                                                                                     long since,
                                                                                     int limit,
                                                                                     const std::map<std::string, std::string>& params) {
    return async_request<std::vector<FundingRate>>(context_, [this, symbol, since, limit, params]() {
        return fetch_funding_rate_history(symbol, since, limit, params);
    });
}

} // namespace ccxt
