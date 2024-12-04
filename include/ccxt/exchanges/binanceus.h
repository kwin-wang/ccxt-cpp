#pragma once

#include "../base/exchange.h"

namespace ccxt {

class Binanceus : public Exchange {
public:
    Binanceus();
    ~Binanceus() override = default;

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

    // Account API
    json fetchMyTrades(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchDeposits(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchWithdrawals(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchDepositAddress(const String& code, const json& params = json::object());
    json withdraw(const String& code, double amount, const String& address, const String& tag = "", const json& params = json::object());

    // Additional Features
    json fetchTime(const json& params = json::object());
    json fetchStatus(const json& params = json::object());
    json fetchCurrencies(const json& params = json::object());
    json fetchTradingFees(const json& params = json::object());
    json fetchFundingFees(const json& params = json::object());
    json fetchTransactionFees(const json& params = json::object());
    json fetchSystemStatus(const json& params = json::object());
    json fetchAccounts(const json& params = json::object());
    json fetchLedger(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());

protected:
    String sign(const String& path, const String& api = "public",
               const String& method = "GET", const json& params = json::object(),
               const std::map<String, String>& headers = {}, const json& body = nullptr) override;

private:
    void initializeApiEndpoints();
    void initializeTimeframes();
    void initializeMarketTypes();
    void initializeOptions();
    void initializeErrorCodes();
    void initializeFees();

    String getBinanceusSymbol(const String& symbol);
    String getCommonSymbol(const String& binanceusSymbol);
    String parseOrderStatus(const String& status);
    String parseOrderType(const String& type);
    String parseOrderSide(const String& side);
    String parseTimeInForce(const String& timeInForce);

    json parseOrder(const json& order, const Market& market = Market());
    json parseTrade(const json& trade, const Market& market = Market());
    json parseTicker(const json& ticker, const Market& market = Market());
    json parseOHLCV(const json& ohlcv, const Market& market = Market(), const String& timeframe = "1m");
    json parseBalance(const json& response);
    json parseFee(const json& fee, const Market& market = Market());
    json parseTransaction(const json& transaction, const String& currency = "");
    json parseDepositAddress(const json& depositAddress, const String& currency = "");

    String createSignature(const String& timestamp, const String& method,
                         const String& path, const String& body = "");
    String getNonce();

    std::map<String, String> timeframes;
    std::map<String, String> marketTypes;
    std::map<String, String> options;
    std::map<int, String> errorCodes;
    std::map<String, json> fees;
    bool hasPublicAPI;
    bool hasPrivateAPI;
    bool hasFiatAPI;
    bool hasMarginAPI;
    bool hasFuturesAPI;
    bool hasOptionsAPI;
};

} // namespace ccxt
