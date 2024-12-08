#ifndef CCXT_HTX_WS_H
#define CCXT_HTX_WS_H

#include "websocket_client.h"
#include "../htx.h"
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

namespace ccxt {

class HTXWS : public WebSocketClient {
public:
    HTXWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, HTX& exchange);

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

protected:
    void handleMessage(const std::string& message) override;

private:
    HTX& exchange_;
    bool authenticated_ = false;
    int nextRequestId_ = 1;
    std::unordered_map<std::string, nlohmann::json> options_;

    // Utility Functions
    std::string sign(const std::string& payload);

    // Message Handlers
    void handleTicker(const nlohmann::json& data);
    void handleOrderBook(const nlohmann::json& data);
    void handleTrade(const nlohmann::json& data);
    void handleOHLCV(const nlohmann::json& data);
    void handleBalance(const nlohmann::json& data);
    void handleOrder(const nlohmann::json& data);
    void handleMyTrade(const nlohmann::json& data);
};

} // namespace ccxt

#endif // CCXT_HTX_WS_H
