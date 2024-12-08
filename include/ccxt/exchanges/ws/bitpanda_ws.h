#ifndef CCXT_EXCHANGE_BITPANDA_WS_H
#define CCXT_EXCHANGE_BITPANDA_WS_H

#include "ccxt/ws_client.h"
#include "../bitpanda.h"
#include <string>
#include <map>
#include <functional>
#include <vector>

namespace ccxt {

class BitpandaWS : public WsClient, public bitpanda {
public:
    BitpandaWS(const Config& config);
    ~BitpandaWS() override = default;

    // Market Data Streams
    void subscribe_ticker(const std::string& symbol, std::function<void(const json&)> callback);
    void subscribe_orderbook(const std::string& symbol, std::function<void(const json&)> callback);
    void subscribe_trades(const std::string& symbol, std::function<void(const json&)> callback);
    void subscribe_candlesticks(const std::string& symbol, const std::string& timeframe, std::function<void(const json&)> callback);
    void subscribe_market_state(const std::string& symbol, std::function<void(const json&)> callback);
    void subscribe_market_ticker(const std::string& symbol, std::function<void(const json&)> callback);

    // Private Data Streams
    void subscribe_account(std::function<void(const json&)> callback);
    void subscribe_orders(std::function<void(const json&)> callback);
    void subscribe_trades_history(std::function<void(const json&)> callback);
    void subscribe_balances(std::function<void(const json&)> callback);

    // Unsubscribe methods
    void unsubscribe_ticker(const std::string& symbol);
    void unsubscribe_orderbook(const std::string& symbol);
    void unsubscribe_trades(const std::string& symbol);
    void unsubscribe_candlesticks(const std::string& symbol, const std::string& timeframe);
    void unsubscribe_market_state(const std::string& symbol);
    void unsubscribe_market_ticker(const std::string& symbol);
    void unsubscribe_account();
    void unsubscribe_orders();
    void unsubscribe_trades_history();
    void unsubscribe_balances();

protected:
    void on_connect() override;
    void on_message(const json& message) override;
    void on_error(const std::string& error) override;
    void on_close() override;
    void authenticate();

private:
    std::map<std::string, std::function<void(const json&)>> callbacks_;
    std::string generate_channel_id(const std::string& channel, const std::string& symbol = "", const std::string& timeframe = "");
    void handle_ticker_update(const json& data);
    void handle_orderbook_update(const json& data);
    void handle_trades_update(const json& data);
    void handle_candlesticks_update(const json& data);
    void handle_market_state_update(const json& data);
    void handle_market_ticker_update(const json& data);
    void handle_account_update(const json& data);
    void handle_orders_update(const json& data);
    void handle_trades_history_update(const json& data);
    void handle_balances_update(const json& data);
    void send_subscribe_message(const std::string& channel, const std::string& symbol = "", const std::string& timeframe = "", bool is_private = false);
    void send_unsubscribe_message(const std::string& channel, const std::string& symbol = "", const std::string& timeframe = "", bool is_private = false);
};

} // namespace ccxt

#endif // CCXT_EXCHANGE_BITPANDA_WS_H
