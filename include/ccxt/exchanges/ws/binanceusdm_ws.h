#ifndef CCXT_BINANCEUSDM_WS_H
#define CCXT_BINANCEUSDM_WS_H

#include "ccxt/exchanges/ws/binance_ws.h"

namespace ccxt {

class binanceusdm_ws : public binance_ws {
public:
    binanceusdm_ws();
    ~binanceusdm_ws() = default;

protected:
    // Market Data Methods
    virtual void watch_ticker_impl(const std::string& symbol, const json& params) override;
    virtual void watch_trades_impl(const std::string& symbol, const json& params) override;
    virtual void watch_ohlcv_impl(const std::string& symbol, const std::string& timeframe, const json& params) override;
    virtual void watch_order_book_impl(const std::string& symbol, const json& params) override;
    virtual void watch_bids_asks_impl(const std::string& symbol, const json& params) override;

    // Private Methods
    virtual void watch_balance_impl(const json& params) override;
    virtual void watch_orders_impl(const std::string& symbol, const json& params) override;
    virtual void watch_my_trades_impl(const std::string& symbol, const json& params) override;
    virtual void watch_positions_impl(const json& params) override;

    // Helper Methods
    virtual std::string get_url() const override;
    virtual std::string get_ws_base() const override;
    virtual std::string get_rest_base() const override;
    virtual std::string get_listen_key() const override;
    virtual void handle_message(const json& message) override;
    virtual void handle_error(const json& message) override;
    virtual void handle_subscription(const json& message) override;
    virtual void handle_user_update(const json& message) override;
    virtual void handle_order_update(const json& message) override;
    virtual void handle_balance_update(const json& message) override;
    virtual void handle_position_update(const json& message) override;

private:
    static constexpr const char* WS_BASE = "wss://fstream.binance.com";
    static constexpr const char* REST_BASE = "https://fapi.binance.com";
};

} // namespace ccxt

#endif // CCXT_BINANCEUSDM_WS_H
