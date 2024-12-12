#pragma once

#include "ccxt/base/exchange.h"
#include <boost/asio.hpp>
#include <boost/future.hpp>

namespace ccxt {

class Bitstamp : public Exchange {
public:
    Bitstamp();
    ~Bitstamp() override = default;

    // Synchronous REST API
    json fetchMarkets(const json& params = json::object()) override;
    json fetchTicker(const String& symbol, const json& params = json::object()) override;
    json fetchTickers(const std::vector<String>& symbols = {}, const json& params = json::object()) override;
    json fetchOrderBook(const String& symbol, int limit = 0, const json& params = json::object()) override;
    json fetchTrades(const String& symbol, int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchOHLCV(const String& symbol, const String& timeframe = "1m", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchBalance(const json& params = json::object()) override;
    json createOrder(const String& symbol, const String& type, const String& side, double amount, double price = 0, const json& params = json::object()) override;
    json cancelOrder(const String& id, const String& symbol = "", const json& params = json::object()) override;
    json fetchOrder(const String& id, const String& symbol = "", const json& params = json::object()) override;
    json fetchMyTrades(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchTransactionFees(const json& params = json::object());
    json fetchTradingFees(const json& params = json::object());
    json withdraw(const String& code, double amount, const String& address, const String& tag = "", const json& params = json::object());

    // Asynchronous REST API
    boost::future<json> fetchMarketsAsync(const json& params = json::object());
    boost::future<json> fetchTickerAsync(const String& symbol, const json& params = json::object());
    boost::future<json> fetchTickersAsync(const std::vector<String>& symbols = {}, const json& params = json::object());
    boost::future<json> fetchOrderBookAsync(const String& symbol, int limit = 0, const json& params = json::object());
    boost::future<json> fetchTradesAsync(const String& symbol, int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchOHLCVAsync(const String& symbol, const String& timeframe = "1m", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchBalanceAsync(const json& params = json::object());
    boost::future<json> createOrderAsync(const String& symbol, const String& type, const String& side, double amount, double price = 0, const json& params = json::object());
    boost::future<json> cancelOrderAsync(const String& id, const String& symbol = "", const json& params = json::object());
    boost::future<json> fetchOrderAsync(const String& id, const String& symbol = "", const json& params = json::object());
    boost::future<json> fetchMyTradesAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchTransactionFeesAsync(const json& params = json::object());
    boost::future<json> fetchTradingFeesAsync(const json& params = json::object());
    boost::future<json> withdrawAsync(const String& code, double amount, const String& address, const String& tag = "", const json& params = json::object());

protected:
    String sign(const String& path, const String& api, const String& method, const json& params,
               const std::map<String, String>& headers, const json& body) override;
    String getNonce() override;
    json parseOrder(const json& order, const Market& market = Market());
    String parseOrderStatus(const String& status);

private:
    boost::asio::io_context io_context;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard;
    std::thread io_thread;
};

} // namespace ccxt
