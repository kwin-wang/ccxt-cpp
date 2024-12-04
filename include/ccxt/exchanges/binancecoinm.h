#pragma once

#include "ccxt/exchanges/binance.h"

namespace ccxt {

class BinanceCoinm : public Binance {
public:
    BinanceCoinm();
    ~BinanceCoinm() override = default;

    // Market data endpoints
    nlohmann::json fetch_markets() override;
    nlohmann::json fetch_funding_rate(const std::string& symbol);
    nlohmann::json fetch_funding_rates(const std::vector<std::string>& symbols = {});
    nlohmann::json fetch_funding_rate_history(const std::string& symbol = "", int since = 0, int limit = 0);
    nlohmann::json fetch_index_ohlcv(const std::string& symbol, const std::string& timeframe = "1m",
                                    int since = 0, int limit = 0);
    nlohmann::json fetch_mark_ohlcv(const std::string& symbol, const std::string& timeframe = "1m",
                                   int since = 0, int limit = 0);
    nlohmann::json fetch_premium_index_ohlcv(const std::string& symbol, const std::string& timeframe = "1m",
                                            int since = 0, int limit = 0);

    // Trading endpoints
    nlohmann::json fetch_leverage_brackets(const std::vector<std::string>& symbols = {});
    nlohmann::json fetch_positions(const std::vector<std::string>& symbols = {});
    nlohmann::json set_leverage(int leverage, const std::string& symbol);
    nlohmann::json set_margin_mode(const std::string& marginMode, const std::string& symbol = "");
    nlohmann::json set_position_mode(bool hedged);
    nlohmann::json add_margin(const std::string& symbol, double amount);
    nlohmann::json reduce_margin(const std::string& symbol, double amount);

protected:
    std::string sign(const std::string& path, const std::string& api = "public",
                    const std::string& method = "GET", const nlohmann::json& params = nlohmann::json({}),
                    const std::map<std::string, std::string>& headers = {}) override;

private:
    nlohmann::json parse_funding_rate(const nlohmann::json& fundingRate, const nlohmann::json& market = nlohmann::json({}));
    nlohmann::json parse_position(const nlohmann::json& position, const nlohmann::json& market = nlohmann::json({}));
    nlohmann::json parse_leverage_brackets(const nlohmann::json& response, const nlohmann::json& market = nlohmann::json({}));
    std::string get_settlement_currency(const std::string& market);
};

} // namespace ccxt
