#ifndef CCXT_KRAKENFUTURES_H
#define CCXT_KRAKENFUTURES_H

#include "ccxt/base/exchange.h"
#include <future>

namespace ccxt {

class KrakenFutures : public Exchange {
public:
    KrakenFutures(const ExchangeConfig& config = ExchangeConfig());
    ~KrakenFutures() = default;

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
    std::vector<Order> fetchCanceledOrders(const std::string& symbol = "", int since = 0, int limit = 0, const Params& params = Params()) override;
    
    // Account Methods - Sync
    Balance fetchBalance(const Params& params = Params()) override;
    std::vector<Position> fetchPositions(const std::vector<std::string>& symbols = std::vector<std::string>(), const Params& params = Params()) override;
    Leverage setLeverage(int leverage, const std::string& symbol = "", const Params& params = Params()) override;
    std::vector<LeverageTier> fetchLeverageTiers(const std::vector<std::string>& symbols = std::vector<std::string>(), const Params& params = Params()) override;
    
    // Funding Methods - Sync
    std::vector<FundingRate> fetchFundingRates(const std::vector<std::string>& symbols = std::vector<std::string>(), const Params& params = Params()) override;
    FundingRate fetchFundingRate(const std::string& symbol, const Params& params = Params()) override;
    std::vector<FundingRateHistory> fetchFundingRateHistory(const std::string& symbol = "", int since = 0, int limit = 0, const Params& params = Params()) override;
    
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
    std::future<std::vector<Order>> fetchCanceledOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const Params& params = Params());
    
    // Account Methods - Async
    std::future<Balance> fetchBalanceAsync(const Params& params = Params());
    std::future<std::vector<Position>> fetchPositionsAsync(const std::vector<std::string>& symbols = std::vector<std::string>(), const Params& params = Params());
    std::future<Leverage> setLeverageAsync(int leverage, const std::string& symbol = "", const Params& params = Params());
    std::future<std::vector<LeverageTier>> fetchLeverageTiersAsync(const std::vector<std::string>& symbols = std::vector<std::string>(), const Params& params = Params());
    
    // Funding Methods - Async
    std::future<std::vector<FundingRate>> fetchFundingRatesAsync(const std::vector<std::string>& symbols = std::vector<std::string>(), const Params& params = Params());
    std::future<FundingRate> fetchFundingRateAsync(const std::string& symbol, const Params& params = Params());
    std::future<std::vector<FundingRateHistory>> fetchFundingRateHistoryAsync(const std::string& symbol = "", int since = 0, int limit = 0, const Params& params = Params());

protected:
    // API Endpoints
    std::string apiPublic() const { return "https://futures.kraken.com/derivatives/api/v3"; }
    std::string apiPrivate() const { return "https://futures.kraken.com/derivatives/api/v3"; }
    std::string apiCharts() const { return "https://futures.kraken.com/api/charts"; }
    std::string apiHistory() const { return "https://futures.kraken.com/api/history"; }
    
    // Authentication
    std::string sign(const std::string& path, const std::string& api = "public", const std::string& method = "GET",
                    const Params& params = Params(), const std::string& body = "", const std::map<std::string, std::string>& headers = {}) override;
    
    // Parsing Methods
    Market parseMarket(const json& market) override;
    Order parseOrder(const json& order, const Market* market = nullptr) override;
    Trade parseTrade(const json& trade, const Market* market = nullptr) override;
    Position parsePosition(const json& position, const Market* market = nullptr) override;
    Ticker parseTicker(const json& ticker, const Market* market = nullptr) override;
    Balance parseBalance(const json& balance) override;
    OHLCV parseOHLCV(const json& ohlcv, const Market* market = nullptr) override;
    FundingRate parseFundingRate(const json& fundingRate, const Market* market = nullptr) override;
    LeverageTier parseLeverageTier(const json& tier, const Market* market = nullptr) override;
    
    // Helper Methods
    std::string getNonce() const;
    std::string getSignature(const std::string& path, const std::string& method,
                            const std::string& nonce, const std::string& body = "") const;
    void handleErrors(const json& response);
    std::string parseOrderStatus(const std::string& status) const;
    std::string parseTimeInForce(const std::string& timeInForce) const;
    std::string parsePositionSide(const std::string& side) const;
    void validateLeverageInput(double leverage, const std::string& symbol);

private:
    // Helper Methods
    std::string getSignaturePrefix() const;
    std::string getSignatureSuffix() const;
    std::string createSignature(const std::string& path, const std::string& method,
                              const std::string& host, const Params& params,
                              const std::string& nonce, const std::string& body = "") const;
};

} // namespace ccxt

#endif // CCXT_KRAKENFUTURES_H
