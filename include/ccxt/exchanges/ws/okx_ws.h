#ifndef CCXT_OKX_WS_H
#define CCXT_OKX_WS_H

#include "websocket_client.h"
#include "../okx.h"
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

namespace ccxt {

class OKXWS : public WebSocketClient {
public:
    OKXWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, OKX& exchange);

    std::string getEndpoint();
    void authenticate();

    // Market Data Methods
    void watchTicker(const std::string& symbol);
    void watchTickers(const std::vector<std::string>& symbols);
    void watchOrderBook(const std::string& symbol, const std::string& depth = "books");
    void watchTrades(const std::string& symbol);
    void watchOHLCV(const std::string& symbol, const std::string& timeframe);
    void watchMarkPrice(const std::string& symbol);
    void watchMarkPrices(const std::vector<std::string>& symbols);
    void watchFundingRate(const std::string& symbol);
    void watchFundingRates(const std::vector<std::string>& symbols);
    void watchLiquidations(const std::string& symbol);

    // Private Methods
    void watchBalance(const std::string& type = "spot");
    void watchOrders(const std::string& type = "ANY");
    void watchMyTrades(const std::string& type = "ANY");
    void watchPositions();
    void watchMyLiquidations();

    // Trading Methods
    void createOrder(const std::string& symbol, const std::string& type,
                    const std::string& side, double amount, double price = 0.0);
    void editOrder(const std::string& id, const std::string& symbol,
                  const std::string& type, const std::string& side,
                  double amount, double price = 0.0);
    void cancelOrder(const std::string& id, const std::string& symbol);
    void cancelOrders(const std::vector<std::string>& ids, const std::string& symbol);
    void cancelAllOrders(const std::string& symbol);

protected:
    void handleMessage(const std::string& message) override;

private:
    OKX& exchange_;
    bool authenticated_ = false;
    std::unordered_map<std::string, nlohmann::json> options_;
    std::unordered_map<std::string, std::string> subscriptions_;

    // Subscription Methods
    void subscribe(const std::string& channel, const std::string& instId,
                  const nlohmann::json& args = nlohmann::json::object());
    void unsubscribe(const std::string& channel, const std::string& instId);
    std::string sign(const std::string& timestamp, const std::string& method,
                    const std::string& path, const std::string& body = "");

    // Message Handlers
    void handleTicker(const nlohmann::json& data);
    void handleOrderBook(const nlohmann::json& data);
    void handleTrade(const nlohmann::json& data);
    void handleOHLCV(const nlohmann::json& data);
    void handleMarkPrice(const nlohmann::json& data);
    void handleFundingRate(const nlohmann::json& data);
    void handleLiquidation(const nlohmann::json& data);
    void handleBalance(const nlohmann::json& data);
    void handleOrder(const nlohmann::json& data);
    void handleMyTrade(const nlohmann::json& data);
    void handlePosition(const nlohmann::json& data);
    void handleMyLiquidation(const nlohmann::json& data);
    void handleError(const nlohmann::json& data);
};

} // namespace ccxt

#endif // CCXT_OKX_WS_H
