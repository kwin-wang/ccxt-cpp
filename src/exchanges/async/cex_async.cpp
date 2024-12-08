#include "ccxt/exchanges/async/cex_async.h"
#include "ccxt/async_base/async_utils.h"
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace ccxt {

CexAsync::CexAsync(const boost::asio::io_context& context)
    : AsyncExchange(context)
    , Cex()
    , context_(const_cast<boost::asio::io_context&>(context)) {}

// Market Data
boost::future<std::vector<Market>> CexAsync::fetch_markets_async() {
    return async_request<std::vector<Market>>(context_, [this]() { return fetch_markets(); });
}

boost::future<std::vector<Currency>> CexAsync::fetch_currencies_async() {
    return async_request<std::vector<Currency>>(context_, [this]() { return fetch_currencies(); });
}

boost::future<OrderBook> CexAsync::fetch_order_book_async(const std::string& symbol, int limit) {
    return async_request<OrderBook>(context_, [this, symbol, limit]() { return fetch_order_book(symbol, limit); });
}

boost::future<std::map<std::string, Ticker>> CexAsync::fetch_tickers_async() {
    return async_request<std::map<std::string, Ticker>>(context_, [this]() { return fetch_tickers(); });
}

boost::future<Ticker> CexAsync::fetch_ticker_async(const std::string& symbol) {
    return async_request<Ticker>(context_, [this, symbol]() { return fetch_ticker(symbol); });
}

boost::future<std::vector<Trade>> CexAsync::fetch_trades_async(const std::string& symbol, long since, int limit) {
    return async_request<std::vector<Trade>>(context_, [this, symbol, since, limit]() { return fetch_trades(symbol, since, limit); });
}

boost::future<std::vector<OHLCV>> CexAsync::fetch_ohlcv_async(const std::string& symbol,
                                                             const std::string& timeframe,
                                                             long since,
                                                             int limit) {
    return async_request<std::vector<OHLCV>>(context_, [this, symbol, timeframe, since, limit]() {
        return fetch_ohlcv(symbol, timeframe, since, limit);
    });
}

boost::future<long> CexAsync::fetch_time_async() {
    return async_request<long>(context_, [this]() { return fetch_time(); });
}

// Trading
boost::future<Order> CexAsync::create_order_async(const std::string& symbol,
                                                const std::string& type,
                                                const std::string& side,
                                                double amount,
                                                double price,
                                                const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, symbol, type, side, amount, price, params]() {
        return create_order(symbol, type, side, amount, price, params);
    });
}

boost::future<Order> CexAsync::create_stop_order_async(const std::string& symbol,
                                                     const std::string& type,
                                                     const std::string& side,
                                                     double amount,
                                                     double price,
                                                     const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, symbol, type, side, amount, price, params]() {
        return create_stop_order(symbol, type, side, amount, price, params);
    });
}

boost::future<Order> CexAsync::create_trigger_order_async(const std::string& symbol,
                                                        const std::string& type,
                                                        const std::string& side,
                                                        double amount,
                                                        double price,
                                                        const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, symbol, type, side, amount, price, params]() {
        return create_trigger_order(symbol, type, side, amount, price, params);
    });
}

boost::future<Order> CexAsync::cancel_order_async(const std::string& id,
                                                const std::string& symbol,
                                                const std::map<std::string, std::string>& params) {
    return async_request<Order>(context_, [this, id, symbol, params]() { return cancel_order(id, symbol, params); });
}

boost::future<std::vector<Order>> CexAsync::cancel_all_orders_async(const std::string& symbol,
                                                                  const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Order>>(context_, [this, symbol, params]() { return cancel_all_orders(symbol, params); });
}

boost::future<Order> CexAsync::fetch_order_async(const std::string& id, const std::string& symbol) {
    return async_request<Order>(context_, [this, id, symbol]() { return fetch_order(id, symbol); });
}

boost::future<Order> CexAsync::fetch_open_order_async(const std::string& id, const std::string& symbol) {
    return async_request<Order>(context_, [this, id, symbol]() { return fetch_open_order(id, symbol); });
}

boost::future<std::vector<Order>> CexAsync::fetch_open_orders_async(const std::string& symbol) {
    return async_request<std::vector<Order>>(context_, [this, symbol]() { return fetch_open_orders(symbol); });
}

boost::future<std::vector<Order>> CexAsync::fetch_closed_orders_async(const std::string& symbol, long since, int limit) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit]() { return fetch_closed_orders(symbol, since, limit); });
}

boost::future<Order> CexAsync::fetch_closed_order_async(const std::string& id, const std::string& symbol) {
    return async_request<Order>(context_, [this, id, symbol]() { return fetch_closed_order(id, symbol); });
}

// Account
boost::future<std::vector<Account>> CexAsync::fetch_accounts_async() {
    return async_request<std::vector<Account>>(context_, [this]() { return fetch_accounts(); });
}

boost::future<Balance> CexAsync::fetch_balance_async() {
    return async_request<Balance>(context_, [this]() { return fetch_balance(); });
}

boost::future<std::vector<LedgerEntry>> CexAsync::fetch_ledger_async(const std::string& code,
                                                                    long since,
                                                                    int limit,
                                                                    const std::map<std::string, std::string>& params) {
    return async_request<std::vector<LedgerEntry>>(context_, [this, code, since, limit, params]() {
        return fetch_ledger(code, since, limit, params);
    });
}

// Funding
boost::future<DepositAddress> CexAsync::fetch_deposit_address_async(const std::string& code,
                                                                  const std::map<std::string, std::string>& params) {
    return async_request<DepositAddress>(context_, [this, code, params]() { return fetch_deposit_address(code, params); });
}

boost::future<std::vector<Transaction>> CexAsync::fetch_deposits_withdrawals_async(const std::string& code,
                                                                                 long since,
                                                                                 int limit,
                                                                                 const std::map<std::string, std::string>& params) {
    return async_request<std::vector<Transaction>>(context_, [this, code, since, limit, params]() {
        return fetch_deposits_withdrawals(code, since, limit, params);
    });
}

boost::future<std::map<std::string, TradingFee>> CexAsync::fetch_trading_fees_async() {
    return async_request<std::map<std::string, TradingFee>>(context_, [this]() { return fetch_trading_fees(); });
}

boost::future<TransferEntry> CexAsync::transfer_async(const std::string& code,
                                                    double amount,
                                                    const std::string& fromAccount,
                                                    const std::string& toAccount,
                                                    const std::map<std::string, std::string>& params) {
    return async_request<TransferEntry>(context_, [this, code, amount, fromAccount, toAccount, params]() {
        return transfer(code, amount, fromAccount, toAccount, params);
    });
}

} // namespace ccxt
