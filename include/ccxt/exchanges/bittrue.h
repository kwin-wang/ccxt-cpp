#pragma once

#include "ccxt/base/exchange.h"

namespace ccxt {

class BitTrue : public Exchange {
public:
    BitTrue();
    ~BitTrue() override = default;

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
    json fetchMyTrades(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;

    // Account API
    json fetchDeposits(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchWithdrawals(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchDepositAddress(const std::string& code, const json& params = json::object()) override;
    json withdraw(const std::string& code, double amount, const std::string& address, const std::string& tag = "", const json& params = json::object()) override;

protected:
    void sign(Request& request, const std::string& path, const std::string& api = "public",
             const std::string& method = "GET", const json& params = json::object(),
             const json& headers = json::object(), const json& body = json::object()) override;

private:
    // Helper methods for request signing
    std::string getSignature(const std::string& timestamp, const std::string& method,
                           const std::string& path, const std::string& body = "");
    std::string getNonce();

    // Parsing methods
    json parseOrder(const json& order, const Market& market = Market());
    json parseTrade(const json& trade, const Market& market = Market());
    json parseTicker(const json& ticker, const Market& market = Market());
    json parseOHLCV(const json& ohlcv, const Market& market = Market(), const std::string& timeframe = "1m");
    json parseBalance(const json& response);
    json parseFee(const json& fee, const Market& market = Market());
    json parseTransaction(const json& transaction, const std::string& currency = "");
    json parseDepositAddress(const json& depositAddress, const std::string& currency = "");
    std::string parseOrderStatus(const std::string& status);
    std::string parseOrderType(const std::string& type);
    std::string parseOrderSide(const std::string& side);
    std::string parseTimeInForce(const std::string& timeInForce);

    // Helper methods
    std::string getBitTrueSymbol(const std::string& symbol);
    std::string getCommonSymbol(const std::string& bitTrueSymbol);
};

} // namespace ccxt
