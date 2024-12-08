#ifndef CCXT_BYBIT_WS_H
#define CCXT_BYBIT_WS_H

#include "websocket_client.h"
#include "../bybit.h"
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

namespace ccxt {

class BybitWS : public WebSocketClient {
public:
    BybitWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Bybit& exchange);

    std::string getEndpoint() override;
    void authenticate();

    // Market Data Methods
    void watchTicker(const std::string& symbol);
    void watchOrderBook(const std::string& symbol, const std::string& limit = "");
    void watchTrades(const std::string& symbol);
    void watchOHLCV(const std::string& symbol, const std::string& timeframe);

    // Private Methods
    void watchBalance();
    void watchOrders();
    void watchMyTrades();
    void watchPositions();
    void watchMarkPrice(const std::string& symbol);

protected:
    void handleMessage(const std::string& message) override;

private:
    Bybit& exchange_;
    bool authenticated_ = false;
    int nextRequestId_ = 1;
    std::unordered_map<std::string, nlohmann::json> options_;
    std::unordered_map<std::string, std::string> streamBySubscriptionsHash_;
    int streamIndex_ = -1;

    // Utility Functions
    std::string sign(const std::string& payload);
    std::string getStream(const std::string& type, const std::string& subscriptionHash, int numSubscriptions = 1);
    void checkSubscriptionLimit(const std::string& type, const std::string& stream, int numSubscriptions);

    // Message Handlers
    void handleTicker(const nlohmann::json& data);
    void handleOrderBook(const nlohmann::json& data);
    void handleTrade(const nlohmann::json& data);
    void handleOHLCV(const nlohmann::json& data);
    void handleBalance(const nlohmann::json& data);
    void handleOrder(const nlohmann::json& data);
    void handleMyTrade(const nlohmann::json& data);
    void handlePosition(const nlohmann::json& data);
};

} // namespace ccxt

#endif // CCXT_BYBIT_WS_H
