#pragma once

#include "ccxt/base/exchange.h"
#include <boost/thread/future.hpp>

namespace ccxt {

class Bitfinex : public Exchange {
public:
    Bitfinex();
    ~Bitfinex() override = default;

    // Market Data API - Sync
    json fetchMarkets(const json& params = json::object()) override;
    json fetchTicker(const std::string& symbol, const json& params = json::object()) override;
    json fetchTickers(const std::vector<std::string>& symbols = {}, const json& params = json::object()) override;
    json fetchOrderBook(const std::string& symbol, int limit = 0, const json& params = json::object()) override;
    json fetchTrades(const std::string& symbol, int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchOHLCV(const std::string& symbol, const std::string& timeframe = "1m",
                    int since = 0, int limit = 0, const json& params = json::object()) override;

    // Market Data API - Async
    boost::future<json> fetchMarketsAsync(const json& params = json::object());
    boost::future<json> fetchTickerAsync(const std::string& symbol, const json& params = json::object());
    boost::future<json> fetchTickersAsync(const std::vector<std::string>& symbols = {}, const json& params = json::object());
    boost::future<json> fetchOrderBookAsync(const std::string& symbol, int limit = 0, const json& params = json::object());
    boost::future<json> fetchTradesAsync(const std::string& symbol, int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchOHLCVAsync(const std::string& symbol, const std::string& timeframe = "1m",
                                     int since = 0, int limit = 0, const json& params = json::object());

    // Trading API - Sync
    json fetchBalance(const json& params = json::object()) override;
    json createOrder(const std::string& symbol, const std::string& type, const std::string& side,
                    double amount, double price = 0, const json& params = json::object()) override;
    json cancelOrder(const std::string& id, const std::string& symbol = "", const json& params = json::object()) override;
    json fetchOrder(const std::string& id, const std::string& symbol = "", const json& params = json::object()) override;
    json fetchOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchOpenOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchClosedOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;

    // Trading API - Async
    boost::future<json> fetchBalanceAsync(const json& params = json::object());
    boost::future<json> createOrderAsync(const std::string& symbol, const std::string& type, const std::string& side,
                                     double amount, double price = 0, const json& params = json::object());
    boost::future<json> cancelOrderAsync(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    boost::future<json> fetchOrderAsync(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    boost::future<json> fetchOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchOpenOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchClosedOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());

    // Bitfinex specific methods - Sync
    json fetchPositions(const std::string& symbol = "", const json& params = json::object());
    json fetchMyTrades(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchLedger(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchFundingRates(const std::vector<std::string>& symbols = {}, const json& params = json::object());
    json setLeverage(const std::string& symbol, double leverage, const json& params = json::object());
    json fetchDeposits(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchWithdrawals(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());

    // Bitfinex specific methods - Async
    boost::future<json> fetchPositionsAsync(const std::string& symbol = "", const json& params = json::object());
    boost::future<json> fetchMyTradesAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchLedgerAsync(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchFundingRatesAsync(const std::vector<std::string>& symbols = {}, const json& params = json::object());
    boost::future<json> setLeverageAsync(const std::string& symbol, double leverage, const json& params = json::object());
    boost::future<json> fetchDepositsAsync(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchWithdrawalsAsync(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());

protected:
    std::string sign(const std::string& path, const std::string& api = "public",
               const std::string& method = "GET", const json& params = json::object(),
               const std::map<std::string, std::string>& headers = {}, const json& body = nullptr) override;

private:
    void initializeApiEndpoints();
    std::string getTimestamp();
    std::string createSignature(const std::string& path, const std::string& nonce,
                         const std::string& body = "");
    std::string getBitfinexSymbol(const std::string& symbol);
    std::string getCommonSymbol(const std::string& bitfinexSymbol);
    json parseOrderStatus(const std::string& status);
    json parseOrder(const json& order, const Market& market = Market());
    json parsePosition(const json& position, const Market& market = Market());
    json parseTrade(const json& trade, const Market& market = Market());
    json parseLedgerEntry(const json& item);

    std::map<std::string, std::string> timeframes;
    std::string version;  // "v1" or "v2"
    bool isV1;      // Using API v1
    bool isV2;      // Using API v2
};

} // namespace ccxt
