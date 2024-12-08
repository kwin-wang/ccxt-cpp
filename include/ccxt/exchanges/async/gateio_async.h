#ifndef CCXT_EXCHANGES_ASYNC_GATEIO_ASYNC_H
#define CCXT_EXCHANGES_ASYNC_GATEIO_ASYNC_H

#include "ccxt/async_base/exchange.h"
#include "ccxt/exchanges/gateio.h"
#include <boost/asio.hpp>
#include <string>
#include <map>
#include <vector>
#include <optional>

namespace ccxt {

class GateIOAsync : public AsyncExchange, public GateIO {
public:
    explicit GateIOAsync(const boost::asio::io_context& context);
    ~GateIOAsync() override = default;

    // Market Data API
    boost::future<std::vector<Market>> fetch_markets_async(const json& params = json::object());
    boost::future<Ticker> fetch_ticker_async(const String& symbol, const json& params = json::object());
    boost::future<std::vector<Ticker>> fetch_tickers_async(const std::vector<String>& symbols = {},
                                                         const json& params = json::object());
    boost::future<OrderBook> fetch_order_book_async(const String& symbol,
                                                  const std::optional<int>& limit = std::nullopt,
                                                  const json& params = json::object());
    boost::future<std::vector<Trade>> fetch_trades_async(const String& symbol,
                                                       const std::optional<long long>& since = std::nullopt,
                                                       const std::optional<int>& limit = std::nullopt,
                                                       const json& params = json::object());
    boost::future<std::vector<OHLCV>> fetch_ohlcv_async(const String& symbol,
                                                       const String& timeframe = "1m",
                                                       const std::optional<long long>& since = std::nullopt,
                                                       const std::optional<int>& limit = std::nullopt,
                                                       const json& params = json::object());

    // Trading API
    boost::future<Balance> fetch_balance_async(const json& params = json::object());
    boost::future<Order> create_order_async(const String& symbol,
                                          const String& type,
                                          const String& side,
                                          double amount,
                                          const std::optional<double>& price = std::nullopt,
                                          const json& params = json::object());
    boost::future<Order> cancel_order_async(const String& id,
                                          const String& symbol = "",
                                          const json& params = json::object());
    boost::future<Order> fetch_order_async(const String& id,
                                         const String& symbol = "",
                                         const json& params = json::object());
    boost::future<std::vector<Order>> fetch_orders_async(const String& symbol = "",
                                                       const std::optional<long long>& since = std::nullopt,
                                                       const std::optional<int>& limit = std::nullopt,
                                                       const json& params = json::object());
    boost::future<std::vector<Order>> fetch_open_orders_async(const String& symbol = "",
                                                            const std::optional<long long>& since = std::nullopt,
                                                            const std::optional<int>& limit = std::nullopt,
                                                            const json& params = json::object());
    boost::future<std::vector<Order>> fetch_closed_orders_async(const String& symbol = "",
                                                              const std::optional<long long>& since = std::nullopt,
                                                              const std::optional<int>& limit = std::nullopt,
                                                              const json& params = json::object());

    // Gate.io specific async methods
    boost::future<json> fetch_funding_rate_async(const String& symbol,
                                               const json& params = json::object());
    boost::future<std::vector<Position>> fetch_positions_async(const String& symbol = "",
                                                             const json& params = json::object());
    boost::future<json> set_leverage_async(const String& symbol,
                                         double leverage,
                                         const json& params = json::object());
    boost::future<json> set_position_mode_async(const String& hedged,
                                              const json& params = json::object());
    boost::future<json> fetch_settlements_async(const String& symbol,
                                              const std::optional<long long>& since = std::nullopt,
                                              const std::optional<int>& limit = std::nullopt,
                                              const json& params = json::object());
    boost::future<json> fetch_liquidations_async(const String& symbol,
                                               const std::optional<long long>& since = std::nullopt,
                                               const std::optional<int>& limit = std::nullopt,
                                               const json& params = json::object());

protected:
    boost::asio::io_context& context_;
};

} // namespace ccxt

#endif // CCXT_EXCHANGES_ASYNC_GATEIO_ASYNC_H
