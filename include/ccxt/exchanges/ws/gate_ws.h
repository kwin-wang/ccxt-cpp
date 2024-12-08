#ifndef CCXT_EXCHANGE_GATE_WS_H
#define CCXT_EXCHANGE_GATE_WS_H

#include "../../websocket_client.h"
#include "../../gate.h"
#include <string>
#include <map>
#include <vector>

namespace ccxt {

class GateWS : public WebSocketClient {
public:
    GateWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Gate& exchange);
    ~GateWS() = default;

    // Public API
    void watchTicker(const std::string& symbol);
    void watchTickers(const std::vector<std::string>& symbols);
    void watchOrderBook(const std::string& symbol);
    void watchTrades(const std::string& symbol);
    void watchTradesForSymbols(const std::vector<std::string>& symbols);
    void watchOHLCV(const std::string& symbol, const std::string& timeframe);

    // Private API
    void watchBalance();
    void watchOrders();
    void watchMyTrades();
    void watchPositions();
    void watchMyLiquidations();

    // Trading API
    void createOrder(const std::string& symbol, const std::string& type, const std::string& side,
                    double amount, double price = 0.0, const std::map<std::string, std::string>& params = {});
    void cancelOrder(const std::string& id, const std::string& symbol);
    void cancelAllOrders(const std::string& symbol = "");
    void editOrder(const std::string& id, const std::string& symbol, const std::string& type,
                  const std::string& side, double amount, double price = 0.0);

private:
    Gate& exchange_;
    std::map<std::string, std::string> subscriptions_;
    
    std::string getEndpoint(const std::string& type, const std::string& settle = "");
    void subscribe(const std::string& channel, const std::string& symbol, const std::string& settle = "");
    void subscribePrivate(const std::string& channel, const std::string& settle = "");
    void authenticate(const std::string& channel);
    void handleMessage(const std::string& message);
    void handleTickerMessage(const nlohmann::json& data);
    void handleOrderBookMessage(const nlohmann::json& data);
    void handleTradeMessage(const nlohmann::json& data);
    void handleOHLCVMessage(const nlohmann::json& data);
    void handleBalanceMessage(const nlohmann::json& data);
    void handleOrderMessage(const nlohmann::json& data);
    void handlePositionMessage(const nlohmann::json& data);
    void handleLiquidationMessage(const nlohmann::json& data);
};

} // namespace ccxt

#endif // CCXT_EXCHANGE_GATE_WS_H
