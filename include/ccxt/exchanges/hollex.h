#pragma once

#include "ccxt/base/exchange.h"
#include <future>

namespace ccxt {

class Hollex : public Exchange {
public:
    Hollex();
    ~Hollex() override = default;

    // Market Data
    nlohmann::json fetch_markets() override;
    nlohmann::json fetch_ticker(const std::string& symbol);
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
    nlohmann::json fetch_my_trades(const std::string& symbol = "", int since = 0, int limit = 0);

    // Account
    nlohmann::json fetch_balance();
    nlohmann::json fetch_deposit_address(const std::string& code);
    nlohmann::json fetch_deposits(const std::string& code = "", int since = 0, int limit = 0);
    nlohmann::json fetch_withdrawals(const std::string& code = "", int since = 0, int limit = 0);
    nlohmann::json withdraw(const std::string& code, double amount, const std::string& address,
                           const std::string& tag = "", const nlohmann::json& params = {});

    // Async Market Data
    std::future<nlohmann::json> fetch_markets_async();
    std::future<nlohmann::json> fetch_ticker_async(const std::string& symbol);
    std::future<nlohmann::json> fetch_order_book_async(const std::string& symbol, int limit = 0);
    std::future<nlohmann::json> fetch_trades_async(const std::string& symbol, int since = 0, int limit = 0);
    std::future<nlohmann::json> fetch_ohlcv_async(const std::string& symbol, const std::string& timeframe = "1m",
                                                 int since = 0, int limit = 0);

    // Async Trading
    std::future<nlohmann::json> create_order_async(const std::string& symbol, const std::string& type,
                                                  const std::string& side, double amount,
                                                  double price = 0);
    std::future<nlohmann::json> cancel_order_async(const std::string& id, const std::string& symbol = "");
    std::future<nlohmann::json> cancel_all_orders_async(const std::string& symbol = "");
    std::future<nlohmann::json> fetch_order_async(const std::string& id, const std::string& symbol = "");
    std::future<nlohmann::json> fetch_orders_async(const std::string& symbol = "", int since = 0, int limit = 0);
    std::future<nlohmann::json> fetch_open_orders_async(const std::string& symbol = "", int since = 0, int limit = 0);
    std::future<nlohmann::json> fetch_closed_orders_async(const std::string& symbol = "", int since = 0, int limit = 0);
    std::future<nlohmann::json> fetch_my_trades_async(const std::string& symbol = "", int since = 0, int limit = 0);

    // Async Account
    std::future<nlohmann::json> fetch_balance_async();
    std::future<nlohmann::json> fetch_deposit_address_async(const std::string& code);
    std::future<nlohmann::json> fetch_deposits_async(const std::string& code = "", int since = 0, int limit = 0);
    std::future<nlohmann::json> fetch_withdrawals_async(const std::string& code = "", int since = 0, int limit = 0);
    std::future<nlohmann::json> withdraw_async(const std::string& code, double amount, const std::string& address,
                                             const std::string& tag = "", const nlohmann::json& params = {});

protected:
    std::string sign(const std::string& path, const std::string& api = "public",
                    const std::string& method = "GET", const nlohmann::json& params = {},
                    const std::map<std::string, std::string>& headers = {}) override;

private:
    nlohmann::json parse_ticker(const nlohmann::json& ticker, const nlohmann::json& market = {});
    nlohmann::json parse_order(const nlohmann::json& order, const nlohmann::json& market = {});
    nlohmann::json parse_trade(const nlohmann::json& trade, const nlohmann::json& market = {});
    nlohmann::json parse_ohlcv(const nlohmann::json& ohlcv, const nlohmann::json& market = {});
    std::string get_order_status(const std::string& status);
};

} // namespace ccxt
