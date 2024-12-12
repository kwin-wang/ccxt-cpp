#pragma once

#include "ccxt/base/exchange.h"
#include <future>

namespace ccxt {

class KuCoin : public Exchange {
public:
    KuCoin(const ExchangeConfig& config = ExchangeConfig());
    ~KuCoin() override = default;

    // Market Data Methods - Sync
    std::vector<Market> fetchMarkets(const Params& params = Params()) override;
    OrderBook fetchOrderBook(const std::string& symbol, int limit = 0, const Params& params = Params()) override;
    Ticker fetchTicker(const std::string& symbol, const Params& params = Params()) override;
    std::map<std::string, Ticker> fetchTickers(const std::vector<std::string>& symbols = std::vector<std::string>(), const Params& params = Params()) override;
    std::vector<Trade> fetchTrades(const std::string& symbol, int since = 0, int limit = 0, const Params& params = Params()) override;
    std::vector<OHLCV> fetchOHLCV(const std::string& symbol, const std::string& timeframe = "1m", int since = 0, int limit = 0, const Params& params = Params()) override;
    
    // Trading Methods - Sync
    Order createOrder(const std::string& symbol, const std::string& type, const std::string& side,
                     double amount, double price = 0, const Params& params = Params()) override;
    Order cancelOrder(const std::string& id, const std::string& symbol = "", const Params& params = Params()) override;
    std::vector<Order> fetchOpenOrders(const std::string& symbol = "", int since = 0, int limit = 0, const Params& params = Params()) override;
    std::vector<Order> fetchClosedOrders(const std::string& symbol = "", int since = 0, int limit = 0, const Params& params = Params()) override;
    Order fetchOrder(const std::string& id, const std::string& symbol = "", const Params& params = Params()) override;
    
    // Account Methods - Sync
    Balance fetchBalance(const Params& params = Params()) override;
    std::vector<Transaction> fetchDeposits(const std::string& code = "", int since = 0, int limit = 0, const Params& params = Params()) override;
    std::vector<Transaction> fetchWithdrawals(const std::string& code = "", int since = 0, int limit = 0, const Params& params = Params()) override;
    DepositAddress fetchDepositAddress(const std::string& code, const Params& params = Params()) override;
    
    // Market Data Methods - Async
    std::future<std::vector<Market>> fetchMarketsAsync(const Params& params = Params());
    std::future<OrderBook> fetchOrderBookAsync(const std::string& symbol, int limit = 0, const Params& params = Params());
    std::future<Ticker> fetchTickerAsync(const std::string& symbol, const Params& params = Params());
    std::future<std::map<std::string, Ticker>> fetchTickersAsync(const std::vector<std::string>& symbols = std::vector<std::string>(), const Params& params = Params());
    std::future<std::vector<Trade>> fetchTradesAsync(const std::string& symbol, int since = 0, int limit = 0, const Params& params = Params());
    std::future<std::vector<OHLCV>> fetchOHLCVAsync(const std::string& symbol, const std::string& timeframe = "1m", int since = 0, int limit = 0, const Params& params = Params());
    
    // Trading Methods - Async
    std::future<Order> createOrderAsync(const std::string& symbol, const std::string& type, const std::string& side,
                                      double amount, double price = 0, const Params& params = Params());
    std::future<Order> cancelOrderAsync(const std::string& id, const std::string& symbol = "", const Params& params = Params());
    std::future<std::vector<Order>> fetchOpenOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const Params& params = Params());
    std::future<std::vector<Order>> fetchClosedOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const Params& params = Params());
    std::future<Order> fetchOrderAsync(const std::string& id, const std::string& symbol = "", const Params& params = Params());
    
    // Account Methods - Async
    std::future<Balance> fetchBalanceAsync(const Params& params = Params());
    std::future<std::vector<Transaction>> fetchDepositsAsync(const std::string& code = "", int since = 0, int limit = 0, const Params& params = Params());
    std::future<std::vector<Transaction>> fetchWithdrawalsAsync(const std::string& code = "", int since = 0, int limit = 0, const Params& params = Params());
    std::future<DepositAddress> fetchDepositAddressAsync(const std::string& code, const Params& params = Params());

protected:
    // API Endpoints
    std::string apiPublic() const { return "https://api.kucoin.com"; }
    std::string apiPrivate() const { return "https://api.kucoin.com"; }
    std::string apiV1() const { return "/api/v1"; }
    std::string apiV2() const { return "/api/v2"; }
    
    // Authentication
    std::string sign(const std::string& path, const std::string& api = "public", const std::string& method = "GET",
                    const Params& params = Params(), const std::string& body = "", const std::map<std::string, std::string>& headers = {}) override;
    
    // Parsing Methods
    Market parseMarket(const json& market) override;
    Order parseOrder(const json& order, const Market* market = nullptr) override;
    Trade parseTrade(const json& trade, const Market* market = nullptr) override;
    Ticker parseTicker(const json& ticker, const Market* market = nullptr) override;
    Balance parseBalance(const json& balance) override;
    Transaction parseTransaction(const json& transaction, const std::string& type = "") override;
    DepositAddress parseDepositAddress(const json& depositAddress) override;
    OHLCV parseOHLCV(const json& ohlcv, const Market* market = nullptr) override;
    
    // Helper Methods
    std::string getNonce() const;
    std::string getSignature(const std::string& timestamp, const std::string& method,
                            const std::string& requestPath, const std::string& body = "") const;
    void handleErrors(const json& response);
    std::string parseOrderStatus(const std::string& status) const;
    std::string parseTimeInForce(const std::string& timeInForce) const;
    void validateOrder(const std::string& symbol, const std::string& type, const std::string& side,
                      double amount, double price = 0) const;

private:
    // Helper Methods
    std::string getTimestamp() const;
    std::string createSignature(const std::string& timestamp, const std::string& method,
                              const std::string& endpoint, const std::string& body = "") const;
    std::map<std::string, std::string> getAuthHeaders(const std::string& method,
                                                     const std::string& endpoint,
                                                     const std::string& body = "") const;
    std::string getKucoinSymbol(const std::string& symbol) const;
    
    // API Token Management
    void initTokens();
    void refreshTokens();
    std::string getToken(const std::string& type = "spot") const;
    
    std::map<std::string, std::string> tokens;
    std::map<std::string, int64_t> tokenExpiry;
    std::map<std::string, std::string> timeframes;
};

} // namespace ccxt
