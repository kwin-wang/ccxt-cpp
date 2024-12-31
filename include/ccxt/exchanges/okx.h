#pragma once

#include "ccxt/base/exchange.h"

namespace ccxt {

class OKX : public Exchange {
public:
    OKX();
    ~OKX() override = default;

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

    // Additional REST API methods
    json fetchMyTrades(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchLedger(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchDeposits(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchWithdrawals(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchDepositAddress(const std::string& code, const json& params = json::object());
    json fetchFundingRate(const std::string& symbol, const json& params = json::object());
    json fetchFundingRateHistory(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json setLeverage(int leverage, const std::string& symbol = "", const std::string& type = "", const json& params = json::object());
    json setMarginMode(const std::string& marginMode, const std::string& symbol = "", const json& params = json::object());

    // Async REST API methods
    json fetchMarketsAsync(const json& params = json::object());
    json fetchTickerAsync(const std::string& symbol, const json& params = json::object());
    json fetchTickersAsync(const std::vector<std::string>& symbols = {}, const json& params = json::object());
    json fetchOrderBookAsync(const std::string& symbol, int limit = 0, const json& params = json::object());
    json fetchTradesAsync(const std::string& symbol, int since = 0, int limit = 0, const json& params = json::object());
    json fetchOHLCVAsync(const std::string& symbol, const std::string& timeframe = "1m",
                        int since = 0, int limit = 0, const json& params = json::object());
    json fetchBalanceAsync(const json& params = json::object());
    json createOrderAsync(const std::string& symbol, const std::string& type, const std::string& side,
                         double amount, double price = 0, const json& params = json::object());
    json cancelOrderAsync(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    json fetchOrderAsync(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    json fetchOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchOpenOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchClosedOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchMyTradesAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchLedgerAsync(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchDepositsAsync(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchWithdrawalsAsync(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchDepositAddressAsync(const std::string& code, const json& params = json::object());
    json fetchFundingRateAsync(const std::string& symbol, const json& params = json::object());
    json fetchFundingRateHistoryAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json setLeverageAsync(int leverage, const std::string& symbol = "", const std::string& type = "", const json& params = json::object());
    json setMarginModeAsync(const std::string& marginMode, const std::string& symbol = "", const json& params = json::object());

protected:
    std::string sign(const std::string& path, const std::string& api = "public",
               const std::string& method = "GET", const json& params = json::object(),
               const std::map<std::string, std::string>& headers = {}, const json& body = nullptr) override;

private:
    void initializeApiEndpoints();
    std::string getTimestamp();
    std::string createSignature(const std::string& timestamp, const std::string& method, const std::string& requestPath, const std::string& body = "");
    std::map<std::string, std::string> getAuthHeaders(const std::string& method, const std::string& requestPath, const std::string& body = "");
};

} // namespace ccxt
