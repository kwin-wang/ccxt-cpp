#pragma once

#include "ccxt/exchanges/async/exchange_async.h"
#include "ccxt/exchanges/bitbns.h"

namespace ccxt {

class BitBNSAsync : public ExchangeAsync, public BitBNS {
public:
    explicit BitBNSAsync(const boost::asio::io_context& context);
    ~BitBNSAsync() override = default;

    // Market Data
    boost::future<nlohmann::json> fetch_markets_async();
    boost::future<nlohmann::json> fetch_ticker_async(const std::string& symbol);
    boost::future<nlohmann::json> fetch_order_book_async(const std::string& symbol, int limit = 0);
    boost::future<nlohmann::json> fetch_trades_async(const std::string& symbol, int since = 0, int limit = 0);
    boost::future<nlohmann::json> fetch_ohlcv_async(const std::string& symbol, const std::string& timeframe = "1m",
                                                   int since = 0, int limit = 0);

    // Trading
    boost::future<nlohmann::json> create_order_async(const std::string& symbol, const std::string& type,
                                                    const std::string& side, double amount, double price = 0);
    boost::future<nlohmann::json> cancel_order_async(const std::string& id, const std::string& symbol = "");
    boost::future<nlohmann::json> fetch_order_async(const std::string& id, const std::string& symbol = "");
    boost::future<nlohmann::json> fetch_orders_async(const std::string& symbol = "", int since = 0, int limit = 0);
    boost::future<nlohmann::json> fetch_open_orders_async(const std::string& symbol = "", int since = 0, int limit = 0);
    boost::future<nlohmann::json> fetch_closed_orders_async(const std::string& symbol = "", int since = 0, int limit = 0);
    boost::future<nlohmann::json> fetch_my_trades_async(const std::string& symbol = "", int since = 0, int limit = 0);

    // Account
    boost::future<nlohmann::json> fetch_balance_async();
    boost::future<nlohmann::json> fetch_deposits_async(const std::string& code = "", int since = 0, int limit = 0);
    boost::future<nlohmann::json> fetch_withdrawals_async(const std::string& code = "", int since = 0, int limit = 0);
    boost::future<nlohmann::json> fetch_deposit_address_async(const std::string& code);

protected:
    boost::future<nlohmann::json> fetch_async(const std::string& path, const std::string& api = "public",
                                            const std::string& method = "GET",
                                            const nlohmann::json& params = nlohmann::json({}),
                                            const std::map<std::string, std::string>& headers = {});
};

} // namespace ccxt
