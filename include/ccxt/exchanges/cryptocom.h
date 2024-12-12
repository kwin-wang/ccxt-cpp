#ifndef CCXT_EXCHANGE_CRYPTOCOM_H
#define CCXT_EXCHANGE_CRYPTOCOM_H

#include "ccxt/base/exchange.h"
#include "../base/types.h"

namespace ccxt {

class CryptoCom : public Exchange {
public:
    explicit CryptoCom(const ExchangeConfig& config = {});
    ~CryptoCom() override = default;

    std::string id() const override { return "cryptocom"; }
    std::string name() const override { return "Crypto.com"; }
    std::vector<std::string> countries() const override { return {"MT"}; }
    std::string version() const override { return "v2"; }
    int rateLimit() const override { return 10; }  // 100 requests per second
    bool certified() const override { return true; }
    bool pro() const override { return true; }

    // Market Data
    OrderBook fetchOrderBook(const std::string& symbol, int limit = 0, const Params& params = {}) override;
    std::vector<Trade> fetchTrades(const std::string& symbol, int since = 0, int limit = 0, const Params& params = {}) override;
    Ticker fetchTicker(const std::string& symbol, const Params& params = {}) override;
    std::map<std::string, Ticker> fetchTickers(const std::vector<std::string>& symbols = {}, const Params& params = {}) override;
    
    // Trading
    Order createOrder(const std::string& symbol, const std::string& type, const std::string& side,
                     double amount, double price = 0, const Params& params = {}) override;
    Order cancelOrder(const std::string& id, const std::string& symbol = "", const Params& params = {}) override;
    bool cancelAllOrders(const std::vector<std::string>& symbols = {}, const Params& params = {});
    std::vector<Order> fetchOrders(const std::string& symbol = "", int since = 0, int limit = 0, const Params& params = {}) override;
    std::vector<Order> fetchOpenOrders(const std::string& symbol = "", int since = 0, int limit = 0, const Params& params = {}) override;
    std::vector<Order> fetchClosedOrders(const std::string& symbol = "", int since = 0, int limit = 0, const Params& params = {}) override;
    Order fetchOrder(const std::string& id, const std::string& symbol = "", const Params& params = {}) override;

    // Account
    Balance fetchBalance(const Params& params = {}) override;
    std::vector<Account> fetchAccounts(const Params& params = {});
    TradingFees fetchTradingFees(const Params& params = {});
    std::vector<Transaction> fetchDeposits(const std::string& code = "", int since = 0, int limit = 0, const Params& params = {}) override;
    std::vector<Transaction> fetchWithdrawals(const std::string& code = "", int since = 0, int limit = 0, const Params& params = {}) override;
    DepositAddress fetchDepositAddress(const std::string& code, const Params& params = {});
    std::vector<LedgerEntry> fetchLedger(const std::string& code = "", int since = 0, int limit = 0, const Params& params = {});

protected:
    json signRequest(const std::string& path, const std::string& api = "public",
                    const std::string& method = "GET", const Params& params = {},
                    const json& headers = {}, const std::string& body = "") override;

private:
    std::string getSignature(const std::string& requestPath, const std::string& method,
                            const std::string& paramsStr, const std::string& timestamp) const;
    void handleErrors(const json& response);
    std::string getRequestPath(const std::string& api, const std::string& path) const;
    
    // Internal helper methods
    std::string getInstrumentType(const std::string& symbol) const;
    void validateSymbol(const std::string& symbol) const;
    void checkRequiredCredentials() const;
    std::string parseOrderStatus(const std::string& status) const;
    std::string getTimeInForce(const Params& params) const;
};

} // namespace ccxt

#endif // CCXT_EXCHANGE_CRYPTOCOM_H
