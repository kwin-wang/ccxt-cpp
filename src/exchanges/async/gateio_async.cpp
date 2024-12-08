#include "ccxt/exchanges/async/gateio_async.h"
#include "ccxt/async_base/async_request.h"

namespace ccxt {

GateIOAsync::GateIOAsync(const boost::asio::io_context& context)
    : AsyncExchange(context)
    , GateIO()
    , context_(const_cast<boost::asio::io_context&>(context)) {}

// Market Data API
boost::future<std::vector<Market>> GateIOAsync::fetch_markets_async(const json& params) {
    return async_request<std::vector<Market>>(context_, [this, params]() {
        return fetchMarkets(params);
    });
}

boost::future<Ticker> GateIOAsync::fetch_ticker_async(const String& symbol, const json& params) {
    return async_request<Ticker>(context_, [this, symbol, params]() {
        return fetchTicker(symbol, params);
    });
}

boost::future<std::vector<Ticker>> GateIOAsync::fetch_tickers_async(const std::vector<String>& symbols,
                                                                  const json& params) {
    return async_request<std::vector<Ticker>>(context_, [this, symbols, params]() {
        return fetchTickers(symbols, params);
    });
}

boost::future<OrderBook> GateIOAsync::fetch_order_book_async(const String& symbol,
                                                           const std::optional<int>& limit,
                                                           const json& params) {
    return async_request<OrderBook>(context_, [this, symbol, limit, params]() {
        return fetchOrderBook(symbol, limit.value_or(0), params);
    });
}

boost::future<std::vector<Trade>> GateIOAsync::fetch_trades_async(const String& symbol,
                                                                const std::optional<long long>& since,
                                                                const std::optional<int>& limit,
                                                                const json& params) {
    return async_request<std::vector<Trade>>(context_, [this, symbol, since, limit, params]() {
        return fetchTrades(symbol, since.value_or(0), limit.value_or(0), params);
    });
}

boost::future<std::vector<OHLCV>> GateIOAsync::fetch_ohlcv_async(const String& symbol,
                                                                const String& timeframe,
                                                                const std::optional<long long>& since,
                                                                const std::optional<int>& limit,
                                                                const json& params) {
    return async_request<std::vector<OHLCV>>(context_, [this, symbol, timeframe, since, limit, params]() {
        return fetchOHLCV(symbol, timeframe, since.value_or(0), limit.value_or(0), params);
    });
}

// Trading API
boost::future<Balance> GateIOAsync::fetch_balance_async(const json& params) {
    return async_request<Balance>(context_, [this, params]() {
        return fetchBalance(params);
    });
}

boost::future<Order> GateIOAsync::create_order_async(const String& symbol,
                                                   const String& type,
                                                   const String& side,
                                                   double amount,
                                                   const std::optional<double>& price,
                                                   const json& params) {
    return async_request<Order>(context_, [this, symbol, type, side, amount, price, params]() {
        return createOrder(symbol, type, side, amount, price.value_or(0), params);
    });
}

boost::future<Order> GateIOAsync::cancel_order_async(const String& id,
                                                   const String& symbol,
                                                   const json& params) {
    return async_request<Order>(context_, [this, id, symbol, params]() {
        return cancelOrder(id, symbol, params);
    });
}

boost::future<Order> GateIOAsync::fetch_order_async(const String& id,
                                                  const String& symbol,
                                                  const json& params) {
    return async_request<Order>(context_, [this, id, symbol, params]() {
        return fetchOrder(id, symbol, params);
    });
}

boost::future<std::vector<Order>> GateIOAsync::fetch_orders_async(const String& symbol,
                                                                const std::optional<long long>& since,
                                                                const std::optional<int>& limit,
                                                                const json& params) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit, params]() {
        return fetchOrders(symbol, since.value_or(0), limit.value_or(0), params);
    });
}

boost::future<std::vector<Order>> GateIOAsync::fetch_open_orders_async(const String& symbol,
                                                                     const std::optional<long long>& since,
                                                                     const std::optional<int>& limit,
                                                                     const json& params) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit, params]() {
        return fetchOpenOrders(symbol, since.value_or(0), limit.value_or(0), params);
    });
}

boost::future<std::vector<Order>> GateIOAsync::fetch_closed_orders_async(const String& symbol,
                                                                       const std::optional<long long>& since,
                                                                       const std::optional<int>& limit,
                                                                       const json& params) {
    return async_request<std::vector<Order>>(context_, [this, symbol, since, limit, params]() {
        return fetchClosedOrders(symbol, since.value_or(0), limit.value_or(0), params);
    });
}

// Gate.io specific async methods
boost::future<json> GateIOAsync::fetch_funding_rate_async(const String& symbol,
                                                        const json& params) {
    return async_request<json>(context_, [this, symbol, params]() {
        return fetchFundingRate(symbol, params);
    });
}

boost::future<std::vector<Position>> GateIOAsync::fetch_positions_async(const String& symbol,
                                                                      const json& params) {
    return async_request<std::vector<Position>>(context_, [this, symbol, params]() {
        return fetchPositions(symbol, params);
    });
}

boost::future<json> GateIOAsync::set_leverage_async(const String& symbol,
                                                  double leverage,
                                                  const json& params) {
    return async_request<json>(context_, [this, symbol, leverage, params]() {
        return setLeverage(symbol, leverage, params);
    });
}

boost::future<json> GateIOAsync::set_position_mode_async(const String& hedged,
                                                       const json& params) {
    return async_request<json>(context_, [this, hedged, params]() {
        return setPositionMode(hedged, params);
    });
}

boost::future<json> GateIOAsync::fetch_settlements_async(const String& symbol,
                                                       const std::optional<long long>& since,
                                                       const std::optional<int>& limit,
                                                       const json& params) {
    return async_request<json>(context_, [this, symbol, since, limit, params]() {
        return fetchSettlements(symbol, since.value_or(0), limit.value_or(0), params);
    });
}

boost::future<json> GateIOAsync::fetch_liquidations_async(const String& symbol,
                                                        const std::optional<long long>& since,
                                                        const std::optional<int>& limit,
                                                        const json& params) {
    return async_request<json>(context_, [this, symbol, since, limit, params]() {
        return fetchLiquidations(symbol, since.value_or(0), limit.value_or(0), params);
    });
}

} // namespace ccxt
