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
    Json fetchTicker(const std::string& symbol, const json& params = json::object()) override;
    Json fetchTickers(const std::vector<std::string>& symbols = {}, const json& params = json::object()) override;
    Json fetchOrderBook(const std::string& symbol, int limit = 0, const json& params = json::object()) override;
    Json fetchTrades(const std::string& symbol, int since = 0, int limit = 0, const json& params = json::object()) override;
    Json fetchOHLCV(const std::string& symbol, const std::string& timeframe = "1m", int since = 0, int limit = 0, const json& params = json::object()) override;
    Json fetchBalance(const json& params = json::object()) override;
    Json createOrder(const std::string& symbol, const std::string& type, const std::string& side, double amount, double price = 0, const json& params = json::object()) override;
    Json cancelOrder(const std::string& id, const std::string& symbol = "", const json& params = json::object()) override;
    Json cancelAllOrders(const std::string& symbol = "", const json& params = json::object()) override;
    Json fetchOrder(const std::string& id, const std::string& symbol = "", const json& params = json::object()) override;
    Json fetchOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    Json fetchOpenOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    Json fetchClosedOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    Json fetchMyTrades(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;

    // Asynchronous REST API
    boost::future<Json> fetchMarketsAsync(const json& params = json::object());
    boost::future<Json> fetchCurrenciesAsync(const json& params = json::object());
    boost::future<Json> fetchTickerAsync(const std::string& symbol, const json& params = json::object());
    boost::future<Json> fetchTickersAsync(const std::vector<std::string>& symbols = {}, const json& params = json::object());
    boost::future<Json> fetchOrderBookAsync(const std::string& symbol, int limit = 0, const json& params = json::object());
    boost::future<Json> fetchTradesAsync(const std::string& symbol, int since = 0, int limit = 0, const json& params = json::object());
    boost::future<Json> fetchOHLCVAsync(const std::string& symbol, const std::string& timeframe = "1m", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<Json> fetchBalanceAsync(const json& params = json::object());
    boost::future<Json> createOrderAsync(const std::string& symbol, const std::string& type, const std::string& side, double amount, double price = 0, const json& params = json::object());
    boost::future<Json> cancelOrderAsync(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    boost::future<Json> cancelAllOrdersAsync(const std::string& symbol = "", const json& params = json::object());
    boost::future<Json> fetchOrderAsync(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    boost::future<Json> fetchOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<Json> fetchOpenOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<Json> fetchClosedOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<Json> fetchMyTradesAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());

protected:
    void init() override;
    std::string sign(const std::string& path, const std::string& api, const std::string& method, const json& params,
               const std::map<std::string, std::string>& headers, const json& body) override;
    void handleErrors(const std::string& httpCode, const std::string& reason, const std::string& url, const std::string& method,
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
