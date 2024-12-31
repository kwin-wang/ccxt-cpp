#pragma once

#include "ccxt/base/exchange.h"

namespace ccxt {

class Zaif : public Exchange {
public:
    Zaif();
    ~Zaif() override = default;

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

    // Account API
    json fetchMyTrades(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchDeposits(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchWithdrawals(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchDepositAddress(const std::string& code, const json& params = json::object());
    json withdraw(const std::string& code, double amount, const std::string& address, const std::string& tag = "", const json& params = json::object());

    // Margin Trading API
    json fetchMarginMarkets(const json& params = json::object());
    json fetchMarginBalance(const json& params = json::object());
    json createMarginOrder(const std::string& symbol, const std::string& type, const std::string& side,
                         double amount, double price = 0, const json& params = json::object());
    json cancelMarginOrder(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    json fetchMarginOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchMarginOpenOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchMarginPositions(const json& params = json::object());
    json borrowMargin(const std::string& code, double amount, const std::string& symbol = "", const json& params = json::object());
    json repayMargin(const std::string& code, double amount, const std::string& symbol = "", const json& params = json::object());

    // Futures Trading API
    json fetchFuturesMarkets(const json& params = json::object());
    json fetchFuturesBalance(const json& params = json::object());
    json createFuturesOrder(const std::string& symbol, const std::string& type, const std::string& side,
                          double amount, double price = 0, const json& params = json::object());
    json cancelFuturesOrder(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    json fetchFuturesOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchFuturesOpenOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchFuturesPositions(const json& params = json::object());
    json fetchFundingRate(const std::string& symbol, const json& params = json::object());
    json fetchFundingRateHistory(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());

protected:
    std::string sign(const std::string& path, const std::string& api = "public",
               const std::string& method = "GET", const json& params = json::object(),
               const std::map<std::string, std::string>& headers = {}, const json& body = nullptr) override;

private:
    void initializeApiEndpoints();
    std::string getZaifSymbol(const std::string& symbol);
    std::string getCommonSymbol(const std::string& zaifSymbol);
    json parseOrder(const json& order, const Market& market = Market());
    json parseTrade(const json& trade, const Market& market = Market());
    json parseOrderStatus(const std::string& status);
    json parseTicker(const json& ticker, const Market& market = Market());
    json parseOHLCV(const json& ohlcv, const Market& market = Market());
    json parseBalance(const json& response);
    json parseFee(const json& fee, const Market& market = Market());
    json parsePosition(const json& position, const Market& market = Market());
    json parseDepositAddress(const json& depositAddress, const std::string& currency = "");
    json parseTransaction(const json& transaction, const std::string& currency = "");
    std::string createSignature(const std::string& nonce, const std::string& method,
                         const std::string& path, const std::string& body = "");
    std::string createNonce();

    std::map<std::string, std::string> timeframes;
    std::map<std::string, std::string> options;
    std::map<int, std::string> errorCodes;
};

} // namespace ccxt
