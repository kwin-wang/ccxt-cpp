#ifndef CCXT_KUCOINFUTURES_WS_H
#define CCXT_KUCOINFUTURES_WS_H

#include "../../websocket_client.h"
#include "../../kucoinfutures.h"
#include <string>
#include <map>
#include <vector>

namespace ccxt {

class KuCoinFuturesWS : public WebSocketClient {
public:
    KuCoinFuturesWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, KuCoinFutures& exchange);
    ~KuCoinFuturesWS() = default;

    // Public API
    void watchTicker(const std::string& symbol);
    void watchTickers(const std::vector<std::string>& symbols);
    void watchOrderBook(const std::string& symbol);
    void watchTrades(const std::string& symbol);
    void watchOHLCV(const std::string& symbol, const std::string& timeframe);
    void watchMarkPrice(const std::string& symbol);
    void watchFundingRate(const std::string& symbol);
    void watchIndex(const std::string& symbol);
    void watchPremiumIndex(const std::string& symbol);

    // Private API
    void watchBalance();
    void watchOrders();
    void watchMyTrades();
    void watchPositions();

private:
    KuCoinFutures& exchange_;
    std::map<std::string, std::string> subscriptions_;
    int tradesLimit_;
    int snapshotDelay_;
    int snapshotMaxRetries_;
    std::string connectId_;
    std::string token_;
    int64_t pingInterval_;
    int64_t pingTimeout_;

    void authenticate();
    void handleMessage(const std::string& message);
    void ping();
    void negotiate(bool privateChannel);
    std::string getEndpoint(bool privateChannel);
    void subscribe(const std::string& topic, const nlohmann::json& params = nlohmann::json::object());
    void unsubscribe(const std::string& topic, const nlohmann::json& params = nlohmann::json::object());

    // Message Handlers
    void handleTicker(const nlohmann::json& data);
    void handleOrderBook(const nlohmann::json& data);
    void handleTrade(const nlohmann::json& data);
    void handleOHLCV(const nlohmann::json& data);
    void handleMarkPrice(const nlohmann::json& data);
    void handleFundingRate(const nlohmann::json& data);
    void handleIndex(const nlohmann::json& data);
    void handlePremiumIndex(const nlohmann::json& data);
    void handleBalance(const nlohmann::json& data);
    void handleOrder(const nlohmann::json& data);
    void handleMyTrade(const nlohmann::json& data);
    void handlePosition(const nlohmann::json& data);
};

} // namespace ccxt

#endif // CCXT_KUCOINFUTURES_WS_H
