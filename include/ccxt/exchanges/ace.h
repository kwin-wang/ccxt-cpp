#pragma once

#include "ccxt/base/exchange.h"

namespace ccxt {

class ace : public Exchange {
public:
    explicit ace(const Config& config = Config());
    virtual ~ace() = default;

    // Common methods
    void init() override;
    json describe() const override;

    // Synchronous REST API methods
    json fetchMarkets(const json& params = json::object()) override;
    json fetchTicker(const String& symbol, const json& params = json::object()) override;
    json fetchTickers(const std::vector<String>& symbols = {}, const json& params = json::object()) override;
    json fetchOrderBook(const String& symbol, int limit = 0, const json& params = json::object()) override;
    json fetchOHLCV(const String& symbol, const String& timeframe = "1m",
                   int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchBalance(const json& params = json::object()) override;
    json createOrder(const String& symbol, const String& type, const String& side,
                    double amount, double price = 0, const json& params = json::object()) override;
    json cancelOrder(const String& id, const String& symbol = "", const json& params = json::object()) override;
    json fetchOrder(const String& id, const String& symbol = "", const json& params = json::object()) override;
    json fetchOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchOpenOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchClosedOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchMyTrades(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchOrderTrades(const String& id, const String& symbol = "", const json& params = json::object());

    // Asynchronous REST API methods
    void fetchMarketsAsync(const json& params = json::object(), const std::function<void(const json&)> &callback) override;
    void fetchTickerAsync(const String& symbol, const json& params = json::object(), const std::function<void(const json&)> &callback) override;
    void fetchTickersAsync(const std::vector<String>& symbols = {}, const json& params = json::object(), const std::function<void(const json&)> &callback) override;
    void fetchOrderBookAsync(const String& symbol, int limit = 0, const json& params = json::object(), const std::function<void(const json&)> &callback) override;
    void fetchOHLCVAsync(const String& symbol, const String& timeframe = "1m",
                        int since = 0, int limit = 0, const json& params = json::object(), const std::function<void(const json&)> &callback) override;
    void fetchBalanceAsync(const json& params = json::object(), const std::function<void(const json&)> &callback) override;
    void createOrderAsync(const String& symbol, const String& type, const String& side,
                         double amount, double price = 0, const json& params = json::object(), const std::function<void(const json&)> &callback) override;
    void cancelOrderAsync(const String& id, const String& symbol = "", const json& params = json::object(), const std::function<void(const json&)> &callback) override;
    void fetchOrderAsync(const String& id, const String& symbol = "", const json& params = json::object(), const std::function<void(const json&)> &callback) override;
    void fetchOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object(), const std::function<void(const json&)> &callback) override;
    void fetchOpenOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object(), const std::function<void(const json&)> &callback) override;
    void fetchClosedOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object(), const std::function<void(const json&)> &callback) override;
    void fetchMyTradesAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object(), const std::function<void(const json&)> &callback);
    void fetchOrderTradesAsync(const String& id, const String& symbol = "", const json& params = json::object(), const std::function<void(const json&)> &callback);

protected:
    // HTTP methods
    json fetch(const String& url,
              const String& method = "GET",
              const std::map<String, String>& headers = {},
              const String& body = "") override;

    // Utility methods
    String sign(const String& path,
               const String& api = "public",
               const String& method = "GET",
               const std::map<String, String>& params = {},
               const std::map<String, String>& headers = {}) override;

    // Parsing methods
    json parseMarket(const json& market) const;
    json parseTicker(const json& ticker, const json& market = nullptr) const;
    json parseOrderBook(const json& orderBook, const String& symbol, const json& market = nullptr) const;
    json parseOHLCV(const json& ohlcv, const json& market = nullptr) const;
    json parseOrder(const json& order, const json& market = nullptr) const;
    String parseOrderStatus(const String& status) const;
    json parseTrade(const json& trade, const json& market = nullptr) const;
    json parseBalance(const json& balance) const;

private:
    // API Endpoints
    const String publicApiUrl = "https://ace.io/polarisex";
    const String privateApiUrl = "https://ace.io/polarisex/open";
    
    // Trading fees
    const double makerFee = 0.0005;
    const double takerFee = 0.001;
};

} // namespace ccxt
