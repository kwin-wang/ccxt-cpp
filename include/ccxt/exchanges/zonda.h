#ifndef CCXT_EXCHANGE_ZONDA_H
#define CCXT_EXCHANGE_ZONDA_H

#include "ccxt/base/exchange.h"
#include <string>
#include <map>

namespace ccxt {

class Zonda : public Exchange {
public:
    explicit Zonda(const Config& config = Config());
    ~Zonda() override = default;

    // Market Data API
    json fetchMarkets(const json& params = json::object()) override;
    json fetchTicker(const String& symbol, const json& params = json::object()) override;
    json fetchTickers(const std::vector<String>& symbols = {}, const json& params = json::object()) override;
    json fetchOrderBook(const String& symbol, int limit = 0, const json& params = json::object()) override;
    json fetchTrades(const String& symbol, int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchOHLCV(const String& symbol, const String& timeframe = "1m",
                   int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchTradingFees(const json& params = json::object()) override;

    // Trading API
    json fetchBalance(const json& params = json::object()) override;
    json createOrder(const String& symbol, const String& type, const String& side,
                    double amount, double price = 0, const json& params = json::object()) override;
    json cancelOrder(const String& id, const String& symbol = "", const json& params = json::object()) override;
    json fetchOrder(const String& id, const String& symbol = "", const json& params = json::object()) override;
    json fetchOpenOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchMyTrades(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;

    // Account API
    json fetchDepositAddress(const String& code, const json& params = json::object()) override;
    json fetchDepositAddresses(const json& codes, const json& params = json::object()) override;
    json fetchLedger(const String& code = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json transfer(const String& code, double amount, const String& fromAccount, const String& toAccount, const json& params = json::object()) override;
    json withdraw(const String& code, double amount, const String& address, const String& tag = "", const json& params = json::object()) override;

protected:
    void initializeApiEndpoints();
    String sign(const String& path, const String& api = "public",
               const String& method = "GET", const json& params = json::object(),
               const std::map<String, String>& headers = {}, const json& body = nullptr) override;

private:
    String getZondaSymbol(const String& symbol);
    String getCommonSymbol(const String& zondaSymbol);
    json parseTicker(const json& ticker, const Market& market = Market());
    json parseTrade(const json& trade, const Market& market = Market());
    json parseOrder(const json& order, const Market& market = Market());
    json parseOrderStatus(const String& status);
    json parseOHLCV(const json& ohlcv, const Market& market = Market());
    json parseBalance(const json& response);
    json parseFee(const json& fee, const Market& market = Market());
    json parseDepositAddress(const json& depositAddress, const String& currency = "");
    json parseTransaction(const json& transaction, const String& currency = "");
    String createSignature(const String& nonce, const String& method,
                         const String& path, const String& body = "");
    String createNonce();

    std::map<String, String> timeframes;
    std::map<String, String> options;
};

} // namespace ccxt

#endif // CCXT_EXCHANGE_ZONDA_H
