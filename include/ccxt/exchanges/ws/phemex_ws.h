#ifndef CCXT_PHEMEX_WS_H
#define CCXT_PHEMEX_WS_H

#include "ccxt/websocket_client.h"
#include "ccxt/exchanges/phemex.h"
#include <map>
#include <string>
#include <vector>

namespace ccxt {

class PhemexWS : public WebSocketClient {
public:
    PhemexWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Phemex& exchange);

    // Public API
    void watchTicker(const std::string& symbol, const std::map<std::string, std::string>& params = {});
    void watchTickers(const std::vector<std::string>& symbols, const std::map<std::string, std::string>& params = {});
    void watchOrderBook(const std::string& symbol, int limit = 0, const std::map<std::string, std::string>& params = {});
    void watchTrades(const std::string& symbol, const std::map<std::string, std::string>& params = {});
    void watchOHLCV(const std::string& symbol, const std::string& timeframe = "1m", 
                   const std::map<std::string, std::string>& params = {});

    // Private API
    void watchBalance(const std::map<std::string, std::string>& params = {});
    void watchOrders(const std::string& symbol = "", const std::map<std::string, std::string>& params = {});
    void watchMyTrades(const std::string& symbol = "", const std::map<std::string, std::string>& params = {});
    void watchPositions(const std::string& symbol = "", const std::map<std::string, std::string>& params = {});

protected:
    void authenticate();
    void handleMessage(const std::string& message) override;
    std::string getEndpoint(const std::string& type = "public") override;

private:
    void subscribe(const std::string& channel, const std::string& symbol = "", bool isPrivate = false);
    void subscribeMultiple(const std::string& channel, const std::vector<std::string>& symbols, bool isPrivate = false);
    void unsubscribe(const std::string& channel, const std::string& symbol = "");
    void unsubscribeMultiple(const std::string& channel, const std::vector<std::string>& symbols);

    void handleTickerMessage(const nlohmann::json& data);
    void handleOrderBookMessage(const nlohmann::json& data);
    void handleTradeMessage(const nlohmann::json& data);
    void handleOHLCVMessage(const nlohmann::json& data);
    void handleBalanceMessage(const nlohmann::json& data);
    void handleOrderMessage(const nlohmann::json& data);
    void handleMyTradeMessage(const nlohmann::json& data);
    void handlePositionMessage(const nlohmann::json& data);
    void handleErrorMessage(const nlohmann::json& data);
    void handleSubscriptionMessage(const nlohmann::json& data);
    void handleUnsubscriptionMessage(const nlohmann::json& data);
    void handleAuthenticationMessage(const nlohmann::json& data);

    std::string getMarketId(const std::string& symbol);
    std::string getSymbol(const std::string& marketId);
    std::string getChannel(const std::string& channel, const std::string& symbol);
    std::string getInstrumentType(const std::string& symbol);
    int getNextRequestId();

    Order parseWsOrder(const nlohmann::json& order, const Market* market = nullptr);
    Trade parseWsTrade(const nlohmann::json& trade, const Market* market = nullptr);
    Position parseWsPosition(const nlohmann::json& position, const Market* market = nullptr);

    double parseNumber(const std::string& value, double scale = 1.0);
    std::string formatNumber(double value, double scale = 1.0);

    Phemex& exchange_;
    bool authenticated_;
    std::map<std::string, std::string> subscriptions_;
    std::string sessionId_;
    std::map<std::string, int> scales_;
};

} // namespace ccxt

#endif // CCXT_PHEMEX_WS_H
