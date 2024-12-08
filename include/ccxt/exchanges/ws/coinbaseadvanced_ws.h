#ifndef CCXT_EXCHANGE_COINBASEADVANCED_WS_H
#define CCXT_EXCHANGE_COINBASEADVANCED_WS_H

#include "ccxt/ws_client.h"
#include "../coinbaseadvanced.h"
#include <string>
#include <map>
#include <functional>
#include <vector>

namespace ccxt {

class CoinbaseadvancedWS : public WsClient, public coinbaseadvanced {
public:
    CoinbaseadvancedWS(const Config& config);
    ~CoinbaseadvancedWS() override = default;

    // Market Data Streams
    void subscribe_ticker(const std::string& symbol, std::function<void(const json&)> callback);
    void subscribe_orderbook(const std::string& symbol, std::function<void(const json&)> callback);
    void subscribe_trades(const std::string& symbol, std::function<void(const json&)> callback);
    void subscribe_candles(const std::string& symbol, const std::string& interval, std::function<void(const json&)> callback);
    void subscribe_level2(const std::string& symbol, std::function<void(const json&)> callback);
    void subscribe_status(std::function<void(const json&)> callback);

    // Private Data Streams
    void subscribe_user(std::function<void(const json&)> callback);
    void subscribe_orders(std::function<void(const json&)> callback);
    void subscribe_fills(std::function<void(const json&)> callback);
    void subscribe_matches(std::function<void(const json&)> callback);

    // Trading Operations
    void place_order(const std::string& symbol, const std::string& side, const std::string& type,
                    double quantity, double price = 0.0, const std::map<std::string, std::string>& params = {});
    void cancel_order(const std::string& order_id);
    void cancel_all_orders(const std::string& symbol = "");
    void modify_order(const std::string& order_id, const std::string& symbol,
                     double quantity, double price);

    // Unsubscribe methods
    void unsubscribe_ticker(const std::string& symbol);
    void unsubscribe_orderbook(const std::string& symbol);
    void unsubscribe_trades(const std::string& symbol);
    void unsubscribe_candles(const std::string& symbol, const std::string& interval);
    void unsubscribe_level2(const std::string& symbol);
    void unsubscribe_status();
    void unsubscribe_user();
    void unsubscribe_orders();
    void unsubscribe_fills();
    void unsubscribe_matches();

protected:
    void on_connect() override;
    void on_message(const json& message) override;
    void on_error(const std::string& error) override;
    void on_close() override;
    void authenticate();

private:
    std::map<std::string, std::function<void(const json&)>> callbacks_;
    std::string generate_channel_id(const std::string& channel, const std::string& symbol = "", const std::string& interval = "");
    void handle_ticker_update(const json& data);
    void handle_orderbook_update(const json& data);
    void handle_trades_update(const json& data);
    void handle_candles_update(const json& data);
    void handle_level2_update(const json& data);
    void handle_status_update(const json& data);
    void handle_user_update(const json& data);
    void handle_orders_update(const json& data);
    void handle_fills_update(const json& data);
    void handle_matches_update(const json& data);
    void send_subscribe_message(const std::string& channel, const json& params);
    void send_unsubscribe_message(const std::string& channel, const json& params);
    void send_authenticated_request(const std::string& type, const json& params);
    std::string sign_request(const std::string& timestamp, const std::string& method, const std::string& path, const std::string& body = "");
};

} // namespace ccxt

#endif // CCXT_EXCHANGE_COINBASEADVANCED_WS_H
