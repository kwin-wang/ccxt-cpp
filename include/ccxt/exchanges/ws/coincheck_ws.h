#ifndef CCXT_EXCHANGE_COINCHECK_WS_H
#define CCXT_EXCHANGE_COINCHECK_WS_H

#include "../../websocket_client.h"
#include "../../coincheck.h"
#include <string>
#include <map>
#include <vector>

namespace ccxt {

class CoincheckWS : public WebSocketClient {
public:
    CoincheckWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Coincheck& exchange);
    ~CoincheckWS() = default;

    // Public API
    void watchOrderBook(const std::string& symbol, const std::map<std::string, std::string>& params = {});
    void watchTrades(const std::string& symbol, const std::map<std::string, std::string>& params = {});

    // Unsubscribe methods
    void unsubscribe(const std::string& channel, const std::string& symbol = "");
    void unsubscribeAll();

private:
    Coincheck& exchange_;
    std::map<std::string, std::string> subscriptions_;
    std::map<std::string, OrderBook> orderbooks_;
    std::map<std::string, ArrayCache<Trade>> trades_;
    
    // Helper methods
    void subscribe(const std::string& channel, const std::string& symbol = "");
    std::string getEndpoint();
    std::string getMarketId(const std::string& symbol);
    std::string getSymbol(const std::string& marketId);
    std::string getChannel(const std::string& channel, const std::string& symbol);

    // Message handlers
    void handleMessage(const std::string& message);
    void handleOrderBookMessage(const nlohmann::json& data);
    void handleTradeMessage(const nlohmann::json& data);
    void handleErrorMessage(const nlohmann::json& data);
    void handleSubscriptionMessage(const nlohmann::json& data);
    void handleUnsubscriptionMessage(const nlohmann::json& data);

    // Market parsing
    Trade parseWsTrade(const nlohmann::json& trade, const Market* market = nullptr);
    std::map<std::string, std::string> parseMarket(const std::string& marketId);
};

} // namespace ccxt

#endif // CCXT_EXCHANGE_COINCHECK_WS_H
