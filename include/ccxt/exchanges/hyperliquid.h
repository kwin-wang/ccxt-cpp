#pragma once

#include "ccxt/base/exchange.h"
#include <future>

namespace ccxt {

class Hyperliquid : public Exchange {
public:
    Hyperliquid();
    ~Hyperliquid() override = default;

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
    json fetchMyTrades(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;

    // Market Data API - Async
    std::future<json> fetchMarketsAsync(const json& params = json::object());
    std::future<json> fetchTickerAsync(const String& symbol, const json& params = json::object());
    std::future<json> fetchTickersAsync(const std::vector<String>& symbols = {}, const json& params = json::object());
    std::future<json> fetchOrderBookAsync(const String& symbol, int limit = 0, const json& params = json::object());
    std::future<json> fetchTradesAsync(const String& symbol, int since = 0, int limit = 0, const json& params = json::object());
    std::future<json> fetchOHLCVAsync(const String& symbol, const String& timeframe = "1m",
                                     int since = 0, int limit = 0, const json& params = json::object());

    // Trading API - Async
    std::future<json> fetchBalanceAsync(const json& params = json::object());
    std::future<json> createOrderAsync(const String& symbol, const String& type, const String& side,
                                     double amount, double price = 0, const json& params = json::object());
    std::future<json> cancelOrderAsync(const String& id, const String& symbol = "", const json& params = json::object());
    std::future<json> fetchOrderAsync(const String& id, const String& symbol = "", const json& params = json::object());
    std::future<json> fetchOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    std::future<json> fetchOpenOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    std::future<json> fetchClosedOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    std::future<json> fetchMyTradesAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());

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
