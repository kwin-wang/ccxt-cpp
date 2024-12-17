#pragma once

#include "ccxt/base/exchange.h"
#include <future>

namespace ccxt {

class Huobi : public Exchange {
public:
    Huobi();
    ~Huobi() override = default;

    // Market Data API - Sync
    json fetchMarkets(const json& params = json::object()) override;
    json fetchTicker(const String& symbol, const json& params = json::object()) override;
    json fetchTickers(const std::vector<String>& symbols = {}, const json& params = json::object()) override;
    json fetchOrderBook(const String& symbol, int limit = 0, const json& params = json::object()) override;
    json fetchTrades(const String& symbol, int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchOHLCV(const String& symbol, const String& timeframe = "1m",
                    int since = 0, int limit = 0, const json& params = json::object()) override;

    // Trading API - Sync
    json fetchBalance(const json& params = json::object()) override;
    json createOrder(const String& symbol, const String& type, const String& side,
                    double amount, double price = 0, const json& params = json::object()) override;
    json cancelOrder(const String& id, const String& symbol = "", const json& params = json::object()) override;
    json fetchOrder(const String& id, const String& symbol = "", const json& params = json::object()) override;
    json fetchOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchOpenOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchClosedOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;

    // Market Data API - Async
    AsyncPullType fetchMarketsAsync(const json& params = json::object());
    AsyncPullType fetchTickerAsync(const String& symbol, const json& params = json::object());
    AsyncPullType fetchTickersAsync(const std::vector<String>& symbols = {}, const json& params = json::object());
    AsyncPullType fetchOrderBookAsync(const String& symbol, int limit = 0, const json& params = json::object());
    AsyncPullType fetchTradesAsync(const String& symbol, int since = 0, int limit = 0, const json& params = json::object());
    AsyncPullType fetchOHLCVAsync(const String& symbol, const String& timeframe = "1m",
                                     int since = 0, int limit = 0, const json& params = json::object());

    // Trading API - Async
    AsyncPullType fetchBalanceAsync(const json& params = json::object());
    AsyncPullType createOrderAsync(const String& symbol, const String& type, const String& side,
                                     double amount, double price = 0, const json& params = json::object());
    AsyncPullType cancelOrderAsync(const String& id, const String& symbol = "", const json& params = json::object());
    AsyncPullType fetchOrderAsync(const String& id, const String& symbol = "", const json& params = json::object());
    AsyncPullType fetchOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    AsyncPullType fetchOpenOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    AsyncPullType fetchClosedOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());

protected:
    String sign(const String& path, const String& api = "public",
               const String& method = "GET", const json& params = json::object(),
               const std::map<String, String>& headers = {}, const json& body = nullptr) override;

private:
    void initializeApiEndpoints();
    String getTimestamp();
    String createSignature(const String& method, const String& host,
                         const String& path, const std::map<String, String>& params);
    String getAccountId();

    String accountId;
    std::map<String, String> timeframes;
};

} // namespace ccxt
