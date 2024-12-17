#ifndef CCXT_XT_H
#define CCXT_XT_H

#include "ccxt/base/exchange.h"
#include <future>

namespace ccxt {

class Xt : public Exchange {
public:
    Xt();
    ~Xt() = default;

    // Market Data Methods - Sync
    json fetchMarkets(const json& params = json::object()) override;
    json fetchCurrencies(const json& params = json::object()) override;
    json fetchTicker(const String& symbol, const json& params = json::object()) override;
    json fetchTickers(const std::vector<String>& symbols = {}, const json& params = json::object()) override;
    json fetchOrderBook(const String& symbol, int limit = 0, const json& params = json::object()) override;
    json fetchTrades(const String& symbol, int since = 0, int limit = 0,
                    const json& params = json::object()) override;
    json fetchOHLCV(const String& symbol, const String& timeframe = "1m",
                   int since = 0, int limit = 0, const json& params = json::object()) override;

    // Trading Methods - Sync
    json createOrder(const String& symbol, const String& type, const String& side,
                    double amount, double price = 0, const json& params = json::object()) override;
    json cancelOrder(const String& id, const String& symbol = "",
                    const json& params = json::object()) override;
    json fetchOrder(const String& id, const String& symbol = "",
                   const json& params = json::object()) override;
    json fetchOrders(const String& symbol = "", int since = 0, int limit = 0,
                    const json& params = json::object()) override;
    json fetchOpenOrders(const String& symbol = "", int since = 0, int limit = 0,
                        const json& params = json::object()) override;
    json fetchClosedOrders(const String& symbol = "", int since = 0, int limit = 0,
                          const json& params = json::object()) override;
    json fetchMyTrades(const String& symbol = "", int since = 0, int limit = 0,
                      const json& params = json::object()) override;

    // Account Methods - Sync
    json fetchBalance(const json& params = json::object()) override;
    json fetchDeposits(const String& code = "", int since = 0, int limit = 0,
                      const json& params = json::object()) override;
    json fetchWithdrawals(const String& code = "", int since = 0, int limit = 0,
                         const json& params = json::object()) override;

    // Market Data Methods - Async
    AsyncPullType fetchMarketsAsync(const json& params = json::object());
    AsyncPullType fetchCurrenciesAsync(const json& params = json::object());
    AsyncPullType fetchTickerAsync(const String& symbol, const json& params = json::object());
    AsyncPullType fetchTickersAsync(const std::vector<String>& symbols = {},
                                      const json& params = json::object());
    AsyncPullType fetchOrderBookAsync(const String& symbol, int limit = 0,
                                        const json& params = json::object());
    AsyncPullType fetchTradesAsync(const String& symbol, int since = 0, int limit = 0,
                                     const json& params = json::object());
    AsyncPullType fetchOHLCVAsync(const String& symbol, const String& timeframe = "1m",
                                    int since = 0, int limit = 0, const json& params = json::object());

    // Trading Methods - Async
    AsyncPullType createOrderAsync(const String& symbol, const String& type,
                                     const String& side, double amount, double price = 0,
                                     const json& params = json::object());
    AsyncPullType cancelOrderAsync(const String& id, const String& symbol = "",
                                     const json& params = json::object());
    AsyncPullType fetchOrderAsync(const String& id, const String& symbol = "",
                                    const json& params = json::object());
    AsyncPullType fetchOrdersAsync(const String& symbol = "", int since = 0, int limit = 0,
                                     const json& params = json::object());
    AsyncPullType fetchOpenOrdersAsync(const String& symbol = "", int since = 0, int limit = 0,
                                         const json& params = json::object());
    AsyncPullType fetchClosedOrdersAsync(const String& symbol = "", int since = 0, int limit = 0,
                                           const json& params = json::object());
    AsyncPullType fetchMyTradesAsync(const String& symbol = "", int since = 0, int limit = 0,
                                       const json& params = json::object());

    // Account Methods - Async
    AsyncPullType fetchBalanceAsync(const json& params = json::object());
    AsyncPullType fetchDepositsAsync(const String& code = "", int since = 0, int limit = 0,
                                       const json& params = json::object());
    AsyncPullType fetchWithdrawalsAsync(const String& code = "", int since = 0, int limit = 0,
                                          const json& params = json::object());

protected:
    // Helper Methods
    void initializeApiEndpoints();
    json parseTicker(const json& ticker, const Market& market);
    json parseOrder(const json& order, const Market& market = Market());
    json parseTrade(const json& trade, const Market& market = Market());
    json parseOHLCV(const json& ohlcv, const Market& market);
    json parseBalance(const json& response);
    json parseMarket(const json& market);
    json parseCurrency(const json& currency);
    String parseOrderStatus(const String& status);
    String sign(const String& path, const String& api = "public",
               const String& method = "GET", const json& params = json::object(),
               const json& headers = nullptr, const String& body = "") override;

private:
    // Async helper methods
    template<typename Func, typename... Args>
    AsyncPullType async(Func&& func, Args&&... args) {
        return std::async(std::launch::async,
                         std::forward<Func>(func),
                         this,
                         std::forward<Args>(args)...);
    }

    String v1;
    String v2;
};

} // namespace ccxt

#endif // CCXT_XT_H
