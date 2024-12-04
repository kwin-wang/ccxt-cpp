#pragma once

#include "../base/exchange.h"

namespace ccxt {

class Deribit : public Exchange {
public:
    Deribit();
    ~Deribit() override = default;

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

    // Deribit specific methods
    json fetchPositions(const String& symbol = "", const json& params = json::object());
    json fetchDeposits(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchWithdrawals(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchMyTrades(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchLedger(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchOpenInterest(const String& symbol, const json& params = json::object());
    json fetchOptionChain(const String& underlying, const json& params = json::object());
    json fetchVolatilityHistory(const String& symbol, const json& params = json::object());
    json setMarginMode(const String& marginMode, const json& params = json::object());
    json setLeverage(const String& symbol, double leverage, const json& params = json::object());

protected:
    String sign(const String& path, const String& api = "public",
               const String& method = "GET", const json& params = json::object(),
               const std::map<String, String>& headers = {}, const json& body = nullptr) override;

private:
    void initializeApiEndpoints();
    String getTimestamp();
    String createSignature(const String& timestamp, const String& method,
                         const String& path, const String& body = "");
    String getDeribitSymbol(const String& symbol);
    String getCommonSymbol(const String& deribitSymbol);
    json parseOrderStatus(const String& status);
    json parseOrder(const json& order, const Market& market = Market());
    json parsePosition(const json& position, const Market& market = Market());
    json parseTrade(const json& trade, const Market& market = Market());
    json parseOptionContract(const json& contract);

    std::map<String, String> timeframes;
    bool testnet;
    String defaultType;  // "future" or "option"
    String defaultSettlement;  // "BTC" or "ETH" or "USDC"
};

} // namespace ccxt
