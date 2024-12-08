#include "ccxt/exchanges/async/gemini_async.h"
#include "ccxt/async_base/async_request.h"

namespace ccxt {

GeminiAsync::GeminiAsync(const boost::asio::io_context& context)
    : AsyncExchange(context)
    , Gemini()
    , context_(const_cast<boost::asio::io_context&>(context)) {}

// Market Data API
boost::future<std::vector<Market>> GeminiAsync::fetch_markets_async(const json& params) {
    return async_request<std::vector<Market>>(context_, [this, params]() {
        return fetchMarkets(params);
    });
}

boost::future<Ticker> GeminiAsync::fetch_ticker_async(const String& symbol, const json& params) {
    return async_request<Ticker>(context_, [this, symbol, params]() {
        return fetchTicker(symbol, params);
    });
}

boost::future<std::vector<Ticker>> GeminiAsync::fetch_tickers_async(const std::vector<String>& symbols,
                                                                  const json& params) {
    return async_request<std::vector<Ticker>>(context_, [this, symbols, params]() {
        return fetchTickers(symbols, params);
    });
}

boost::future<OrderBook> GeminiAsync::fetch_order_book_async(const String& symbol,
                                                           const std::optional<int>& limit,
                                                           const json& params) {
    return async_request<OrderBook>(context_, [this, symbol, limit, params]() {
        return fetchOrderBook(symbol, limit.value_or(0), params);
    });
}

boost::future<std::vector<Trade>> GeminiAsync::fetch_trades_async(const String& symbol,
                                                                const std::optional<long long>& since,
                                                                const std::optional<int>& limit,
                                                                const json& params) {
    return async_request<std::vector<Trade>>(context_, [this, symbol, since, limit, params]() {
        return fetchTrades(symbol, since.value_or(0), limit.value_or(0), params);
    });
}

boost::future<std::vector<OHLCV>> GeminiAsync::fetch_ohlcv_async(const String& symbol,
                                                                const String& timeframe,
                                                                const std::optional<long long>& since,
                                                                const std::optional<int>& limit,
                                                                const json& params) {
    return async_request<std::vector<OHLCV>>(context_, [this, symbol, timeframe, since, limit, params]() {
        return fetchOHLCV(symbol, timeframe, since.value_or(0), limit.value_or(0), params);
    });
}

// Trading API
boost::future<Balance> GeminiAsync::fetch_balance_async(const json& params) {
    return async_request<Balance>(context_, [this, params]() {
        return fetchBalance(params);
    });
}

boost::future<Order> GeminiAsync::create_order_async(const String& symbol,
                                                   const String& type,
                                                   const String& side,
                                                   double amount,
                                                   const std::optional<double>& price,
                                                   const json& params) {
    return async_request<Order>(context_, [this, symbol, type, side, amount, price, params]() {
        return createOrder(symbol, type, side, amount, price.value_or(0), params);
    });
}

boost::future<Order> GeminiAsync::cancel_order_async(const String& id,
                                                   const String& symbol,
                                                   const json& params) {
    return async_request<Order>(context_, [this, id, symbol, params]() {
        return cancelOrder(id, symbol, params);
    });
}

boost::future<Order> GeminiAsync::fetch_order_async(const String& id,
                                                  const String& symbol,
                                                  const json& params) {
    return async_request<Order>(context_, [this, id, symbol, params]() {
        return fetchOrder(id, symbol, params);
    });
}

boost::future<std::vector<Order>> GeminiAsync::fetch_orders_async(const String& symbol,
                                                                const std::optional<long long>& since,
                                                                const std::optional<int>& limit,
                                                                const json& params) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit, params]() {
        return fetchOrders(symbol, since.value_or(0), limit.value_or(0), params);
    });
}

boost::future<std::vector<Order>> GeminiAsync::fetch_open_orders_async(const String& symbol,
                                                                     const std::optional<long long>& since,
                                                                     const std::optional<int>& limit,
                                                                     const json& params) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit, params]() {
        return fetchOpenOrders(symbol, since.value_or(0), limit.value_or(0), params);
    });
}

