#pragma once

#include "ccxt/base/exchange.h"

namespace ccxt {

class Woo : public Exchange {
public:
    Woo();
    ~Woo() override = default;

    // Market data endpoints
    nlohmann::json fetch_markets() override;
    nlohmann::json fetch_ticker(const std::string& symbol) override;
    nlohmann::json fetch_order_book(const std::string& symbol, int limit = 0) override;
    nlohmann::json fetch_trades(const std::string& symbol, int limit = 0) override;
    nlohmann::json fetch_ohlcv(const std::string& symbol, const std::string& timeframe = "1m", 
                              long since = 0, int limit = 0);

    // Trading endpoints
    nlohmann::json create_order(const std::string& symbol, const std::string& type,
                               const std::string& side, double amount,
                               double price = 0.0) override;
    nlohmann::json cancel_order(const std::string& id, const std::string& symbol = "") override;
    nlohmann::json fetch_order(const std::string& id, const std::string& symbol = "") override;
    nlohmann::json fetch_orders(const std::string& symbol = "", int limit = 0) override;
    nlohmann::json fetch_open_orders(const std::string& symbol = "", int limit = 0);
    nlohmann::json fetch_closed_orders(const std::string& symbol = "", int limit = 0);

protected:
    std::string sign(const std::string& path, const std::string& api = "public",
                    const std::string& method = "GET", const nlohmann::json& params = nlohmann::json({}),
                    const std::map<std::string, std::string>& headers = {}) override;

private:
    std::string get_network(const std::string& network);
    nlohmann::json parse_order(const nlohmann::json& order, const nlohmann::json& market = nlohmann::json({}));
    nlohmann::json parse_trade(const nlohmann::json& trade, const nlohmann::json& market = nlohmann::json({}));
    nlohmann::json parse_ticker(const nlohmann::json& ticker, const nlohmann::json& market = nlohmann::json({}));
};

} // namespace ccxt
