#ifndef CCXT_EXCHANGE_MEXC_WS_H
#define CCXT_EXCHANGE_MEXC_WS_H

#include "../../websocket_client.h"
#include "../../mexc.h"
#include <string>
#include <map>
#include <vector>

namespace ccxt {

class MexcWS : public WebSocketClient {
public:
    MexcWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Mexc& exchange);
    ~MexcWS() = default;

    // Public API
    void watchTicker(const std::string& symbol, bool miniTicker = false);
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
    Mexc& exchange_;
    std::map<std::string, std::string> subscriptions_;
    std::string listenKey_;
    long long listenKeyExpiry_;
    
    std::string getEndpoint(const std::string& type);
    void subscribe(const std::string& channel, const std::string& symbol, bool isPrivate = false);
    void subscribePublic(const std::string& channel, const std::string& symbol);
    void subscribePrivate(const std::string& channel);
    void authenticate();
    void createListenKey();
    void extendListenKey();
    void handleMessage(const std::string& message);
    void handleTickerMessage(const nlohmann::json& data);
    void handleOrderBookMessage(const nlohmann::json& data);
    void handleTradeMessage(const nlohmann::json& data);
    void handleOHLCVMessage(const nlohmann::json& data);
    void handleBalanceMessage(const nlohmann::json& data);
    void handleOrderMessage(const nlohmann::json& data);
    void handleMyTradeMessage(const nlohmann::json& data);
    void ping();
};

} // namespace ccxt

#endif // CCXT_EXCHANGE_MEXC_WS_H
