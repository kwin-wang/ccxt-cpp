#ifndef CCXT_WS_BLOCKCHAINCOM_WS_H
#define CCXT_WS_BLOCKCHAINCOM_WS_H

#include "ccxt/ws_client.h"
#include "../blockchaincom.h"
#include <string>
#include <map>
#include <functional>

namespace ccxt {

class BlockchaincomWS : public WsClient, public blockchaincom {
public:
    BlockchaincomWS(const Config& config);
    ~BlockchaincomWS() override = default;

    // Market Data Streams
    void subscribe_ticker(const std::string& symbol, std::function<void(const json&)> callback);
    void subscribe_orderbook(const std::string& symbol, std::function<void(const json&)> callback);
    void subscribe_trades(const std::string& symbol, std::function<void(const json&)> callback);
    void subscribe_ohlcv(const std::string& symbol, const std::string& timeframe, std::function<void(const json&)> callback);

    // Private Data Streams (requires authentication)
    void subscribe_orders(const std::string& symbol, std::function<void(const json&)> callback);
    void subscribe_balance(std::function<void(const json&)> callback);
    void subscribe_positions(std::function<void(const json&)> callback);
    void subscribe_executions(const std::string& symbol, std::function<void(const json&)> callback);

    // Unsubscribe methods
    void unsubscribe_ticker(const std::string& symbol);
    void unsubscribe_orderbook(const std::string& symbol);
    void unsubscribe_trades(const std::string& symbol);
    void unsubscribe_ohlcv(const std::string& symbol, const std::string& timeframe);
    void unsubscribe_orders(const std::string& symbol);
    void unsubscribe_balance();
    void unsubscribe_positions();
    void unsubscribe_executions(const std::string& symbol);

protected:
    void on_connect() override;
    void on_message(const json& message) override;
    void on_error(const std::string& error) override;
    void on_close() override;
    void authenticate();

private:
    std::map<std::string, std::function<void(const json&)>> callbacks_;
    std::string generate_channel_id(const std::string& channel, const std::string& symbol);
    void handle_ticker_update(const json& data);
    void handle_orderbook_update(const json& data);
    void handle_trades_update(const json& data);
    void handle_ohlcv_update(const json& data);
    void handle_orders_update(const json& data);
    void handle_balance_update(const json& data);
    void handle_positions_update(const json& data);
    void handle_executions_update(const json& data);
    void send_subscribe_message(const std::string& channel, const std::string& symbol = "");
    void send_unsubscribe_message(const std::string& channel, const std::string& symbol = "");
};

} // namespace ccxt

#endif // CCXT_WS_BLOCKCHAINCOM_WS_H
