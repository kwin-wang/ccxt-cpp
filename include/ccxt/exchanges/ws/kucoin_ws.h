#ifndef CCXT_EXCHANGE_KUCOIN_WS_H
#define CCXT_EXCHANGE_KUCOIN_WS_H

#include "../../websocket_client.h"
#include "../../kucoin.h"
#include <string>
#include <map>
#include <vector>

namespace ccxt {

class KucoinWS : public WebSocketClient {
public:
    KucoinWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Kucoin& exchange);
    ~KucoinWS() = default;

    // Public API
    void watchTicker(const std::string& symbol);
    void watchTickers(const std::vector<std::string>& symbols);
    void watchOrderBook(const std::string& symbol);
    void watchTrades(const std::string& symbol);
    void watchOHLCV(const std::string& symbol, const std::string& timeframe);
    void watchBidsAsks(const std::string& symbol);

    // Private API
    void watchBalance();
    void watchOrders();
    void watchMyTrades();

private:
    Kucoin& exchange_;
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
};

} // namespace ccxt

#endif // CCXT_EXCHANGE_KUCOIN_WS_H
