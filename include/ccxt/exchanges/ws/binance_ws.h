#ifndef CCXT_BINANCE_WS_H
#define CCXT_BINANCE_WS_H

#include "websocket_client.h"
#include "../binance.h"
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

namespace ccxt {

class BinanceWS : public WebSocketClient {
public:
    BinanceWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Binance& exchange);

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
    Binance& exchange_;
    bool checksumEnabled_;
    int nextRequestId_ = 1;
    bool authenticated_ = false;
    std::unordered_map<std::string, int> streamLimits_;
    std::unordered_map<std::string, int> subscriptionLimits_;
    std::unordered_map<std::string, nlohmann::json> options_;

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

#endif // CCXT_BINANCE_WS_H
