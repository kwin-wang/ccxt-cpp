#ifndef CCXT_HTX_H
#define CCXT_HTX_H

#include "ccxt/base/exchange.h"

namespace ccxt {

class HTX : public Exchange {
public:
    HTX(const Params& params = Params());
    ~HTX() = default;

    // Market Data
    OrderBook fetchOrderBook(const std::string& symbol, int limit = 0, const Params& params = Params()) override;
    std::vector<Trade> fetchTrades(const std::string& symbol, int since = 0, int limit = 0, const Params& params = Params()) override;
    Ticker fetchTicker(const std::string& symbol, const Params& params = Params()) override;
    std::map<std::string, Ticker> fetchTickers(const std::vector<std::string>& symbols = std::vector<std::string>(), const Params& params = Params()) override;

    // Trading
    Order createOrder(const std::string& symbol, const std::string& type, const std::string& side,
                     double amount, double price = 0, const Params& params = Params()) override;
    Order cancelOrder(const std::string& id, const std::string& symbol = "", const Params& params = Params()) override;
    std::vector<Order> fetchOrders(const std::string& symbol = "", int since = 0, int limit = 0, const Params& params = Params()) override;
    std::vector<Order> fetchOpenOrders(const std::string& symbol = "", int since = 0, int limit = 0, const Params& params = Params()) override;
    std::vector<Order> fetchClosedOrders(const std::string& symbol = "", int since = 0, int limit = 0, const Params& params = Params()) override;
    Order fetchOrder(const std::string& id, const std::string& symbol = "", const Params& params = Params()) override;

    // Account
    Balance fetchBalance(const Params& params = Params()) override;
    std::vector<Account> fetchAccounts(const Params& params = Params()) override;
    TradingFees fetchTradingFees(const Params& params = Params()) override;
    
    // Funding
    std::vector<Transaction> fetchDeposits(const std::string& code = "", int since = 0, int limit = 0, const Params& params = Params()) override;
    std::vector<Transaction> fetchWithdrawals(const std::string& code = "", int since = 0, int limit = 0, const Params& params = Params()) override;
    DepositAddress fetchDepositAddress(const std::string& code, const Params& params = Params()) override;
    std::vector<LedgerEntry> fetchLedger(const std::string& code = "", int since = 0, int limit = 0, const Params& params = Params()) override;

protected:
    // API Endpoints
    std::string apiV1Public() const { return "https://api.huobi.pro/v1"; }
    std::string apiV1Private() const { return "https://api.huobi.pro/v1"; }
    std::string apiV2Public() const { return "https://api.huobi.pro/v2"; }
    std::string apiV2Private() const { return "https://api.huobi.pro/v2"; }

    // Authentication
    std::string sign(const std::string& path, const std::string& api = "public", const std::string& method = "GET",
                    const Params& params = Params(), const std::string& body = "", const std::map<std::string, std::string>& headers = {}) override;

    // Parsing Methods
    Order parseOrder(const json& order, const Market* market = nullptr) override;
    Trade parseTrade(const json& trade, const Market* market = nullptr) override;
    Ticker parseTicker(const json& ticker, const Market* market = nullptr) override;
    Balance parseBalance(const json& balance) override;
    Transaction parseTransaction(const json& transaction, const json& currency = json(), const std::string& type = "") override;
    LedgerEntry parseLedgerEntry(const json& item, const json& currency = json()) override;
    std::string parseOrderStatus(const std::string& status) const;
    std::string parseTimeInForce(const std::string& timeInForce) const;

private:
    // Helper Methods
    std::string getSignaturePrefix() const;
    std::string getSignatureSuffix() const;
    std::string createSignature(const std::string& path, const std::string& method,
                              const std::string& host, const Params& params,
                              const std::string& timestamp) const;
};

} // namespace ccxt

#endif // CCXT_HTX_H
