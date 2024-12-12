#pragma once

#include "ccxt/base/exchange.h"
#include <future>

namespace ccxt {

class KuCoinFutures : public Exchange {
public:
    KuCoinFutures(const ExchangeConfig& config = ExchangeConfig());
    ~KuCoinFutures() override = default;

    // Market Data - Sync
    std::vector<Market> fetchMarkets(const Params& params = Params()) override;
    OrderBook fetchOrderBook(const std::string& symbol, int limit = 0, const Params& params = Params()) override;
    Ticker fetchTicker(const std::string& symbol, const Params& params = Params()) override;
    std::map<std::string, Ticker> fetchTickers(const std::vector<std::string>& symbols = std::vector<std::string>(), const Params& params = Params()) override;
    std::vector<Trade> fetchTrades(const std::string& symbol, int since = 0, int limit = 0, const Params& params = Params()) override;
    std::vector<OHLCV> fetchOHLCV(const std::string& symbol, const std::string& timeframe = "1m", int since = 0, int limit = 0, const Params& params = Params()) override;
    FundingRate fetchFundingRate(const std::string& symbol, const Params& params = Params());
    std::map<std::string, FundingRate> fetchFundingRates(const std::vector<std::string>& symbols = std::vector<std::string>(), const Params& params = Params());
    std::vector<FundingRate> fetchFundingRateHistory(const std::string& symbol = "", int since = 0, int limit = 0, const Params& params = Params());
    
    // Trading - Sync
    Order createOrder(const std::string& symbol, const std::string& type, const std::string& side,
                     double amount, double price = 0, const Params& params = Params()) override;
    Order cancelOrder(const std::string& id, const std::string& symbol = "", const Params& params = Params()) override;
    std::vector<Order> cancelAllOrders(const std::string& symbol = "", const Params& params = Params());
    Order fetchOrder(const std::string& id, const std::string& symbol = "", const Params& params = Params()) override;
    std::vector<Order> fetchOrders(const std::string& symbol = "", int since = 0, int limit = 0, const Params& params = Params()) override;
    std::vector<Order> fetchOpenOrders(const std::string& symbol = "", int since = 0, int limit = 0, const Params& params = Params()) override;
    std::vector<Order> fetchClosedOrders(const std::string& symbol = "", int since = 0, int limit = 0, const Params& params = Params()) override;
    
    // Account - Sync
    Balance fetchBalance(const Params& params = Params()) override;
    std::vector<Position> fetchPositions(const std::vector<std::string>& symbols = std::vector<std::string>(), const Params& params = Params());
    Position fetchPosition(const std::string& symbol, const Params& params = Params());
    std::vector<LeverageTier> fetchLeverageTiers(const std::vector<std::string>& symbols = std::vector<std::string>(), const Params& params = Params());
    MarginModification setLeverage(int leverage, const std::string& symbol, const Params& params = Params());
    MarginMode setMarginMode(const std::string& marginMode, const std::string& symbol = "", const Params& params = Params());
    
    // Market Data - Async
    std::future<std::vector<Market>> fetchMarketsAsync(const Params& params = Params());
    std::future<OrderBook> fetchOrderBookAsync(const std::string& symbol, int limit = 0, const Params& params = Params());
    std::future<Ticker> fetchTickerAsync(const std::string& symbol, const Params& params = Params());
    std::future<std::map<std::string, Ticker>> fetchTickersAsync(const std::vector<std::string>& symbols = std::vector<std::string>(), const Params& params = Params());
    std::future<std::vector<Trade>> fetchTradesAsync(const std::string& symbol, int since = 0, int limit = 0, const Params& params = Params());
    std::future<std::vector<OHLCV>> fetchOHLCVAsync(const std::string& symbol, const std::string& timeframe = "1m", int since = 0, int limit = 0, const Params& params = Params());
    std::future<FundingRate> fetchFundingRateAsync(const std::string& symbol, const Params& params = Params());
    std::future<std::map<std::string, FundingRate>> fetchFundingRatesAsync(const std::vector<std::string>& symbols = std::vector<std::string>(), const Params& params = Params());
    std::future<std::vector<FundingRate>> fetchFundingRateHistoryAsync(const std::string& symbol = "", int since = 0, int limit = 0, const Params& params = Params());
    
    // Trading - Async
    std::future<Order> createOrderAsync(const std::string& symbol, const std::string& type, const std::string& side,
                                      double amount, double price = 0, const Params& params = Params());
    std::future<Order> cancelOrderAsync(const std::string& id, const std::string& symbol = "", const Params& params = Params());
    std::future<std::vector<Order>> cancelAllOrdersAsync(const std::string& symbol = "", const Params& params = Params());
    std::future<Order> fetchOrderAsync(const std::string& id, const std::string& symbol = "", const Params& params = Params());
    std::future<std::vector<Order>> fetchOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const Params& params = Params());
    std::future<std::vector<Order>> fetchOpenOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const Params& params = Params());
    std::future<std::vector<Order>> fetchClosedOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const Params& params = Params());
    
    // Account - Async
    std::future<Balance> fetchBalanceAsync(const Params& params = Params());
    std::future<std::vector<Position>> fetchPositionsAsync(const std::vector<std::string>& symbols = std::vector<std::string>(), const Params& params = Params());
    std::future<Position> fetchPositionAsync(const std::string& symbol, const Params& params = Params());
    std::future<std::vector<LeverageTier>> fetchLeverageTiersAsync(const std::vector<std::string>& symbols = std::vector<std::string>(), const Params& params = Params());
    std::future<MarginModification> setLeverageAsync(int leverage, const std::string& symbol, const Params& params = Params());
    std::future<MarginMode> setMarginModeAsync(const std::string& marginMode, const std::string& symbol = "", const Params& params = Params());

protected:
    // API Endpoints
    std::string apiPublic() const { return "https://api-futures.kucoin.com"; }
    std::string apiPrivate() const { return "https://api-futures.kucoin.com"; }
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
    Position parsePosition(const json& position, const Market* market = nullptr);
    FundingRate parseFundingRate(const json& fundingRate, const Market* market = nullptr);
    LeverageTier parseLeverageTier(const json& leverageTier, const Market* market = nullptr);
    MarginMode parseMarginMode(const json& marginMode, const Market* market = nullptr);
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
    std::string getKuCoinFuturesSymbol(const std::string& symbol) const;
    
    // API Token Management
    void initTokens();
    void refreshTokens();
    std::string getToken(const std::string& type = "futures") const;
    
    std::map<std::string, std::string> tokens;
    std::map<std::string, int64_t> tokenExpiry;
    std::map<std::string, std::string> timeframes;
};

} // namespace ccxt
