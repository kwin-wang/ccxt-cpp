#pragma once

#include "ccxt/base/exchange.h"

namespace ccxt {

class GateIO : public Exchange {
public:
    GateIO();
    ~GateIO() override = default;

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

    // Gate.io specific methods
    json fetchFundingRate(const std::string& symbol, const json& params = json::object());
    json fetchPositions(const std::string& symbol = "", const json& params = json::object());
    json setLeverage(const std::string& symbol, double leverage, const json& params = json::object());
    json setPositionMode(const std::string& hedged, const json& params = json::object());
    json fetchSettlements(const std::string& symbol, int since = 0, int limit = 0, const json& params = json::object());
    json fetchLiquidations(const std::string& symbol, int since = 0, int limit = 0, const json& params = json::object());

protected:
    std::string sign(const std::string& path, const std::string& api = "public",
               const std::string& method = "GET", const json& params = json::object(),
               const std::map<std::string, std::string>& headers = {}, const json& body = nullptr) override;

private:
    void initializeApiEndpoints();
    std::string getTimestamp();
    std::string createSignature(const std::string& method, const std::string& path,
                         const std::string& querystd::string = "", const std::string& body = "");
    std::string getGateSymbol(const std::string& symbol);
    std::string getCommonSymbol(const std::string& gateSymbol);
    json parseOrderStatus(const std::string& status);
    json parseOrder(const json& order, const Market& market = Market());
    json parsePosition(const json& position, const Market& market = Market());
    json parseTrade(const json& trade, const Market& market = Market());

    std::map<std::string, std::string> timeframes;
    std::string defaultType;  // "spot", "futures", "delivery"
    bool settle;  // Use settle=1 for futures API
};

} // namespace ccxt
