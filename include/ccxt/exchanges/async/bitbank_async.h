#ifndef CCXT_EXCHANGES_ASYNC_BITBANK_ASYNC_H
#define CCXT_EXCHANGES_ASYNC_BITBANK_ASYNC_H

#include "ccxt/async_base/exchange_async.h"
#include "ccxt/exchanges/bitbank.h"
#include <boost/asio.hpp>

namespace ccxt {

class BitbankAsync : public ExchangeAsync, public Bitbank {
public:
    explicit BitbankAsync(const boost::asio::io_context& context);
    ~BitbankAsync() override = default;

    // Market Data
    async_result<json> fetch_markets(const json& params = json::object()) const override;
    async_result<json> fetch_order_book(const std::string& symbol, int limit = 0, const json& params = json::object()) const override;
    async_result<json> fetch_ticker(const std::string& symbol, const json& params = json::object()) const override;
    async_result<json> fetch_trades(const std::string& symbol, int since = 0, int limit = 0, const json& params = json::object()) const override;
    async_result<json> fetch_trading_fees(const json& params = json::object()) const override;
    async_result<json> fetch_ohlcv(const std::string& symbol, const std::string& timeframe = "1m", int since = 0, int limit = 0, const json& params = json::object()) const override;

    // Trading
    async_result<json> create_order(const std::string& symbol, const std::string& type, const std::string& side, double amount, double price = 0, const json& params = json::object()) const override;
    async_result<json> cancel_order(const std::string& id, const std::string& symbol = "", const json& params = json::object()) const override;
    async_result<json> fetch_order(const std::string& id, const std::string& symbol = "", const json& params = json::object()) const override;
    async_result<json> fetch_open_orders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) const override;
    async_result<json> fetch_my_trades(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) const override;

    // Account
    async_result<json> fetch_balance(const json& params = json::object()) const override;
    async_result<json> fetch_deposit_address(const std::string& code, const json& params = json::object()) const override;

    // Funding
    async_result<json> withdraw(const std::string& code, double amount, const std::string& address, const std::string& tag = "", const json& params = json::object()) const override;
};

} // namespace ccxt

#endif // CCXT_EXCHANGES_ASYNC_BITBANK_ASYNC_H
