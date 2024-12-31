#pragma once

#include "ccxt/base/exchange.h"

namespace ccxt {

class Vertex : public Exchange {
public:
    Vertex();
    ~Vertex() override = default;

    // Market Data API
    json fetchMarkets(const json& params = json::object()) override;
    json fetchTicker(const std::string& symbol, const json& params = json::object()) override;
    json fetchTickers(const std::vector<std::string>& symbols = {}, const json& params = json::object()) override;
    json fetchOrderBook(const std::string& symbol, int limit = 0, const json& params = json::object()) override;
    json fetchTrades(const std::string& symbol, int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchOHLCV(const std::string& symbol, const std::string& timeframe = "1m",
                    int since = 0, int limit = 0, const json& params = json::object()) override;

    // Trading API
    json fetchBalance(const json& params = json::object()) override;
    json createOrder(const std::string& symbol, const std::string& type, const std::string& side,
                    double amount, double price = 0, const json& params = json::object()) override;
    json cancelOrder(const std::string& id, const std::string& symbol = "", const json& params = json::object()) override;
    json fetchOrder(const std::string& id, const std::string& symbol = "", const json& params = json::object()) override;
    json fetchOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchOpenOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchClosedOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;

    // Vertex specific methods
    json fetchMyTrades(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchPositions(const json& params = json::object());
    json fetchLeverage(const std::string& symbol, const json& params = json::object());
    json setLeverage(const std::string& symbol, double leverage, const json& params = json::object());
    json fetchFundingRate(const std::string& symbol, const json& params = json::object());
    json fetchFundingRateHistory(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchIndexOHLCV(const std::string& symbol, const std::string& timeframe = "1m",
                        int since = 0, int limit = 0, const json& params = json::object());
    json fetchMarkOHLCV(const std::string& symbol, const std::string& timeframe = "1m",
                       int since = 0, int limit = 0, const json& params = json::object());

protected:
    std::string sign(const std::string& path, const std::string& api = "public",
               const std::string& method = "GET", const json& params = json::object(),
               const std::map<std::string, std::string>& headers = {}, const json& body = nullptr) override;

private:
    void initializeApiEndpoints();
    std::string getVertexSymbol(const std::string& symbol);
    std::string getCommonSymbol(const std::string& vertexSymbol);
    json parseOrder(const json& order, const Market& market = Market());
    json parseTrade(const json& trade, const Market& market = Market());
    json parsePosition(const json& position, const Market& market = Market());
    json parseTicker(const json& ticker, const Market& market = Market());
    json parseOHLCV(const json& ohlcv, const Market& market = Market());
    json parseBalance(const json& response);
    json parseFundingRate(const json& fundingRate, const Market& market = Market());
    json parseLeverage(const json& leverage, const Market& market = Market());
    std::string parseOrderStatus(const std::string& status);
    std::string createSignature(const std::string& timestamp, const std::string& method,
                         const std::string& path, const std::string& body = "");

    std::map<std::string, std::string> timeframes;
    std::map<std::string, std::string> options;
    std::map<int, std::string> errorCodes;
};

} // namespace ccxt
