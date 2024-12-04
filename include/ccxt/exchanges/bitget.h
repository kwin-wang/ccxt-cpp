#pragma once

#include "../base/exchange.h"

namespace ccxt {

class Bitget : public Exchange {
public:
    Bitget();
    ~Bitget() override = default;

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

    // Bitget specific methods
    json fetchPositions(const String& symbols = "", const json& params = json::object());
    json fetchPositionRisk(const String& symbols = "", const json& params = json::object());
    json setLeverage(int leverage, const String& symbol, const json& params = json::object());
    json setMarginMode(const String& marginMode, const String& symbol = "", const json& params = json::object());
    json fetchFundingRate(const String& symbol, const json& params = json::object());
    json fetchFundingRates(const std::vector<String>& symbols = {}, const json& params = json::object());
    json fetchFundingHistory(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchMyTrades(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchDeposits(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchWithdrawals(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchDepositAddress(const String& code, const json& params = json::object());
    json transfer(const String& code, double amount, const String& fromAccount,
                 const String& toAccount, const json& params = json::object());
    json fetchTransfers(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());

protected:
    String sign(const String& path, const String& api = "public",
               const String& method = "GET", const json& params = json::object(),
               const std::map<String, String>& headers = {}, const json& body = nullptr) override;

private:
    void initializeApiEndpoints();
    String getTimestamp();
    String createSignature(const String& timestamp, const String& method,
                         const String& requestPath, const String& body = "");
    String getBitgetSymbol(const String& symbol, const String& type = "spot");
    String getCommonSymbol(const String& bitgetSymbol);
    json parseOrder(const json& order, const Market& market = Market());
    json parseTrade(const json& trade, const Market& market = Market());
    json parsePosition(const json& position, const Market& market = Market());
    json parseTransfer(const json& transfer);
    json parseOrderStatus(const String& status);
    json parseLedgerEntryType(const String& type);
    String getDefaultType();

    std::map<String, String> timeframes;
    std::map<String, String> options;
    bool unifiedMargin;
};

} // namespace ccxt
