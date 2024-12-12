#pragma once

#include "ccxt/base/exchange.h"

namespace ccxt {

class OKCoin : public Exchange {
public:
    OKCoin();
    ~OKCoin() override = default;

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

    // Async REST API methods
    json fetchTickerAsync(const String& symbol, const json& params = json::object());
    json fetchBalanceAsync(const json& params = json::object());
    json createOrderAsync(const String& symbol, const String& type,
                         const String& side, double amount,
                         double price = 0, const json& params = json::object());
    json cancelOrderAsync(const String& id, const String& symbol = "",
                         const json& params = json::object());
    json fetchOrderAsync(const String& id, const String& symbol = "",
                        const json& params = json::object());
    json fetchOrdersAsync(const String& symbol = "", int since = 0,
                         int limit = 0, const json& params = json::object());
    json fetchOpenOrdersAsync(const String& symbol = "", int since = 0,
                            int limit = 0, const json& params = json::object());
    json fetchClosedOrdersAsync(const String& symbol = "", int since = 0,
                              int limit = 0, const json& params = json::object());
    json fetchMyTradesAsync(const String& symbol = "", int since = 0,
                           int limit = 0, const json& params = json::object());
    json fetchMarketsAsync(const json& params = json::object());
    json fetchOrderBookAsync(const String& symbol, int limit = 0,
                           const json& params = json::object());
    json fetchOHLCVAsync(const String& symbol, const String& timeframe = "1m",
                        int since = 0, int limit = 0,
                        const json& params = json::object());

    // OKCoin specific methods
    json fetchMyTrades(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchLedger(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchDeposits(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchWithdrawals(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchDepositAddress(const String& code, const json& params = json::object());
    json withdraw(const String& code, double amount, const String& address,
                 const String& tag = "", const json& params = json::object());
    json fetchFundingRate(const String& symbol, const json& params = json::object());
    json fetchFundingRateHistory(const String& symbol = "", int since = 0,
                                int limit = 0, const json& params = json::object());
    json setLeverage(int leverage, const String& symbol = "",
                    const String& type = "", const json& params = json::object());
    json setMarginMode(const String& marginMode, const String& symbol = "",
                      const json& params = json::object());

protected:
    String sign(const String& path, const String& api = "public",
               const String& method = "GET", const json& params = json::object(),
               const std::map<String, String>& headers = {}, const json& body = nullptr) override;

private:
    void initializeApiEndpoints();
    String getOKCoinSymbol(const String& symbol);
    String getCommonSymbol(const String& okcoinSymbol);
    json parseOrder(const json& order, const Market& market = Market());
    json parseTrade(const json& trade, const Market& market = Market());
    json parseTransaction(const json& transaction);
    json parseOrderStatus(const String& status);
    json parseTradingFee(const json& fee, const Market& market = Market());
    json parseTransactionType(const String& type);
    json parseOrderSide(const String& orderType);
    String createSignature(const String& timestamp, const String& method,
                         const String& requestPath, const String& body = "");
    String getTimestamp();

    std::map<String, String> timeframes;
    std::map<String, String> options;
    bool hasPrivateAPI;
    String version;
};

} // namespace ccxt
