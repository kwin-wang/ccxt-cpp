#pragma once

#include "ccxt/base/exchange.h"
#include <future>

namespace ccxt {

class Luno : public Exchange {
public:
    Luno();
    ~Luno() override = default;

    // Market Data API
    json fetchMarkets(const json& params = json()) override;
    json fetchTicker(const String& symbol, const json& params = json()) override;
    json fetchTickers(const std::vector<String>& symbols = {}, const json& params = json()) override;
    json fetchOrderBook(const String& symbol, int limit = 0, const json& params = json()) override;
    json fetchTrades(const String& symbol, int since = 0, int limit = 0, const json& params = json()) override;
    json fetchOHLCV(const String& symbol, const String& timeframe = "1m",
                    int since = 0, int limit = 0, const json& params = json()) override;

    // Trading API
    json fetchBalance(const json& params = json()) override;
    json createOrder(const String& symbol, const String& type, const String& side,
                    double amount, double price = 0, const json& params = json()) override;
    json cancelOrder(const String& id, const String& symbol = "", const json& params = json()) override;
    json fetchOrder(const String& id, const String& symbol = "", const json& params = json()) override;
    json fetchOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json()) override;
    json fetchOpenOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json()) override;
    json fetchClosedOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json()) override;
    json fetchMyTrades(const String& symbol = "", int since = 0, int limit = 0, const json& params = json()) override;

    // Account API
    json fetchAccounts(const json& params = json());
    json fetchLedger(const String& code, int since = 0, int limit = 0, const json& params = json());
    json fetchTradingFee(const String& symbol, const json& params = json());

    // Parse Methods
    json parseTicker(const json& ticker, const Market& market);
    json parseTrade(const json& trade, const Market& market);
    json parseOrder(const json& order, const Market& market);
    json parseLedgerEntry(const json& item, const Currency& currency);
    json parseTradingFee(const json& fee, const Market& market);
    String getAccountId(const String& type, const String& currency);

    // Async Market Data API
    AsyncPullType asyncFetchMarkets(const json& params = json());
    AsyncPullType asyncFetchTicker(const String& symbol, const json& params = json());
    AsyncPullType asyncFetchTickers(const std::vector<String>& symbols = {}, const json& params = json());
    AsyncPullType asyncFetchOrderBook(const String& symbol, int limit = 0, const json& params = json());
    AsyncPullType asyncFetchTrades(const String& symbol, int since = 0, int limit = 0, const json& params = json());
    AsyncPullType asyncFetchOHLCV(const String& symbol, const String& timeframe = "1m",
                                     int since = 0, int limit = 0, const json& params = json());

    // Async Trading API
    AsyncPullType asyncFetchBalance(const json& params = json());
    AsyncPullType asyncCreateOrder(const String& symbol, const String& type, const String& side,
                                     double amount, double price = 0, const json& params = json());
    AsyncPullType asyncCancelOrder(const String& id, const String& symbol = "", const json& params = json());
    AsyncPullType asyncFetchOrder(const String& id, const String& symbol = "", const json& params = json());
    AsyncPullType asyncFetchOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json());
    AsyncPullType asyncFetchOpenOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json());
    AsyncPullType asyncFetchClosedOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json());
    AsyncPullType asyncFetchMyTrades(const String& symbol = "", int since = 0, int limit = 0, const json& params = json());

    // Async Account API
    AsyncPullType asyncFetchAccounts(const json& params = json());
    AsyncPullType asyncFetchLedger(const String& code, int since = 0, int limit = 0, const json& params = json());
    AsyncPullType asyncFetchTradingFee(const String& symbol, const json& params = json());

protected:
    String sign(const String& path, const String& api = "public",
               const String& method = "GET", const json& params = json(),
               const std::map<String, String>& headers = {}, const json& body = nullptr) override;

private:
    String getCurrencyId(const String& code);
};

} // namespace ccxt
