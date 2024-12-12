#pragma once

#include "ccxt/base/exchange.h"
#include <boost/asio/io_context.hpp>

namespace ccxt {

class CoinEx : public Exchange {
public:
    explicit CoinEx(boost::asio::io_context& context);
    ~CoinEx() override = default;

    // Market Data
    json fetch_markets() override;
    json fetch_ticker(const std::string& symbol);
    json fetch_order_book(const std::string& symbol, int limit = 0);
    json fetch_trades(const std::string& symbol, int since = 0, int limit = 0);
    json fetch_ohlcv(const std::string& symbol, const std::string& timeframe = "1m",
                   int since = 0, int limit = 0);

    // Market Data Async
    std::future<json> fetch_markets_async() override;
    std::future<json> fetch_ticker_async(const std::string& symbol);
    std::future<json> fetch_order_book_async(const std::string& symbol, int limit = 0);
    std::future<json> fetch_trades_async(const std::string& symbol, int since = 0, int limit = 0);
    std::future<json> fetch_ohlcv_async(const std::string& symbol,
                                     const std::string& timeframe = "1m",
                                     int since = 0,
                                     int limit = 0);

    // Trading
    json create_order(const std::string& symbol, const std::string& type,
                    const std::string& side, double amount,
                    double price = 0);
    json cancel_order(const std::string& id, const std::string& symbol = "");
    json cancel_all_orders(const std::string& symbol = "");
    json fetch_order(const std::string& id, const std::string& symbol = "");
    json fetch_orders(const std::string& symbol = "", int since = 0, int limit = 0);
    json fetch_open_orders(const std::string& symbol = "", int since = 0, int limit = 0);
    json fetch_closed_orders(const std::string& symbol = "", int since = 0, int limit = 0);
    json fetch_my_trades(const std::string& symbol = "", int since = 0, int limit = 0);

    // Trading Async
    std::future<json> create_order_async(const std::string& symbol, const std::string& type,
                                     const std::string& side, double amount,
                                     double price = 0);
    std::future<json> cancel_order_async(const std::string& id, const std::string& symbol = "");
    std::future<json> cancel_all_orders_async(const std::string& symbol = "");
    std::future<json> fetch_order_async(const std::string& id, const std::string& symbol = "");
    std::future<json> fetch_orders_async(const std::string& symbol = "", int since = 0, int limit = 0);
    std::future<json> fetch_open_orders_async(const std::string& symbol = "", int since = 0, int limit = 0);
    std::future<json> fetch_closed_orders_async(const std::string& symbol = "", int since = 0, int limit = 0);
    std::future<json> fetch_my_trades_async(const std::string& symbol = "", int since = 0, int limit = 0);

    // Account
    json fetch_balance();
    json fetch_deposit_address(const std::string& code);
    json fetch_deposits(const std::string& code = "", int since = 0, int limit = 0);
    json fetch_withdrawals(const std::string& code = "", int since = 0, int limit = 0);
    json withdraw(const std::string& code, double amount, const std::string& address,
                const std::string& tag = "", const json& params = json::object());

    // Account Async
    std::future<json> fetch_balance_async();
    std::future<json> fetch_deposit_address_async(const std::string& code);
    std::future<json> fetch_deposits_async(const std::string& code = "", int since = 0, int limit = 0);
    std::future<json> fetch_withdrawals_async(const std::string& code = "", int since = 0, int limit = 0);
    std::future<json> withdraw_async(const std::string& code,
                                 double amount,
                                 const std::string& address,
                                 const std::string& tag = "",
                                 const json& params = json::object());

    // Margin Trading
    json fetch_margin_balance();
    json create_margin_order(const std::string& symbol, const std::string& type,
                          const std::string& side, double amount,
                          double price = 0);
    json borrow_margin(const std::string& code, double amount, const std::string& symbol = "");
    json repay_margin(const std::string& code, double amount, const std::string& symbol = "");

    // Margin Trading Async
    std::future<json> fetch_margin_balance_async();
    std::future<json> create_margin_order_async(const std::string& symbol,
                                            const std::string& type,
                                            const std::string& side,
                                            double amount,
                                            double price = 0);
    std::future<json> borrow_margin_async(const std::string& code,
                                      double amount,
                                      const std::string& symbol = "");
    std::future<json> repay_margin_async(const std::string& code,
                                     double amount,
                                     const std::string& symbol = "");

protected:
    std::string sign(const std::string& path, const std::string& api = "public",
                   const std::string& method = "GET", const json& params = json::object(),
                   const std::map<std::string, std::string>& headers = {}) override;

    // Async HTTP methods
    std::future<json> fetch_async(const std::string& url,
                               const std::string& method = "GET",
                               const std::map<std::string, std::string>& headers = {},
                               const std::string& body = "") override;

private:
    json parse_ticker(const json& ticker, const json& market = json::object());
    json parse_trade(const json& trade, const json& market = json::object());
    json parse_order(const json& order, const json& market = json::object());
    json parse_balance(const json& response);
    json parse_deposit_address(const json& depositAddress);
    json parse_transaction(const json& transaction, const std::string& currency = "");
    
    std::string get_market_id(const std::string& symbol);
    std::string get_currency_id(const std::string& code);
    std::string get_tonce();

    boost::asio::io_context& context_;
};

} // namespace ccxt
