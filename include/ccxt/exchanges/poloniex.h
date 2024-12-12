#pragma once

#include "ccxt/base/exchange.h"

namespace ccxt {

class Poloniex : public Exchange {
public:
    Poloniex();
    ~Poloniex() override = default;

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

    // Poloniex specific methods
    json fetchMyTrades(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchDeposits(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchWithdrawals(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchDepositAddress(const String& code, const json& params = json::object());
    json fetchTransactionFees(const json& params = json::object());
    json fetchTradingFees(const json& params = json::object());
    json withdraw(const String& code, double amount, const String& address,
                 const String& tag = "", const json& params = json::object());
    json fetchLeverageTiers(const std::vector<String>& symbols = {}, const json& params = json::object());
    json fetchPositions(const std::vector<String>& symbols = {}, const json& params = json::object());
    json fetchLedger(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());

protected:
    String sign(const String& path, const String& api = "public",
               const String& method = "GET", const json& params = json::object(),
               const std::map<String, String>& headers = {}, const json& body = nullptr) override;

private:
    void initializeApiEndpoints();
    String getPoloniexSymbol(const String& symbol);
    String getCommonSymbol(const String& poloniexSymbol);
    json parseOrder(const json& order, const Market& market = Market());
    json parseTrade(const json& trade, const Market& market = Market());
    json parseTransaction(const json& transaction);
    json parseOrderStatus(const String& status);
    json parseTradingFee(const json& fee, const Market& market = Market());
    json parseTransactionType(const String& type);
    json parseOrderSide(const String& orderType);
    String getNonce();
    String createSignature(const String& path, const String& nonce, const json& body);

    std::map<String, String> timeframes;
    std::map<String, String> options;
    bool hasPrivateAPI;
    int64_t lastNonce;
};

} // namespace ccxt
