#include "ccxt/exchanges/async/bitbank_async.h"
#include "ccxt/async_base/async_utils.h"

namespace ccxt {

BitbankAsync::BitbankAsync(const boost::asio::io_context& context)
    : ExchangeAsync(context)
    , Bitbank() {}

// Market Data
async_result<json> BitbankAsync::fetch_markets(const json& params) const {
    return async_fetch_markets(params);
}

async_result<json> BitbankAsync::fetch_order_book(const std::string& symbol, int limit, const json& params) const {
    return async_fetch_order_book(symbol, limit, params);
}

async_result<json> BitbankAsync::fetch_ticker(const std::string& symbol, const json& params) const {
    return async_fetch_ticker(symbol, params);
}

async_result<json> BitbankAsync::fetch_trades(const std::string& symbol, int since, int limit, const json& params) const {
    return async_fetch_trades(symbol, since, limit, params);
}

async_result<json> BitbankAsync::fetch_trading_fees(const json& params) const {
    return async_fetch_trading_fees(params);
}

async_result<json> BitbankAsync::fetch_ohlcv(const std::string& symbol, const std::string& timeframe, int since, int limit, const json& params) const {
    return async_fetch_ohlcv(symbol, timeframe, since, limit, params);
}

// Trading
async_result<json> BitbankAsync::create_order(const std::string& symbol, const std::string& type, const std::string& side, double amount, double price, const json& params) const {
    return async_create_order(symbol, type, side, amount, price, params);
}

async_result<json> BitbankAsync::cancel_order(const std::string& id, const std::string& symbol, const json& params) const {
    return async_cancel_order(id, symbol, params);
}

async_result<json> BitbankAsync::fetch_order(const std::string& id, const std::string& symbol, const json& params) const {
    return async_fetch_order(id, symbol, params);
}

async_result<json> BitbankAsync::fetch_open_orders(const std::string& symbol, int since, int limit, const json& params) const {
    return async_fetch_open_orders(symbol, since, limit, params);
}

async_result<json> BitbankAsync::fetch_my_trades(const std::string& symbol, int since, int limit, const json& params) const {
    return async_fetch_my_trades(symbol, since, limit, params);
}

// Account
async_result<json> BitbankAsync::fetch_balance(const json& params) const {
    return async_fetch_balance(params);
}

async_result<json> BitbankAsync::fetch_deposit_address(const std::string& code, const json& params) const {
    return async_fetch_deposit_address(code, params);
}

// Funding
async_result<json> BitbankAsync::withdraw(const std::string& code, double amount, const std::string& address, const std::string& tag, const json& params) const {
    return async_withdraw(code, amount, address, tag, params);
}

} // namespace ccxt
