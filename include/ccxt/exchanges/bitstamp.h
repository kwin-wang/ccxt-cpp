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
    json fetchTicker(const std::string& symbol, const json& params = json::object()) override;
    json fetchTickers(const std::vector<std::string>& symbols = {}, const json& params = json::object()) override;
    json fetchOrderBook(const std::string& symbol, int limit = 0, const json& params = json::object()) override;
    json fetchTrades(const std::string& symbol, int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchOHLCV(const std::string& symbol, const std::string& timeframe = "1m", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchBalance(const json& params = json::object()) override;
    json createOrder(const std::string& symbol, const std::string& type, const std::string& side, double amount, double price = 0, const json& params = json::object()) override;
    json cancelOrder(const std::string& id, const std::string& symbol = "", const json& params = json::object()) override;
    json fetchOrder(const std::string& id, const std::string& symbol = "", const json& params = json::object()) override;
    json fetchMyTrades(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchTransactionFees(const json& params = json::object());
    json fetchTradingFees(const json& params = json::object());
    json withdraw(const std::string& code, double amount, const std::string& address, const std::string& tag = "", const json& params = json::object());

    // Asynchronous REST API
    boost::future<json> fetchMarketsAsync(const json& params = json::object());
    boost::future<json> fetchTickerAsync(const std::string& symbol, const json& params = json::object());
    boost::future<json> fetchTickersAsync(const std::vector<std::string>& symbols = {}, const json& params = json::object());
    boost::future<json> fetchOrderBookAsync(const std::string& symbol, int limit = 0, const json& params = json::object());
    boost::future<json> fetchTradesAsync(const std::string& symbol, int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchOHLCVAsync(const std::string& symbol, const std::string& timeframe = "1m", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchBalanceAsync(const json& params = json::object());
    boost::future<json> createOrderAsync(const std::string& symbol, const std::string& type, const std::string& side, double amount, double price = 0, const json& params = json::object());
    boost::future<json> cancelOrderAsync(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    boost::future<json> fetchOrderAsync(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    boost::future<json> fetchMyTradesAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchTransactionFeesAsync(const json& params = json::object());
    boost::future<json> fetchTradingFeesAsync(const json& params = json::object());
    boost::future<json> withdrawAsync(const std::string& code, double amount, const std::string& address, const std::string& tag = "", const json& params = json::object());

protected:
    std::string sign(const std::string& path, const std::string& api, const std::string& method, const json& params,
               const std::map<std::string, std::string>& headers, const json& body) override;
    std::string getNonce() override;
    json parseOrder(const json& order, const Market& market = Market());
    std::string parseOrderStatus(const std::string& status);

private:
    boost::asio::io_context io_context;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard;
    std::thread io_thread;
};

} // namespace ccxt
