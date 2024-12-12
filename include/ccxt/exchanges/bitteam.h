#ifndef CCXT_EXCHANGE_BITTEAM_H
#define CCXT_EXCHANGE_BITTEAM_H

#include "ccxt/base/exchange.h"
#include <boost/asio.hpp>
#include <boost/future.hpp>

namespace ccxt {

class bitteam : public Exchange {
public:
    bitteam(const Config& config = Config());
    ~bitteam() override;

    static Exchange* create(const Config& config = Config()) {
        return new bitteam(config);
    }

    // Synchronous REST API
    Json fetchMarkets(const json& params = json::object()) override;
    Json fetchCurrencies(const json& params = json::object()) override;
    Json fetchTicker(const String& symbol, const json& params = json::object()) override;
    Json fetchTickers(const std::vector<String>& symbols = {}, const json& params = json::object()) override;
    Json fetchOrderBook(const String& symbol, int limit = 0, const json& params = json::object()) override;
    Json fetchTrades(const String& symbol, int since = 0, int limit = 0, const json& params = json::object()) override;
    Json fetchOHLCV(const String& symbol, const String& timeframe = "1m", int since = 0, int limit = 0, const json& params = json::object()) override;
    Json fetchBalance(const json& params = json::object()) override;
    Json createOrder(const String& symbol, const String& type, const String& side, double amount, double price = 0, const json& params = json::object()) override;
    Json cancelOrder(const String& id, const String& symbol = "", const json& params = json::object()) override;
    Json cancelAllOrders(const String& symbol = "", const json& params = json::object()) override;
    Json fetchOrder(const String& id, const String& symbol = "", const json& params = json::object()) override;
    Json fetchOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    Json fetchOpenOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    Json fetchClosedOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    Json fetchMyTrades(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;

    // Asynchronous REST API
    boost::future<Json> fetchMarketsAsync(const json& params = json::object());
    boost::future<Json> fetchCurrenciesAsync(const json& params = json::object());
    boost::future<Json> fetchTickerAsync(const String& symbol, const json& params = json::object());
    boost::future<Json> fetchTickersAsync(const std::vector<String>& symbols = {}, const json& params = json::object());
    boost::future<Json> fetchOrderBookAsync(const String& symbol, int limit = 0, const json& params = json::object());
    boost::future<Json> fetchTradesAsync(const String& symbol, int since = 0, int limit = 0, const json& params = json::object());
    boost::future<Json> fetchOHLCVAsync(const String& symbol, const String& timeframe = "1m", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<Json> fetchBalanceAsync(const json& params = json::object());
    boost::future<Json> createOrderAsync(const String& symbol, const String& type, const String& side, double amount, double price = 0, const json& params = json::object());
    boost::future<Json> cancelOrderAsync(const String& id, const String& symbol = "", const json& params = json::object());
    boost::future<Json> cancelAllOrdersAsync(const String& symbol = "", const json& params = json::object());
    boost::future<Json> fetchOrderAsync(const String& id, const String& symbol = "", const json& params = json::object());
    boost::future<Json> fetchOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<Json> fetchOpenOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<Json> fetchClosedOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<Json> fetchMyTradesAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());

protected:
    void init() override;
    String sign(const String& path, const String& api, const String& method, const json& params,
               const std::map<String, String>& headers, const json& body) override;
    void handleErrors(const String& httpCode, const String& reason, const String& url, const String& method,
                     const json& headers, const json& body, const json& response, const json& requestHeaders,
                     const json& requestBody) override;

    // Helper methods
    Json parseTicker(const Json& ticker, const Json& market = Json());
    Json parseTrade(const Json& trade, const Json& market = Json());
    Json parseOrder(const Json& order, const Json& market = Json());
    Json parseTransaction(const Json& transaction, const Json& currency = Json());

private:
    boost::asio::io_context io_context;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard;
    std::thread io_thread;

    static const std::string defaultBaseURL;
    static const std::string defaultVersion;
    static const int defaultRateLimit;
    static const bool defaultPro;
};

} // namespace ccxt

#endif // CCXT_EXCHANGE_BITTEAM_H
