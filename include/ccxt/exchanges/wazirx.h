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
    json fetchTicker(const std::string& symbol, const json& params = {}) override;
    json fetchTickers(const std::vector<std::string>& symbols = {}, const json& params = {}) override;
    json fetchOrderBook(const std::string& symbol, int limit = 0, const json& params = {}) override;
    json fetchOHLCV(const std::string& symbol, const std::string& timeframe = "1m",
                    int since = 0, int limit = 0, const json& params = {}) override;
    json fetchTrades(const std::string& symbol, int since = 0, int limit = 0,
                    const json& params = {}) override;

    // Trading Methods
    json createOrder(const std::string& symbol, const std::string& type, const std::string& side,
                    double amount, double price = 0, const json& params = {}) override;
    json cancelOrder(const std::string& id, const std::string& symbol = "",
                    const json& params = {}) override;
    json fetchOrder(const std::string& id, const std::string& symbol = "",
                   const json& params = {}) override;
    json fetchOrders(const std::string& symbol = "", int since = 0, int limit = 0,
                    const json& params = {}) override;
    json fetchOpenOrders(const std::string& symbol = "", int since = 0, int limit = 0,
                        const json& params = {}) override;
    json fetchClosedOrders(const std::string& symbol = "", int since = 0, int limit = 0,
                          const json& params = {}) override;
    json fetchMyTrades(const std::string& symbol = "", int since = 0, int limit = 0,
                      const json& params = {}) override;

    // Account Methods
    json fetchBalance(const json& params = {}) override;
    json fetchDeposits(const std::string& code = "", int since = 0, int limit = 0,
                      const json& params = {}) override;
    json fetchWithdrawals(const std::string& code = "", int since = 0, int limit = 0,
                         const json& params = {}) override;

protected:
    // Helper Methods
    void initializeApiEndpoints();
    json parseTicker(const json& ticker, const Market& market);
    json parseOrder(const json& order, const Market& market = Market());
    json parseTrade(const json& trade, const Market& market = Market());
    json parseOHLCV(const json& ohlcv, const Market& market);
    json parseBalance(const json& response);
    std::string sign(const std::string& path, const std::string& api = "public",
               const std::string& method = "GET", const json& params = {},
               const json& headers = nullptr, const std::string& body = "") override;
};

} // namespace ccxt

#endif // CCXT_WAZIRX_H
