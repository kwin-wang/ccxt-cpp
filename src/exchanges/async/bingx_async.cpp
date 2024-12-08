#include "ccxt/exchanges/async/bingx_async.h"
#include "ccxt/async_base/async_utils.h"

namespace ccxt {

BingXAsync::BingXAsync(const boost::asio::io_context& context)
    : ExchangeAsync(context)
    , BingX() {}

// Market Data
async_result<json> BingXAsync::fetch_markets(const json& params) const {
    return async_fetch_markets(params);
}

async_result<json> BingXAsync::fetch_currencies(const json& params) const {
    return async_fetch_currencies(params);
}

async_result<json> BingXAsync::fetch_order_book(const std::string& symbol, int limit, const json& params) const {
    return async_fetch_order_book(symbol, limit, params);
}

async_result<json> BingXAsync::fetch_ticker(const std::string& symbol, const json& params) const {
    return async_fetch_ticker(symbol, params);
}

async_result<json> BingXAsync::fetch_tickers(const std::vector<std::string>& symbols, const json& params) const {
    return async_fetch_tickers(symbols, params);
}

async_result<json> BingXAsync::fetch_trades(const std::string& symbol, int since, int limit, const json& params) const {
    return async_fetch_trades(symbol, since, limit, params);
}

async_result<json> BingXAsync::fetch_ohlcv(const std::string& symbol, const std::string& timeframe, int since, int limit, const json& params) const {
    return async_fetch_ohlcv(symbol, timeframe, since, limit, params);
}

// Trading
async_result<json> BingXAsync::create_order(const std::string& symbol, const std::string& type, const std::string& side, double amount, double price, const json& params) const {
    return async_create_order(symbol, type, side, amount, price, params);
}

async_result<json> BingXAsync::cancel_order(const std::string& id, const std::string& symbol, const json& params) const {
    return async_cancel_order(id, symbol, params);
}

async_result<json> BingXAsync::cancel_all_orders(const std::string& symbol, const json& params) const {
    return async_cancel_all_orders(symbol, params);
}

async_result<json> BingXAsync::fetch_order(const std::string& id, const std::string& symbol, const json& params) const {
    return async_fetch_order(id, symbol, params);
}

async_result<json> BingXAsync::fetch_orders(const std::string& symbol, int since, int limit, const json& params) const {
    return async_fetch_orders(symbol, since, limit, params);
}

async_result<json> BingXAsync::fetch_open_orders(const std::string& symbol, int since, int limit, const json& params) const {
    return async_fetch_open_orders(symbol, since, limit, params);
}

async_result<json> BingXAsync::fetch_closed_orders(const std::string& symbol, int since, int limit, const json& params) const {
    return async_fetch_closed_orders(symbol, since, limit, params);
}

// Account
async_result<json> BingXAsync::fetch_balance(const json& params) const {
    return async_fetch_balance(params);
}

async_result<json> BingXAsync::fetch_positions(const std::vector<std::string>& symbols, const json& params) const {
    return async_fetch_positions(symbols, params);
}

async_result<json> BingXAsync::fetch_position(const std::string& symbol, const json& params) const {
    return async_fetch_position(symbol, params);
}

async_result<json> BingXAsync::set_leverage(int leverage, const std::string& symbol, const json& params) const {
    return async_set_leverage(leverage, symbol, params);
}

async_result<json> BingXAsync::set_margin_mode(const std::string& marginMode, const std::string& symbol, const json& params) const {
    return async_set_margin_mode(marginMode, symbol, params);
}

async_result<json> BingXAsync::set_position_mode(bool hedged, const std::string& symbol, const json& params) const {
    return async_set_position_mode(hedged, symbol, params);
}

// Funding
async_result<json> BingXAsync::fetch_deposit_address(const std::string& code, const json& params) const {
    return async_fetch_deposit_address(code, params);
}

async_result<json> BingXAsync::fetch_deposits(const std::string& code, int since, int limit, const json& params) const {
    return async_fetch_deposits(code, since, limit, params);
}

async_result<json> BingXAsync::fetch_withdrawals(const std::string& code, int since, int limit, const json& params) const {
    return async_fetch_withdrawals(code, since, limit, params);
}

async_result<json> BingXAsync::transfer(const std::string& code, double amount, const std::string& fromAccount, const std::string& toAccount, const json& params) const {
    return async_transfer(code, amount, fromAccount, toAccount, params);
}

// Perpetual Swap
async_result<json> BingXAsync::fetch_funding_rate(const std::string& symbol, const json& params) const {
    return async_fetch_funding_rate(symbol, params);
}

async_result<json> BingXAsync::fetch_funding_rates(const std::vector<std::string>& symbols, const json& params) const {
    return async_fetch_funding_rates(symbols, params);
}

async_result<json> BingXAsync::fetch_funding_rate_history(const std::string& symbol, int since, int limit, const json& params) const {
    return async_fetch_funding_rate_history(symbol, since, limit, params);
}

} // namespace ccxt
