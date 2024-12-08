#ifndef CCXT_EXCHANGES_ASYNC_BINGX_ASYNC_H
#define CCXT_EXCHANGES_ASYNC_BINGX_ASYNC_H

#include "ccxt/async_base/exchange_async.h"
#include "ccxt/exchanges/bingx.h"
#include <boost/asio.hpp>

namespace ccxt {

class BingXAsync : public ExchangeAsync, public BingX {
public:
    explicit BingXAsync(const boost::asio::io_context& context);
    ~BingXAsync() override = default;

    // Market Data
    async_result<json> fetch_markets(const json& params = json::object()) const override;
    async_result<json> fetch_currencies(const json& params = json::object()) const override;
    async_result<json> fetch_order_book(const std::string& symbol, int limit = 0, const json& params = json::object()) const override;
    async_result<json> fetch_ticker(const std::string& symbol, const json& params = json::object()) const override;
    async_result<json> fetch_tickers(const std::vector<std::string>& symbols = std::vector<std::string>(), const json& params = json::object()) const override;
    async_result<json> fetch_trades(const std::string& symbol, int since = 0, int limit = 0, const json& params = json::object()) const override;
    async_result<json> fetch_ohlcv(const std::string& symbol, const std::string& timeframe = "1m", int since = 0, int limit = 0, const json& params = json::object()) const override;
    
    // Trading
    async_result<json> create_order(const std::string& symbol, const std::string& type, const std::string& side, double amount, double price = 0, const json& params = json::object()) const override;
    async_result<json> cancel_order(const std::string& id, const std::string& symbol = "", const json& params = json::object()) const override;
    async_result<json> cancel_all_orders(const std::string& symbol = "", const json& params = json::object()) const override;
    async_result<json> fetch_order(const std::string& id, const std::string& symbol = "", const json& params = json::object()) const override;
    async_result<json> fetch_orders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) const override;
    async_result<json> fetch_open_orders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) const override;
    async_result<json> fetch_closed_orders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) const override;

    // Account
    async_result<json> fetch_balance(const json& params = json::object()) const override;
    async_result<json> fetch_positions(const std::vector<std::string>& symbols = std::vector<std::string>(), const json& params = json::object()) const override;
    async_result<json> fetch_position(const std::string& symbol, const json& params = json::object()) const override;
    async_result<json> set_leverage(int leverage, const std::string& symbol = "", const json& params = json::object()) const override;
    async_result<json> set_margin_mode(const std::string& marginMode, const std::string& symbol = "", const json& params = json::object()) const override;
    async_result<json> set_position_mode(bool hedged, const std::string& symbol = "", const json& params = json::object()) const override;

    // Funding
    async_result<json> fetch_deposit_address(const std::string& code, const json& params = json::object()) const override;
    async_result<json> fetch_deposits(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object()) const override;
    async_result<json> fetch_withdrawals(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object()) const override;
    async_result<json> transfer(const std::string& code, double amount, const std::string& fromAccount, const std::string& toAccount, const json& params = json::object()) const override;

    // Perpetual Swap
    async_result<json> fetch_funding_rate(const std::string& symbol, const json& params = json::object()) const override;
    async_result<json> fetch_funding_rates(const std::vector<std::string>& symbols = std::vector<std::string>(), const json& params = json::object()) const override;
    async_result<json> fetch_funding_rate_history(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) const override;
};

} // namespace ccxt

#endif // CCXT_EXCHANGES_ASYNC_BINGX_ASYNC_H
