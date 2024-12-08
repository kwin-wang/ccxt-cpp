#include "ccxt/exchanges/async/blofin_async.h"

namespace ccxt {

BlofinAsync::BlofinAsync(const boost::asio::io_context& context)
    : ExchangeAsync(context)
    , Blofin() {}

boost::future<json> BlofinAsync::fetchAsync(const String& path, const String& api,
                                          const String& method, const json& params,
                                          const std::map<String, String>& headers) {
    return ExchangeAsync::fetchAsync(path, api, method, params, headers);
}

boost::future<json> BlofinAsync::fetchTimeAsync(const json& params) {
    return fetchAsync("/api/v1/time", "public", "GET", params);
}

boost::future<json> BlofinAsync::fetchMarketsAsync(const json& params) {
    return fetchAsync("/api/v1/instruments", "public", "GET", params);
}

boost::future<json> BlofinAsync::fetchCurrenciesAsync(const json& params) {
    return fetchAsync("/api/v1/assets", "public", "GET", params);
}

boost::future<json> BlofinAsync::fetchTickerAsync(const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    return fetchAsync("/api/v1/tickers/" + market_id, "public", "GET", params);
}

boost::future<json> BlofinAsync::fetchTickersAsync(const std::vector<String>& symbols, const json& params) {
    return fetchAsync("/api/v1/tickers", "public", "GET", params);
}

boost::future<json> BlofinAsync::fetchOrderBookAsync(const String& symbol, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/depth/" + market_id, "public", "GET", request);
}

boost::future<json> BlofinAsync::fetchTradesAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (limit > 0) {
        request["limit"] = limit;
    }
    if (since > 0) {
        request["startTime"] = since;
    }
    return fetchAsync("/api/v1/trades/" + market_id, "public", "GET", request);
}

boost::future<json> BlofinAsync::fetchOHLCVAsync(const String& symbol, const String& timeframe,
                                                int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    request["interval"] = timeframe;
    if (since > 0) {
        request["startTime"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/klines/" + market_id, "public", "GET", request);
}

boost::future<json> BlofinAsync::fetchBalanceAsync(const json& params) {
    return fetchAsync("/api/v1/account/balance", "private", "GET", params);
}

boost::future<json> BlofinAsync::createOrderAsync(const String& symbol, const String& type,
                                                const String& side, double amount,
                                                double price, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"instId", market_id},
        {"side", side},
        {"ordType", type},
        {"sz", this->amount_to_precision(symbol, amount)}
    };
    if ((type == "limit") && (price > 0)) {
        request["px"] = this->price_to_precision(symbol, price);
    }
    request.update(params);
    return fetchAsync("/api/v1/trade/order", "private", "POST", request);
}

boost::future<json> BlofinAsync::cancelOrderAsync(const String& id, const String& symbol, const json& params) {
    json request = params;
    request["ordId"] = id;
    if (!symbol.empty()) {
        request["instId"] = this->market_id(symbol);
    }
    return fetchAsync("/api/v1/trade/cancel-order", "private", "POST", request);
}

boost::future<json> BlofinAsync::cancelAllOrdersAsync(const String& symbol, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["instId"] = this->market_id(symbol);
    }
    return fetchAsync("/api/v1/trade/cancel-batch-orders", "private", "POST", request);
}

boost::future<json> BlofinAsync::fetchOrderAsync(const String& id, const String& symbol, const json& params) {
    json request = params;
    request["ordId"] = id;
    if (!symbol.empty()) {
        request["instId"] = this->market_id(symbol);
    }
    return fetchAsync("/api/v1/trade/order", "private", "GET", request);
}

boost::future<json> BlofinAsync::fetchOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["instId"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["startTime"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/trade/orders", "private", "GET", request);
}

boost::future<json> BlofinAsync::fetchOpenOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["instId"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["startTime"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/trade/open-orders", "private", "GET", request);
}

boost::future<json> BlofinAsync::fetchClosedOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["instId"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["startTime"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/trade/closed-orders", "private", "GET", request);
}

boost::future<json> BlofinAsync::fetchMyTradesAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["instId"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["startTime"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/trade/fills", "private", "GET", request);
}

boost::future<json> BlofinAsync::fetchPositionsAsync(const std::vector<String>& symbols, const json& params) {
    json request = params;
    if (!symbols.empty()) {
        request["instIds"] = symbols;
    }
    return fetchAsync("/api/v1/account/positions", "private", "GET", request);
}

boost::future<json> BlofinAsync::fetchPositionAsync(const String& symbol, const json& params) {
    json request = params;
    request["instId"] = this->market_id(symbol);
    return fetchAsync("/api/v1/account/position", "private", "GET", request);
}

boost::future<json> BlofinAsync::fetchAccountsAsync(const json& params) {
    return fetchAsync("/api/v1/account/accounts", "private", "GET", params);
}

boost::future<json> BlofinAsync::fetchTradingFeesAsync(const json& params) {
    return fetchAsync("/api/v1/account/trade-fee", "private", "GET", params);
}

boost::future<json> BlofinAsync::fetchFundingFeesAsync(const json& params) {
    return fetchAsync("/api/v1/account/funding-fee", "private", "GET", params);
}

boost::future<json> BlofinAsync::fetchLeverageAsync(const String& symbol, const json& params) {
    json request = params;
    request["instId"] = this->market_id(symbol);
    return fetchAsync("/api/v1/account/leverage", "private", "GET", request);
}

boost::future<json> BlofinAsync::fetchDepositAddressAsync(const String& code, const json& params) {
    json request = params;
    request["ccy"] = code;
    return fetchAsync("/api/v1/asset/deposit-address", "private", "GET", request);
}

boost::future<json> BlofinAsync::fetchDepositsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["ccy"] = code;
    }
    if (since > 0) {
        request["startTime"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/asset/deposit-history", "private", "GET", request);
}

boost::future<json> BlofinAsync::fetchWithdrawalsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["ccy"] = code;
    }
    if (since > 0) {
        request["startTime"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/asset/withdrawal-history", "private", "GET", request);
}

boost::future<json> BlofinAsync::withdrawAsync(const String& code, double amount,
                                             const String& address, const String& tag,
                                             const json& params) {
    json request = {
        {"ccy", code},
        {"amt", this->number_to_string(amount)},
        {"addr", address}
    };
    if (!tag.empty()) {
        request["tag"] = tag;
    }
    request.update(params);
    return fetchAsync("/api/v1/asset/withdrawal", "private", "POST", request);
}

boost::future<json> BlofinAsync::fetchTransactionsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["ccy"] = code;
    }
    if (since > 0) {
        request["startTime"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/asset/bills", "private", "GET", request);
}

boost::future<json> BlofinAsync::fetchFundingRateAsync(const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    return fetchAsync("/api/v1/public/funding-rate/" + market_id, "public", "GET", params);
}

boost::future<json> BlofinAsync::fetchFundingRateHistoryAsync(const String& symbol,
                                                             int since, int limit,
                                                             const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["instId"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["startTime"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/public/funding-rate-history", "public", "GET", request);
}

boost::future<json> BlofinAsync::fetchFundingHistoryAsync(const String& symbol,
                                                         int since, int limit,
                                                         const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["instId"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["startTime"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/trade/funding-history", "private", "GET", request);
}

} // namespace ccxt
