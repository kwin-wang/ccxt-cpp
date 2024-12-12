#pragma once

#include "ccxt/base/exchange.h"
#include <boost/thread/future.hpp>

namespace ccxt {

class BitBNS : public Exchange {
public:
    BitBNS();
    ~BitBNS() override = default;

    // Market Data - Sync
    nlohmann::json fetch_markets() override;
    nlohmann::json fetch_ticker(const std::string& symbol);
    nlohmann::json fetch_order_book(const std::string& symbol, int limit = 0);
    nlohmann::json fetch_trades(const std::string& symbol, int since = 0, int limit = 0);
    nlohmann::json fetch_ohlcv(const std::string& symbol, const std::string& timeframe = "1m",
                              int since = 0, int limit = 0);

    // Market Data - Async
    boost::future<nlohmann::json> fetch_markets_async(const nlohmann::json& params = nlohmann::json::object());
    boost::future<nlohmann::json> fetch_ticker_async(const std::string& symbol, const nlohmann::json& params = nlohmann::json::object());
    boost::future<nlohmann::json> fetch_order_book_async(const std::string& symbol, int limit = 0, const nlohmann::json& params = nlohmann::json::object());
    boost::future<nlohmann::json> fetch_trades_async(const std::string& symbol, int since = 0, int limit = 0, const nlohmann::json& params = nlohmann::json::object());
    boost::future<nlohmann::json> fetch_ohlcv_async(const std::string& symbol, const std::string& timeframe = "1m",
                                                  int since = 0, int limit = 0, const nlohmann::json& params = nlohmann::json::object());

    // Trading - Sync
    nlohmann::json create_order(const std::string& symbol, const std::string& type,
                               const std::string& side, double amount,
                               double price = 0);
    nlohmann::json cancel_order(const std::string& id, const std::string& symbol = "");
    nlohmann::json fetch_order(const std::string& id, const std::string& symbol = "");
    nlohmann::json fetch_orders(const std::string& symbol = "", int since = 0, int limit = 0);
    nlohmann::json fetch_open_orders(const std::string& symbol = "", int since = 0, int limit = 0);
    nlohmann::json fetch_closed_orders(const std::string& symbol = "", int since = 0, int limit = 0);
    nlohmann::json fetch_my_trades(const std::string& symbol = "", int since = 0, int limit = 0);

    // Trading - Async
    boost::future<nlohmann::json> create_order_async(const std::string& symbol, const std::string& type,
                                                   const std::string& side, double amount,
                                                   double price = 0, const nlohmann::json& params = nlohmann::json::object());
    boost::future<nlohmann::json> cancel_order_async(const std::string& id, const std::string& symbol = "", const nlohmann::json& params = nlohmann::json::object());
    boost::future<nlohmann::json> fetch_order_async(const std::string& id, const std::string& symbol = "", const nlohmann::json& params = nlohmann::json::object());
    boost::future<nlohmann::json> fetch_orders_async(const std::string& symbol = "", int since = 0, int limit = 0, const nlohmann::json& params = nlohmann::json::object());
    boost::future<nlohmann::json> fetch_open_orders_async(const std::string& symbol = "", int since = 0, int limit = 0, const nlohmann::json& params = nlohmann::json::object());
    boost::future<nlohmann::json> fetch_closed_orders_async(const std::string& symbol = "", int since = 0, int limit = 0, const nlohmann::json& params = nlohmann::json::object());
    boost::future<nlohmann::json> fetch_my_trades_async(const std::string& symbol = "", int since = 0, int limit = 0, const nlohmann::json& params = nlohmann::json::object());

    // Account - Sync
    nlohmann::json fetch_balance();
    nlohmann::json fetch_deposits(const std::string& code = "", int since = 0, int limit = 0);
    nlohmann::json fetch_withdrawals(const std::string& code = "", int since = 0, int limit = 0);
    nlohmann::json fetch_deposit_address(const std::string& code);

    // Account - Async
    boost::future<nlohmann::json> fetch_balance_async(const nlohmann::json& params = nlohmann::json::object());
    boost::future<nlohmann::json> fetch_deposits_async(const std::string& code = "", int since = 0, int limit = 0, const nlohmann::json& params = nlohmann::json::object());
    boost::future<nlohmann::json> fetch_withdrawals_async(const std::string& code = "", int since = 0, int limit = 0, const nlohmann::json& params = nlohmann::json::object());
    boost::future<nlohmann::json> fetch_deposit_address_async(const std::string& code, const nlohmann::json& params = nlohmann::json::object());

protected:
    std::string sign(const std::string& path, const std::string& api = "public",
                    const std::string& method = "GET", const nlohmann::json& params = nlohmann::json({}),
                    const std::map<std::string, std::string>& headers = {}) override;

private:
    nlohmann::json parse_ticker(const nlohmann::json& ticker, const nlohmann::json& market = nlohmann::json({}));
    nlohmann::json parse_trade(const nlohmann::json& trade, const nlohmann::json& market = nlohmann::json({}));
    nlohmann::json parse_order(const nlohmann::json& order, const nlohmann::json& market = nlohmann::json({}));
    nlohmann::json parse_balance(const nlohmann::json& response);
    nlohmann::json parse_transaction(const nlohmann::json& transaction, const std::string& currency = "");
    
    std::string get_version_string();
    std::string get_payload_hash(const std::string& payload);
};

} // namespace ccxt
