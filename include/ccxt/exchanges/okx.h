#pragma once

#include "ccxt/base/exchange.h"

namespace ccxt {

class OKX : public Exchange {
public:
    OKX();
    ~OKX() override = default;

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

    // Additional REST API methods
    json fetchMyTrades(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchLedger(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchDeposits(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchWithdrawals(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchDepositAddress(const String& code, const json& params = json::object());
    json fetchFundingRate(const String& symbol, const json& params = json::object());
    json fetchFundingRateHistory(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json setLeverage(int leverage, const String& symbol = "", const String& type = "", const json& params = json::object());
    json setMarginMode(const String& marginMode, const String& symbol = "", const json& params = json::object());

    // Async REST API methods
    json fetchMarketsAsync(const json& params = json::object());
    json fetchTickerAsync(const String& symbol, const json& params = json::object());
    json fetchTickersAsync(const std::vector<String>& symbols = {}, const json& params = json::object());
    json fetchOrderBookAsync(const String& symbol, int limit = 0, const json& params = json::object());
    json fetchTradesAsync(const String& symbol, int since = 0, int limit = 0, const json& params = json::object());
    json fetchOHLCVAsync(const String& symbol, const String& timeframe = "1m",
                        int since = 0, int limit = 0, const json& params = json::object());
    json fetchBalanceAsync(const json& params = json::object());
    json createOrderAsync(const String& symbol, const String& type, const String& side,
                         double amount, double price = 0, const json& params = json::object());
    json cancelOrderAsync(const String& id, const String& symbol = "", const json& params = json::object());
    json fetchOrderAsync(const String& id, const String& symbol = "", const json& params = json::object());
    json fetchOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchOpenOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchClosedOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchMyTradesAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchLedgerAsync(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchDepositsAsync(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchWithdrawalsAsync(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchDepositAddressAsync(const String& code, const json& params = json::object());
    json fetchFundingRateAsync(const String& symbol, const json& params = json::object());
    json fetchFundingRateHistoryAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json setLeverageAsync(int leverage, const String& symbol = "", const String& type = "", const json& params = json::object());
    json setMarginModeAsync(const String& marginMode, const String& symbol = "", const json& params = json::object());

protected:
    String sign(const String& path, const String& api = "public",
               const String& method = "GET", const json& params = json::object(),
               const std::map<String, String>& headers = {}, const json& body = nullptr) override;

private:
    void initializeApiEndpoints();
    String getTimestamp();
    String createSignature(const String& timestamp, const String& method, const String& requestPath, const String& body = "");
    std::map<String, String> getAuthHeaders(const String& method, const String& requestPath, const String& body = "");
};

} // namespace ccxt
