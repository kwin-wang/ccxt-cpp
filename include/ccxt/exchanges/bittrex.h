#pragma once

#include "ccxt/base/exchange.h"

namespace ccxt {

class Bittrex : public Exchange {
public:
    Bittrex();
    ~Bittrex() override = default;

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

    // Bittrex specific methods
    json fetchMyTrades(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchDeposits(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchWithdrawals(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchDepositAddress(const std::string& code, const json& params = json::object());
    json createDepositAddress(const std::string& code, const json& params = json::object());
    json withdraw(const std::string& code, double amount, const std::string& address,
                 const std::string& tag = "", const json& params = json::object());
    json fetchCurrencies(const json& params = json::object());
    json fetchTransactionFees(const json& params = json::object());
    json fetchTradingFees(const json& params = json::object());
    json fetchDepositFees(const json& params = json::object());
    json fetchWithdrawalFees(const json& params = json::object());

protected:
    std::string sign(const std::string& path, const std::string& api = "public",
               const std::string& method = "GET", const json& params = json::object(),
               const std::map<std::string, std::string>& headers = {}, const json& body = nullptr) override;

private:
    void initializeApiEndpoints();
    std::string getBittrexSymbol(const std::string& symbol);
    std::string getCommonSymbol(const std::string& bittrexSymbol);
    json parseOrder(const json& order, const Market& market = Market());
    json parseTrade(const json& trade, const Market& market = Market());
    json parseTransaction(const json& transaction);
    json parseOrderStatus(const std::string& status);
    json parseTradingFee(const json& fee, const Market& market = Market());
    json parseTransactionType(const std::string& type);
    json parseOrderSide(const std::string& orderType);
    std::string createSignature(const std::string& timestamp, const std::string& uri,
                         const std::string& method, const std::string& body = "");
    std::string getTimestamp();

    std::map<std::string, std::string> timeframes;
    std::map<std::string, std::string> options;
    bool hasPrivateAPI;
};

} // namespace ccxt
