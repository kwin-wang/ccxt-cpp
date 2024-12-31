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
    json fetchTicker(const std::string& symbol, const json& params = json::object()) override;
    json fetchTickers(const std::vector<std::string>& symbols = {}, const json& params = json::object()) override;
    json fetchOrderBook(const std::string& symbol, int limit = 0, const json& params = json::object()) override;
    json fetchTrades(const std::string& symbol, int since = 0, int limit = 0,
                    const json& params = json::object()) override;
    json fetchOHLCV(const std::string& symbol, const std::string& timeframe = "1m",
                   int since = 0, int limit = 0, const json& params = json::object()) override;

    // Trading Methods - Sync
    json createOrder(const std::string& symbol, const std::string& type, const std::string& side,
                    double amount, double price = 0, const json& params = json::object()) override;
    json cancelOrder(const std::string& id, const std::string& symbol = "",
                    const json& params = json::object()) override;
    json fetchOrder(const std::string& id, const std::string& symbol = "",
                   const json& params = json::object()) override;
    json fetchOrders(const std::string& symbol = "", int since = 0, int limit = 0,
                    const json& params = json::object()) override;
    json fetchOpenOrders(const std::string& symbol = "", int since = 0, int limit = 0,
                        const json& params = json::object()) override;
    json fetchClosedOrders(const std::string& symbol = "", int since = 0, int limit = 0,
                          const json& params = json::object()) override;
    json fetchMyTrades(const std::string& symbol = "", int since = 0, int limit = 0,
                      const json& params = json::object()) override;

    // Account Methods - Sync
    json fetchBalance(const json& params = json::object()) override;
    json fetchDeposits(const std::string& code = "", int since = 0, int limit = 0,
                      const json& params = json::object()) override;
    json fetchWithdrawals(const std::string& code = "", int since = 0, int limit = 0,
                         const json& params = json::object()) override;

    // Market Data Methods - Async
    AsyncPullType fetchMarketsAsync(const json& params = json::object());
    AsyncPullType fetchCurrenciesAsync(const json& params = json::object());
    AsyncPullType fetchTickerAsync(const std::string& symbol, const json& params = json::object());
    AsyncPullType fetchTickersAsync(const std::vector<std::string>& symbols = {},
                                      const json& params = json::object());
    AsyncPullType fetchOrderBookAsync(const std::string& symbol, int limit = 0,
                                        const json& params = json::object());
    AsyncPullType fetchTradesAsync(const std::string& symbol, int since = 0, int limit = 0,
                                     const json& params = json::object());
    AsyncPullType fetchOHLCVAsync(const std::string& symbol, const std::string& timeframe = "1m",
                                    int since = 0, int limit = 0, const json& params = json::object());

    // Trading Methods - Async
    AsyncPullType createOrderAsync(const std::string& symbol, const std::string& type,
                                     const std::string& side, double amount, double price = 0,
                                     const json& params = json::object());
    AsyncPullType cancelOrderAsync(const std::string& id, const std::string& symbol = "",
                                     const json& params = json::object());
    AsyncPullType fetchOrderAsync(const std::string& id, const std::string& symbol = "",
                                    const json& params = json::object());
    AsyncPullType fetchOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0,
                                     const json& params = json::object());
    AsyncPullType fetchOpenOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0,
                                         const json& params = json::object());
    AsyncPullType fetchClosedOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0,
                                           const json& params = json::object());
    AsyncPullType fetchMyTradesAsync(const std::string& symbol = "", int since = 0, int limit = 0,
                                       const json& params = json::object());

    // Account Methods - Async
    AsyncPullType fetchBalanceAsync(const json& params = json::object());
    AsyncPullType fetchDepositsAsync(const std::string& code = "", int since = 0, int limit = 0,
                                       const json& params = json::object());
    AsyncPullType fetchWithdrawalsAsync(const std::string& code = "", int since = 0, int limit = 0,
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
    std::string parseOrderStatus(const std::string& status);
    std::string sign(const std::string& path, const std::string& api = "public",
               const std::string& method = "GET", const json& params = json::object(),
               const json& headers = nullptr, const std::string& body = "") override;

private:
    // Async helper methods
    template<typename Func, typename... Args>
    AsyncPullType async(Func&& func, Args&&... args) {
        return std::async(std::launch::async,
                         std::forward<Func>(func),
                         this,
                         std::forward<Args>(args)...);
    }

    std::string v1;
    std::string v2;
};

} // namespace ccxt

#endif // CCXT_XT_H
