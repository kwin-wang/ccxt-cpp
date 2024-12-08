#ifndef CCXT_BLOFIN_WS_H
#define CCXT_BLOFIN_WS_H

#include "exchange_ws.h"

namespace ccxt {

class blofin_ws : public exchange_ws {
public:
    blofin_ws();
    ~blofin_ws() = default;

    // Market Data Methods
    Response watchTicker(const std::string& symbol, const Dict& params = Dict());
    Response watchTickers(const std::vector<std::string>& symbols = std::vector<std::string>(), const Dict& params = Dict());
    Response watchTrades(const std::string& symbol, const Dict& params = Dict());
    Response watchOrderBook(const std::string& symbol, const int limit = 0, const Dict& params = Dict());
    Response watchOHLCV(const std::string& symbol, const std::string& timeframe, const Dict& params = Dict());

    // Private Methods
    Response watchBalance(const Dict& params = Dict());
    Response watchOrders(const std::string& symbol = "", const Dict& params = Dict());
    Response watchMyTrades(const std::string& symbol = "", const Dict& params = Dict());
    Response watchPositions(const Dict& params = Dict());

protected:
    void handleMessage(const json& message) override;
    void handleError(const json& message) override;
    void authenticate(const Dict& params = Dict()) override;
    
    // Message Handlers
    void handleTickerMessage(const json& message);
    void handleTradesMessage(const json& message);
    void handleOrderBookMessage(const json& message);
    void handleOHLCVMessage(const json& message);
    void handleBalanceMessage(const json& message);
    void handleOrderMessage(const json& message);
    void handleMyTradesMessage(const json& message);
    void handlePositionMessage(const json& message);
    void handleSubscriptionStatus(const json& message);
    void handleAuthenticationMessage(const json& message);

private:
    std::string getSymbolId(const std::string& symbol);
    void subscribePublic(const std::string& channel, const std::string& symbol);
    void subscribePrivate(const std::string& channel, const std::string& symbol = "");
    std::string getSignature(const std::string& timestamp, const std::string& method, 
                            const std::string& requestPath, const std::string& body = "");
    
    // Cache for market data
    std::map<std::string, OrderBook> orderbooks;
    std::map<std::string, std::vector<Trade>> trades;
    std::map<std::string, Ticker> tickers;
    std::map<std::string, std::vector<OHLCV>> ohlcvs;
    std::map<std::string, Position> positions;
    
    // Authentication state
    bool authenticated;
    int64_t lastPingTimestamp;
    int pingInterval;
};

} // namespace ccxt

#endif // CCXT_BLOFIN_WS_H
