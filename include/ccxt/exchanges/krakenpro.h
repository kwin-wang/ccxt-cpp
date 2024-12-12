#pragma once

#include "ccxt/base/exchange.h"

namespace ccxt {

class KrakenPro : public Exchange {
public:
    KrakenPro();
    ~KrakenPro() override = default;

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

    // Kraken Pro specific methods
    json fetchPositions(const String& symbol = "", const json& params = json::object());
    json fetchLeverage(const String& symbol, const json& params = json::object());
    json setLeverage(const String& symbol, double leverage, const json& params = json::object());
    json setMarginMode(const String& symbol, const String& marginMode, const json& params = json::object());
    json fetchFundingRate(const String& symbol, const json& params = json::object());
    json fetchFundingRates(const std::vector<String>& symbols = {}, const json& params = json::object());
    json fetchFundingHistory(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchIndexOHLCV(const String& symbol, const String& timeframe = "1m",
                        int since = 0, int limit = 0, const json& params = json::object());
    json fetchMarkOHLCV(const String& symbol, const String& timeframe = "1m",
                       int since = 0, int limit = 0, const json& params = json::object());
    json fetchPremiumIndexOHLCV(const String& symbol, const String& timeframe = "1m",
                               int since = 0, int limit = 0, const json& params = json::object());
    json fetchMyTrades(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchDeposits(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchWithdrawals(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());

protected:
    String sign(const String& path, const String& api = "public",
               const String& method = "GET", const json& params = json::object(),
               const std::map<String, String>& headers = {}, const json& body = nullptr) override;

private:
    void initializeApiEndpoints();
    String getTimestamp();
    String createSignature(const String& timestamp, const String& method,
                         const String& path, const String& body = "");
    String getKrakenSymbol(const String& symbol);
    String getCommonSymbol(const String& krakenSymbol);
    json parseOrder(const json& order, const Market& market = Market());
    json parseTrade(const json& trade, const Market& market = Market());
    json parsePosition(const json& position, const Market& market = Market());
    json parseOrderStatus(const String& status);
    double parseNumber(const String& numberString);
    String formatNumber(double number);
    String getOrderId(const json& order);

    std::map<String, String> timeframes;
    bool testnet;
    String defaultType;  // "spot" or "futures"
    std::map<String, String> options;
};

} // namespace ccxt
