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
    json fetchTicker(const std::string& symbol, const json& params = json::object()) override;
    json fetchTickers(const std::vector<std::string>& symbols = {}, const json& params = json::object()) override;
    json fetchOrderBook(const std::string& symbol, int limit = 0, const json& params = json::object()) override;
    json fetchTrades(const std::string& symbol, int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchOHLCV(const std::string& symbol, const std::string& timeframe = "1m",
                    int since = 0, int limit = 0, const json& params = json::object()) override;

    // Trading API - Sync
    json fetchBalance(const json& params = json::object()) override;
    json createOrder(const std::string& symbol, const std::string& type, const std::string& side,
                    double amount, double price = 0, const json& params = json::object()) override;
    json cancelOrder(const std::string& id, const std::string& symbol = "", const json& params = json::object()) override;
    json fetchOrder(const std::string& id, const std::string& symbol = "", const json& params = json::object()) override;
    json fetchOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchOpenOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchClosedOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;

    // Market Data API - Async
    AsyncPullType fetchMarketsAsync(const json& params = json::object());
    AsyncPullType fetchTickerAsync(const std::string& symbol, const json& params = json::object());
    AsyncPullType fetchTickersAsync(const std::vector<std::string>& symbols = {}, const json& params = json::object());
    AsyncPullType fetchOrderBookAsync(const std::string& symbol, int limit = 0, const json& params = json::object());
    AsyncPullType fetchTradesAsync(const std::string& symbol, int since = 0, int limit = 0, const json& params = json::object());
    AsyncPullType fetchOHLCVAsync(const std::string& symbol, const std::string& timeframe = "1m",
                                     int since = 0, int limit = 0, const json& params = json::object());

    // Trading API - Async
    AsyncPullType fetchBalanceAsync(const json& params = json::object());
    AsyncPullType createOrderAsync(const std::string& symbol, const std::string& type, const std::string& side,
                                     double amount, double price = 0, const json& params = json::object());
    AsyncPullType cancelOrderAsync(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    AsyncPullType fetchOrderAsync(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    AsyncPullType fetchOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    AsyncPullType fetchOpenOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    AsyncPullType fetchClosedOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());

protected:
    std::string sign(const std::string& path, const std::string& api = "public",
               const std::string& method = "GET", const json& params = json::object(),
               const std::map<std::string, std::string>& headers = {}, const json& body = nullptr) override;

private:
    void initializeApiEndpoints();
    std::string getTimestamp();
    std::string createSignature(const std::string& method, const std::string& host,
                         const std::string& path, const std::map<std::string, std::string>& params);
    std::string getAccountId();

    std::string accountId;
    std::map<std::string, std::string> timeframes;
};

} // namespace ccxt
