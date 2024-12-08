#ifndef CCXT_BITHUMB_WS_H
#define CCXT_BITHUMB_WS_H

#include "exchange_ws.h"

namespace ccxt {

class bithumb_ws : public exchange_ws {
public:
    bithumb_ws();
    ~bithumb_ws() = default;

    // Market Data Methods
    Response watchTicker(const std::string& symbol, const Dict& params = Dict());
    Response watchTickers(const std::vector<std::string>& symbols = std::vector<std::string>(), const Dict& params = Dict());
    Response watchTrades(const std::string& symbol, const Dict& params = Dict());
    Response watchOrderBook(const std::string& symbol, const int limit = 0, const Dict& params = Dict());

protected:
    void handleMessage(const json& message) override;
    void handleError(const json& message) override;
    
    // Message Handlers
    void handleTickerMessage(const json& message);
    void handleTradesMessage(const json& message);
    void handleOrderBookMessage(const json& message);
    void handleSubscriptionStatus(const json& message);

private:
    std::string getSymbolId(const std::string& symbol);
    void subscribeToChannel(const std::string& channel, const std::string& symbol);
    
    // Cache for market data
    std::map<std::string, OrderBook> orderbooks;
    std::map<std::string, std::vector<Trade>> trades;
    std::map<std::string, Ticker> tickers;
};

} // namespace ccxt

#endif // CCXT_BITHUMB_WS_H
