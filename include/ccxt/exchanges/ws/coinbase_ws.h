#ifndef CCXT_EXCHANGE_COINBASE_WS_H
#define CCXT_EXCHANGE_COINBASE_WS_H

#include "../../websocket_client.h"
#include "../../coinbase.h"
#include <string>
#include <map>
#include <vector>

namespace ccxt {

class CoinbaseWS : public WebSocketClient {
public:
    CoinbaseWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Coinbase& exchange);
    ~CoinbaseWS() = default;

    // Public API
    void watchTicker(const std::string& symbol);
    void watchTickers(const std::vector<std::string>& symbols);
    void watchOrderBook(const std::string& symbol);
    void watchOrderBookForSymbols(const std::vector<std::string>& symbols);
    void watchTrades(const std::string& symbol);
    void watchTradesForSymbols(const std::vector<std::string>& symbols);

    // Private API
    void watchOrders();

private:
    Coinbase& exchange_;
    std::map<std::string, std::string> subscriptions_;
    
    void subscribe(const std::string& channel, const std::string& symbol = "", bool isPrivate = false);
    void subscribeMultiple(const std::string& channel, const std::vector<std::string>& symbols, bool isPrivate = false);
    void authenticate(const std::string& channel, const std::vector<std::string>& productIds);
    void handleMessage(const std::string& message);
    void handleTickerMessage(const nlohmann::json& data);
    void handleOrderBookMessage(const nlohmann::json& data);
    void handleTradeMessage(const nlohmann::json& data);
    void handleOrderMessage(const nlohmann::json& data);
};

} // namespace ccxt

#endif // CCXT_EXCHANGE_COINBASE_WS_H
