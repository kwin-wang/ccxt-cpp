#include "ccxt/exchanges/async/binance_async.h"
#include "ccxt/async_base/async_utils.h"
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace ccxt {

BinanceAsync::BinanceAsync(const boost::asio::io_context& context)
    : AsyncExchange(context)
    , Binance()
    , context_(const_cast<boost::asio::io_context&>(context)) {}

// Market Data
boost::future<std::vector<Market>> BinanceAsync::fetch_markets_async() {
    return async_request<std::vector<Market>>(context_, [this]() { return fetch_markets(); });
}

boost::future<std::vector<Currency>> BinanceAsync::fetch_currencies_async() {
    return async_request<std::vector<Currency>>(context_, [this]() { return fetch_currencies(); });
}

boost::future<OrderBook> BinanceAsync::fetch_order_book_async(const std::string& symbol, int limit) {
    return async_request<OrderBook>(context_, [this, symbol, limit]() { return fetch_order_book(symbol, limit); });
}

boost::future<std::map<std::string, OrderBook>> BinanceAsync::fetch_order_books_async(const std::vector<std::string>& symbols, int limit) {
    return async_request<std::map<std::string, OrderBook>>(context_, [this, symbols, limit]() { return fetch_order_books(symbols, limit); });
}

boost::future<std::map<std::string, Ticker>> BinanceAsync::fetch_tickers_async() {
    return async_request<std::map<std::string, Ticker>>(context_, [this]() { return fetch_tickers(); });
}

boost::future<Ticker> BinanceAsync::fetch_ticker_async(const std::string& symbol) {
    return async_request<Ticker>(context_, [this, symbol]() { return fetch_ticker(symbol); });
}

boost::future<std::vector<Trade>> BinanceAsync::fetch_trades_async(const std::string& symbol, long since, int limit) {
    return async_request<std::vector<Trade>>(context_, [this, symbol, since, limit]() { return fetch_trades(symbol, since, limit); });
}

boost::future<std::vector<OHLCV>> BinanceAsync::fetch_ohlcv_async(const std::string& symbol,
                                                                 const std::string& timeframe,
                                                                 long since,
                                                                 int limit) {
    return async_request<std::vector<OHLCV>>(context_, [this, symbol, timeframe, since, limit]() {
        return fetch_ohlcv(symbol, timeframe, since, limit);
    });
}

// Trading
boost::future<Order> BinanceAsync::create_order_async(const std::string& symbol,
                                                    const std::string& type,
                                                    const std::string& side,
                                                    double amount,
                                                    double price,
                                                    const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, symbol, type, side, amount, price, params]() {
        return create_order(symbol, type, side, amount, price, params);
    });
}

boost::future<std::vector<Order>> BinanceAsync::create_orders_async(const std::vector<OrderRequest>& orders,
                                                                  const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Order>>(context_, [this, orders, params]() { return create_orders(orders, params); });
}

boost::future<Order> BinanceAsync::create_limit_buy_order_async(const std::string& symbol,
                                                              double amount,
                                                              double price,
                                                              const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, symbol, amount, price, params]() {
        return create_limit_buy_order(symbol, amount, price, params);
    });
}

boost::future<Order> BinanceAsync::create_limit_sell_order_async(const std::string& symbol,
                                                               double amount,
                                                               double price,
                                                               const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, symbol, amount, price, params]() {
        return create_limit_sell_order(symbol, amount, price, params);
    });
}

boost::future<Order> BinanceAsync::create_market_buy_order_async(const std::string& symbol,
                                                               double amount,
                                                               const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, symbol, amount, params]() {
        return create_market_buy_order(symbol, amount, params);
    });
}

boost::future<Order> BinanceAsync::create_market_sell_order_async(const std::string& symbol,
                                                                double amount,
                                                                const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, symbol, amount, params]() {
        return create_market_sell_order(symbol, amount, params);
    });
}

boost::future<Order> BinanceAsync::create_stop_order_async(const std::string& symbol,
                                                         const std::string& type,
                                                         const std::string& side,
                                                         double amount,
                                                         double price,
                                                         const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, symbol, type, side, amount, price, params]() {
        return create_stop_order(symbol, type, side, amount, price, params);
    });
}

