#ifndef CCXT_ONETRADING_ASYNC_H
#define CCXT_ONETRADING_ASYNC_H

#include "../onetrading.h"
#include "../../async_client.h"
#include <string>
#include <map>
#include <future>

namespace ccxt {

class onetrading_async : public async_client, public onetrading {
public:
    explicit onetrading_async(const Config& config);
    ~onetrading_async() override = default;

    // Public API - Market Data
    std::future<json> fetch_markets();
    std::future<json> fetch_currencies();
    std::future<json> fetch_ticker(const std::string& symbol);
    std::future<json> fetch_tickers(const std::vector<std::string>& symbols = {});
    std::future<json> fetch_order_book(const std::string& symbol, int limit = 100);
    std::future<json> fetch_trades(const std::string& symbol, int limit = 100);
    std::future<json> fetch_ohlcv(const std::string& symbol, const std::string& timeframe = "1m", 
                                 long since = 0, int limit = 100);
    std::future<json> fetch_trading_fees(const std::string& symbol = "");
    std::future<json> fetch_time();

    // Private API - Trading
    std::future<json> create_order(const std::string& symbol, const std::string& type, const std::string& side,
                                 double amount, double price = 0, const std::map<std::string, std::string>& params = {});
    std::future<json> cancel_order(const std::string& id, const std::string& symbol = "");
    std::future<json> cancel_all_orders(const std::string& symbol = "");
    std::future<json> edit_order(const std::string& id, const std::string& symbol, const std::string& type,
                               const std::string& side, double amount, double price = 0,
                               const std::map<std::string, std::string>& params = {});

    // Private API - Account/Balance
    std::future<json> fetch_balance();
    std::future<json> fetch_open_orders(const std::string& symbol = "");
    std::future<json> fetch_closed_orders(const std::string& symbol = "", long since = 0, int limit = 100);
    std::future<json> fetch_order(const std::string& id, const std::string& symbol = "");
    std::future<json> fetch_orders(const std::string& symbol = "", long since = 0, int limit = 100);
    std::future<json> fetch_my_trades(const std::string& symbol = "", long since = 0, int limit = 100);
    std::future<json> fetch_ledger(const std::string& code = "", long since = 0, int limit = 100);
    std::future<json> fetch_deposits(const std::string& code = "", long since = 0, int limit = 100);
    std::future<json> fetch_withdrawals(const std::string& code = "", long since = 0, int limit = 100);
    
    // Account Management
    std::future<json> fetch_accounts();
    std::future<json> create_deposit_address(const std::string& code, const std::map<std::string, std::string>& params = {});
    std::future<json> fetch_deposit_address(const std::string& code, const std::map<std::string, std::string>& params = {});
    std::future<json> withdraw(const std::string& code, double amount, const std::string& address,
                             const std::string& tag = "", const std::map<std::string, std::string>& params = {});

protected:
    std::string sign(const std::string& path, const std::string& method, const std::string& body,
                    const std::map<std::string, std::string>& headers) override;
    void sign_request(const std::string& method, const std::string& path, std::map<std::string, std::string>& headers,
                     std::string& body);

private:
    std::future<json> private_get(const std::string& path, const std::map<std::string, std::string>& params = {});
    std::future<json> private_post(const std::string& path, const std::map<std::string, std::string>& params = {});
    std::future<json> private_put(const std::string& path, const std::map<std::string, std::string>& params = {});
    std::future<json> private_delete(const std::string& path, const std::map<std::string, std::string>& params = {});
    std::future<json> public_get(const std::string& path, const std::map<std::string, std::string>& params = {});
};

} // namespace ccxt

#endif // CCXT_ONETRADING_ASYNC_H
