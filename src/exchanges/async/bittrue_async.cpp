#include "ccxt/exchanges/async/bittrue_async.h"

namespace ccxt {

BittrueAsync::BittrueAsync(const boost::asio::io_context& context)
    : ExchangeAsync(context)
    , Bittrue() {}

boost::future<json> BittrueAsync::fetchAsync(const String& path, const String& api,
                                         const String& method, const json& params,
                                         const std::map<String, String>& headers) {
    return ExchangeAsync::fetchAsync(path, api, method, params, headers);
}

boost::future<json> BittrueAsync::fetchTimeAsync(const json& params) {
    return fetchAsync("/api/v1/time", "public", "GET", params);
}

boost::future<json> BittrueAsync::fetchMarketsAsync(const json& params) {
    return fetchAsync("/api/v1/exchangeInfo", "public", "GET", params);
}

boost::future<json> BittrueAsync::fetchCurrenciesAsync(const json& params) {
    return fetchAsync("/api/v1/capital/config/getall", "private", "GET", params);
}

boost::future<json> BittrueAsync::fetchTickerAsync(const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    request["symbol"] = market_id;
    return fetchAsync("/api/v1/ticker/24hr", "public", "GET", request);
}

boost::future<json> BittrueAsync::fetchTickersAsync(const std::vector<String>& symbols, const json& params) {
    return fetchAsync("/api/v1/ticker/24hr", "public", "GET", params);
}

boost::future<json> BittrueAsync::fetchOrderBookAsync(const String& symbol, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    request["symbol"] = market_id;
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/depth", "public", "GET", request);
}

boost::future<json> BittrueAsync::fetchTradesAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    request["symbol"] = market_id;
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/trades", "public", "GET", request);
}

boost::future<json> BittrueAsync::fetchOHLCVAsync(const String& symbol, const String& timeframe,
                                              int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    request["symbol"] = market_id;
    request["interval"] = timeframe;
    if (since > 0) {
        request["startTime"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/klines", "public", "GET", request);
}

boost::future<json> BittrueAsync::fetchBalanceAsync(const json& params) {
    return fetchAsync("/api/v1/account", "private", "GET", params);
}

boost::future<json> BittrueAsync::createOrderAsync(const String& symbol, const String& type,
                                              const String& side, double amount,
                                              double price, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"symbol", market_id},
        {"side", side},
        {"type", type},
        {"quantity", this->amount_to_precision(symbol, amount)}
    };
    if ((type == "LIMIT") || (type == "STOP_LOSS_LIMIT") || (type == "TAKE_PROFIT_LIMIT")) {
        request["price"] = this->price_to_precision(symbol, price);
    }
    request.update(params);
    return fetchAsync("/api/v1/order", "private", "POST", request);
}

boost::future<json> BittrueAsync::cancelOrderAsync(const String& id, const String& symbol, const json& params) {
    json request = params;
    request["orderId"] = id;
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    return fetchAsync("/api/v1/order", "private", "DELETE", request);
}

boost::future<json> BittrueAsync::cancelAllOrdersAsync(const String& symbol, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    return fetchAsync("/api/v1/openOrders", "private", "DELETE", request);
}

boost::future<json> BittrueAsync::fetchOrderAsync(const String& id, const String& symbol, const json& params) {
    json request = params;
    request["orderId"] = id;
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    return fetchAsync("/api/v1/order", "private", "GET", request);
}

boost::future<json> BittrueAsync::fetchOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["startTime"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/allOrders", "private", "GET", request);
}

boost::future<json> BittrueAsync::fetchOpenOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["startTime"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/openOrders", "private", "GET", request);
}

boost::future<json> BittrueAsync::fetchClosedOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["startTime"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/historyOrders", "private", "GET", request);
}

boost::future<json> BittrueAsync::fetchMyTradesAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["startTime"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/myTrades", "private", "GET", request);
}

boost::future<json> BittrueAsync::fetchAccountsAsync(const json& params) {
    return fetchAsync("/api/v1/account", "private", "GET", params);
}

boost::future<json> BittrueAsync::fetchTradingFeesAsync(const json& params) {
    return fetchAsync("/api/v1/account/tradingFee", "private", "GET", params);
}

boost::future<json> BittrueAsync::fetchFundingFeesAsync(const json& params) {
    return fetchAsync("/api/v1/capital/config/getall", "private", "GET", params);
}

boost::future<json> BittrueAsync::fetchDepositAddressAsync(const String& code, const json& params) {
    json request = params;
    request["coin"] = code;
    return fetchAsync("/api/v1/capital/deposit/address", "private", "GET", request);
}

boost::future<json> BittrueAsync::fetchDepositsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["coin"] = code;
    }
    if (since > 0) {
        request["startTime"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/capital/deposit/hisrec", "private", "GET", request);
}

boost::future<json> BittrueAsync::fetchWithdrawalsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["coin"] = code;
    }
    if (since > 0) {
        request["startTime"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/capital/withdraw/history", "private", "GET", request);
}

boost::future<json> BittrueAsync::withdrawAsync(const String& code, double amount,
                                           const String& address, const String& tag,
                                           const json& params) {
    json request = {
        {"coin", code},
        {"amount", this->number_to_string(amount)},
        {"address", address}
    };
    if (!tag.empty()) {
        request["addressTag"] = tag;
    }
    request.update(params);
    return fetchAsync("/api/v1/capital/withdraw/apply", "private", "POST", request);
}

boost::future<json> BittrueAsync::fetchTransactionsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["coin"] = code;
    }
    if (since > 0) {
        request["startTime"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/capital/deposit/hisrec", "private", "GET", request);
}

} // namespace ccxt
