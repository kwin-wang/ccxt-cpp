#pragma once

#include "ccxt/base/exchange.h"

namespace ccxt {

class Lbank : public Exchange {
public:
    Lbank();
    ~Lbank() override = default;

    // Market Data API
    json fetchMarkets(const json& params = json::object()) override;
    json fetchTicker(const std::string& symbol, const json& params = json::object()) override;
    json fetchTickers(const std::vector<std::string>& symbols = {}, const json& params = json::object()) override;
    json fetchOrderBook(const std::string& symbol, int limit = 0, const json& params = json::object()) override;
    json fetchTrades(const std::string& symbol, int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchOHLCV(const std::string& symbol, const std::string& timeframe = "1m",
                    int since = 0, int limit = 0, const json& params = json::object()) override;

    // Async Market Data API
    AsyncPullType asyncFetchMarkets(const json& params = json::object());
    AsyncPullType asyncFetchTicker(const std::string& symbol, const json& params = json::object());
    AsyncPullType asyncFetchTickers(const std::vector<std::string>& symbols = {}, const json& params = json::object());
    AsyncPullType asyncFetchOrderBook(const std::string& symbol, int limit = 0, const json& params = json::object());
    AsyncPullType asyncFetchTrades(const std::string& symbol, int since = 0, int limit = 0, const json& params = json::object());
    AsyncPullType asyncFetchOHLCV(const std::string& symbol, const std::string& timeframe = "1m",
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
    AsyncPullType asyncFetchBalance(const json& params = json::object());
    AsyncPullType asyncCreateOrder(const std::string& symbol, const std::string& type, const std::string& side,
                                     double amount, double price = 0, const json& params = json::object());
    AsyncPullType asyncCancelOrder(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    AsyncPullType asyncFetchOrder(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    AsyncPullType asyncFetchOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    AsyncPullType asyncFetchOpenOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    AsyncPullType asyncFetchClosedOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());

    // Account API
    json fetchMyTrades(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchDeposits(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchWithdrawals(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchDepositAddress(const std::string& code, const json& params = json::object());
    json withdraw(const std::string& code, double amount, const std::string& address, const std::string& tag = "", const json& params = json::object());

    // Async Account API
    AsyncPullType asyncFetchMyTrades(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    AsyncPullType asyncFetchDeposits(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    AsyncPullType asyncFetchWithdrawals(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    AsyncPullType asyncFetchDepositAddress(const std::string& code, const json& params = json::object());
    AsyncPullType asyncWithdraw(const std::string& code, double amount, const std::string& address, const std::string& tag = "", const json& params = json::object());

    // Additional Features
    json fetchCurrencies(const json& params = json::object());
    json fetchTradingFees(const json& params = json::object());
    json fetchFundingFees(const json& params = json::object());
    json fetchTransactionFees(const json& params = json::object());
    json fetchSystemStatus(const json& params = json::object());
    json fetchTime(const json& params = json::object());

    // Async Additional Features
    AsyncPullType asyncFetchCurrencies(const json& params = json::object());
    AsyncPullType asyncFetchTradingFees(const json& params = json::object());
    AsyncPullType asyncFetchFundingFees(const json& params = json::object());
    AsyncPullType asyncFetchTransactionFees(const json& params = json::object());
    AsyncPullType asyncFetchSystemStatus(const json& params = json::object());
    AsyncPullType asyncFetchTime(const json& params = json::object());

protected:
    std::string sign(const std::string& path, const std::string& api = "public",
               const std::string& method = "GET", const json& params = json::object(),
               const std::map<std::string, std::string>& headers = {}, const json& body = nullptr) override;

private:
    void initializeApiEndpoints();
    std::string getLbankSymbol(const std::string& symbol);
    std::string getCommonSymbol(const std::string& lbankSymbol);
    json parseOrder(const json& order, const Market& market = Market());
    json parseTrade(const json& trade, const Market& market = Market());
    json parseOrderStatus(const std::string& status);
    json parseTicker(const json& ticker, const Market& market = Market());
    json parseOHLCV(const json& ohlcv, const Market& market = Market());
    json parseBalance(const json& response);
    json parseFee(const json& fee, const Market& market = Market());
    json parseTransaction(const json& transaction, const std::string& currency = "");
    json parseDepositAddress(const json& depositAddress, const std::string& currency = "");
    std::string createSignature(const std::string& timestamp, const std::string& method,
                         const std::string& path, const std::string& body = "");
    std::string getNonce();

    std::map<std::string, std::string> timeframes;
    std::map<std::string, std::string> options;
    std::map<int, std::string> errorCodes;
    std::map<std::string, std::string> currencyIds;
    bool hasPublicAPI;
    bool hasPrivateAPI;
};

} // namespace ccxt
