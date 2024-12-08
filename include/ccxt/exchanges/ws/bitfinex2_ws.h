#ifndef CCXT_BITFINEX2_WS_H
#define CCXT_BITFINEX2_WS_H

#include "ccxt/exchange_ws.h"
#include <map>
#include <set>

namespace ccxt {

class bitfinex2_ws : public exchange_ws {
public:
    bitfinex2_ws();
    ~bitfinex2_ws() = default;

protected:
    // Market Data Methods
    virtual void watch_ticker_impl(const std::string& symbol, const json& params) override;
    virtual void watch_trades_impl(const std::string& symbol, const json& params) override;
    virtual void watch_ohlcv_impl(const std::string& symbol, const std::string& timeframe, const json& params) override;
    virtual void watch_order_book_impl(const std::string& symbol, const json& params) override;

    // Private Methods
    virtual void watch_balance_impl(const json& params) override;
    virtual void watch_orders_impl(const std::string& symbol, const json& params) override;
    virtual void watch_my_trades_impl(const std::string& symbol, const json& params) override;
    virtual void watch_positions_impl(const json& params) override;

    // Helper Methods
    virtual std::string get_url() const override;
    virtual void handle_message(const json& message) override;
    virtual void handle_error(const json& message) override;
    virtual void handle_subscription(const json& message) override;
    virtual void authenticate() override;

private:
    static constexpr const char* WS_BASE = "wss://api-pub.bitfinex.com/ws/2";
    static constexpr const char* WS_PRIVATE = "wss://api.bitfinex.com/ws/2";

    // Channel Management
    std::map<int, std::string> channelIds;
    std::map<int, std::string> channelTypes;
    std::map<int, std::string> channelSymbols;
    std::set<std::string> subscribedSymbols;

    // Message Handlers
    void handle_ticker_update(const json& data, const std::string& symbol);
    void handle_trades_update(const json& data, const std::string& symbol);
    void handle_ohlcv_update(const json& data, const std::string& symbol);
    void handle_order_book_update(const json& data, const std::string& symbol);
    void handle_balance_update(const json& data);
    void handle_order_update(const json& data);
    void handle_position_update(const json& data);

    // Helper Methods
    void subscribe_public(const std::string& channel, const std::string& symbol, const json& params = json::object());
    void subscribe_private(const std::string& channel, const std::string& symbol = "", const json& params = json::object());
    std::string get_channel_key(const std::string& channel, const std::string& symbol);
    void parse_channel_id(const json& message);
    std::string get_timeframe_code(const std::string& timeframe);
    
    // Authentication
    std::string sign_request(const json& data);
    long long get_nonce();
};

} // namespace ccxt

#endif // CCXT_BITFINEX2_WS_H
