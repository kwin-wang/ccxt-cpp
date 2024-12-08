#ifndef CCXT_EXCHANGE_COINCATCH_WS_H
#define CCXT_EXCHANGE_COINCATCH_WS_H

#include "../../websocket_client.h"
#include "../../coincatch.h"
#include <string>
#include <map>
#include <vector>

namespace ccxt {

class CoincatchWS : public WebSocketClient {
public:
    CoincatchWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Coincatch& exchange);
    ~CoincatchWS() = default;

    // Public API
    void watchTicker(const std::string& symbol, const std::map<std::string, std::string>& params = {});
    void watchTickers(const std::vector<std::string>& symbols, const std::map<std::string, std::string>& params = {});
    void watchOrderBook(const std::string& symbol, int limit = 0, const std::map<std::string, std::string>& params = {});
    void watchOrderBookForSymbols(const std::vector<std::string>& symbols, int limit = 0, const std::map<std::string, std::string>& params = {});
    void watchTrades(const std::string& symbol, const std::map<std::string, std::string>& params = {});
    void watchTradesForSymbols(const std::vector<std::string>& symbols, const std::map<std::string, std::string>& params = {});
    void watchOHLCV(const std::string& symbol, const std::string& timeframe, const std::map<std::string, std::string>& params = {});

    // Private API
    void watchBalance(const std::map<std::string, std::string>& params = {});
    void watchOrders(const std::string& symbol = "", const std::map<std::string, std::string>& params = {});
    void watchPositions(const std::vector<std::string>& symbols = {}, const std::map<std::string, std::string>& params = {});

    // Unsubscribe methods
    void unwatchTicker(const std::string& symbol);
    void unwatchOrderBook(const std::string& symbol);
    void unwatchTrades(const std::string& symbol);
    void unwatchOHLCV(const std::string& symbol, const std::string& timeframe);

private:
    Coincatch& exchange_;
    std::map<std::string, std::string> subscriptions_;
    
    // Helper methods
    std::string getEndpoint(const std::string& type);
    void subscribe(const std::string& channel, const std::string& symbol, const std::string& instType = "");
    void subscribePrivate(const std::string& channel, const std::string& instType = "");
    void unsubscribe(const std::string& channel, const std::string& symbol, const std::string& instType = "");
    void authenticate();
    std::string getInstType(const std::string& symbol);
    std::string getInstId(const std::string& symbol);

    // Message handlers
    void handleMessage(const std::string& message);
    void handleTickerMessage(const nlohmann::json& data);
    void handleOrderBookMessage(const nlohmann::json& data);
    void handleTradeMessage(const nlohmann::json& data);
    void handleOHLCVMessage(const nlohmann::json& data);
    void handleBalanceMessage(const nlohmann::json& data);
    void handleOrderMessage(const nlohmann::json& data);
    void handlePositionMessage(const nlohmann::json& data);
    void handleErrorMessage(const nlohmann::json& data);
    void handlePong(const nlohmann::json& data);
    void handleSubscriptionStatus(const nlohmann::json& data);
    void handleUnsubscriptionStatus(const nlohmann::json& data);
    void handleAuthenticate(const nlohmann::json& data);

    // Market parsing
    std::string parseMarketId(const std::string& symbol);
    std::string parseSymbol(const std::string& marketId);
    std::map<std::string, std::string> parseMarket(const std::string& marketId);
};

} // namespace ccxt

#endif // CCXT_EXCHANGE_COINCATCH_WS_H
