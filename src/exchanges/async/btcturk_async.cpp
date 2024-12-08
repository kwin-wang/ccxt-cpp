#include "ccxt/exchanges/async/btcturk_async.h"

namespace ccxt {

BtcturkAsync::BtcturkAsync(const boost::asio::io_context& context)
    : ExchangeAsync(context)
    , Btcturk() {}

boost::future<json> BtcturkAsync::fetchAsync(const String& path, const String& api,
                                           const String& method, const json& params,
                                           const std::map<String, String>& headers) {
    return ExchangeAsync::fetchAsync(path, api, method, params, headers);
}

boost::future<json> BtcturkAsync::fetchMarketsAsync(const json& params) {
    return fetchAsync("/api/v2/server/exchangeinfo", "public", "GET", params);
}

boost::future<json> BtcturkAsync::fetchCurrenciesAsync(const json& params) {
    return fetchAsync("/api/v2/server/exchangeinfo", "public", "GET", params);
}

boost::future<json> BtcturkAsync::fetchTickerAsync(const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    return fetchAsync("/api/v2/ticker?pairSymbol=" + market_id, "public", "GET", params);
}

boost::future<json> BtcturkAsync::fetchTickersAsync(const std::vector<String>& symbols, const json& params) {
    return fetchAsync("/api/v2/ticker", "public", "GET", params);
}

boost::future<json> BtcturkAsync::fetchOrderBookAsync(const String& symbol, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v2/orderbook?pairSymbol=" + market_id, "public", "GET", request);
}

boost::future<json> BtcturkAsync::fetchTradesAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (limit > 0) {
        request["last"] = limit;
    }
    return fetchAsync("/api/v2/trades?pairSymbol=" + market_id, "public", "GET", request);
}

boost::future<json> BtcturkAsync::fetchOHLCVAsync(const String& symbol, const String& timeframe,
                                                 int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    request["resolution"] = timeframe;
    if (since > 0) {
        request["from"] = since;
    }
    if (limit > 0) {
        request["to"] = this->sum(since, limit * this->parse_timeframe(timeframe) * 1000);
    }
    return fetchAsync("/api/v2/ohlc?pairSymbol=" + market_id, "public", "GET", request);
}

boost::future<json> BtcturkAsync::fetchBalanceAsync(const json& params) {
    return fetchAsync("/api/v1/users/balances", "private", "GET", params);
}

boost::future<json> BtcturkAsync::createOrderAsync(const String& symbol, const String& type,
                                                 const String& side, double amount,
                                                 double price, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"pairSymbol", market_id},
        {"orderType", side},
        {"orderMethod", type},
        {"quantity", this->amount_to_precision(symbol, amount)}
    };
    if ((type == "limit") && (price > 0)) {
        request["price"] = this->price_to_precision(symbol, price);
    }
    request.update(params);
    return fetchAsync("/api/v1/order", "private", "POST", request);
}

boost::future<json> BtcturkAsync::cancelOrderAsync(const String& id, const String& symbol, const json& params) {
    return fetchAsync("/api/v1/order?id=" + id, "private", "DELETE", params);
}

boost::future<json> BtcturkAsync::cancelAllOrdersAsync(const String& symbol, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["pairSymbol"] = this->market_id(symbol);
    }
    return fetchAsync("/api/v1/order/all", "private", "DELETE", request);
}

boost::future<json> BtcturkAsync::fetchOrderAsync(const String& id, const String& symbol, const json& params) {
    return fetchAsync("/api/v1/order?id=" + id, "private", "GET", params);
}

boost::future<json> BtcturkAsync::fetchOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["pairSymbol"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["startTime"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/allOrders", "private", "GET", request);
}

boost::future<json> BtcturkAsync::fetchOpenOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["pairSymbol"] = this->market_id(symbol);
    }
    return fetchAsync("/api/v1/openOrders", "private", "GET", request);
}

boost::future<json> BtcturkAsync::fetchClosedOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["pairSymbol"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["startTime"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/allOrders", "private", "GET", request);
}

boost::future<json> BtcturkAsync::fetchMyTradesAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["pairSymbol"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["startTime"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/userTrades", "private", "GET", request);
}

boost::future<json> BtcturkAsync::fetchTradingFeesAsync(const json& params) {
    return fetchAsync("/api/v2/server/exchangeinfo", "public", "GET", params);
}

boost::future<json> BtcturkAsync::fetchDepositAddressAsync(const String& code, const json& params) {
    json request = {
        {"currency", code}
    };
    request.update(params);
    return fetchAsync("/api/v1/deposit/address", "private", "GET", request);
}

boost::future<json> BtcturkAsync::fetchDepositsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["startTime"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/deposits", "private", "GET", request);
}

boost::future<json> BtcturkAsync::fetchWithdrawalsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["startTime"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/withdrawals", "private", "GET", request);
}

boost::future<json> BtcturkAsync::withdrawAsync(const String& code, double amount,
                                              const String& address, const String& tag,
                                              const json& params) {
    json request = {
        {"currency", code},
        {"amount", this->number_to_string(amount)},
        {"address", address}
    };
    if (!tag.empty()) {
        request["tag"] = tag;
    }
    request.update(params);
    return fetchAsync("/api/v1/withdraw", "private", "POST", request);
}

boost::future<json> BtcturkAsync::fetchTransactionsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["startTime"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/transactions", "private", "GET", request);
}

} // namespace ccxt
