#include "ccxt/exchanges/async/delta_async.h"
#include "ccxt/async_base/async_request.h"

namespace ccxt {

DeltaAsync::DeltaAsync(const boost::asio::io_context& context)
    : AsyncExchange(context)
    , delta()
    , context_(const_cast<boost::asio::io_context&>(context)) {}

// Market Data
boost::future<std::vector<Market>> DeltaAsync::fetch_markets_async() {
    return async_request<std::vector<Market>>(context_, [this]() { return fetch_markets(); });
}

boost::future<std::map<std::string, Currency>> DeltaAsync::fetch_currencies_async() {
    return async_request<std::map<std::string, Currency>>(context_, [this]() { return fetch_currencies(); });
}

boost::future<Ticker> DeltaAsync::fetch_ticker_async(const std::string& symbol) {
    return async_request<Ticker>(context_, [this, symbol]() { return fetch_ticker(symbol); });
}

boost::future<std::map<std::string, Ticker>> DeltaAsync::fetch_tickers_async(const std::vector<std::string>& symbols) {
    return async_request<std::map<std::string, Ticker>>(context_, [this, symbols]() { return fetch_tickers(symbols); });
}

boost::future<OrderBook> DeltaAsync::fetch_order_book_async(const std::string& symbol, const std::optional<int>& limit) {
    return async_request<OrderBook>(context_, [this, symbol, limit]() { return fetch_order_book(symbol, limit); });
}

boost::future<std::vector<OHLCV>> DeltaAsync::fetch_ohlcv_async(const std::string& symbol,
                                                               const std::string& timeframe,
                                                               const std::optional<long long>& since,
                                                               const std::optional<int>& limit) {
    return async_request<std::vector<OHLCV>>(context_, [this, symbol, timeframe, since, limit]() {
        return fetch_ohlcv(symbol, timeframe, since, limit);
    });
}

boost::future<std::map<std::string, FundingRate>> DeltaAsync::fetch_funding_rates_async() {
    return async_request<std::map<std::string, FundingRate>>(context_, [this]() { return fetch_funding_rates(); });
}

boost::future<std::map<std::string, Greeks>> DeltaAsync::fetch_greeks_async() {
    return async_request<std::map<std::string, Greeks>>(context_, [this]() { return fetch_greeks(); });
}

// Trading
boost::future<Order> DeltaAsync::create_order_async(const std::string& symbol,
                                                  const std::string& type,
                                                  const std::string& side,
                                                  double amount,
                                                  const std::optional<double>& price) {
    return async_request<Order>(context_, [this, symbol, type, side, amount, price]() {
        return create_order(symbol, type, side, amount, price);
    });
}

boost::future<Order> DeltaAsync::cancel_order_async(const std::string& id, const std::string& symbol) {
    return async_request<Order>(context_, [this, id, symbol]() { return cancel_order(id, symbol); });
}

boost::future<Order> DeltaAsync::fetch_order_async(const std::string& id, const std::string& symbol) {
    return async_request<Order>(context_, [this, id, symbol]() { return fetch_order(id, symbol); });
}

boost::future<std::vector<Order>> DeltaAsync::fetch_open_orders_async(const std::string& symbol,
                                                                    const std::optional<long long>& since,
                                                                    const std::optional<int>& limit) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit]() {
        return fetch_open_orders(symbol, since, limit);
    });
}

boost::future<std::vector<Order>> DeltaAsync::fetch_closed_orders_async(const std::string& symbol,
                                                                      const std::optional<long long>& since,
                                                                      const std::optional<int>& limit) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit]() {
        return fetch_closed_orders(symbol, since, limit);
    });
}

boost::future<std::vector<Trade>> DeltaAsync::fetch_my_trades_async(const std::string& symbol,
                                                                  const std::optional<long long>& since,
                                                                  const std::optional<int>& limit) {
    return async_request<std::vector<Trade>>(context_, [this, symbol, since, limit]() {
        return fetch_my_trades(symbol, since, limit);
    });
}

// Account
boost::future<Balance> DeltaAsync::fetch_balance_async() {
    return async_request<Balance>(context_, [this]() { return fetch_balance(); });
}

boost::future<std::vector<LedgerEntry>> DeltaAsync::fetch_ledger_async(const std::optional<std::string>& code,
                                                                      const std::optional<long long>& since,
                                                                      const std::optional<int>& limit) {
    return async_request<std::vector<LedgerEntry>>(context_, [this, code, since, limit]() {
        return fetch_ledger(code, since, limit);
    });
}

boost::future<DepositAddress> DeltaAsync::fetch_deposit_address_async(const std::string& code,
                                                                    const std::optional<std::string>& network) {
    return async_request<DepositAddress>(context_, [this, code, network]() {
        return fetch_deposit_address(code, network);
    });
}

} // namespace ccxt