boost::future<Order> BinanceAsync::edit_order_async(const std::string& id,
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

boost::future<Order> BinanceAsync::cancel_order_async(const std::string& id,
                                                    const std::string& symbol,
                                                    const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, id, symbol, params]() { return cancel_order(id, symbol, params); });
}

boost::future<std::vector<Order>> BinanceAsync::cancel_orders_async(const std::vector<std::string>& ids,
                                                                  const std::string& symbol,
                                                                  const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Order>>(context_, [this, ids, symbol, params]() { return cancel_orders(ids, symbol, params); });
}

boost::future<std::vector<Order>> BinanceAsync::cancel_all_orders_async(const std::string& symbol,
                                                                      const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Order>>(context_, [this, symbol, params]() { return cancel_all_orders(symbol, params); });
}

boost::future<Order> BinanceAsync::fetch_order_async(const std::string& id, const std::string& symbol) {
    return async_request<Order>(context_, [this, id, symbol]() { return fetch_order(id, symbol); });
}

boost::future<std::vector<Order>> BinanceAsync::fetch_open_orders_async(const std::string& symbol) {
    return async_request<std::vector<Order>>(context_, [this, symbol]() { return fetch_open_orders(symbol); });
}

boost::future<std::vector<Order>> BinanceAsync::fetch_closed_orders_async(const std::string& symbol, long since, int limit) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit]() { return fetch_closed_orders(symbol, since, limit); });
}

boost::future<std::vector<Order>> BinanceAsync::fetch_canceled_orders_async(const std::string& symbol, long since, int limit) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit]() { return fetch_canceled_orders(symbol, since, limit); });
}

// Trading History
boost::future<std::vector<Trade>> BinanceAsync::fetch_my_trades_async(const std::string& symbol, long since, int limit) {
    return async_request<std::vector<Trade>>(context_, [this, symbol, since, limit]() { return fetch_my_trades(symbol, since, limit); });
}

// Account
boost::future<Balance> BinanceAsync::fetch_balance_async() {
    return async_request<Balance>(context_, [this]() { return fetch_balance(); });
}

boost::future<std::vector<LedgerEntry>> BinanceAsync::fetch_ledger_async(const std::string& code,
                                                                        long since,
                                                                        int limit,
                                                                        const std::map<std::string, std::string>& params) {
    return async_request<std::vector<LedgerEntry>>(context_, [this, code, since, limit, params]() {
        return fetch_ledger(code, since, limit, params);
    });
}

// Margin
boost::future<MarginModification> BinanceAsync::add_margin_async(const std::string& symbol,
                                                               double amount,
                                                               const std::map<std::string, std::string>& params) {
    return async_request<MarginModification>(context_, [this, symbol, amount, params]() {
        return add_margin(symbol, amount, params);
    });
}

boost::future<std::vector<BorrowInterest>> BinanceAsync::fetch_borrow_interest_async(const std::string& code,
                                                                                   long since,
                                                                                   int limit,
                                                                                   const std::map<std::string, std::string>& params) {
    return async_request<std::vector<BorrowInterest>>(context_, [this, code, since, limit, params]() {
        return fetch_borrow_interest(code, since, limit, params);
    });
}

boost::future<std::vector<CrossBorrowRate>> BinanceAsync::fetch_cross_borrow_rate_async(const std::string& code,
                                                                                      const std::map<std::string, std::string>& params) {
    return async_request<std::vector<CrossBorrowRate>>(context_, [this, code, params]() {
        return fetch_cross_borrow_rate(code, params);
    });
}

boost::future<IsolatedBorrowRate> BinanceAsync::fetch_isolated_borrow_rate_async(const std::string& symbol,
                                                                               const std::map<std::string, std::string>& params) {
    return async_request<IsolatedBorrowRate>(context_, [this, symbol, params]() {
        return fetch_isolated_borrow_rate(symbol, params);
    });
}

boost::future<std::map<std::string, IsolatedBorrowRates>> BinanceAsync::fetch_isolated_borrow_rates_async(const std::map<std::string, std::string>& params) {
    return async_request<std::map<std::string, IsolatedBorrowRates>>(context_, [this, params]() {
        return fetch_isolated_borrow_rates(params);
    });
}

