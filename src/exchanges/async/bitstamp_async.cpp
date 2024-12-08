#include "ccxt/exchanges/async/bitstamp_async.h"

namespace ccxt {

BitstampAsync::BitstampAsync(const boost::asio::io_context& context)
    : ExchangeAsync(context)
    , Bitstamp() {}

boost::future<json> BitstampAsync::fetchAsync(const String& path, const String& api,
                                          const String& method, const json& params,
                                          const std::map<String, String>& headers) {
    return ExchangeAsync::fetchAsync(path, api, method, params, headers);
}

boost::future<json> BitstampAsync::fetchTimeAsync(const json& params) {
    return fetchAsync("/api/v2/time", "public", "GET", params);
}

boost::future<json> BitstampAsync::fetchMarketsAsync(const json& params) {
    return fetchAsync("/api/v2/trading-pairs-info", "public", "GET", params);
}

boost::future<json> BitstampAsync::fetchCurrenciesAsync(const json& params) {
    return fetchAsync("/api/v2/currencies", "public", "GET", params);
}

boost::future<json> BitstampAsync::fetchTickerAsync(const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    return fetchAsync("/api/v2/ticker/" + market_id, "public", "GET", params);
}

boost::future<json> BitstampAsync::fetchTickersAsync(const std::vector<String>& symbols, const json& params) {
    return fetchAsync("/api/v2/ticker", "public", "GET", params);
}

boost::future<json> BitstampAsync::fetchOrderBookAsync(const String& symbol, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (limit > 0) {
        request["limit_orders"] = limit;
    }
    return fetchAsync("/api/v2/order_book/" + market_id, "public", "GET", request);
}

boost::future<json> BitstampAsync::fetchTradesAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v2/transactions/" + market_id, "public", "GET", request);
}

boost::future<json> BitstampAsync::fetchOHLCVAsync(const String& symbol, const String& timeframe,
                                               int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    request["step"] = this->timeframes[timeframe];
    if (since > 0) {
        request["start"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v2/ohlc/" + market_id, "public", "GET", request);
}

boost::future<json> BitstampAsync::fetchBalanceAsync(const json& params) {
    return fetchAsync("/api/v2/balance", "private", "POST", params);
}

boost::future<json> BitstampAsync::createOrderAsync(const String& symbol, const String& type,
                                               const String& side, double amount,
                                               double price, const json& params) {
    String market_id = this->market_id(symbol);
    String order_type = side + "/" + type;
    json request = {
        {"amount", this->amount_to_precision(symbol, amount)}
    };
    if ((type == "limit") && (price > 0)) {
        request["price"] = this->price_to_precision(symbol, price);
    }
    request.update(params);
    return fetchAsync("/api/v2/" + order_type + "/" + market_id, "private", "POST", request);
}

boost::future<json> BitstampAsync::cancelOrderAsync(const String& id, const String& symbol, const json& params) {
    json request = params;
    request["id"] = id;
    return fetchAsync("/api/v2/cancel_order", "private", "POST", request);
}

boost::future<json> BitstampAsync::cancelAllOrdersAsync(const String& symbol, const json& params) {
    return fetchAsync("/api/v2/cancel_all_orders", "private", "POST", params);
}

boost::future<json> BitstampAsync::fetchOrderAsync(const String& id, const String& symbol, const json& params) {
    json request = params;
    request["id"] = id;
    return fetchAsync("/api/v2/order_status", "private", "POST", request);
}

boost::future<json> BitstampAsync::fetchOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["pair"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["time"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v2/orders", "private", "POST", request);
}

boost::future<json> BitstampAsync::fetchOpenOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["pair"] = this->market_id(symbol);
    }
    return fetchAsync("/api/v2/open_orders/all", "private", "POST", request);
}

boost::future<json> BitstampAsync::fetchClosedOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["pair"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["time"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v2/user_transactions", "private", "POST", request);
}

boost::future<json> BitstampAsync::fetchMyTradesAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["pair"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["time"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v2/user_transactions", "private", "POST", request);
}

boost::future<json> BitstampAsync::fetchAccountsAsync(const json& params) {
    return fetchAsync("/api/v2/balance", "private", "POST", params);
}

boost::future<json> BitstampAsync::fetchTradingFeesAsync(const json& params) {
    return fetchAsync("/api/v2/fees/trading", "private", "POST", params);
}

boost::future<json> BitstampAsync::fetchFundingFeesAsync(const json& params) {
    return fetchAsync("/api/v2/fees/funding", "private", "POST", params);
}

boost::future<json> BitstampAsync::fetchDepositAddressAsync(const String& code, const json& params) {
    json request = params;
    request["currency"] = code;
    return fetchAsync("/api/v2/deposit-address", "private", "POST", request);
}

boost::future<json> BitstampAsync::fetchDepositsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["time"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v2/user_transactions", "private", "POST", request);
}

boost::future<json> BitstampAsync::fetchWithdrawalsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["time"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v2/user_transactions", "private", "POST", request);
}

boost::future<json> BitstampAsync::withdrawAsync(const String& code, double amount,
                                            const String& address, const String& tag,
                                            const json& params) {
    json request = {
        {"amount", this->number_to_string(amount)},
        {"address", address}
    };
    if (!tag.empty()) {
        request["destination_tag"] = tag;
    }
    request.update(params);
    return fetchAsync("/api/v2/withdrawal/" + code, "private", "POST", request);
}

boost::future<json> BitstampAsync::fetchTransactionsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["time"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v2/user_transactions", "private", "POST", request);
}

boost::future<json> BitstampAsync::fetchLedgerAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["time"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v2/user_transactions", "private", "POST", request);
}

} // namespace ccxt
