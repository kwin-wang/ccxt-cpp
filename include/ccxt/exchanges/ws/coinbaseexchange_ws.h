#ifndef CCXT_EXCHANGE_COINBASEEXCHANGE_WS_H
#define CCXT_EXCHANGE_COINBASEEXCHANGE_WS_H

#include "ccxt/ws_client.h"
#include "../coinbaseexchange.h"
#include <string>
#include <map>
#include <functional>
#include <vector>

namespace ccxt {

class CoinbaseExchangeWS : public WsClient, public coinbaseexchange {
public:
    CoinbaseExchangeWS(const Config& config);
    ~CoinbaseExchangeWS() override = default;

    // Market Data Streams
    void subscribe_ticker(const std::string& symbol, std::function<void(const json&)> callback);
    void subscribe_tickers(const std::vector<std::string>& symbols, std::function<void(const json&)> callback);
    void subscribe_orderbook(const std::string& symbol, std::function<void(const json&)> callback);
    void subscribe_trades(const std::string& symbol, std::function<void(const json&)> callback);
    void subscribe_level2(const std::string& symbol, std::function<void(const json&)> callback);
    void subscribe_status(std::function<void(const json&)> callback);
    void subscribe_heartbeat(const std::string& symbol, std::function<void(const json&)> callback);

    // Private Data Streams
    void subscribe_user(std::function<void(const json&)> callback);
    void subscribe_orders(const std::string& symbol, std::function<void(const json&)> callback);
    void subscribe_matches(const std::string& symbol, std::function<void(const json&)> callback);
    void subscribe_full(const std::string& symbol, std::function<void(const json&)> callback);

    // Unsubscribe methods
    void unsubscribe_ticker(const std::string& symbol);
    void unsubscribe_orderbook(const std::string& symbol);
    void unsubscribe_trades(const std::string& symbol);
    void unsubscribe_level2(const std::string& symbol);
    void unsubscribe_status();
    void unsubscribe_heartbeat(const std::string& symbol);
    void unsubscribe_user();
    void unsubscribe_orders(const std::string& symbol);
    void unsubscribe_matches(const std::string& symbol);
    void unsubscribe_full(const std::string& symbol);

protected:
    void on_connect() override;
    void on_message(const json& message) override;
    void on_error(const std::string& error) override;
    void on_close() override;
    void authenticate();

private:
    std::map<std::string, std::function<void(const json&)>> callbacks_;
    std::string generate_channel_id(const std::string& channel, const std::string& symbol = "");
    void handle_ticker_update(const json& data);
    void handle_orderbook_update(const json& data);
    void handle_trades_update(const json& data);
    void handle_level2_update(const json& data);
    void handle_status_update(const json& data);
    void handle_heartbeat_update(const json& data);
    void handle_user_update(const json& data);
    void handle_orders_update(const json& data);
    void handle_matches_update(const json& data);
    void handle_full_update(const json& data);
    void send_subscribe_message(const std::string& channel, const std::string& symbol = "", bool is_private = false);
    void send_unsubscribe_message(const std::string& channel, const std::string& symbol = "", bool is_private = false);
};

} // namespace ccxt

#endif // CCXT_EXCHANGE_COINBASEEXCHANGE_WS_H
