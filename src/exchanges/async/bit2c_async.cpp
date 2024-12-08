#include "ccxt/exchanges/async/bit2c_async.h"
#include "ccxt/async_base/async_utils.h"

namespace ccxt {

Bit2CAsync::Bit2CAsync(const boost::asio::io_context& context)
    : ExchangeAsync(context)
    , Bit2C() {}

// Market Data
async_result<json> Bit2CAsync::fetch_markets(const json& params) const {
    return async_fetch_markets(params);
}

async_result<json> Bit2CAsync::fetch_order_book(const std::string& symbol, int limit, const json& params) const {
    return async_fetch_order_book(symbol, limit, params);
}

async_result<json> Bit2CAsync::fetch_ticker(const std::string& symbol, const json& params) const {
    return async_fetch_ticker(symbol, params);
}

async_result<json> Bit2CAsync::fetch_trades(const std::string& symbol, int since, int limit, const json& params) const {
    return async_fetch_trades(symbol, since, limit, params);
}

async_result<json> Bit2CAsync::fetch_trading_fees(const json& params) const {
    return async_fetch_trading_fees(params);
}

// Trading
async_result<json> Bit2CAsync::create_order(const std::string& symbol, const std::string& type, const std::string& side, double amount, double price, const json& params) const {
    return async_create_order(symbol, type, side, amount, price, params);
}

async_result<json> Bit2CAsync::cancel_order(const std::string& id, const std::string& symbol, const json& params) const {
    return async_cancel_order(id, symbol, params);
}

async_result<json> Bit2CAsync::fetch_order(const std::string& id, const std::string& symbol, const json& params) const {
    return async_fetch_order(id, symbol, params);
}

async_result<json> Bit2CAsync::fetch_open_orders(const std::string& symbol, int since, int limit, const json& params) const {
    return async_fetch_open_orders(symbol, since, limit, params);
}

async_result<json> Bit2CAsync::fetch_my_trades(const std::string& symbol, int since, int limit, const json& params) const {
    return async_fetch_my_trades(symbol, since, limit, params);
}

// Account
async_result<json> Bit2CAsync::fetch_balance(const json& params) const {
    return async_fetch_balance(params);
}

async_result<json> Bit2CAsync::fetch_deposit_address(const std::string& code, const json& params) const {
    return async_fetch_deposit_address(code, params);
}

} // namespace ccxt