boost::future<std::vector<LeverageTier>> BinanceAsync::fetch_market_leverage_tiers_async(const std::string& symbol,
                                                                                       const std::map<std::string, std::string>& params) {
    return async_request<std::vector<LeverageTier>>(context_, [this, symbol, params]() {
        return fetch_market_leverage_tiers(symbol, params);
    });
}

boost::future<std::map<std::string, MarginMode>> BinanceAsync::fetch_margin_modes_async(const std::vector<std::string>& symbols) {
    return async_request<std::map<std::string, MarginMode>>(context_, [this, symbols]() { return fetch_margin_modes(symbols); });
}

// Funding
boost::future<DepositAddress> BinanceAsync::fetch_deposit_address_async(const std::string& code,
                                                                      const std::map<std::string, std::string>& params) {
    return async_request<DepositAddress>(context_, [this, code, params]() { return fetch_deposit_address(code, params); });
}

boost::future<std::vector<Transaction>> BinanceAsync::fetch_deposits_async(const std::string& code,
                                                                         long since,
                                                                         int limit,
                                                                         const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Transaction>>(context_, [this, code, since, limit, params]() {
        return fetch_deposits(code, since, limit, params);
    });
}

boost::future<std::vector<Transaction>> BinanceAsync::fetch_withdrawals_async(const std::string& code,
                                                                            long since,
                                                                            int limit,
                                                                            const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Transaction>>(context_, [this, code, since, limit, params]() {
        return fetch_withdrawals(code, since, limit, params);
    });
}

boost::future<std::vector<Transaction>> BinanceAsync::fetch_deposits_withdrawals_async(const std::string& code,
                                                                                     long since,
                                                                                     int limit,
                                                                                     const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Transaction>>(context_, [this, code, since, limit, params]() {
        return fetch_deposits_withdrawals(code, since, limit, params);
    });
}

boost::future<std::map<std::string, TradingFee>> BinanceAsync::fetch_trading_fees_async() {
    return async_request<std::map<std::string, TradingFee>>(context_, [this]() { return fetch_trading_fees(); });
}

// Convert
boost::future<Conversion> BinanceAsync::create_convert_trade_async(const std::string& fromCurrency,
                                                                 const std::string& toCurrency,
                                                                 double amount,
                                                                 const std::map<std::string, std::string>& params) {
    return async_request<Conversion>(context_, [this, fromCurrency, toCurrency, amount, params]() {
        return create_convert_trade(fromCurrency, toCurrency, amount, params);
    });
}

boost::future<Conversion> BinanceAsync::fetch_convert_trade_async(const std::string& id,
                                                                const std::map<std::string, std::string>& params) {
    return async_request<Conversion>(context_, [this, id, params]() { return fetch_convert_trade(id, params); });
}

boost::future<Conversion> BinanceAsync::fetch_convert_quote_async(const std::string& fromCurrency,
                                                                const std::string& toCurrency,
                                                                double amount,
                                                                const std::map<std::string, std::string>& params) {
    return async_request<Conversion>(context_, [this, fromCurrency, toCurrency, amount, params]() {
        return fetch_convert_quote(fromCurrency, toCurrency, amount, params);
    });
}

// Futures and Perpetual Swaps
boost::future<std::map<std::string, FundingRate>> BinanceAsync::fetch_funding_rates_async() {
    return async_request<std::map<std::string, FundingRate>>(context_, [this]() { return fetch_funding_rates(); });
}

boost::future<FundingRate> BinanceAsync::fetch_funding_rate_async(const std::string& symbol) {
    return async_request<FundingRate>(context_, [this, symbol]() { return fetch_funding_rate(symbol); });
}

boost::future<std::vector<FundingRate>> BinanceAsync::fetch_funding_rate_history_async(const std::string& symbol,
                                                                                     long since,
                                                                                     int limit,
                                                                                     const std::map<std::string, std::string>& params) {
    return async_request<std::vector<FundingRate>>(context_, [this, symbol, since, limit, params]() {
        return fetch_funding_rate_history(symbol, since, limit, params);
    });
}

} // namespace ccxt
