#pragma once

#include <string>
#include <map>
#include <vector>
#include <optional>
#include <future>
#include <nlohmann/json.hpp>
#include "types.h"
#include "config.h"
#include "exchange_base.h"

namespace ccxt {

using json = nlohmann::json;
using String = std::string;

class Exchange : public ExchangeBase {
public:
    Exchange(const Config& config = Config()) : ExchangeBase(config) {}
    virtual ~Exchange() = default;

    // Common methods
    virtual void init();
    virtual json describe() const;

    // Synchronous REST API methods
    virtual json fetchMarkets(const json& params = json::object());
    virtual json fetchTicker(const String& symbol, const json& params = json::object());
    virtual json fetchTickers(const std::vector<String>& symbols = {}, const json& params = json::object());
    virtual json fetchOrderBook(const String& symbol, int limit = 0, const json& params = json::object());
    virtual json fetchTrades(const String& symbol, int since = 0, int limit = 0, const json& params = json::object());
    virtual json fetchOHLCV(const String& symbol, const String& timeframe = "1m",
                          int since = 0, int limit = 0, const json& params = json::object());
    virtual json fetchBalance(const json& params = json::object());
    virtual json createOrder(const String& symbol, const String& type, const String& side,
                           double amount, double price = 0, const json& params = json::object());
    virtual json cancelOrder(const String& id, const String& symbol = "", const json& params = json::object());
    virtual json fetchOrder(const String& id, const String& symbol = "", const json& params = json::object());
    virtual json fetchOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    virtual json fetchOpenOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    virtual json fetchClosedOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());

    // Asynchronous REST API methods
    virtual std::future<json> fetchMarketsAsync(const json& params = json::object());
    virtual std::future<json> fetchTickerAsync(const String& symbol, const json& params = json::object());
    virtual std::future<json> fetchTickersAsync(const std::vector<String>& symbols = {}, const json& params = json::object());
    virtual std::future<json> fetchOrderBookAsync(const String& symbol, int limit = 0, const json& params = json::object());
    virtual std::future<json> fetchTradesAsync(const String& symbol, int since = 0, int limit = 0, const json& params = json::object());
    virtual std::future<json> fetchOHLCVAsync(const String& symbol, const String& timeframe = "1m",
                                           int since = 0, int limit = 0, const json& params = json::object());
    virtual std::future<json> fetchBalanceAsync(const json& params = json::object());
    virtual std::future<json> createOrderAsync(const String& symbol, const String& type, const String& side,
                                           double amount, double price = 0, const json& params = json::object());
    virtual std::future<json> cancelOrderAsync(const String& id, const String& symbol = "", const json& params = json::object());
    virtual std::future<json> fetchOrderAsync(const String& id, const String& symbol = "", const json& params = json::object());
    virtual std::future<json> fetchOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    virtual std::future<json> fetchOpenOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    virtual std::future<json> fetchClosedOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());

protected:
    // Synchronous HTTP methods
    virtual json fetch(const String& url,
                      const String& method = "GET",
                      const std::map<String, String>& headers = {},
                      const String& body = "");

    // Asynchronous HTTP methods
    virtual std::future<json> fetchAsync(const String& url,
                                     const String& method = "GET",
                                     const std::map<String, String>& headers = {},
                                     const String& body = "");

    // Utility methods
    virtual String sign(const String& path, const String& api = "public",
                     const String& method = "GET",
                     const std::map<String, String>& params = {},
                     const std::map<String, String>& headers = {});

    // Parsing methods
    virtual json parseMarket(const json& market) const;
    virtual json parseTicker(const json& ticker, const json& market = json::object()) const;
    virtual json parseOrderBook(const json& orderbook, const String& symbol = "", const json& market = json::object()) const;
    virtual json parseOHLCV(const json& ohlcv, const json& market = json::object()) const;
    virtual json parseOrder(const json& order, const json& market = json::object()) const;
    virtual json parseTrade(const json& trade, const json& market = json::object()) const;
    virtual json parseBalance(const json& balance) const;
};

} // namespace ccxt
