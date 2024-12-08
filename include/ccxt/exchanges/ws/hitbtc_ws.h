#ifndef CCXT_EXCHANGE_HITBTC_WS_H
#define CCXT_EXCHANGE_HITBTC_WS_H

#include "ccxt/ws_client.h"
#include "../hitbtc.h"
#include <string>
#include <map>
#include <functional>
#include <vector>

namespace ccxt {

class HitbtcWS : public WsClient, public hitbtc {
public:
    HitbtcWS(const Config& config);
    ~HitbtcWS() override = default;

    // Market Data Streams
    void subscribe_ticker(const std::string& symbol, std::function<void(const json&)> callback);
    void subscribe_orderbook(const std::string& symbol, std::function<void(const json&)> callback);
    void subscribe_trades(const std::string& symbol, std::function<void(const json&)> callback);
    void subscribe_candles(const std::string& symbol, const std::string& timeframe, std::function<void(const json&)> callback);
    void subscribe_mini_ticker(const std::string& symbol, std::function<void(const json&)> callback);

    // Private Data Streams
    void subscribe_reports(std::function<void(const json&)> callback);
    void subscribe_trading(std::function<void(const json&)> callback);
    void subscribe_account(std::function<void(const json&)> callback);
    void subscribe_transactions(std::function<void(const json&)> callback);

    // Trading Operations
    void place_order(const std::string& symbol, const std::string& side, const std::string& type,
                    double quantity, double price = 0.0, const std::map<std::string, std::string>& params = {});
    void cancel_order(const std::string& order_id);
    void cancel_all_orders(const std::string& symbol = "");
    void replace_order(const std::string& order_id, const std::string& symbol,
                      const std::string& side, const std::string& type,
                      double quantity, double price = 0.0);

    // Unsubscribe methods
    void unsubscribe_ticker(const std::string& symbol);
    void unsubscribe_orderbook(const std::string& symbol);
    void unsubscribe_trades(const std::string& symbol);
    void unsubscribe_candles(const std::string& symbol, const std::string& timeframe);
    void unsubscribe_mini_ticker(const std::string& symbol);
    void unsubscribe_reports();
    void unsubscribe_trading();
    void unsubscribe_account();
    void unsubscribe_transactions();

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
    void handle_candles_update(const json& data);
    void handle_mini_ticker_update(const json& data);
    void handle_reports_update(const json& data);
    void handle_trading_update(const json& data);
    void handle_account_update(const json& data);
    void handle_transactions_update(const json& data);
    void send_subscribe_message(const std::string& method, const json& params);
    void send_unsubscribe_message(const std::string& method, const json& params);
    void send_authenticated_request(const std::string& method, const json& params);
};

} // namespace ccxt

#endif // CCXT_EXCHANGE_HITBTC_WS_H
