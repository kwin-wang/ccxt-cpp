#ifndef CCXT_BEQUANT_WS_H
#define CCXT_BEQUANT_WS_H

#include "../../ws_client.h"
#include "../bequant.h"
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

namespace ccxt {

class BequantWS : public WebSocketClient {
public:
    BequantWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Bequant& exchange);
    virtual ~BequantWS() = default;

    // Market Data Methods
    void watchTicker(const std::string& symbol);
    void watchTickers(const std::vector<std::string>& symbols);
    void watchOrderBook(const std::string& symbol, const int limit = 100);
    void watchTrades(const std::string& symbol, const int limit = 100);
    void watchOHLCV(const std::string& symbol, const std::string& timeframe);

    // Private Methods
    void watchBalance();
    void watchOrders(const std::string& symbol = "");
    void watchMyTrades(const std::string& symbol = "");

protected:
    void authenticate();
    void handleMessage(const std::string& message) override;

private:
    Bequant& exchange_;
    std::string apiKey_;
    std::string apiSecret_;
    std::map<std::string, std::string> subscriptions_;
    bool authenticated_;
    int64_t lastNonce_;

    // Message Handlers
    void handleTicker(const nlohmann::json& data);
    void handleOrderBook(const nlohmann::json& data, bool isSnapshot);
    void handleTrade(const nlohmann::json& data);
    void handleOHLCV(const nlohmann::json& data);
    void handleBalance(const nlohmann::json& data);
    void handleOrder(const nlohmann::json& data);
    void handleMyTrade(const nlohmann::json& data);

    // Helper Methods
    void subscribe(const std::string& channel, const std::string& symbol = "", const nlohmann::json& params = nlohmann::json::object());
    void unsubscribe(const std::string& channel, const std::string& symbol = "");
    std::string getStreamUrl(bool isPrivate = false) const;
    int64_t getNonce();
    std::string sign(const std::string& path, const std::string& nonce, const std::string& data = "") const;
};

} // namespace ccxt

#endif // CCXT_BEQUANT_WS_H
