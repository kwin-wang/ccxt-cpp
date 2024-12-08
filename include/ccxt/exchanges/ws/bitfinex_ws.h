#ifndef CCXT_BITFINEX_WS_H
#define CCXT_BITFINEX_WS_H

#include "websocket_client.h"
#include "../bitfinex.h"
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

namespace ccxt {

class BitfinexWS : public WebSocketClient {
public:
    BitfinexWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Bitfinex& exchange);

    std::string getEndpoint();
    std::string getPrivateEndpoint();
    void authenticate();

    // Market Data Methods
    void watchTicker(const std::string& symbol);
    void watchOrderBook(const std::string& symbol, const std::string& prec = "P0", const std::string& freq = "F0");
    void watchTrades(const std::string& symbol);

    // Private Methods
    void watchBalance();
    void watchOrders();
    void watchMyTrades();
    void createOrder(const std::string& symbol, const std::string& type,
                    const std::string& side, double amount, double price);
    void cancelOrder(const std::string& id);
    void cancelAllOrders();

protected:
    void handleMessage(const std::string& message) override;

private:
    Bitfinex& exchange_;
    bool authenticated_ = false;
    std::unordered_map<std::string, nlohmann::json> options_;
    std::unordered_map<int, std::string> channelMap_;

    // Subscription Methods
    void subscribe(const std::string& channel, const std::string& symbol,
                  const nlohmann::json& params = nlohmann::json::object());

    // Message Handlers
    void handleTicker(int channelId, const nlohmann::json& data);
    void handleOrderBook(int channelId, const nlohmann::json& data);
    void handleTrade(int channelId, const nlohmann::json& data);
    void handleBalance(int channelId, const nlohmann::json& data);
    void handleOrder(int channelId, const nlohmann::json& data);
    void handleMyTrade(int channelId, const nlohmann::json& data);
    void handleHeartbeat(int channelId);
    void handleSubscribed(const nlohmann::json& data);
    void handleError(const nlohmann::json& data);
};

} // namespace ccxt

#endif // CCXT_BITFINEX_WS_H
