#ifndef CCXT_ASCENDEX_WS_H
#define CCXT_ASCENDEX_WS_H

#include "../../ws_client.h"
#include "../ascendex.h"
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

namespace ccxt {

class AscendexWS : public WebSocketClient {
public:
    AscendexWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Ascendex& exchange);
    virtual ~AscendexWS() = default;

    // Market Data Methods
    void watchTicker(const std::string& symbol);
    void watchTickers(const std::vector<std::string>& symbols);
    void watchOrderBook(const std::string& symbol);
    void watchTrades(const std::string& symbol);
    void watchOHLCV(const std::string& symbol, const std::string& timeframe);

    // Private Methods
    void watchBalance();
    void watchOrders();
    void watchMyTrades();
    void watchPositions();

protected:
    void authenticate();
    void handleMessage(const std::string& message) override;

private:
    Ascendex& exchange_;
    std::string apiKey_;
    std::string apiSecret_;
    std::map<std::string, std::string> subscriptions_;
    int64_t lastPingTimestamp_;
    int64_t pingInterval_;
    bool authenticated_;

    // Message Handlers
    void handleTicker(const nlohmann::json& data);
    void handleOrderBook(const nlohmann::json& data);
    void handleTrade(const nlohmann::json& data);
    void handleOHLCV(const nlohmann::json& data);
    void handleBalance(const nlohmann::json& data);
    void handleOrder(const nlohmann::json& data);
    void handleMyTrade(const nlohmann::json& data);
    void handlePosition(const nlohmann::json& data);

    // Helper Methods
    void subscribe(const std::string& channel, const std::vector<std::string>& symbols = {}, bool isPrivate = false);
    void unsubscribe(const std::string& channel, const std::vector<std::string>& symbols = {}, bool isPrivate = false);
    std::string getStreamUrl(bool isPrivate = false) const;
    std::string sign(const std::string& message) const;
    void ping();
    void startPingLoop();
};

} // namespace ccxt

#endif // CCXT_ASCENDEX_WS_H
