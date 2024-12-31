#pragma once

#include "ccxt/base/exchange.h"

namespace ccxt {

class Gemini : public Exchange {
public:
    Gemini();
    ~Gemini() override = default;

    // Market Data API
    json fetchMarkets(const json& params = json::object()) override;
    json fetchTicker(const std::string& symbol, const json& params = json::object()) override;
    json fetchTickers(const std::vector<std::string>& symbols = {}, const json& params = json::object()) override;
    json fetchOrderBook(const std::string& symbol, int limit = 0, const json& params = json::object()) override;
    json fetchTrades(const std::string& symbol, int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchOHLCV(const std::string& symbol, const std::string& timeframe = "1m",
                    int since = 0, int limit = 0, const json& params = json::object()) override;

    // Async Market Data API
    AsyncPullType fetchMarketsAsync(const json& params = json::object());
    AsyncPullType fetchTickerAsync(const std::string& symbol, const json& params = json::object());
    AsyncPullType fetchTickersAsync(const std::vector<std::string>& symbols = {}, const json& params = json::object());
    AsyncPullType fetchOrderBookAsync(const std::string& symbol, int limit = 0, const json& params = json::object());
    AsyncPullType fetchTradesAsync(const std::string& symbol, int since = 0, int limit = 0, const json& params = json::object());
    AsyncPullType fetchOHLCVAsync(const std::string& symbol, const std::string& timeframe = "1m",
                    int since = 0, int limit = 0, const json& params = json::object());

    // Trading API
    json fetchBalance(const json& params = json::object()) override;
    json createOrder(const std::string& symbol, const std::string& type, const std::string& side,
                    double amount, double price = 0, const json& params = json::object()) override;
    json cancelOrder(const std::string& id, const std::string& symbol = "", const json& params = json::object()) override;
    json fetchOrder(const std::string& id, const std::string& symbol = "", const json& params = json::object()) override;
    json fetchOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchOpenOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchClosedOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;

    // Async Trading API
    AsyncPullType fetchBalanceAsync(const json& params = json::object());
    AsyncPullType createOrderAsync(const std::string& symbol, const std::string& type, const std::string& side,
                    double amount, double price = 0, const json& params = json::object());
    AsyncPullType cancelOrderAsync(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    AsyncPullType fetchOrderAsync(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    AsyncPullType fetchOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    AsyncPullType fetchOpenOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    AsyncPullType fetchClosedOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());

    // Gemini specific methods
    json fetchMyTrades(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchDeposits(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchWithdrawals(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchTransfers(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json transfer(const std::string& code, double amount, const std::string& fromAccount,
                 const std::string& toAccount, const json& params = json::object());
    json fetchDepositAddress(const std::string& code, const json& params = json::object());
    json createDepositAddress(const std::string& code, const json& params = json::object());
    json withdraw(const std::string& code, double amount, const std::string& address,
                 const std::string& tag = "", const json& params = json::object());
    json fetchLedger(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchPaymentMethods(const json& params = json::object());

    // Async Gemini specific methods
    AsyncPullType fetchMyTradesAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    AsyncPullType fetchDepositsAsync(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    AsyncPullType fetchWithdrawalsAsync(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    AsyncPullType fetchTransfersAsync(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    AsyncPullType transferAsync(const std::string& code, double amount, const std::string& fromAccount,
                 const std::string& toAccount, const json& params = json::object());
    AsyncPullType fetchDepositAddressAsync(const std::string& code, const json& params = json::object());
    AsyncPullType createDepositAddressAsync(const std::string& code, const json& params = json::object());
    AsyncPullType withdrawAsync(const std::string& code, double amount, const std::string& address,
                 const std::string& tag = "", const json& params = json::object());
    AsyncPullType fetchLedgerAsync(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    AsyncPullType fetchPaymentMethodsAsync(const json& params = json::object());

protected:
    std::string sign(const std::string& path, const std::string& api = "public",
               const std::string& method = "GET", const json& params = json::object(),
               const std::map<std::string, std::string>& headers = {}, const json& body = nullptr) override;

private:
    void initializeApiEndpoints();
    std::string getTimestamp();
    std::string createSignature(const std::string& request_path, const std::string& method,
                         const std::string& body, const std::string& nonce);
    std::string getGeminiSymbol(const std::string& symbol);
    std::string getCommonSymbol(const std::string& geminiSymbol);
    json parseOrder(const json& order, const Market& market = Market());
    json parseTrade(const json& trade, const Market& market = Market());
    json parseTransaction(const json& transaction, const std::string& currency = "");
    json parseTransfer(const json& transfer);
    json parseLedgerEntry(const json& item, const std::string& currency = "");
    json parseOrderStatus(const std::string& status);

    std::map<std::string, std::string> timeframes;
    bool sandbox;
    std::string version;
    std::map<std::string, std::string> options;
};

} // namespace ccxt
