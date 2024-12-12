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
    json fetchTicker(const String& symbol, const json& params = json::object()) override;
    json fetchTickers(const std::vector<String>& symbols = {}, const json& params = json::object()) override;
    json fetchOrderBook(const String& symbol, int limit = 0, const json& params = json::object()) override;
    json fetchTrades(const String& symbol, int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchOHLCV(const String& symbol, const String& timeframe = "1m",
                    int since = 0, int limit = 0, const json& params = json::object()) override;

    // Market Data API - Async
    boost::future<json> fetchMarketsAsync(const json& params = json::object());
    boost::future<json> fetchTickerAsync(const String& symbol, const json& params = json::object());
    boost::future<json> fetchTickersAsync(const std::vector<String>& symbols = {}, const json& params = json::object());
    boost::future<json> fetchOrderBookAsync(const String& symbol, int limit = 0, const json& params = json::object());
    boost::future<json> fetchTradesAsync(const String& symbol, int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchOHLCVAsync(const String& symbol, const String& timeframe = "1m",
                                     int since = 0, int limit = 0, const json& params = json::object());

    // Trading API - Sync
    json fetchBalance(const json& params = json::object()) override;
    json createOrder(const String& symbol, const String& type, const String& side,
                    double amount, double price = 0, const json& params = json::object()) override;
    json cancelOrder(const String& id, const String& symbol = "", const json& params = json::object()) override;
    json fetchOrder(const String& id, const String& symbol = "", const json& params = json::object()) override;
    json fetchOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchOpenOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchClosedOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;

    // Trading API - Async
    boost::future<json> fetchBalanceAsync(const json& params = json::object());
    boost::future<json> createOrderAsync(const String& symbol, const String& type, const String& side,
                                     double amount, double price = 0, const json& params = json::object());
    boost::future<json> cancelOrderAsync(const String& id, const String& symbol = "", const json& params = json::object());
    boost::future<json> fetchOrderAsync(const String& id, const String& symbol = "", const json& params = json::object());
    boost::future<json> fetchOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchOpenOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchClosedOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());

    // Bitfinex specific methods - Sync
    json fetchPositions(const String& symbol = "", const json& params = json::object());
    json fetchMyTrades(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchLedger(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchFundingRates(const std::vector<String>& symbols = {}, const json& params = json::object());
    json setLeverage(const String& symbol, double leverage, const json& params = json::object());
    json fetchDeposits(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchWithdrawals(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());

    // Bitfinex specific methods - Async
    boost::future<json> fetchPositionsAsync(const String& symbol = "", const json& params = json::object());
    boost::future<json> fetchMyTradesAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchLedgerAsync(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchFundingRatesAsync(const std::vector<String>& symbols = {}, const json& params = json::object());
    boost::future<json> setLeverageAsync(const String& symbol, double leverage, const json& params = json::object());
    boost::future<json> fetchDepositsAsync(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchWithdrawalsAsync(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());

protected:
    String sign(const String& path, const String& api = "public",
               const String& method = "GET", const json& params = json::object(),
               const std::map<String, String>& headers = {}, const json& body = nullptr) override;

private:
    void initializeApiEndpoints();
    String getTimestamp();
    String createSignature(const String& path, const String& nonce,
                         const String& body = "");
    String getBitfinexSymbol(const String& symbol);
    String getCommonSymbol(const String& bitfinexSymbol);
    json parseOrderStatus(const String& status);
    json parseOrder(const json& order, const Market& market = Market());
    json parsePosition(const json& position, const Market& market = Market());
    json parseTrade(const json& trade, const Market& market = Market());
    json parseLedgerEntry(const json& item);

    std::map<String, String> timeframes;
    String version;  // "v1" or "v2"
    bool isV1;      // Using API v1
    bool isV2;      // Using API v2
};

} // namespace ccxt
