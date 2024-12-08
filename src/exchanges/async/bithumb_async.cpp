#include "ccxt/exchanges/async/bithumb_async.h"

namespace ccxt {

BithumbAsync::BithumbAsync(const boost::asio::io_context& context)
    : ExchangeAsync(context)
    , Bithumb() {}

boost::future<json> BithumbAsync::fetchAsync(const String& path, const String& api,
                                         const String& method, const json& params,
                                         const std::map<String, String>& headers) {
    return ExchangeAsync::fetchAsync(path, api, method, params, headers);
}

boost::future<json> BithumbAsync::fetchMarketsAsync(const json& params) {
    return fetchAsync("/public/ticker/ALL", "public", "GET", params);
}

boost::future<json> BithumbAsync::fetchTickerAsync(const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    return fetchAsync("/public/ticker/" + market_id, "public", "GET", params);
}

boost::future<json> BithumbAsync::fetchTickersAsync(const std::vector<String>& symbols, const json& params) {
    return fetchAsync("/public/ticker/ALL", "public", "GET", params);
}

boost::future<json> BithumbAsync::fetchOrderBookAsync(const String& symbol, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (limit > 0) {
        request["count"] = limit;
    }
    return fetchAsync("/public/orderbook/" + market_id, "public", "GET", request);
}

boost::future<json> BithumbAsync::fetchTradesAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (limit > 0) {
        request["count"] = limit;
    }
    return fetchAsync("/public/transaction_history/" + market_id, "public", "GET", request);
}

boost::future<json> BithumbAsync::fetchOHLCVAsync(const String& symbol, const String& timeframe,
                                               int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (since > 0) {
        request["time"] = since;
    }
    return fetchAsync("/public/candlestick/" + market_id + "/" + timeframe, "public", "GET", request);
}

boost::future<json> BithumbAsync::fetchBalanceAsync(const json& params) {
    return fetchAsync("/info/balance", "private", "GET", params);
}

boost::future<json> BithumbAsync::createOrderAsync(const String& symbol, const String& type,
                                               const String& side, double amount,
                                               double price, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"order_currency", market_id},
        {"units", std::to_string(amount)},
        {"type", side}
    };
    if (price > 0) {
        request["price"] = std::to_string(price);
    }
    request.update(params);
    String endpoint = (side == "bid") ? "/trade/place" : "/trade/place";
    return fetchAsync(endpoint, "private", "POST", request);
}

boost::future<json> BithumbAsync::cancelOrderAsync(const String& id, const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"order_id", id},
        {"order_currency", market_id}
    };
    request.update(params);
    return fetchAsync("/trade/cancel", "private", "POST", request);
}

boost::future<json> BithumbAsync::fetchOrderAsync(const String& id, const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"order_id", id},
        {"order_currency", market_id}
    };
    request.update(params);
    return fetchAsync("/info/order_detail", "private", "GET", request);
}

boost::future<json> BithumbAsync::fetchOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {{"order_currency", market_id}};
    if (since > 0) {
        request["after"] = since;
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    request.update(params);
    return fetchAsync("/info/orders", "private", "GET", request);
}

boost::future<json> BithumbAsync::fetchOpenOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"order_currency", market_id},
        {"status", "open"}
    };
    if (since > 0) {
        request["after"] = since;
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    request.update(params);
    return fetchAsync("/info/orders", "private", "GET", request);
}

boost::future<json> BithumbAsync::fetchClosedOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"order_currency", market_id},
        {"status", "completed"}
    };
    if (since > 0) {
        request["after"] = since;
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    request.update(params);
    return fetchAsync("/info/orders", "private", "GET", request);
}

boost::future<json> BithumbAsync::fetchMyTradesAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {{"order_currency", market_id}};
    if (since > 0) {
        request["after"] = since;
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    request.update(params);
    return fetchAsync("/info/user_transactions", "private", "GET", request);
}

boost::future<json> BithumbAsync::fetchDepositAddressAsync(const String& code, const json& params) {
    return fetchAsync("/info/wallet_address", "private", "GET", {{"currency", code}});
}

boost::future<json> BithumbAsync::fetchDepositsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["after"] = since;
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    return fetchAsync("/info/deposit_history", "private", "GET", request);
}

boost::future<json> BithumbAsync::fetchWithdrawalsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["after"] = since;
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    return fetchAsync("/info/withdrawal_history", "private", "GET", request);
}

boost::future<json> BithumbAsync::withdrawAsync(const String& code, double amount,
                                            const String& address, const String& tag,
                                            const json& params) {
    json request = {
        {"currency", code},
        {"units", std::to_string(amount)},
        {"address", address}
    };
    if (!tag.empty()) {
        request["destination"] = tag;
    }
    request.update(params);
    return fetchAsync("/trade/btc_withdrawal", "private", "POST", request);
}

boost::future<json> BithumbAsync::fetchTransactionsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["after"] = since;
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    return fetchAsync("/info/user_transactions", "private", "GET", request);
}

boost::future<json> BithumbAsync::fetchTradingFeesAsync(const json& params) {
    return fetchAsync("/info/trading_fee", "private", "GET", params);
}

boost::future<json> BithumbAsync::fetchFundingFeesAsync(const json& params) {
    return fetchAsync("/info/withdrawal_fee", "private", "GET", params);
}

} // namespace ccxt
