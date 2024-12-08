#ifndef CCXT_INDEPENDENTRESERVE_WS_H
#define CCXT_INDEPENDENTRESERVE_WS_H

#include "ccxt/websocket_client.h"
#include "../independentreserve.h"
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

namespace ccxt {

class IndependentReserveWS : public WebSocketClient {
public:
    IndependentReserveWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, IndependentReserve& exchange);

    std::string getEndpoint() override;
    void authenticate();

    // Public API Methods
    void watchTicker(const std::string& symbol);
    void watchOrderBook(const std::string& symbol, const std::string& limit = "");
    void watchTrades(const std::string& symbol);
    void watchStatus();

    // Private API Methods
    void watchBalance();
    void watchOrders(const std::string& symbol = "");
    void watchMyTrades(const std::string& symbol = "");

protected:
    void handleMessage(const std::string& message) override;

private:
    IndependentReserve& exchange_;
    bool authenticated_ = false;
    int nextRequestId_ = 1;
    std::unordered_map<std::string, nlohmann::json> options_;
    std::unordered_map<std::string, std::string> subscriptions_;

    // Utility Functions
    std::string sign(const std::string& payload);
    std::string getMarketId(const std::string& symbol);
    std::string getSymbol(const std::string& marketId);
    void subscribe(const std::string& channel, const std::string& symbol = "");
    void unsubscribe(const std::string& channel, const std::string& symbol = "");
    int getNextRequestId();

    // Message Handlers
    void handleTicker(const nlohmann::json& data);
    void handleOrderBook(const nlohmann::json& data);
    void handleTrade(const nlohmann::json& data);
    void handleStatus(const nlohmann::json& data);
    void handleBalance(const nlohmann::json& data);
    void handleOrder(const nlohmann::json& data);
    void handleMyTrade(const nlohmann::json& data);

    // Helper Functions
    void handleHeartbeat();
    void handleSubscriptionResponse(const nlohmann::json& data);
    void handleAuthenticationResponse(const nlohmann::json& data);
};

} // namespace ccxt

#endif // CCXT_INDEPENDENTRESERVE_WS_H
