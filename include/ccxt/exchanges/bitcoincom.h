#ifndef CCXT_BITCOINCOM_H
#define CCXT_BITCOINCOM_H

#include "../base/exchange.h"
#include <string>
#include <map>

namespace ccxt {

class bitcoincom : public exchange {
public:
    explicit bitcoincom(const Config& config);
    ~bitcoincom() override = default;

    // Public API - Market Data
    json fetch_markets() override;
    json fetch_currencies() override;
    json fetch_ticker(const std::string& symbol);
    json fetch_tickers(const std::vector<std::string>& symbols = {});
    json fetch_order_book(const std::string& symbol, int limit = 100);
    json fetch_trades(const std::string& symbol, int limit = 100);
    json fetch_ohlcv(const std::string& symbol, const std::string& timeframe = "1m", 
                    long since = 0, int limit = 100);
    json fetch_trading_fees(const std::string& symbol = "");

    // Private API - Trading
    json create_order(const std::string& symbol, const std::string& type, const std::string& side,
                     double amount, double price = 0, const std::map<std::string, std::string>& params = {});
    json cancel_order(const std::string& id, const std::string& symbol = "");
    json cancel_all_orders(const std::string& symbol = "");
    json edit_order(const std::string& id, const std::string& symbol, const std::string& type,
                   const std::string& side, double amount, double price = 0,
                   const std::map<std::string, std::string>& params = {});

    // Private API - Account/Balance
    json fetch_balance();
    json fetch_open_orders(const std::string& symbol = "");
    json fetch_closed_orders(const std::string& symbol = "", long since = 0, int limit = 100);
    json fetch_order(const std::string& id, const std::string& symbol = "");
    json fetch_orders(const std::string& symbol = "", long since = 0, int limit = 100);
    json fetch_my_trades(const std::string& symbol = "", long since = 0, int limit = 100);
    json fetch_trading_fee(const std::string& symbol);

    // Account Management
    json fetch_deposit_address(const std::string& code, const std::map<std::string, std::string>& params = {});
    json fetch_deposits(const std::string& code = "", long since = 0, int limit = 100);
    json fetch_withdrawals(const std::string& code = "", long since = 0, int limit = 100);
    json withdraw(const std::string& code, double amount, const std::string& address,
                 const std::string& tag = "", const std::map<std::string, std::string>& params = {});

protected:
    void init() override;
    std::string get_signed_url(const std::string& path, const std::string& method = "GET",
                              const std::map<std::string, std::string>& params = {});
    std::string sign(const std::string& path, const std::string& method, const std::string& body,
                    const std::map<std::string, std::string>& headers) override;

private:
    json private_get(const std::string& path, const std::map<std::string, std::string>& params = {});
    json private_post(const std::string& path, const std::map<std::string, std::string>& params = {});
    json private_put(const std::string& path, const std::map<std::string, std::string>& params = {});
    json private_delete(const std::string& path, const std::map<std::string, std::string>& params = {});
    json public_get(const std::string& path, const std::map<std::string, std::string>& params = {});
};

} // namespace ccxt

#endif // CCXT_BITCOINCOM_H
