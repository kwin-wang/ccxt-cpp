#pragma once

#include "ccxt/base/exchange.h"

namespace ccxt {

class GateIO : public Exchange {
public:
    GateIO();
    ~GateIO() override = default;

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

    // Gate.io specific methods
    json fetchFundingRate(const String& symbol, const json& params = json::object());
    json fetchPositions(const String& symbol = "", const json& params = json::object());
    json setLeverage(const String& symbol, double leverage, const json& params = json::object());
    json setPositionMode(const String& hedged, const json& params = json::object());
    json fetchSettlements(const String& symbol, int since = 0, int limit = 0, const json& params = json::object());
    json fetchLiquidations(const String& symbol, int since = 0, int limit = 0, const json& params = json::object());

protected:
    String sign(const String& path, const String& api = "public",
               const String& method = "GET", const json& params = json::object(),
               const std::map<String, String>& headers = {}, const json& body = nullptr) override;

private:
    void initializeApiEndpoints();
    String getTimestamp();
    String createSignature(const String& method, const String& path,
                         const String& queryString = "", const String& body = "");
    String getGateSymbol(const String& symbol);
    String getCommonSymbol(const String& gateSymbol);
    json parseOrderStatus(const String& status);
    json parseOrder(const json& order, const Market& market = Market());
    json parsePosition(const json& position, const Market& market = Market());
    json parseTrade(const json& trade, const Market& market = Market());

    std::map<String, String> timeframes;
    String defaultType;  // "spot", "futures", "delivery"
    bool settle;  // Use settle=1 for futures API
};

} // namespace ccxt
