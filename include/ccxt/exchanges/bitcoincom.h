#ifndef CCXT_BITCOINCOM_H
#define CCXT_BITCOINCOM_H

#include "ccxt/base/exchange.h"
#include <string>
#include <map>
#include <boost/thread/future.hpp>

namespace ccxt {

class bitcoincom : public exchange {
public:
    explicit bitcoincom(const Config& config);
    ~bitcoincom() override = default;

    // Public API - Market Data - Sync
    json fetch_markets() override;
    json fetch_currencies() override;
    json fetch_ticker(const std::string& symbol);
    json fetch_tickers(const std::vector<std::string>& symbols = {});
    json fetch_order_book(const std::string& symbol, int limit = 100);
    json fetch_trades(const std::string& symbol, int limit = 100);
    json fetch_ohlcv(const std::string& symbol, const std::string& timeframe = "1m", 
                    long since = 0, int limit = 100);
    json fetch_trading_fees(const std::string& symbol = "");

    // Public API - Market Data - Async
    boost::future<json> fetch_markets_async(const json& params = json::object());
    boost::future<json> fetch_currencies_async(const json& params = json::object());
    boost::future<json> fetch_ticker_async(const std::string& symbol, const json& params = json::object());
    boost::future<json> fetch_tickers_async(const std::vector<std::string>& symbols = {}, const json& params = json::object());
    boost::future<json> fetch_order_book_async(const std::string& symbol, int limit = 100, const json& params = json::object());
    boost::future<json> fetch_trades_async(const std::string& symbol, int limit = 100, const json& params = json::object());
    boost::future<json> fetch_ohlcv_async(const std::string& symbol, const std::string& timeframe = "1m",
                                       long since = 0, int limit = 100, const json& params = json::object());
    boost::future<json> fetch_trading_fees_async(const std::string& symbol = "", const json& params = json::object());

    // Private API - Trading - Sync
    json create_order(const std::string& symbol, const std::string& type, const std::string& side,
                     double amount, double price = 0, const std::map<std::string, std::string>& params = {});
    json cancel_order(const std::string& id, const std::string& symbol = "");
    json cancel_all_orders(const std::string& symbol = "");
    json edit_order(const std::string& id, const std::string& symbol, const std::string& type,
                   const std::string& side, double amount, double price = 0,
                   const std::map<std::string, std::string>& params = {});

    // Private API - Trading - Async
    boost::future<json> create_order_async(const std::string& symbol, const std::string& type,
                                       const std::string& side, double amount, double price = 0,
                                       const json& params = json::object());
    boost::future<json> cancel_order_async(const std::string& id, const std::string& symbol = "",
                                       const json& params = json::object());
    boost::future<json> cancel_all_orders_async(const std::string& symbol = "",
                                            const json& params = json::object());
    boost::future<json> edit_order_async(const std::string& id, const std::string& symbol,
                                     const std::string& type, const std::string& side,
                                     double amount, double price = 0,
                                     const json& params = json::object());

    // Private API - Account/Balance - Sync
    json fetch_balance();
    json fetch_open_orders(const std::string& symbol = "");
    json fetch_closed_orders(const std::string& symbol = "", long since = 0, int limit = 100);
    json fetch_order(const std::string& id, const std::string& symbol = "");
    json fetch_orders(const std::string& symbol = "", long since = 0, int limit = 100);
    json fetch_my_trades(const std::string& symbol = "", long since = 0, int limit = 100);
    json fetch_trading_fee(const std::string& symbol);

    // Private API - Account/Balance - Async
    boost::future<json> fetch_balance_async(const json& params = json::object());
    boost::future<json> fetch_open_orders_async(const std::string& symbol = "",
                                            const json& params = json::object());
    boost::future<json> fetch_closed_orders_async(const std::string& symbol = "",
                                              long since = 0, int limit = 100,
                                              const json& params = json::object());
    boost::future<json> fetch_order_async(const std::string& id, const std::string& symbol = "",
                                      const json& params = json::object());
    boost::future<json> fetch_orders_async(const std::string& symbol = "",
                                       long since = 0, int limit = 100,
                                       const json& params = json::object());
    boost::future<json> fetch_my_trades_async(const std::string& symbol = "",
                                          long since = 0, int limit = 100,
                                          const json& params = json::object());
    boost::future<json> fetch_trading_fee_async(const std::string& symbol,
                                            const json& params = json::object());

    // Account Management - Sync
    json fetch_deposit_address(const std::string& code, const std::map<std::string, std::string>& params = {});
    json fetch_deposits(const std::string& code = "", long since = 0, int limit = 100);
    json fetch_withdrawals(const std::string& code = "", long since = 0, int limit = 100);
    json withdraw(const std::string& code, double amount, const std::string& address,
                 const std::string& tag = "", const std::map<std::string, std::string>& params = {});

    // Account Management - Async
    boost::future<json> fetch_deposit_address_async(const std::string& code,
                                                const json& params = json::object());
    boost::future<json> fetch_deposits_async(const std::string& code = "",
                                         long since = 0, int limit = 100,
                                         const json& params = json::object());
    boost::future<json> fetch_withdrawals_async(const std::string& code = "",
                                            long since = 0, int limit = 100,
                                            const json& params = json::object());
    boost::future<json> withdraw_async(const std::string& code, double amount,
                                   const std::string& address, const std::string& tag = "",
                                   const json& params = json::object());

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
