#ifndef CCXT_ALPACA_WS_H
#define CCXT_ALPACA_WS_H

#include "../../ws_client.h"
#include "../alpaca.h"
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

namespace ccxt {

class AlpacaWS : public WebSocketClient {
public:
    AlpacaWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Alpaca& exchange);
    virtual ~AlpacaWS() = default;

    // Market Data Methods
    void watchTicker(const std::string& symbol);
    void watchTickers(const std::vector<std::string>& symbols);
    void watchOrderBook(const std::string& symbol);
    void watchTrades(const std::string& symbol);
    void watchBidsAsks(const std::string& symbol);
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
    Alpaca& exchange_;
    std::string apiKey_;
    std::string apiSecret_;
    std::map<std::string, std::string> subscriptions_;
    int tradesLimit_;

    // Message Handlers
    void handleTrade(const nlohmann::json& data);
    void handleQuote(const nlohmann::json& data);
    void handleBar(const nlohmann::json& data);
    void handleTradingStatus(const nlohmann::json& data);
    void handleLuldBand(const nlohmann::json& data);
    void handleOrder(const nlohmann::json& data);
    void handlePosition(const nlohmann::json& data);
    void handleTrade(const nlohmann::json& data, bool isPrivate);
    void handleBalance(const nlohmann::json& data);

    // Helper Methods
    void subscribe(const std::string& channel, const std::vector<std::string>& symbols = {});
    void unsubscribe(const std::string& channel, const std::vector<std::string>& symbols = {});
    std::string getStreamUrl(bool isPrivate = false) const;
};

} // namespace ccxt

#endif // CCXT_ALPACA_WS_H
