#ifndef CCXT_EXCHANGE_GEMINI_WS_H
#define CCXT_EXCHANGE_GEMINI_WS_H

#include "../../websocket_client.h"
#include "../../gemini.h"
#include <string>
#include <map>
#include <vector>

namespace ccxt {

class GeminiWS : public WebSocketClient {
public:
    GeminiWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Gemini& exchange);
    ~GeminiWS() = default;

    // Public API
    void watchOrderBook(const std::string& symbol);
    void watchOrderBookForSymbols(const std::vector<std::string>& symbols);
    void watchTrades(const std::string& symbol);
    void watchTradesForSymbols(const std::vector<std::string>& symbols);
    void watchBidsAsks(const std::string& symbol);
    void watchOHLCV(const std::string& symbol, const std::string& timeframe);

    // Private API
    void watchOrders();

private:
    Gemini& exchange_;
    std::map<std::string, std::string> subscriptions_;
    std::map<std::string, int> limits_;
    bool newUpdates_;

    void authenticate();
    void handleMessage(const std::string& message);
    std::string getEndpoint();
    void subscribe(const std::string& channel, const nlohmann::json& params = nlohmann::json::object());
    void unsubscribe(const std::string& channel, const nlohmann::json& params = nlohmann::json::object());
    void handleTradeMessage(const nlohmann::json& message);
    void handleOrderBookMessage(const nlohmann::json& message);
    void handleOrderMessage(const nlohmann::json& message);
};

} // namespace ccxt

#endif // CCXT_EXCHANGE_GEMINI_WS_H
