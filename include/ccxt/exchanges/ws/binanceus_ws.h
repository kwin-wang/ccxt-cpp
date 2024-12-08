#ifndef CCXT_BINANCEUS_WS_H
#define CCXT_BINANCEUS_WS_H

#include "../../ws_client.h"
#include "../binanceus.h"
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

namespace ccxt {

class BinanceUSWS : public WebSocketClient {
public:
    BinanceUSWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, BinanceUS& exchange);
    virtual ~BinanceUSWS() = default;

    // Market Data Methods
    void watchTicker(const std::string& symbol);
    void watchTickers(const std::vector<std::string>& symbols);
    void watchOrderBook(const std::string& symbol, const int limit = 100);
    void watchTrades(const std::string& symbol);
    void watchOHLCV(const std::string& symbol, const std::string& timeframe);
    void watchBidsAsks(const std::string& symbol);

    // Private Methods
    void watchBalance();
    void watchOrders(const std::string& symbol = "");
    void watchMyTrades(const std::string& symbol = "");

protected:
    void authenticate();
    void handleMessage(const std::string& message) override;
    void listenKey();
    void startListenKeyTimer();

private:
    BinanceUS& exchange_;
    std::string apiKey_;
    std::string apiSecret_;
    std::map<std::string, std::string> subscriptions_;
    std::string listenKey_;
    bool authenticated_;
    int64_t lastPingTimestamp_;
    int64_t pingInterval_;

    // Message Handlers
    void handleTicker(const nlohmann::json& data);
    void handleOrderBook(const nlohmann::json& data, bool isSnapshot);
    void handleTrade(const nlohmann::json& data);
    void handleOHLCV(const nlohmann::json& data);
    void handleBalance(const nlohmann::json& data);
    void handleOrder(const nlohmann::json& data);
    void handleMyTrade(const nlohmann::json& data);
    void handleOutboundAccountInfo(const nlohmann::json& data);
    void handleExecutionReport(const nlohmann::json& data);

    // Helper Methods
    void subscribe(const std::string& channel, const std::string& symbol = "", const nlohmann::json& params = nlohmann::json::object());
    void unsubscribe(const std::string& channel, const std::string& symbol = "");
    std::string getStreamUrl(bool isPrivate = false) const;
    std::string sign(const std::string& path, const std::string& method = "GET", const nlohmann::json& params = nlohmann::json::object()) const;
    void ping();
    void startPingLoop();
    std::string getSymbol(const std::string& market) const;
};

} // namespace ccxt

#endif // CCXT_BINANCEUS_WS_H
