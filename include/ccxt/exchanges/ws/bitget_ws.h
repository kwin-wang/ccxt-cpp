#ifndef CCXT_BITGET_WS_H
#define CCXT_BITGET_WS_H

#include "websocket_client.h"
#include "../bitget.h"
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

namespace ccxt {

class BitgetWS : public WebSocketClient {
public:
    BitgetWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Bitget& exchange);

    std::string getEndpoint() override;
    void authenticate();

    // Market Data Methods
    void watchTicker(const std::string& symbol);
    void watchTickers(const std::vector<std::string>& symbols);
    void watchOrderBook(const std::string& symbol, const std::string& limit = "");
    void watchTrades(const std::string& symbol);
    void watchOHLCV(const std::string& symbol, const std::string& timeframe);
    void watchBidsAsks(const std::string& symbol);

    // Private Methods
    void watchBalance();
    void watchOrders();
    void watchMyTrades();
    void watchPositions();

protected:
    void handleMessage(const std::string& message) override;

private:
    Bitget& exchange_;
    bool authenticated_ = false;
    int nextRequestId_ = 1;
    std::unordered_map<std::string, nlohmann::json> options_;
    std::unordered_map<std::string, std::string> subscriptions_;

    // Utility Functions
    std::string sign(const std::string& timestamp, const std::string& method,
                    const std::string& path, const std::string& body = "");
    void subscribe(const std::string& channel, const std::string& instId,
                  const nlohmann::json& args = nlohmann::json::object());
    void unsubscribe(const std::string& channel, const std::string& instId);

    // Message Handlers
    void handleTicker(const nlohmann::json& data);
    void handleOrderBook(const nlohmann::json& data);
    void handleTrade(const nlohmann::json& data);
    void handleOHLCV(const nlohmann::json& data);
    void handleBidsAsks(const nlohmann::json& data);
    void handleBalance(const nlohmann::json& data);
    void handleOrder(const nlohmann::json& data);
    void handleMyTrade(const nlohmann::json& data);
    void handlePosition(const nlohmann::json& data);
    void handleError(const nlohmann::json& data);
};

} // namespace ccxt

#endif // CCXT_BITGET_WS_H
