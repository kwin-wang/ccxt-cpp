#ifndef CCXT_MIXCOIN_H
#define CCXT_MIXCOIN_H

#include "../exchange.h"
#include <future>

namespace ccxt {

class MixCoin : public Exchange {
public:
    MixCoin(const Config& config = Config());
    ~MixCoin() override = default;

    // Market Data API
    json fetchMarkets(const json& params = json()) override;
    json fetchTicker(const std::string& symbol, const json& params = json()) override;
    json fetchTickers(const std::vector<std::string>& symbols = {}, const json& params = json()) override;
    json fetchOrderBook(const std::string& symbol, int limit = 0, const json& params = json()) override;
    json fetchTrades(const std::string& symbol, int since = 0, int limit = 0, const json& params = json()) override;
    json fetchOHLCV(const std::string& symbol, const std::string& timeframe = "1m",
                    int since = 0, int limit = 0, const json& params = json()) override;

    // Trading API
    json fetchBalance(const json& params = json()) override;
    json createOrder(const std::string& symbol, const std::string& type, const std::string& side,
                    double amount, double price = 0, const json& params = json()) override;
    json cancelOrder(const std::string& id, const std::string& symbol = "", const json& params = json()) override;
    json fetchOrder(const std::string& id, const std::string& symbol = "", const json& params = json()) override;
    json fetchOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json()) override;
    json fetchOpenOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json()) override;
    json fetchClosedOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json()) override;
    json fetchMyTrades(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json()) override;

    // Account API
    json fetchAccounts(const json& params = json());
    json fetchLedger(const std::string& code, int since = 0, int limit = 0, const json& params = json());
    json fetchTradingFee(const std::string& symbol, const json& params = json());

    // Async Market Data API
    AsyncPullType asyncFetchMarkets(const json& params = json());
    AsyncPullType asyncFetchTicker(const std::string& symbol, const json& params = json());
    AsyncPullType asyncFetchTickers(const std::vector<std::string>& symbols = {}, const json& params = json());
    AsyncPullType asyncFetchOrderBook(const std::string& symbol, int limit = 0, const json& params = json());
    AsyncPullType asyncFetchTrades(const std::string& symbol, int since = 0, int limit = 0, const json& params = json());
    AsyncPullType asyncFetchOHLCV(const std::string& symbol, const std::string& timeframe = "1m",
                                     int since = 0, int limit = 0, const json& params = json());

    // Async Trading API
    AsyncPullType asyncFetchBalance(const json& params = json());
    AsyncPullType asyncCreateOrder(const std::string& symbol, const std::string& type, const std::string& side,
                                     double amount, double price = 0, const json& params = json());
    AsyncPullType asyncCancelOrder(const std::string& id, const std::string& symbol = "", const json& params = json());
    AsyncPullType asyncFetchOrder(const std::string& id, const std::string& symbol = "", const json& params = json());
    AsyncPullType asyncFetchOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json());
    AsyncPullType asyncFetchOpenOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json());
    AsyncPullType asyncFetchClosedOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json());
    AsyncPullType asyncFetchMyTrades(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json());

    // Async Account API
    AsyncPullType asyncFetchAccounts(const json& params = json());
    AsyncPullType asyncFetchLedger(const std::string& code, int since = 0, int limit = 0, const json& params = json());
    AsyncPullType asyncFetchTradingFee(const std::string& symbol, const json& params = json());

protected:
    std::string sign(const std::string& path, const std::string& api = "public",
               const std::string& method = "GET", const json& params = json(),
               const std::map<std::string, std::string>& headers = {}, const json& body = nullptr) override;

private:
    // Parse Methods
    json parseTicker(const json& ticker, const Market& market);
    json parseTrade(const json& trade, const Market& market);
    json parseOrder(const json& order, const Market& market);
    json parseLedgerEntry(const json& item, const Currency& currency);
    json parseTradingFee(const json& fee, const Market& market);
    std::string getAccountId(const std::string& type, const std::string& currency);
};

} // namespace ccxt

#endif // CCXT_MIXCOIN_H
