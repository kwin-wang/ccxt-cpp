#pragma once

#include "ccxt/exchange.h"

namespace ccxt {

class KuCoinFutures : public Exchange {
public:
    KuCoinFutures();
    ~KuCoinFutures() override = default;

    // Market Data
    nlohmann::json fetch_markets() override;
    nlohmann::json fetch_ticker(const std::string& symbol);
    nlohmann::json fetch_funding_rate(const std::string& symbol);
    nlohmann::json fetch_funding_rates(const std::vector<std::string>& symbols = {});
    nlohmann::json fetch_funding_rate_history(const std::string& symbol = "", int since = 0, int limit = 0);
    nlohmann::json fetch_order_book(const std::string& symbol, int limit = 0);
    nlohmann::json fetch_trades(const std::string& symbol, int since = 0, int limit = 0);
    nlohmann::json fetch_ohlcv(const std::string& symbol, const std::string& timeframe = "1m",
                              int since = 0, int limit = 0);

    // Trading
    nlohmann::json create_order(const std::string& symbol, const std::string& type,
                               const std::string& side, double amount,
                               double price = 0);
    nlohmann::json cancel_order(const std::string& id, const std::string& symbol = "");
    nlohmann::json cancel_all_orders(const std::string& symbol = "");
    nlohmann::json fetch_order(const std::string& id, const std::string& symbol = "");
    nlohmann::json fetch_orders(const std::string& symbol = "", int since = 0, int limit = 0);
    nlohmann::json fetch_open_orders(const std::string& symbol = "", int since = 0, int limit = 0);
    nlohmann::json fetch_closed_orders(const std::string& symbol = "", int since = 0, int limit = 0);

    // Account
    nlohmann::json fetch_balance();
    nlohmann::json fetch_positions(const std::vector<std::string>& symbols = {});
    nlohmann::json fetch_position(const std::string& symbol);
    nlohmann::json fetch_leverage_tiers(const std::vector<std::string>& symbols = {});
    nlohmann::json set_leverage(int leverage, const std::string& symbol);
    nlohmann::json set_margin_mode(const std::string& marginMode, const std::string& symbol = "");

protected:
    std::string sign(const std::string& path, const std::string& api = "public",
                    const std::string& method = "GET", const nlohmann::json& params = nlohmann::json({}),
                    const std::map<std::string, std::string>& headers = {}) override;

private:
    nlohmann::json parse_ticker(const nlohmann::json& ticker, const nlohmann::json& market = nlohmann::json({}));
    nlohmann::json parse_trade(const nlohmann::json& trade, const nlohmann::json& market = nlohmann::json({}));
    nlohmann::json parse_order(const nlohmann::json& order, const nlohmann::json& market = nlohmann::json({}));
    nlohmann::json parse_position(const nlohmann::json& position, const nlohmann::json& market = nlohmann::json({}));
    nlohmann::json parse_funding_rate(const nlohmann::json& fundingRate, const nlohmann::json& market = nlohmann::json({}));
    nlohmann::json parse_balance(const nlohmann::json& response);
    
    std::string get_settlement_currency(const std::string& market);
    std::string get_access_token();
    std::string get_server_time();
};

} // namespace ccxt
