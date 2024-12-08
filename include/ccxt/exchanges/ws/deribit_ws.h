#ifndef CCXT_EXCHANGE_DERIBIT_WS_H
#define CCXT_EXCHANGE_DERIBIT_WS_H

#include "../../websocket_client.h"
#include "../../deribit.h"
#include <string>
#include <map>
#include <vector>

namespace ccxt {

class DeribitWS : public WebSocketClient {
public:
    DeribitWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Deribit& exchange);
    ~DeribitWS() = default;

    // Public API
    void watchTicker(const std::string& symbol, const std::map<std::string, std::string>& params = {});
    void watchTickers(const std::vector<std::string>& symbols, const std::map<std::string, std::string>& params = {});
    void watchOrderBook(const std::string& symbol, int limit = 50, const std::map<std::string, std::string>& params = {});
    void watchOrderBookForSymbols(const std::vector<std::string>& symbols, int limit = 50, const std::map<std::string, std::string>& params = {});
    void watchTrades(const std::string& symbol, const std::map<std::string, std::string>& params = {});
    void watchTradesForSymbols(const std::vector<std::string>& symbols, const std::map<std::string, std::string>& params = {});
    void watchOHLCV(const std::string& symbol, const std::string& timeframe = "1m", const std::map<std::string, std::string>& params = {});
    void watchOHLCVForSymbols(const std::vector<std::string>& symbols, const std::string& timeframe = "1m", const std::map<std::string, std::string>& params = {});
    void watchBidsAsks(const std::vector<std::string>& symbols, const std::map<std::string, std::string>& params = {});

    // Private API
    void watchBalance(const std::map<std::string, std::string>& params = {});
    void watchOrders(const std::string& symbol = "", const std::map<std::string, std::string>& params = {});
    void watchMyTrades(const std::string& symbol = "", const std::map<std::string, std::string>& params = {});

private:
    Deribit& exchange_;
    std::map<std::string, std::string> subscriptions_;
    std::map<std::string, OrderBook> orderbooks_;
    std::map<std::string, ArrayCache<Trade>> trades_;
    std::map<std::string, Ticker> tickers_;
    bool authenticated_;
    int requestId_;
    
    // Helper methods
    void authenticate();
    std::string sign(const std::string& path, const std::string& method, const std::map<std::string, std::string>& params = {});
    void subscribe(const std::string& channel, const std::string& symbol = "", bool isPrivate = false);
    void subscribeMultiple(const std::string& channel, const std::vector<std::string>& symbols, bool isPrivate = false);
    void unsubscribe(const std::string& channel, const std::string& symbol = "");
    void unsubscribeMultiple(const std::string& channel, const std::vector<std::string>& symbols);
    std::string getEndpoint(const std::string& type = "public");
    std::string getMarketId(const std::string& symbol);
    std::string getSymbol(const std::string& marketId);
    std::string getChannel(const std::string& channel, const std::string& symbol);
    int getNextRequestId();

    // Message handlers
    void handleMessage(const std::string& message);
    void handleTickerMessage(const nlohmann::json& data);
    void handleOrderBookMessage(const nlohmann::json& data);
    void handleTradeMessage(const nlohmann::json& data);
    void handleOHLCVMessage(const nlohmann::json& data);
    void handleBalanceMessage(const nlohmann::json& data);
    void handleOrderMessage(const nlohmann::json& data);
    void handleMyTradeMessage(const nlohmann::json& data);
    void handleErrorMessage(const nlohmann::json& data);
    void handleSubscriptionMessage(const nlohmann::json& data);
    void handleUnsubscriptionMessage(const nlohmann::json& data);
    void handleAuthenticationMessage(const nlohmann::json& data);

    // Market parsing
    Ticker parseWsTicker(const nlohmann::json& ticker, const Market* market = nullptr);
    Trade parseWsTrade(const nlohmann::json& trade, const Market* market = nullptr);
    std::map<std::string, std::string> parseMarket(const std::string& marketId);
};

} // namespace ccxt

#endif // CCXT_EXCHANGE_DERIBIT_WS_H
