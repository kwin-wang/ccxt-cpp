#ifndef CCXT_BITCOINCOM_WS_H
#define CCXT_BITCOINCOM_WS_H

#include "ccxt/exchange_ws.h"

namespace ccxt {

class bitcoincom_ws : public exchange_ws {
public:
    bitcoincom_ws();
    ~bitcoincom_ws() = default;

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

    // Helper Methods
    virtual std::string get_url() const override;
    virtual void handle_message(const json& message) override;
    virtual void handle_error(const json& message) override;
    virtual void handle_subscription(const json& message) override;
    virtual void authenticate() override;

    // Message Handlers
    void handle_ticker(const json& message);
    void handle_trade(const json& message);
    void handle_ohlcv(const json& message);
    void handle_order_book(const json& message);
    void handle_balance(const json& message);
    void handle_order(const json& message);

private:
    static constexpr const char* WS_BASE = "wss://api.exchange.bitcoin.com/api/2/ws";
    
    void subscribe_private(const std::string& channel, const std::string& symbol = "");
    void subscribe_public(const std::string& channel, const std::string& symbol);
    
    std::string generate_client_id();
    void parse_ob_snapshot(const json& data, json& result);
    void parse_ob_update(const json& data, json& result);
};

} // namespace ccxt

#endif // CCXT_BITCOINCOM_WS_H
