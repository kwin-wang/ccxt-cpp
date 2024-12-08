#ifndef CCXT_WOO_WS_H
#define CCXT_WOO_WS_H

#include "ccxt/client/websocketclient.h"
#include "ccxt/exchanges/woo.h"
#include <string>
#include <map>
#include <vector>

namespace ccxt {

class WooWS : public WebSocketClient {
public:
    WooWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Woo& exchange);

    // Public API methods
    void watchTicker(const std::string& symbol);
    void watchTickers(const std::vector<std::string>& symbols);
    void watchOrderBook(const std::string& symbol, const int limit = 100);
    void watchTrades(const std::string& symbol);
    void watchOHLCV(const std::string& symbol, const std::string& timeframe = "1m");

    // Private API methods
    void watchBalance();
    void watchOrders(const std::string& symbol = "");
    void watchMyTrades(const std::string& symbol = "");

protected:
    std::string getEndpoint(const std::string& type = "");
    void authenticate();
    std::string generateSignature(const std::string& timestamp);
    void ping();

    void subscribe(const std::string& channel, const std::string& symbol = "", bool isPrivate = false);
    void unsubscribe(const std::string& channel, const std::string& symbol = "");

    // Message handlers
    void handleMessage(const std::string& message);
    void handleTickerMessage(const nlohmann::json& data);
    void handleOrderBookMessage(const nlohmann::json& data);
    void handleTradeMessage(const nlohmann::json& data);
    void handleOHLCVMessage(const nlohmann::json& data);
    void handleBalanceMessage(const nlohmann::json& data);
    void handleOrderMessage(const nlohmann::json& data);
    void handleMyTradeMessage(const nlohmann::json& data);
    void handleErrorMessage(const nlohmann::json& data);
    void handleAuthMessage(const nlohmann::json& data);
    void handlePongMessage(const nlohmann::json& data);

private:
    Woo& exchange_;
    bool authenticated_;
    std::map<std::string, std::string> subscriptions_;
    int64_t sequenceNumber_;

    int64_t getNextSequenceNumber();
    std::string normalizeSymbol(const std::string& symbol);
    void startPingLoop();
};

} // namespace ccxt

#endif // CCXT_WOO_WS_H
