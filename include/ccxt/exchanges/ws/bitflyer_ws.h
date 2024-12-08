#ifndef CCXT_BITFLYER_WS_H
#define CCXT_BITFLYER_WS_H

#include "ccxt/exchange_ws.h"
#include <map>
#include <set>

namespace ccxt {

class bitflyer_ws : public exchange_ws {
public:
    bitflyer_ws();
    ~bitflyer_ws() = default;

protected:
    // Market Data Methods
    virtual void watch_ticker_impl(const std::string& symbol, const json& params) override;
    virtual void watch_trades_impl(const std::string& symbol, const json& params) override;
    virtual void watch_order_book_impl(const std::string& symbol, const json& params) override;

    // Private Methods
    virtual void watch_balance_impl(const json& params) override;
    virtual void watch_orders_impl(const std::string& symbol, const json& params) override;
    virtual void watch_my_trades_impl(const std::string& symbol, const json& params) override;

    // Helper Methods
    virtual std::string get_url() const override;
    virtual void handle_message(const json& message) override;
    virtual void handle_error(const json& message) override;
    virtual void handle_subscription(const json& message) override;
    virtual void authenticate() override;

private:
    static constexpr const char* WS_BASE = "wss://ws.lightstream.bitflyer.com";
    static constexpr const char* WS_PRIVATE = "wss://ws.lightstream.bitflyer.com";

    // Channel Management
    std::map<std::string, std::string> channelSymbols;
    std::set<std::string> subscribedChannels;

    // Message Handlers
    void handle_ticker_update(const json& data);
    void handle_trades_update(const json& data);
    void handle_order_book_update(const json& data);
    void handle_order_book_snapshot(const json& data);
    void handle_balance_update(const json& data);
    void handle_order_update(const json& data);
    void handle_trade_update(const json& data);

    // Helper Methods
    void subscribe_public(const std::string& channel, const std::string& symbol);
    void subscribe_private(const std::string& channel, const std::string& symbol = "");
    std::string get_channel_name(const std::string& channel, const std::string& symbol);
    void parse_message_type(const json& message);
    
    // Authentication
    std::string sign_request(const json& data);
    long long get_timestamp();
    std::string get_auth_header();

    // Order Book Management
    std::map<std::string, json> orderbooks;
    void initialize_order_book(const std::string& symbol);
    void update_order_book(const std::string& symbol, const json& delta);
};

} // namespace ccxt

#endif // CCXT_BITFLYER_WS_H
