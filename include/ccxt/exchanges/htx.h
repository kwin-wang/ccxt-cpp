#pragma once

#include "ccxt/base/exchange.h"
#include <future>

namespace ccxt {

class HTX : public Exchange {
public:
    HTX();
    ~HTX() override = default;

    // Market Data
    nlohmann::json fetch_markets() override;
    nlohmann::json fetch_ticker(const std::string& symbol);
    nlohmann::json fetch_tickers(const std::vector<std::string>& symbols = {});
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
    nlohmann::json fetch_positions();
    nlohmann::json fetch_deposit_address(const std::string& code);
    nlohmann::json fetch_deposits(const std::string& code = "", int since = 0, int limit = 0);
    nlohmann::json fetch_withdrawals(const std::string& code = "", int since = 0, int limit = 0);
    nlohmann::json withdraw(const std::string& code, double amount, const std::string& address,
                           const std::string& tag = "", const nlohmann::json& params = {});

    // Margin Trading
    nlohmann::json fetch_margin_balance();
    nlohmann::json fetch_margin_positions();
    nlohmann::json create_margin_order(const std::string& symbol, const std::string& type,
                                     const std::string& side, double amount,
                                     double price = 0);
    nlohmann::json borrow_margin(const std::string& code, double amount,
                                const std::string& symbol = "", const nlohmann::json& params = {});
    nlohmann::json repay_margin(const std::string& code, double amount,
                               const std::string& symbol = "", const nlohmann::json& params = {});

    // Futures Trading
    nlohmann::json fetch_futures_positions();
    nlohmann::json create_futures_order(const std::string& symbol, const std::string& type,
                                      const std::string& side, double amount,
                                      double price = 0);
    nlohmann::json fetch_funding_rate(const std::string& symbol);
    nlohmann::json fetch_funding_history(const std::string& symbol = "", int since = 0, int limit = 0);

    // Async Market Data
    std::future<nlohmann::json> fetch_markets_async();
    std::future<nlohmann::json> fetch_ticker_async(const std::string& symbol);
    std::future<nlohmann::json> fetch_tickers_async(const std::vector<std::string>& symbols = {});
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
    std::future<nlohmann::json> fetch_positions_async();
    std::future<nlohmann::json> fetch_deposit_address_async(const std::string& code);
    std::future<nlohmann::json> fetch_deposits_async(const std::string& code = "", int since = 0, int limit = 0);
    std::future<nlohmann::json> fetch_withdrawals_async(const std::string& code = "", int since = 0, int limit = 0);
    std::future<nlohmann::json> withdraw_async(const std::string& code, double amount, const std::string& address,
                                             const std::string& tag = "", const nlohmann::json& params = {});

    // Async Margin Trading
    std::future<nlohmann::json> fetch_margin_balance_async();
    std::future<nlohmann::json> fetch_margin_positions_async();
    std::future<nlohmann::json> create_margin_order_async(const std::string& symbol, const std::string& type,
                                                         const std::string& side, double amount,
                                                         double price = 0);
    std::future<nlohmann::json> borrow_margin_async(const std::string& code, double amount,
                                                   const std::string& symbol = "", const nlohmann::json& params = {});
    std::future<nlohmann::json> repay_margin_async(const std::string& code, double amount,
                                                  const std::string& symbol = "", const nlohmann::json& params = {});

    // Async Futures Trading
    std::future<nlohmann::json> fetch_futures_positions_async();
    std::future<nlohmann::json> create_futures_order_async(const std::string& symbol, const std::string& type,
                                                          const std::string& side, double amount,
                                                          double price = 0);
    std::future<nlohmann::json> fetch_funding_rate_async(const std::string& symbol);
    std::future<nlohmann::json> fetch_funding_history_async(const std::string& symbol = "", int since = 0, int limit = 0);

protected:
    std::string sign(const std::string& path, const std::string& api = "public",
                    const std::string& method = "GET", const nlohmann::json& params = {},
                    const std::map<std::string, std::string>& headers = {}) override;

private:
    nlohmann::json parse_ticker(const nlohmann::json& ticker, const nlohmann::json& market = {});
    nlohmann::json parse_order(const nlohmann::json& order, const nlohmann::json& market = {});
    nlohmann::json parse_trade(const nlohmann::json& trade, const nlohmann::json& market = {});
    nlohmann::json parse_ohlcv(const nlohmann::json& ohlcv, const nlohmann::json& market = {});
    nlohmann::json parse_position(const nlohmann::json& position, const nlohmann::json& market = {});
    nlohmann::json parse_funding_rate(const nlohmann::json& fundingRate, const nlohmann::json& market = {});
    std::string get_order_status(const std::string& status);
    std::string get_position_side(const std::string& side);
};

} // namespace ccxt
