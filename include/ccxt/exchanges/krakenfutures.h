#ifndef CCXT_KRAKENFUTURES_H
#define CCXT_KRAKENFUTURES_H

#include "../exchange.h"

namespace ccxt {

class KrakenFutures : public Exchange {
public:
    KrakenFutures(const Params& params = Params());
    ~KrakenFutures() = default;

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
    std::vector<Position> fetchPositions(const std::vector<std::string>& symbols = std::vector<std::string>(), const Params& params = Params()) override;
    std::vector<Account> fetchAccounts(const Params& params = Params()) override;
    TradingFees fetchTradingFees(const Params& params = Params()) override;
    
    // Funding Methods
    std::vector<Transaction> fetchDeposits(const std::string& code = "", int since = 0, int limit = 0, const Params& params = Params()) override;
    std::vector<Transaction> fetchWithdrawals(const std::string& code = "", int since = 0, int limit = 0, const Params& params = Params()) override;
    DepositAddress fetchDepositAddress(const std::string& code, const Params& params = Params()) override;
    std::vector<LedgerEntry> fetchLedger(const std::string& code = "", int since = 0, int limit = 0, const Params& params = Params()) override;

    // Futures/Derivatives Methods
    std::vector<FundingRate> fetchFundingRates(const std::vector<std::string>& symbols = std::vector<std::string>(), const Params& params = Params()) override;
    FundingRate fetchFundingRate(const std::string& symbol, const Params& params = Params()) override;
    Leverage setLeverage(int leverage, const std::string& symbol = "", const Params& params = Params()) override;
    MarginMode setMarginMode(const std::string& symbol, const std::string& marginMode, const Params& params = Params()) override;

protected:
    // API Endpoints
    std::string apiPublic() const { return "https://futures.kraken.com/derivatives/api/v3"; }
    std::string apiPrivate() const { return "https://futures.kraken.com/derivatives/api/v3"; }
    
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
    FundingRate parseFundingRate(const json& fundingRate, const Market* market = nullptr) override;
    
    // Helper Methods
    std::string parseOrderStatus(const std::string& status) const;
    std::string parseTimeInForce(const std::string& timeInForce) const;
    std::string parsePositionSide(const std::string& side) const;

private:
    // Helper Methods
    std::string getSignaturePrefix() const;
    std::string getSignatureSuffix() const;
    std::string createSignature(const std::string& path, const std::string& method,
                              const std::string& host, const Params& params,
                              const std::string& nonce) const;
    std::string getNonce() const;
};

} // namespace ccxt

#endif // CCXT_KRAKENFUTURES_H
