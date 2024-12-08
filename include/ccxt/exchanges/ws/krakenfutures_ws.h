#ifndef CCXT_KRAKENFUTURES_WS_H
#define CCXT_KRAKENFUTURES_WS_H

#include "websocket_client.h"
#include "../krakenfutures.h"
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

namespace ccxt {

class KrakenFuturesWS : public WebSocketClient {
public:
    KrakenFuturesWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, KrakenFutures& exchange);

    std::string getEndpoint();
    std::string getPrivateEndpoint();
    void authenticate();

    // Public API Methods
    void watchTicker(const std::string& symbol);
    void watchOrderBook(const std::string& symbol, const std::string& limit = "");
    void watchTrades(const std::string& symbol);
    void watchOHLCV(const std::string& symbol, const std::string& timeframe);
    void watchMarkPrice(const std::string& symbol);
    void watchFundingRate(const std::string& symbol);

    // Private API Methods
    void watchBalance();
    void watchOrders();
    void watchMyTrades();
    void watchPositions();
    void createOrder(const std::string& symbol, const std::string& type,
                    const std::string& side, double amount, double price,
                    const std::unordered_map<std::string, std::string>& params = {});
    void editOrder(const std::string& id, const std::string& symbol,
                  const std::string& type, const std::string& side,
                  double amount, double price);
    void cancelOrder(const std::string& id);
    void cancelAllOrders();

protected:
    void handleMessage(const std::string& message) override;

private:
    KrakenFutures& exchange_;
    bool authenticated_ = false;
    std::unordered_map<std::string, nlohmann::json> options_;
    std::unordered_map<std::string, std::string> subscriptions_;

    // Message Handlers
    void handleTicker(const nlohmann::json& data);
    void handleOrderBook(const nlohmann::json& data);
    void handleTrade(const nlohmann::json& data);
    void handleOHLCV(const nlohmann::json& data);
    void handleMarkPrice(const nlohmann::json& data);
    void handleFundingRate(const nlohmann::json& data);
    void handleBalance(const nlohmann::json& data);
    void handleOrder(const nlohmann::json& data);
    void handleMyTrade(const nlohmann::json& data);
    void handlePosition(const nlohmann::json& data);
    void handleOrderResponse(const nlohmann::json& data);

    // Utility Functions
    std::string sign(const std::string& path, const std::string& nonce, const std::string& postData);
    std::string getMarketId(const std::string& symbol);
    std::string getSymbol(const std::string& marketId);
    void subscribe(const std::string& channel, const std::string& symbol = "");
    void unsubscribe(const std::string& channel, const std::string& symbol = "");
};

} // namespace ccxt

#endif // CCXT_KRAKENFUTURES_WS_H
