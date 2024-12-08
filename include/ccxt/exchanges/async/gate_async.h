#ifndef CCXT_GATE_ASYNC_H
#define CCXT_GATE_ASYNC_H

#include "../gate.h"
#include "../../async_client.h"
#include <string>
#include <map>
#include <future>

namespace ccxt {

class gate_async : public async_client, public gate {
public:
    explicit gate_async(const Config& config);
    ~gate_async() override = default;

    // Public API - Market Data
    std::future<json> fetch_time();
    std::future<json> fetch_markets();
    std::future<json> fetch_currencies();
    std::future<json> fetch_ticker(const std::string& symbol);
    std::future<json> fetch_tickers(const std::vector<std::string>& symbols = {});
    std::future<json> fetch_order_book(const std::string& symbol, int limit = 100);
    std::future<json> fetch_trades(const std::string& symbol, int limit = 100);
    std::future<json> fetch_ohlcv(const std::string& symbol, const std::string& timeframe = "1m", 
                                 long since = 0, int limit = 100);
    std::future<json> fetch_funding_rate(const std::string& symbol);
    std::future<json> fetch_funding_rates(const std::vector<std::string>& symbols = {});
    std::future<json> fetch_trading_fees(const std::string& symbol = "");
    std::future<json> fetch_contract_size(const std::string& symbol);

    // Private API - Trading
    std::future<json> create_order(const std::string& symbol, const std::string& type, const std::string& side,
                                 double amount, double price = 0, const std::map<std::string, std::string>& params = {});
    std::future<json> create_reduce_only_order(const std::string& symbol, const std::string& type,
                                             const std::string& side, double amount, double price = 0,
                                             const std::map<std::string, std::string>& params = {});
    std::future<json> cancel_order(const std::string& id, const std::string& symbol = "");
    std::future<json> cancel_all_orders(const std::string& symbol = "");
    std::future<json> edit_order(const std::string& id, const std::string& symbol, const std::string& type,
                               const std::string& side, double amount, double price = 0,
                               const std::map<std::string, std::string>& params = {});

    // Private API - Account/Balance
    std::future<json> fetch_balance();
    std::future<json> fetch_spot_balance();
    std::future<json> fetch_futures_balance();
    std::future<json> fetch_margin_balance();
    std::future<json> fetch_open_orders(const std::string& symbol = "");
    std::future<json> fetch_closed_orders(const std::string& symbol = "", long since = 0, int limit = 100);
    std::future<json> fetch_order(const std::string& id, const std::string& symbol = "");
    std::future<json> fetch_orders(const std::string& symbol = "", long since = 0, int limit = 100);
    std::future<json> fetch_my_trades(const std::string& symbol = "", long since = 0, int limit = 100);
    std::future<json> fetch_positions(const std::string& symbol = "");
    std::future<json> fetch_leverage_tiers();
    std::future<json> fetch_funding_history(const std::string& symbol = "", long since = 0, int limit = 100);

    // Private API - Margin Trading
    std::future<json> fetch_borrowing_rate(const std::string& code);
    std::future<json> fetch_borrowing_rates();
    std::future<json> fetch_borrowing_interest(const std::string& code);
    std::future<json> fetch_leverage(const std::string& symbol);
    std::future<json> set_leverage(int leverage, const std::string& symbol);
    std::future<json> borrow_margin(const std::string& code, double amount);
    std::future<json> repay_margin(const std::string& code, double amount);

    // Account Management
    std::future<json> fetch_deposit_address(const std::string& code, const std::map<std::string, std::string>& params = {});
    std::future<json> fetch_deposit_addresses(const std::vector<std::string>& codes = {});
    std::future<json> fetch_deposits(const std::string& code = "", long since = 0, int limit = 100);
    std::future<json> fetch_withdrawals(const std::string& code = "", long since = 0, int limit = 100);
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

#endif // CCXT_GATE_ASYNC_H
