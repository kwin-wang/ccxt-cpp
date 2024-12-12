#pragma once

#include "ccxt/base/exchange.h"

namespace ccxt {

class Vertex : public Exchange {
public:
    Vertex();
    ~Vertex() override = default;

    // Market Data API
    json fetchMarkets(const json& params = json::object()) override;
    json fetchTicker(const String& symbol, const json& params = json::object()) override;
    json fetchTickers(const std::vector<String>& symbols = {}, const json& params = json::object()) override;
    json fetchOrderBook(const String& symbol, int limit = 0, const json& params = json::object()) override;
    json fetchTrades(const String& symbol, int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchOHLCV(const String& symbol, const String& timeframe = "1m",
                    int since = 0, int limit = 0, const json& params = json::object()) override;

    // Trading API
    json fetchBalance(const json& params = json::object()) override;
    json createOrder(const String& symbol, const String& type, const String& side,
                    double amount, double price = 0, const json& params = json::object()) override;
    json cancelOrder(const String& id, const String& symbol = "", const json& params = json::object()) override;
    json fetchOrder(const String& id, const String& symbol = "", const json& params = json::object()) override;
    json fetchOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchOpenOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchClosedOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;

    // Vertex specific methods
    json fetchMyTrades(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchPositions(const json& params = json::object());
    json fetchLeverage(const String& symbol, const json& params = json::object());
    json setLeverage(const String& symbol, double leverage, const json& params = json::object());
    json fetchFundingRate(const String& symbol, const json& params = json::object());
    json fetchFundingRateHistory(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchIndexOHLCV(const String& symbol, const String& timeframe = "1m",
                        int since = 0, int limit = 0, const json& params = json::object());
    json fetchMarkOHLCV(const String& symbol, const String& timeframe = "1m",
                       int since = 0, int limit = 0, const json& params = json::object());

protected:
    String sign(const String& path, const String& api = "public",
               const String& method = "GET", const json& params = json::object(),
               const std::map<String, String>& headers = {}, const json& body = nullptr) override;

private:
    void initializeApiEndpoints();
    String getVertexSymbol(const String& symbol);
    String getCommonSymbol(const String& vertexSymbol);
    json parseOrder(const json& order, const Market& market = Market());
    json parseTrade(const json& trade, const Market& market = Market());
    json parsePosition(const json& position, const Market& market = Market());
    json parseTicker(const json& ticker, const Market& market = Market());
    json parseOHLCV(const json& ohlcv, const Market& market = Market());
    json parseBalance(const json& response);
    json parseFundingRate(const json& fundingRate, const Market& market = Market());
    json parseLeverage(const json& leverage, const Market& market = Market());
    String parseOrderStatus(const String& status);
    String createSignature(const String& timestamp, const String& method,
                         const String& path, const String& body = "");

    std::map<String, String> timeframes;
    std::map<String, String> options;
    std::map<int, String> errorCodes;
};

} // namespace ccxt
