#pragma once

#include "ccxt/base/exchange.h"

namespace ccxt {

class Kuna : public Exchange {
public:
    Kuna();
    ~Kuna() override = default;

    // Market data endpoints
    nlohmann::json fetch_markets() override;
    nlohmann::json fetch_ticker(const std::string& symbol) override;
    nlohmann::json fetch_tickers(const std::vector<std::string>& symbols = {});
    nlohmann::json fetch_order_book(const std::string& symbol, int limit = 0) override;
    nlohmann::json fetch_trades(const std::string& symbol, int limit = 0) override;
    nlohmann::json fetch_balance() override;
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
    nlohmann::json fetch_my_trades(const std::string& symbol = "", int since = 0, int limit = 0);

    // Additional features
    nlohmann::json fetch_deposit_address(const std::string& code, const nlohmann::json& params = nlohmann::json({}));
    nlohmann::json fetch_deposits(const std::string& code = "", int since = 0, int limit = 0);
    nlohmann::json fetch_withdrawals(const std::string& code = "", int since = 0, int limit = 0);

protected:
    std::string sign(const std::string& path, const std::string& api = "public",
                    const std::string& method = "GET", const nlohmann::json& params = nlohmann::json({}),
                    const std::map<std::string, std::string>& headers = {}) override;

private:
    nlohmann::json parse_ticker(const nlohmann::json& ticker, const nlohmann::json& market = nlohmann::json({}));
    nlohmann::json parse_order(const nlohmann::json& order, const nlohmann::json& market = nlohmann::json({}));
    nlohmann::json parse_trade(const nlohmann::json& trade, const nlohmann::json& market = nlohmann::json({}));
    nlohmann::json parse_ohlcv(const nlohmann::json& ohlcv, const nlohmann::json& market = nlohmann::json({}));
    nlohmann::json parse_transaction(const nlohmann::json& transaction);
    std::string get_market_id(const std::string& symbol);
};

} // namespace ccxt
