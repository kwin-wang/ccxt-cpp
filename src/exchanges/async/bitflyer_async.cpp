#include "ccxt/exchanges/async/bitflyer_async.h"

namespace ccxt {

BitflyerAsync::BitflyerAsync(const boost::asio::io_context& context)
    : ExchangeAsync(context)
    , Bitflyer() {}

boost::future<json> BitflyerAsync::fetchAsync(const String& path, const String& api,
                                           const String& method, const json& params,
                                           const std::map<String, String>& headers) {
    return ExchangeAsync::fetchAsync(path, api, method, params, headers);
}

boost::future<json> BitflyerAsync::fetchMarketsAsync(const json& params) {
    return fetchAsync("/v1/markets", "public", "GET", params);
}

boost::future<json> BitflyerAsync::fetchTickerAsync(const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    return fetchAsync("/v1/ticker", "public", "GET", {{"product_code", market_id}});
}

boost::future<json> BitflyerAsync::fetchTickersAsync(const std::vector<String>& symbols, const json& params) {
    return fetchAsync("/v1/ticker/all", "public", "GET", params);
}

boost::future<json> BitflyerAsync::fetchOrderBookAsync(const String& symbol, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (limit > 0) {
        request["size"] = limit;
    }
    return fetchAsync("/v1/board", "public", "GET", {{"product_code", market_id}});
}

boost::future<json> BitflyerAsync::fetchTradesAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (limit > 0) {
        request["count"] = limit;
    }
    if (since > 0) {
        request["before"] = since;
    }
    return fetchAsync("/v1/executions", "public", "GET", {{"product_code", market_id}});
}

boost::future<json> BitflyerAsync::fetchOHLCVAsync(const String& symbol, const String& timeframe,
                                                 int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (limit > 0) {
        request["count"] = limit;
    }
    if (since > 0) {
        request["before"] = since;
    }
    return fetchAsync("/v1/candlesticks", "public", "GET",
                     {{"product_code", market_id}, {"period", timeframe}});
}

boost::future<json> BitflyerAsync::fetchBalanceAsync(const json& params) {
    return fetchAsync("/v1/me/getbalance", "private", "GET", params);
}

boost::future<json> BitflyerAsync::createOrderAsync(const String& symbol, const String& type,
                                                  const String& side, double amount,
                                                  double price, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"product_code", market_id},
        {"child_order_type", type},
        {"side", side},
        {"size", std::to_string(amount)}
    };
    if (price > 0) {
        request["price"] = std::to_string(price);
    }
    request.update(params);
    return fetchAsync("/v1/me/sendchildorder", "private", "POST", request);
}

boost::future<json> BitflyerAsync::cancelOrderAsync(const String& id, const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"product_code", market_id},
        {"child_order_id", id}
    };
    request.update(params);
    return fetchAsync("/v1/me/cancelchildorder", "private", "POST", request);
}

boost::future<json> BitflyerAsync::fetchOrderAsync(const String& id, const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"product_code", market_id},
        {"child_order_id", id}
    };
    request.update(params);
    return fetchAsync("/v1/me/getchildorders", "private", "GET", request);
}

boost::future<json> BitflyerAsync::fetchOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {{"product_code", market_id}};
    if (since > 0) {
        request["before"] = since;
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    request.update(params);
    return fetchAsync("/v1/me/getchildorders", "private", "GET", request);
}

boost::future<json> BitflyerAsync::fetchOpenOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"product_code", market_id},
        {"child_order_state", "ACTIVE"}
    };
    if (since > 0) {
        request["before"] = since;
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    request.update(params);
    return fetchAsync("/v1/me/getchildorders", "private", "GET", request);
}

boost::future<json> BitflyerAsync::fetchClosedOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"product_code", market_id},
        {"child_order_state", "COMPLETED"}
    };
    if (since > 0) {
        request["before"] = since;
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    request.update(params);
    return fetchAsync("/v1/me/getchildorders", "private", "GET", request);
}

boost::future<json> BitflyerAsync::fetchMyTradesAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {{"product_code", market_id}};
    if (since > 0) {
        request["before"] = since;
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    request.update(params);
    return fetchAsync("/v1/me/getexecutions", "private", "GET", request);
}

boost::future<json> BitflyerAsync::fetchDepositsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["before"] = since;
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    return fetchAsync("/v1/me/getdeposits", "private", "GET", request);
}

boost::future<json> BitflyerAsync::fetchWithdrawalsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["before"] = since;
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    return fetchAsync("/v1/me/getwithdrawals", "private", "GET", request);
}

boost::future<json> BitflyerAsync::fetchDepositAddressAsync(const String& code, const json& params) {
    json request = {{"currency", code}};
    request.update(params);
    return fetchAsync("/v1/me/getaddresses", "private", "GET", request);
}

boost::future<json> BitflyerAsync::withdrawAsync(const String& code, double amount,
                                               const String& address, const String& tag,
                                               const json& params) {
    json request = {
        {"currency_code", code},
        {"amount", std::to_string(amount)},
        {"address", address}
    };
    if (!tag.empty()) {
        request["payment_id"] = tag;
    }
    request.update(params);
    return fetchAsync("/v1/me/withdraw", "private", "POST", request);
}

boost::future<json> BitflyerAsync::fetchCurrenciesAsync(const json& params) {
    return fetchAsync("/v1/me/getcurrencies", "private", "GET", params);
}

boost::future<json> BitflyerAsync::fetchTradingFeesAsync(const json& params) {
    return fetchAsync("/v1/me/gettradingcommission", "private", "GET", params);
}

boost::future<json> BitflyerAsync::fetchFundingFeesAsync(const json& params) {
    return fetchAsync("/v1/me/getwithdrawals", "private", "GET", params);
}

boost::future<json> BitflyerAsync::fetchTransactionsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["before"] = since;
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    return fetchAsync("/v1/me/getbalancehistory", "private", "GET", request);
}

} // namespace ccxt
