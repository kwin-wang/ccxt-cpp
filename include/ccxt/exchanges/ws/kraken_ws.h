#ifndef CCXT_KRAKEN_WS_H
#define CCXT_KRAKEN_WS_H

#include "websocket_client.h"
#include "../kraken.h"
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

namespace ccxt {

class KrakenWS : public WebSocketClient {
public:
    KrakenWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Kraken& exchange);

    std::string getEndpoint();
    std::string getPrivateEndpoint();
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
    void createOrder(const std::string& symbol, const std::string& type,
                    const std::string& side, double amount, double price);
    void editOrder(const std::string& id, const std::string& symbol,
                  const std::string& type, const std::string& side,
                  double amount, double price);
    void cancelOrder(const std::string& id);
    void cancelAllOrders();

protected:
    void handleMessage(const std::string& message) override;

private:
    Kraken& exchange_;
    bool authenticated_ = false;
    std::unordered_map<std::string, nlohmann::json> options_;

    // Message Handlers
    void handleTicker(const nlohmann::json& data);
    void handleOrderBook(const nlohmann::json& data);
    void handleTrade(const nlohmann::json& data);
    void handleOHLCV(const nlohmann::json& data);
    void handleBalance(const nlohmann::json& data);
    void handleOrder(const nlohmann::json& data);
    void handleMyTrade(const nlohmann::json& data);
    void handleOrderResponse(const nlohmann::json& data);
};

} // namespace ccxt

#endif // CCXT_KRAKEN_WS_H
