#ifndef CCXT_EXCHANGE_COINBASEINTERNATIONAL_WS_H
#define CCXT_EXCHANGE_COINBASEINTERNATIONAL_WS_H

#include "../../websocket_client.h"
#include "../../coinbaseinternational.h"
#include <string>
#include <map>
#include <vector>

namespace ccxt {

class CoinbaseInternationalWS : public WebSocketClient {
public:
    CoinbaseInternationalWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, CoinbaseInternational& exchange);
    ~CoinbaseInternationalWS() = default;

    // Public API
    void watchTicker(const std::string& symbol, const std::map<std::string, std::string>& params = {});
    void watchTickers(const std::vector<std::string>& symbols, const std::map<std::string, std::string>& params = {});
    void watchOrderBook(const std::string& symbol, const std::map<std::string, std::string>& params = {});
    void watchTrades(const std::string& symbol, const std::map<std::string, std::string>& params = {});
    void watchOHLCV(const std::string& symbol, const std::string& timeframe, const std::map<std::string, std::string>& params = {});

    // Private API
    void watchBalance(const std::map<std::string, std::string>& params = {});
    void watchOrders(const std::string& symbol = "", const std::map<std::string, std::string>& params = {});
    void watchMyTrades(const std::string& symbol = "", const std::map<std::string, std::string>& params = {});
    void watchPositions(const std::vector<std::string>& symbols = {}, const std::map<std::string, std::string>& params = {});

    // Unsubscribe methods
    void unsubscribe(const std::string& channel, const std::string& symbol = "");
    void unsubscribeAll();

private:
    CoinbaseInternational& exchange_;
    std::map<std::string, std::string> subscriptions_;
    bool authenticated_;
    
    // Helper methods
    void subscribe(const std::string& channel, const std::string& symbol = "", bool isPrivate = false);
    void subscribeMultiple(const std::string& channel, const std::vector<std::string>& symbols, bool isPrivate = false);
    void authenticate();
    std::string getEndpoint(const std::string& type);
    std::string getMarketId(const std::string& symbol);
    std::string getSymbol(const std::string& marketId);
    std::string getChannel(const std::string& channel, const std::string& symbol);

    // Message handlers
    void handleMessage(const std::string& message);
    void handleTickerMessage(const nlohmann::json& data);
    void handleOrderBookMessage(const nlohmann::json& data);
    void handleTradeMessage(const nlohmann::json& data);
    void handleOHLCVMessage(const nlohmann::json& data);
    void handleBalanceMessage(const nlohmann::json& data);
    void handleOrderMessage(const nlohmann::json& data);
    void handleMyTradeMessage(const nlohmann::json& data);
    void handlePositionMessage(const nlohmann::json& data);
    void handleErrorMessage(const nlohmann::json& data);
    void handleSubscriptionMessage(const nlohmann::json& data);
    void handleUnsubscriptionMessage(const nlohmann::json& data);
    void handleAuthenticationMessage(const nlohmann::json& data);
    void handleHeartbeat(const nlohmann::json& data);

    // Market parsing
    std::map<std::string, std::string> parseMarket(const std::string& marketId);
    std::string parseTimeframe(const std::string& timeframe);
};

} // namespace ccxt

#endif // CCXT_EXCHANGE_COINBASEINTERNATIONAL_WS_H
