#ifndef CCXT_LBANK_WS_H
#define CCXT_LBANK_WS_H

#include "../../websocket_client.h"
#include "../../lbank.h"
#include <string>
#include <map>
#include <vector>

namespace ccxt {

class LBankWS : public WebSocketClient {
public:
    LBankWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, LBank& exchange);
    ~LBankWS() = default;

    // Public API
    void watchTicker(const std::string& symbol);
    void watchOrderBook(const std::string& symbol, const std::string& limit = "");
    void watchTrades(const std::string& symbol);
    void watchOHLCV(const std::string& symbol, const std::string& timeframe);

    // Private API
    void watchBalance();
    void watchOrders();
    void watchMyTrades();

private:
    LBank& exchange_;
    std::map<std::string, std::string> subscriptions_;
    int tradesLimit_;
    bool authenticated_;

    void authenticate();
    void handleMessage(const std::string& message);
    void ping();
    void subscribe(const std::string& channel, const std::string& symbol);
    void unsubscribe(const std::string& channel, const std::string& symbol);

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

#endif // CCXT_LBANK_WS_H
