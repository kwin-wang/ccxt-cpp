#pragma once

#include "ccxt/base/exchange.h"

namespace ccxt {

class FTX : public Exchange {
public:
    FTX();
    ~FTX() override = default;

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

    // FTX specific methods
    json fetchPositions(const std::string& symbols = "", const json& params = json::object());
    json fetchLeverage(const std::string& symbol, const json& params = json::object());
    json setLeverage(const std::string& leverage, const std::string& symbol = "", const json& params = json::object());
    json setMarginMode(const std::string& marginMode, const std::string& symbol = "", const json& params = json::object());
    json fetchFundingRate(const std::string& symbol, const json& params = json::object());
    json fetchFundingRates(const std::vector<std::string>& symbols = {}, const json& params = json::object());
    json fetchFundingHistory(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchIndexOHLCV(const std::string& symbol, const std::string& timeframe = "1m",
                        int since = 0, int limit = 0, const json& params = json::object());
    json fetchMarkOHLCV(const std::string& symbol, const std::string& timeframe = "1m",
                       int since = 0, int limit = 0, const json& params = json::object());
    json fetchMyTrades(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchDeposits(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchWithdrawals(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchDepositAddress(const std::string& code, const json& params = json::object());
    json transfer(const std::string& code, double amount, const std::string& fromAccount,
                 const std::string& toAccount, const json& params = json::object());
    json fetchTransfers(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());

protected:
    std::string sign(const std::string& path, const std::string& api = "public",
               const std::string& method = "GET", const json& params = json::object(),
               const std::map<std::string, std::string>& headers = {}, const json& body = nullptr) override;

private:
    void initializeApiEndpoints();
    std::string getTimestamp();
    std::string createSignature(const std::string& timestamp, const std::string& method,
                         const std::string& path, const std::string& body = "");
    std::string getFTXSymbol(const std::string& symbol);
    std::string getCommonSymbol(const std::string& ftxSymbol);
    json parseOrder(const json& order, const Market& market = Market());
    json parseTrade(const json& trade, const Market& market = Market());
    json parsePosition(const json& position, const Market& market = Market());
    json parseTransfer(const json& transfer);
    json parseOrderStatus(const std::string& status);
    json parseLedgerEntryType(const std::string& type);
    std::string getSubAccountName(const json& params = json::object());

    std::map<std::string, std::string> timeframes;
    bool testnet;
    std::string subAccountName;
    std::map<std::string, std::string> options;
};

} // namespace ccxt
