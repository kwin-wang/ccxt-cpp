#ifndef CCXT_EXCHANGE_CURRENCYCOM_WS_H
#define CCXT_EXCHANGE_CURRENCYCOM_WS_H

#include "ccxt/ws_client.h"
#include "../currencycom.h"
#include <string>
#include <map>
#include <functional>
#include <vector>

namespace ccxt {

class CurrencycomWS : public WsClient, public currencycom {
public:
    CurrencycomWS(const Config& config);
    ~CurrencycomWS() override = default;

    // Market Data Streams
    void subscribe_ticker(const std::string& symbol, std::function<void(const json&)> callback);
    void subscribe_miniTicker(const std::string& symbol, std::function<void(const json&)> callback);
    void subscribe_orderbook(const std::string& symbol, std::function<void(const json&)> callback);
    void subscribe_trades(const std::string& symbol, std::function<void(const json&)> callback);
    void subscribe_kline(const std::string& symbol, const std::string& interval, std::function<void(const json&)> callback);
    void subscribe_aggTrades(const std::string& symbol, std::function<void(const json&)> callback);

    // Private Data Streams
    void subscribe_account(std::function<void(const json&)> callback);
    void subscribe_orders(std::function<void(const json&)> callback);
    void subscribe_balance(std::function<void(const json&)> callback);
    void subscribe_positions(std::function<void(const json&)> callback);

    // Trading Operations
    void place_order(const std::string& symbol, const std::string& side, const std::string& type,
                    double quantity, double price = 0.0, const std::map<std::string, std::string>& params = {});
    void cancel_order(const std::string& order_id, const std::string& symbol);
    void cancel_all_orders(const std::string& symbol = "");
    void modify_order(const std::string& order_id, const std::string& symbol,
                     double quantity, double price);

    // Unsubscribe methods
    void unsubscribe_ticker(const std::string& symbol);
    void unsubscribe_miniTicker(const std::string& symbol);
    void unsubscribe_orderbook(const std::string& symbol);
    void unsubscribe_trades(const std::string& symbol);
    void unsubscribe_kline(const std::string& symbol, const std::string& interval);
    void unsubscribe_aggTrades(const std::string& symbol);
    void unsubscribe_account();
    void unsubscribe_orders();
    void unsubscribe_balance();
    void unsubscribe_positions();

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
    void handle_miniTicker_update(const json& data);
    void handle_orderbook_update(const json& data);
    void handle_trades_update(const json& data);
    void handle_kline_update(const json& data);
    void handle_aggTrades_update(const json& data);
    void handle_account_update(const json& data);
    void handle_orders_update(const json& data);
    void handle_balance_update(const json& data);
    void handle_positions_update(const json& data);
    void send_subscribe_message(const std::string& method, const json& params);
    void send_unsubscribe_message(const std::string& method, const json& params);
    void send_authenticated_request(const std::string& method, const json& params);
    std::string sign_request(const std::string& timestamp, const std::string& method, const std::string& path, const std::string& body = "");
};

} // namespace ccxt

#endif // CCXT_EXCHANGE_CURRENCYCOM_WS_H
