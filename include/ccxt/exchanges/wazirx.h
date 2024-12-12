#ifndef CCXT_WAZIRX_H
#define CCXT_WAZIRX_H

#include "ccxt/base/exchange.h"

namespace ccxt {

class Wazirx : public Exchange {
public:
    Wazirx();
    ~Wazirx() = default;

    // Market Data Methods
    json fetchMarkets(const json& params = {}) override;
    json fetchTicker(const String& symbol, const json& params = {}) override;
    json fetchTickers(const std::vector<String>& symbols = {}, const json& params = {}) override;
    json fetchOrderBook(const String& symbol, int limit = 0, const json& params = {}) override;
    json fetchOHLCV(const String& symbol, const String& timeframe = "1m",
                    int since = 0, int limit = 0, const json& params = {}) override;
    json fetchTrades(const String& symbol, int since = 0, int limit = 0,
                    const json& params = {}) override;

    // Trading Methods
    json createOrder(const String& symbol, const String& type, const String& side,
                    double amount, double price = 0, const json& params = {}) override;
    json cancelOrder(const String& id, const String& symbol = "",
                    const json& params = {}) override;
    json fetchOrder(const String& id, const String& symbol = "",
                   const json& params = {}) override;
    json fetchOrders(const String& symbol = "", int since = 0, int limit = 0,
                    const json& params = {}) override;
    json fetchOpenOrders(const String& symbol = "", int since = 0, int limit = 0,
                        const json& params = {}) override;
    json fetchClosedOrders(const String& symbol = "", int since = 0, int limit = 0,
                          const json& params = {}) override;
    json fetchMyTrades(const String& symbol = "", int since = 0, int limit = 0,
                      const json& params = {}) override;

    // Account Methods
    json fetchBalance(const json& params = {}) override;
    json fetchDeposits(const String& code = "", int since = 0, int limit = 0,
                      const json& params = {}) override;
    json fetchWithdrawals(const String& code = "", int since = 0, int limit = 0,
                         const json& params = {}) override;

protected:
    // Helper Methods
    void initializeApiEndpoints();
    json parseTicker(const json& ticker, const Market& market);
    json parseOrder(const json& order, const Market& market = Market());
    json parseTrade(const json& trade, const Market& market = Market());
    json parseOHLCV(const json& ohlcv, const Market& market);
    json parseBalance(const json& response);
    String sign(const String& path, const String& api = "public",
               const String& method = "GET", const json& params = {},
               const json& headers = nullptr, const String& body = "") override;
};

} // namespace ccxt

#endif // CCXT_WAZIRX_H
