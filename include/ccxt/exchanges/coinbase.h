#pragma once

#include "ccxt/base/exchange.h"
#include <boost/future.hpp>

namespace ccxt {

class Coinbase : public Exchange {
public:
    Coinbase();
    ~Coinbase() override = default;

    // Market Data API - Synchronous
    json fetchMarkets(const json& params = json::object()) override;
    json fetchTicker(const String& symbol, const json& params = json::object()) override;
    json fetchTickers(const std::vector<String>& symbols = {}, const json& params = json::object()) override;
    json fetchOrderBook(const String& symbol, int limit = 0, const json& params = json::object()) override;
    json fetchTrades(const String& symbol, int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchOHLCV(const String& symbol, const String& timeframe = "1m",
                    int since = 0, int limit = 0, const json& params = json::object()) override;

    // Trading API - Synchronous
    json fetchBalance(const json& params = json::object()) override;
    json createOrder(const String& symbol, const String& type, const String& side,
                    double amount, double price = 0, const json& params = json::object()) override;
    json cancelOrder(const String& id, const String& symbol = "", const json& params = json::object()) override;
    json fetchOrder(const String& id, const String& symbol = "", const json& params = json::object()) override;
    json fetchOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchOpenOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchClosedOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;

    // Market Data API - Asynchronous
    boost::future<json> fetchMarketsAsync(const json& params = json::object()) const;
    boost::future<json> fetchTickerAsync(const String& symbol, const json& params = json::object()) const;
    boost::future<json> fetchTickersAsync(const std::vector<String>& symbols = {}, const json& params = json::object()) const;
    boost::future<json> fetchOrderBookAsync(const String& symbol, int limit = 0, const json& params = json::object()) const;
    boost::future<json> fetchTradesAsync(const String& symbol, int since = 0, int limit = 0, const json& params = json::object()) const;
    boost::future<json> fetchOHLCVAsync(const String& symbol, const String& timeframe = "1m",
                                     int since = 0, int limit = 0, const json& params = json::object()) const;

    // Trading API - Asynchronous
    boost::future<json> fetchBalanceAsync(const json& params = json::object()) const;
    boost::future<json> createOrderAsync(const String& symbol, const String& type, const String& side,
                                     double amount, double price = 0, const json& params = json::object());
    boost::future<json> cancelOrderAsync(const String& id, const String& symbol = "", const json& params = json::object());
    boost::future<json> fetchOrderAsync(const String& id, const String& symbol = "", const json& params = json::object()) const;
    boost::future<json> fetchOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) const;
    boost::future<json> fetchOpenOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) const;
    boost::future<json> fetchClosedOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) const;

protected:
    String sign(const String& path, const String& api = "public",
               const String& method = "GET", const json& params = json::object(),
               const std::map<String, String>& headers = {}, const json& body = nullptr) override;

private:
    void initializeApiEndpoints();
    String getTimestamp();
    String createSignature(const String& timestamp, const String& method,
                         const String& requestPath, const String& body = "");
    std::map<String, String> getAuthHeaders(const String& method,
                                          const String& requestPath,
                                          const String& body = "");
};

} // namespace ccxt
