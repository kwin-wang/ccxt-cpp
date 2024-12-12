#pragma once

#include "ccxt/base/exchange.h"

namespace ccxt {

class Gemini : public Exchange {
public:
    Gemini();
    ~Gemini() override = default;

    // Market Data API
    json fetchMarkets(const json& params = json::object()) override;
    json fetchTicker(const String& symbol, const json& params = json::object()) override;
    json fetchTickers(const std::vector<String>& symbols = {}, const json& params = json::object()) override;
    json fetchOrderBook(const String& symbol, int limit = 0, const json& params = json::object()) override;
    json fetchTrades(const String& symbol, int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchOHLCV(const String& symbol, const String& timeframe = "1m",
                    int since = 0, int limit = 0, const json& params = json::object()) override;

    // Async Market Data API
    std::future<json> fetchMarketsAsync(const json& params = json::object());
    std::future<json> fetchTickerAsync(const String& symbol, const json& params = json::object());
    std::future<json> fetchTickersAsync(const std::vector<String>& symbols = {}, const json& params = json::object());
    std::future<json> fetchOrderBookAsync(const String& symbol, int limit = 0, const json& params = json::object());
    std::future<json> fetchTradesAsync(const String& symbol, int since = 0, int limit = 0, const json& params = json::object());
    std::future<json> fetchOHLCVAsync(const String& symbol, const String& timeframe = "1m",
                    int since = 0, int limit = 0, const json& params = json::object());

    // Trading API
    json fetchBalance(const json& params = json::object()) override;
    json createOrder(const String& symbol, const String& type, const String& side,
                    double amount, double price = 0, const json& params = json::object()) override;
    json cancelOrder(const String& id, const String& symbol = "", const json& params = json::object()) override;
    json fetchOrder(const String& id, const String& symbol = "", const json& params = json::object()) override;
    json fetchOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchOpenOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchClosedOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;

    // Async Trading API
    std::future<json> fetchBalanceAsync(const json& params = json::object());
    std::future<json> createOrderAsync(const String& symbol, const String& type, const String& side,
                    double amount, double price = 0, const json& params = json::object());
    std::future<json> cancelOrderAsync(const String& id, const String& symbol = "", const json& params = json::object());
    std::future<json> fetchOrderAsync(const String& id, const String& symbol = "", const json& params = json::object());
    std::future<json> fetchOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    std::future<json> fetchOpenOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    std::future<json> fetchClosedOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());

    // Gemini specific methods
    json fetchMyTrades(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchDeposits(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchWithdrawals(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchTransfers(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json transfer(const String& code, double amount, const String& fromAccount,
                 const String& toAccount, const json& params = json::object());
    json fetchDepositAddress(const String& code, const json& params = json::object());
    json createDepositAddress(const String& code, const json& params = json::object());
    json withdraw(const String& code, double amount, const String& address,
                 const String& tag = "", const json& params = json::object());
    json fetchLedger(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchPaymentMethods(const json& params = json::object());

    // Async Gemini specific methods
    std::future<json> fetchMyTradesAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    std::future<json> fetchDepositsAsync(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    std::future<json> fetchWithdrawalsAsync(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    std::future<json> fetchTransfersAsync(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    std::future<json> transferAsync(const String& code, double amount, const String& fromAccount,
                 const String& toAccount, const json& params = json::object());
    std::future<json> fetchDepositAddressAsync(const String& code, const json& params = json::object());
    std::future<json> createDepositAddressAsync(const String& code, const json& params = json::object());
    std::future<json> withdrawAsync(const String& code, double amount, const String& address,
                 const String& tag = "", const json& params = json::object());
    std::future<json> fetchLedgerAsync(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    std::future<json> fetchPaymentMethodsAsync(const json& params = json::object());

protected:
    String sign(const String& path, const String& api = "public",
               const String& method = "GET", const json& params = json::object(),
               const std::map<String, String>& headers = {}, const json& body = nullptr) override;

private:
    void initializeApiEndpoints();
    String getTimestamp();
    String createSignature(const String& request_path, const String& method,
                         const String& body, const String& nonce);
    String getGeminiSymbol(const String& symbol);
    String getCommonSymbol(const String& geminiSymbol);
    json parseOrder(const json& order, const Market& market = Market());
    json parseTrade(const json& trade, const Market& market = Market());
    json parseTransaction(const json& transaction, const String& currency = "");
    json parseTransfer(const json& transfer);
    json parseLedgerEntry(const json& item, const String& currency = "");
    json parseOrderStatus(const String& status);

    std::map<String, String> timeframes;
    bool sandbox;
    String version;
    std::map<String, String> options;
};

} // namespace ccxt