boost::future<std::vector<Order>> GeminiAsync::fetch_closed_orders_async(const String& symbol,
                                                                       const std::optional<long long>& since,
                                                                       const std::optional<int>& limit,
                                                                       const json& params) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit, params]() {
        return fetchClosedOrders(symbol, since.value_or(0), limit.value_or(0), params);
    });
}

// Gemini specific async methods
boost::future<std::vector<Trade>> GeminiAsync::fetch_my_trades_async(const String& symbol,
                                                                   const std::optional<long long>& since,
                                                                   const std::optional<int>& limit,
                                                                   const json& params) {
    return async_request<std::vector<Trade>>(context_, [this, symbol, since, limit, params]() {
        return fetchMyTrades(symbol, since.value_or(0), limit.value_or(0), params);
    });
}

boost::future<std::vector<Transaction>> GeminiAsync::fetch_deposits_async(const String& code,
                                                                        const std::optional<long long>& since,
                                                                        const std::optional<int>& limit,
                                                                        const json& params) {
    return async_request<std::vector<Transaction>>(context_, [this, code, since, limit, params]() {
        return fetchDeposits(code, since.value_or(0), limit.value_or(0), params);
    });
}

boost::future<std::vector<Transaction>> GeminiAsync::fetch_withdrawals_async(const String& code,
                                                                           const std::optional<long long>& since,
                                                                           const std::optional<int>& limit,
                                                                           const json& params) {
    return async_request<std::vector<Transaction>>(context_, [this, code, since, limit, params]() {
        return fetchWithdrawals(code, since.value_or(0), limit.value_or(0), params);
    });
}

boost::future<std::vector<Transfer>> GeminiAsync::fetch_transfers_async(const String& code,
                                                                      const std::optional<long long>& since,
                                                                      const std::optional<int>& limit,
                                                                      const json& params) {
    return async_request<std::vector<Transfer>>(context_, [this, code, since, limit, params]() {
        return fetchTransfers(code, since.value_or(0), limit.value_or(0), params);
    });
}

boost::future<Transfer> GeminiAsync::transfer_async(const String& code,
                                                  double amount,
                                                  const String& fromAccount,
                                                  const String& toAccount,
                                                  const json& params) {
    return async_request<Transfer>(context_, [this, code, amount, fromAccount, toAccount, params]() {
        return transfer(code, amount, fromAccount, toAccount, params);
    });
}

boost::future<DepositAddress> GeminiAsync::fetch_deposit_address_async(const String& code,
                                                                     const json& params) {
    return async_request<DepositAddress>(context_, [this, code, params]() {
        return fetchDepositAddress(code, params);
    });
}

boost::future<DepositAddress> GeminiAsync::create_deposit_address_async(const String& code,
                                                                      const json& params) {
    return async_request<DepositAddress>(context_, [this, code, params]() {
        return createDepositAddress(code, params);
    });
}

boost::future<Transaction> GeminiAsync::withdraw_async(const String& code,
                                                     double amount,
                                                     const String& address,
                                                     const std::optional<String>& tag,
                                                     const json& params) {
    return async_request<Transaction>(context_, [this, code, amount, address, tag, params]() {
        return withdraw(code, amount, address, tag.value_or(""), params);
    });
}

boost::future<std::vector<LedgerEntry>> GeminiAsync::fetch_ledger_async(const String& code,
                                                                       const std::optional<long long>& since,
                                                                       const std::optional<int>& limit,
                                                                       const json& params) {
    return async_request<std::vector<LedgerEntry>>(context_, [this, code, since, limit, params]() {
        return fetchLedger(code, since.value_or(0), limit.value_or(0), params);
    });
}

boost::future<json> GeminiAsync::fetch_payment_methods_async(const json& params) {
    return async_request<json>(context_, [this, params]() {
        return fetchPaymentMethods(params);
    });
}

} // namespace ccxt
