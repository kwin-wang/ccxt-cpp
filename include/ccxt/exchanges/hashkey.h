#pragma once

#include "ccxt/base/exchange.h"
#include <future>

namespace ccxt {

class Hashkey : public Exchange {
public:
    Hashkey();
    ~Hashkey() override = default;

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
    nlohmann::json cancel_all_orders(const std::string& symbol = "");
    nlohmann::json fetch_order(const std::string& id, const std::string& symbol = "") override;
    nlohmann::json fetch_orders(const std::string& symbol = "", int limit = 0) override;
    nlohmann::json fetch_open_orders(const std::string& symbol = "", int limit = 0);
    nlohmann::json fetch_closed_orders(const std::string& symbol = "", int limit = 0);
    nlohmann::json fetch_my_trades(const std::string& symbol = "", int since = 0, int limit = 0);

    // Async market data endpoints
    std::future<nlohmann::json> fetch_markets_async();
    std::future<nlohmann::json> fetch_ticker_async(const std::string& symbol);
    std::future<nlohmann::json> fetch_tickers_async(const std::vector<std::string>& symbols = {});
    std::future<nlohmann::json> fetch_order_book_async(const std::string& symbol, int limit = 0);
    std::future<nlohmann::json> fetch_trades_async(const std::string& symbol, int limit = 0);
    std::future<nlohmann::json> fetch_balance_async();
    std::future<nlohmann::json> fetch_ohlcv_async(const std::string& symbol, const std::string& timeframe = "1m",
                                                 long since = 0, int limit = 0);

    // Async trading endpoints
    std::future<nlohmann::json> create_order_async(const std::string& symbol, const std::string& type,
                                                  const std::string& side, double amount,
                                                  double price = 0.0);
    std::future<nlohmann::json> cancel_order_async(const std::string& id, const std::string& symbol = "");
    std::future<nlohmann::json> cancel_all_orders_async(const std::string& symbol = "");
    std::future<nlohmann::json> fetch_order_async(const std::string& id, const std::string& symbol = "");
    std::future<nlohmann::json> fetch_orders_async(const std::string& symbol = "", int limit = 0);
    std::future<nlohmann::json> fetch_open_orders_async(const std::string& symbol = "", int limit = 0);
    std::future<nlohmann::json> fetch_closed_orders_async(const std::string& symbol = "", int limit = 0);
    std::future<nlohmann::json> fetch_my_trades_async(const std::string& symbol = "", int since = 0, int limit = 0);

protected:
    std::string sign(const std::string& path, const std::string& api = "public",
                    const std::string& method = "GET", const nlohmann::json& params = nlohmann::json({}),
                    const std::map<std::string, std::string>& headers = {}) override;

private:
    nlohmann::json parse_ticker(const nlohmann::json& ticker, const nlohmann::json& market = nlohmann::json({}));
    nlohmann::json parse_order(const nlohmann::json& order, const nlohmann::json& market = nlohmann::json({}));
    nlohmann::json parse_trade(const nlohmann::json& trade, const nlohmann::json& market = nlohmann::json({}));
    nlohmann::json parse_ohlcv(const nlohmann::json& ohlcv, const nlohmann::json& market = nlohmann::json({}));
    std::string get_order_status(const std::string& status);
};

} // namespace ccxt
