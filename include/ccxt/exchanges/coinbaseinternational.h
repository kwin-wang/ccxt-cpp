#ifndef CCXT_COINBASEINTERNATIONAL_H
#define CCXT_COINBASEINTERNATIONAL_H

#include "../base/exchange.h"

namespace ccxt {

class coinbaseinternational : public exchange {
public:
    explicit coinbaseinternational(const Config& config = Config());
    ~coinbaseinternational() override = default;

    // Market Data
    json fetch_markets(const json& params = json()) override;
    json fetch_currencies(const json& params = json()) override;
    json fetch_ticker(const std::string& symbol, const json& params = json()) override;
    json fetch_order_book(const std::string& symbol, int limit = 0, const json& params = json()) override;
    json fetch_trades(const std::string& symbol, int since = 0, int limit = 0, const json& params = json()) override;
    json fetch_ohlcv(const std::string& symbol, const std::string& timeframe = "1m", int since = 0, int limit = 0, const json& params = json()) override;
    json fetch_trading_fees(const std::string& symbol = "", const json& params = json()) override;

    // Trading
    json create_order(const std::string& symbol, const std::string& type, const std::string& side,
                     double amount, double price = 0, const json& params = json()) override;
    json cancel_order(const std::string& id, const std::string& symbol = "", const json& params = json()) override;
    json cancel_all_orders(const std::string& symbol = "", const json& params = json()) override;
    json edit_order(const std::string& id, const std::string& symbol, const std::string& type, const std::string& side,
                   double amount, double price = 0, const json& params = json()) override;

    // Account
    json fetch_balance(const json& params = json()) override;
    json fetch_open_orders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json()) override;
    json fetch_closed_orders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json()) override;
    json fetch_my_trades(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json()) override;
    json fetch_order(const std::string& id, const std::string& symbol = "", const json& params = json()) override;
    json fetch_deposit_address(const std::string& code, const json& params = json()) override;
    json fetch_deposits(const std::string& code = "", int since = 0, int limit = 0, const json& params = json()) override;
    json fetch_withdrawals(const std::string& code = "", int since = 0, int limit = 0, const json& params = json()) override;
    json withdraw(const std::string& code, double amount, const std::string& address, const std::string& tag = "", const json& params = json()) override;

protected:
    void sign(Request& request, const std::string& path, const std::string& api = "public",
             const std::string& method = "GET", const json& params = json(),
             const json& headers = json(), const json& body = json()) override;

private:
    std::string get_signature(const std::string& timestamp, const std::string& method,
                            const std::string& path, const std::string& body = "");
};

} // namespace ccxt

#endif // CCXT_COINBASEINTERNATIONAL_H
