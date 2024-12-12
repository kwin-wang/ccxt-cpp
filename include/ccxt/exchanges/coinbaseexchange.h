#ifndef CCXT_COINBASEEXCHANGE_H
#define CCXT_COINBASEEXCHANGE_H

#include "ccxt/base/exchange.h"
#include <boost/future.hpp>

namespace ccxt {

class CoinbaseExchange : public Exchange {
public:
    CoinbaseExchange(const Params& params = Params());
    ~CoinbaseExchange() = default;

    // Market Data Methods
    OrderBook fetchOrderBook(const std::string& symbol, int limit = 0, const Params& params = Params()) override;
    std::vector<Trade> fetchTrades(const std::string& symbol, int since = 0, int limit = 0, const Params& params = Params()) override;
    Ticker fetchTicker(const std::string& symbol, const Params& params = Params()) override;
    std::map<std::string, Ticker> fetchTickers(const std::vector<std::string>& symbols = std::vector<std::string>(), const Params& params = Params()) override;
    
    // Trading Methods
    Order createOrder(const std::string& symbol, const std::string& type, const std::string& side,
                     double amount, double price = 0, const Params& params = Params()) override;
    Order cancelOrder(const std::string& id, const std::string& symbol = "", const Params& params = Params()) override;
    std::vector<Order> fetchOrders(const std::string& symbol = "", int since = 0, int limit = 0, const Params& params = Params()) override;
    std::vector<Order> fetchOpenOrders(const std::string& symbol = "", int since = 0, int limit = 0, const Params& params = Params()) override;
    std::vector<Order> fetchClosedOrders(const std::string& symbol = "", int since = 0, int limit = 0, const Params& params = Params()) override;
    Order fetchOrder(const std::string& id, const std::string& symbol = "", const Params& params = Params()) override;
    
    // Account Methods
    Balance fetchBalance(const Params& params = Params()) override;
    std::vector<Account> fetchAccounts(const Params& params = Params()) override;
    TradingFees fetchTradingFees(const Params& params = Params()) override;
    
    // Funding Methods
    std::vector<Transaction> fetchDeposits(const std::string& code = "", int since = 0, int limit = 0, const Params& params = Params()) override;
    std::vector<Transaction> fetchWithdrawals(const std::string& code = "", int since = 0, int limit = 0, const Params& params = Params()) override;
    DepositAddress fetchDepositAddress(const std::string& code, const Params& params = Params()) override;
    std::vector<LedgerEntry> fetchLedger(const std::string& code = "", int since = 0, int limit = 0, const Params& params = Params()) override;

    // Advanced Trading Methods
    std::vector<Position> fetchPositions(const std::vector<std::string>& symbols = std::vector<std::string>(), const Params& params = Params()) override;
    MarginMode setMarginMode(const std::string& symbol, const std::string& marginMode, const Params& params = Params()) override;

    // Market Data Methods - Asynchronous
    boost::future<OrderBook> fetchOrderBookAsync(const std::string& symbol, int limit = 0, const Params& params = Params()) const;
    boost::future<std::vector<Trade>> fetchTradesAsync(const std::string& symbol, int since = 0, int limit = 0, const Params& params = Params()) const;
    boost::future<Ticker> fetchTickerAsync(const std::string& symbol, const Params& params = Params()) const;
    boost::future<std::map<std::string, Ticker>> fetchTickersAsync(const std::vector<std::string>& symbols = std::vector<std::string>(), const Params& params = Params()) const;

    // Trading Methods - Asynchronous
    boost::future<Order> createOrderAsync(const std::string& symbol, const std::string& type, const std::string& side,
                                      double amount, double price = 0, const Params& params = Params());
    boost::future<Order> cancelOrderAsync(const std::string& id, const std::string& symbol = "", const Params& params = Params());
    boost::future<std::vector<Order>> fetchOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const Params& params = Params()) const;
    boost::future<std::vector<Order>> fetchOpenOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const Params& params = Params()) const;
    boost::future<std::vector<Order>> fetchClosedOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const Params& params = Params()) const;
    boost::future<Order> fetchOrderAsync(const std::string& id, const std::string& symbol = "", const Params& params = Params()) const;

    // Account Methods - Asynchronous
    boost::future<Balance> fetchBalanceAsync(const Params& params = Params()) const;
    boost::future<std::vector<Account>> fetchAccountsAsync(const Params& params = Params()) const;
    boost::future<TradingFees> fetchTradingFeesAsync(const Params& params = Params()) const;

    // Funding Methods - Asynchronous
    boost::future<std::vector<Transaction>> fetchDepositsAsync(const std::string& code = "", int since = 0, int limit = 0, const Params& params = Params()) const;
    boost::future<std::vector<Transaction>> fetchWithdrawalsAsync(const std::string& code = "", int since = 0, int limit = 0, const Params& params = Params()) const;
    boost::future<DepositAddress> fetchDepositAddressAsync(const std::string& code, const Params& params = Params()) const;
    boost::future<std::vector<LedgerEntry>> fetchLedgerAsync(const std::string& code = "", int since = 0, int limit = 0, const Params& params = Params()) const;

protected:
    // API Endpoints
    std::string apiPublic() const { return "https://api.exchange.coinbase.com"; }
    std::string apiPrivate() const { return "https://api.exchange.coinbase.com"; }
    std::string apiVersion() const { return "v3"; }
    
    // Authentication
    std::string sign(const std::string& path, const std::string& api = "public", const std::string& method = "GET",
                    const Params& params = Params(), const std::string& body = "", const std::map<std::string, std::string>& headers = {}) override;
    
    // Parsing Methods
    Order parseOrder(const json& order, const Market* market = nullptr) override;
    Trade parseTrade(const json& trade, const Market* market = nullptr) override;
    Position parsePosition(const json& position, const Market* market = nullptr) override;
    Ticker parseTicker(const json& ticker, const Market* market = nullptr) override;
    Balance parseBalance(const json& balance) override;
    Transaction parseTransaction(const json& transaction, const json& currency = json(), const std::string& type = "") override;
    LedgerEntry parseLedgerEntry(const json& item, const json& currency = json()) override;
    
    // Helper Methods
    std::string parseOrderStatus(const std::string& status) const;
    std::string parseTimeInForce(const std::string& timeInForce) const;
    std::string parseOrderType(const std::string& type) const;
    std::string parseOrderSide(const std::string& side) const;

private:
    // Helper Methods
    std::string getSignaturePrefix() const;
    std::string getSignatureSuffix() const;
    std::string createSignature(const std::string& timestamp, const std::string& method,
                              const std::string& requestPath, const std::string& body = "") const;
    std::string getTimestamp() const;
    std::string generateCBAccessSign(const std::string& message) const;
};

} // namespace ccxt

#endif // CCXT_COINBASEEXCHANGE_H
