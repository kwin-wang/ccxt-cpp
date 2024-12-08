#include "ccxt/exchanges/async/bitbns_async.h"

namespace ccxt {

BitBNSAsync::BitBNSAsync(const boost::asio::io_context& context)
    : ExchangeAsync(context)
    , BitBNS() {}

boost::future<nlohmann::json> BitBNSAsync::fetch_async(const std::string& path, const std::string& api,
                                                     const std::string& method, const nlohmann::json& params,
                                                     const std::map<std::string, std::string>& headers) {
    return ExchangeAsync::fetch_async(path, api, method, params, headers);
}

boost::future<nlohmann::json> BitBNSAsync::fetch_markets_async() {
    return fetch_async("/v1/exchangeData/assets", "public", "GET");
}

boost::future<nlohmann::json> BitBNSAsync::fetch_ticker_async(const std::string& symbol) {
    return fetch_async("/v1/ticker/24hr", "public", "GET", {{"symbol", symbol}});
}

boost::future<nlohmann::json> BitBNSAsync::fetch_order_book_async(const std::string& symbol, int limit) {
    nlohmann::json params;
    if (limit > 0) {
        params["limit"] = limit;
    }
    return fetch_async("/v1/depth", "public", "GET", {{"symbol", symbol}});
}

boost::future<nlohmann::json> BitBNSAsync::fetch_trades_async(const std::string& symbol, int since, int limit) {
    nlohmann::json params;
    if (since > 0) {
        params["since"] = since;
    }
    if (limit > 0) {
        params["limit"] = limit;
    }
    return fetch_async("/v1/trades", "public", "GET", {{"symbol", symbol}});
}

boost::future<nlohmann::json> BitBNSAsync::fetch_ohlcv_async(const std::string& symbol, const std::string& timeframe,
                                                           int since, int limit) {
    nlohmann::json params;
    if (since > 0) {
        params["since"] = since;
    }
    if (limit > 0) {
        params["limit"] = limit;
    }
    params["interval"] = timeframe;
    return fetch_async("/v1/klines", "public", "GET", {{"symbol", symbol}});
}

boost::future<nlohmann::json> BitBNSAsync::create_order_async(const std::string& symbol, const std::string& type,
                                                            const std::string& side, double amount, double price) {
    nlohmann::json params = {
        {"symbol", symbol},
        {"side", side},
        {"type", type},
        {"quantity", amount}
    };
    if (price > 0) {
        params["price"] = price;
    }
    return fetch_async("/v1/order", "private", "POST", params);
}

boost::future<nlohmann::json> BitBNSAsync::cancel_order_async(const std::string& id, const std::string& symbol) {
    nlohmann::json params = {{"orderId", id}};
    if (!symbol.empty()) {
        params["symbol"] = symbol;
    }
    return fetch_async("/v1/order", "private", "DELETE", params);
}

boost::future<nlohmann::json> BitBNSAsync::fetch_order_async(const std::string& id, const std::string& symbol) {
    nlohmann::json params = {{"orderId", id}};
    if (!symbol.empty()) {
        params["symbol"] = symbol;
    }
    return fetch_async("/v1/order", "private", "GET", params);
}

boost::future<nlohmann::json> BitBNSAsync::fetch_orders_async(const std::string& symbol, int since, int limit) {
    nlohmann::json params;
    if (!symbol.empty()) {
        params["symbol"] = symbol;
    }
    if (since > 0) {
        params["since"] = since;
    }
    if (limit > 0) {
        params["limit"] = limit;
    }
    return fetch_async("/v1/allOrders", "private", "GET", params);
}

boost::future<nlohmann::json> BitBNSAsync::fetch_open_orders_async(const std::string& symbol, int since, int limit) {
    nlohmann::json params;
    if (!symbol.empty()) {
        params["symbol"] = symbol;
    }
    if (since > 0) {
        params["since"] = since;
    }
    if (limit > 0) {
        params["limit"] = limit;
    }
    return fetch_async("/v1/openOrders", "private", "GET", params);
}

boost::future<nlohmann::json> BitBNSAsync::fetch_closed_orders_async(const std::string& symbol, int since, int limit) {
    nlohmann::json params;
    if (!symbol.empty()) {
        params["symbol"] = symbol;
    }
    if (since > 0) {
        params["since"] = since;
    }
    if (limit > 0) {
        params["limit"] = limit;
    }
    return fetch_async("/v1/myTrades", "private", "GET", params);
}

boost::future<nlohmann::json> BitBNSAsync::fetch_my_trades_async(const std::string& symbol, int since, int limit) {
    nlohmann::json params;
    if (!symbol.empty()) {
        params["symbol"] = symbol;
    }
    if (since > 0) {
        params["since"] = since;
    }
    if (limit > 0) {
        params["limit"] = limit;
    }
    return fetch_async("/v1/myTrades", "private", "GET", params);
}

boost::future<nlohmann::json> BitBNSAsync::fetch_balance_async() {
    return fetch_async("/v1/account", "private", "GET");
}

boost::future<nlohmann::json> BitBNSAsync::fetch_deposits_async(const std::string& code, int since, int limit) {
    nlohmann::json params;
    if (!code.empty()) {
        params["currency"] = code;
    }
    if (since > 0) {
        params["since"] = since;
    }
    if (limit > 0) {
        params["limit"] = limit;
    }
    return fetch_async("/v1/depositHistory", "private", "GET", params);
}

boost::future<nlohmann::json> BitBNSAsync::fetch_withdrawals_async(const std::string& code, int since, int limit) {
    nlohmann::json params;
    if (!code.empty()) {
        params["currency"] = code;
    }
    if (since > 0) {
        params["since"] = since;
    }
    if (limit > 0) {
        params["limit"] = limit;
    }
    return fetch_async("/v1/withdrawHistory", "private", "GET", params);
}

boost::future<nlohmann::json> BitBNSAsync::fetch_deposit_address_async(const std::string& code) {
    return fetch_async("/v1/depositAddress", "private", "GET", {{"currency", code}});
}

} // namespace ccxt
