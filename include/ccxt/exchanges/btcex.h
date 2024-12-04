#pragma once

#include "../base/exchange.h"

namespace ccxt {

class Btcex : public Exchange {
public:
    Btcex();
    ~Btcex() override = default;

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

    // Futures API
    json fetchFuturesMarkets(const json& params = json::object());
    json fetchFuturesBalance(const json& params = json::object());
    json createFuturesOrder(const String& symbol, const String& type, const String& side,
                         double amount, double price = 0, const json& params = json::object());
    json cancelFuturesOrder(const String& id, const String& symbol = "", const json& params = json::object());
    json fetchFuturesPosition(const String& symbol = "", const json& params = json::object());
    json fetchFuturesPositions(const std::vector<String>& symbols = {}, const json& params = json::object());
    json fetchFuturesFundingRate(const String& symbol, const json& params = json::object());
    json fetchFuturesFundingRateHistory(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json setLeverage(const String& symbol, int leverage, const json& params = json::object());
    json setMarginMode(const String& symbol, const String& marginMode, const json& params = json::object());

    // Options API
    json fetchOptionsMarkets(const json& params = json::object());
    json fetchOptionsBalance(const json& params = json::object());
    json createOptionsOrder(const String& symbol, const String& type, const String& side,
                         double amount, double price = 0, const json& params = json::object());
    json cancelOptionsOrder(const String& id, const String& symbol = "", const json& params = json::object());
    json fetchOptionsPosition(const String& symbol = "", const json& params = json::object());
    json fetchOptionsPositions(const std::vector<String>& symbols = {}, const json& params = json::object());

protected:
    String sign(const String& path, const String& api = "public",
               const String& method = "GET", const json& params = json::object(),
               const std::map<String, String>& headers = {}, const json& body = nullptr) override;

private:
    void initializeApiEndpoints();
    String getBtcexSymbol(const String& symbol);
    String getCommonSymbol(const String& btcexSymbol);
    json parseOrder(const json& order, const Market& market = Market());
    json parseTrade(const json& trade, const Market& market = Market());
    json parseOrderStatus(const String& status);
    json parseTicker(const json& ticker, const Market& market = Market());
    json parseOHLCV(const json& ohlcv, const Market& market = Market());
    json parseBalance(const json& response);
    json parseFee(const json& fee, const Market& market = Market());
    json parsePosition(const json& position, const Market& market = Market());
    json parseFundingRate(const json& fundingRate, const Market& market = Market());
    json parseDepositAddress(const json& depositAddress, const String& currency = "");
    json parseTransaction(const json& transaction, const String& currency = "");
    String createSignature(const String& timestamp, const String& method,
                         const String& path, const String& body = "");

    std::map<String, String> timeframes;
    std::map<String, String> options;
    std::map<int, String> errorCodes;
};

} // namespace ccxt
