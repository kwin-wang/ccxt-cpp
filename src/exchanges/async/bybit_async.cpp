#include "ccxt/exchanges/async/bybit_async.h"

namespace ccxt {

BybitAsync::BybitAsync(const boost::asio::io_context& context)
    : ExchangeAsync(context)
    , Bybit() {}

boost::future<json> BybitAsync::fetchAsync(const String& path, const String& api,
                                         const String& method, const json& params,
                                         const std::map<String, String>& headers) {
    return ExchangeAsync::fetchAsync(path, api, method, params, headers);
}

boost::future<json> BybitAsync::fetchTimeAsync(const json& params) {
    return fetchAsync("/v5/market/time", "public", "GET", params);
}

boost::future<json> BybitAsync::fetchMarketsAsync(const json& params) {
    return fetchAsync("/v5/market/instruments-info", "public", "GET", params);
}

boost::future<json> BybitAsync::fetchCurrenciesAsync(const json& params) {
    return fetchAsync("/v5/asset/coins", "public", "GET", params);
}

boost::future<json> BybitAsync::fetchTickerAsync(const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"symbol", market_id}
    };
    request.update(params);
    return fetchAsync("/v5/market/tickers", "public", "GET", request);
}

boost::future<json> BybitAsync::fetchTickersAsync(const std::vector<String>& symbols, const json& params) {
    json request = params;
    if (!symbols.empty()) {
        request["symbols"] = symbols;
    }
    return fetchAsync("/v5/market/tickers", "public", "GET", request);
}

boost::future<json> BybitAsync::fetchOrderBookAsync(const String& symbol, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"symbol", market_id}
    };
    if (limit > 0) {
        request["limit"] = limit;
    }
    request.update(params);
    return fetchAsync("/v5/market/orderbook", "public", "GET", request);
}

boost::future<json> BybitAsync::fetchTradesAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"symbol", market_id}
    };
    if (limit > 0) {
        request["limit"] = limit;
    }
    request.update(params);
    return fetchAsync("/v5/market/trades", "public", "GET", request);
}

boost::future<json> BybitAsync::fetchOHLCVAsync(const String& symbol, const String& timeframe,
                                               int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"symbol", market_id},
        {"interval", timeframe}
    };
    if (since > 0) {
        request["start"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    request.update(params);
    return fetchAsync("/v5/market/kline", "public", "GET", request);
}

boost::future<json> BybitAsync::fetchBalanceAsync(const json& params) {
    return fetchAsync("/v5/account/wallet-balance", "private", "GET", params);
}

boost::future<json> BybitAsync::createOrderAsync(const String& symbol, const String& type,
                                               const String& side, double amount,
                                               double price, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"symbol", market_id},
        {"side", side},
        {"orderType", type},
        {"qty", this->amount_to_precision(symbol, amount)}
    };
    if ((type == "Limit") && (price > 0)) {
        request["price"] = this->price_to_precision(symbol, price);
    }
    request.update(params);
    return fetchAsync("/v5/order/create", "private", "POST", request);
}

boost::future<json> BybitAsync::cancelOrderAsync(const String& id, const String& symbol, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    request["orderId"] = id;
    return fetchAsync("/v5/order/cancel", "private", "POST", request);
}

boost::future<json> BybitAsync::cancelAllOrdersAsync(const String& symbol, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    return fetchAsync("/v5/order/cancel-all", "private", "POST", request);
}

boost::future<json> BybitAsync::editOrderAsync(const String& id, const String& symbol,
                                             const String& type, const String& side,
                                             double amount, double price,
                                             const json& params) {
    json request = params;
    request["orderId"] = id;
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    if (amount > 0) {
        request["qty"] = this->amount_to_precision(symbol, amount);
    }
    if (price > 0) {
        request["price"] = this->price_to_precision(symbol, price);
    }
    return fetchAsync("/v5/order/amend", "private", "POST", request);
}

boost::future<json> BybitAsync::fetchOrderAsync(const String& id, const String& symbol, const json& params) {
    json request = params;
    request["orderId"] = id;
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    return fetchAsync("/v5/order/history", "private", "GET", request);
}

boost::future<json> BybitAsync::fetchOrdersAsync(const String& symbol, int since, int limit, const json& params) {
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
    return fetchAsync("/v5/order/history", "private", "GET", request);
}

boost::future<json> BybitAsync::fetchOpenOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/v5/order/realtime", "private", "GET", request);
}

boost::future<json> BybitAsync::fetchClosedOrdersAsync(const String& symbol, int since, int limit, const json& params) {
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
    return fetchAsync("/v5/order/history", "private", "GET", request);
}

boost::future<json> BybitAsync::fetchMyTradesAsync(const String& symbol, int since, int limit, const json& params) {
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
    return fetchAsync("/v5/execution/list", "private", "GET", request);
}

boost::future<json> BybitAsync::fetchPositionsAsync(const std::vector<String>& symbols, const json& params) {
    json request = params;
    if (!symbols.empty()) {
        request["symbol"] = this->market_ids(symbols);
    }
    return fetchAsync("/v5/position/list", "private", "GET", request);
}

boost::future<json> BybitAsync::fetchPositionAsync(const String& symbol, const json& params) {
    json request = params;
    request["symbol"] = this->market_id(symbol);
    return fetchAsync("/v5/position/list", "private", "GET", request);
}

boost::future<json> BybitAsync::setLeverageAsync(double leverage, const String& symbol, const json& params) {
    json request = {
        {"leverage", leverage}
    };
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    request.update(params);
    return fetchAsync("/v5/position/set-leverage", "private", "POST", request);
}

boost::future<json> BybitAsync::setMarginModeAsync(const String& marginMode,
                                                  const String& symbol,
                                                  const json& params) {
    json request = {
        {"marginMode", marginMode}
    };
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    request.update(params);
    return fetchAsync("/v5/position/switch-isolated", "private", "POST", request);
}

boost::future<json> BybitAsync::fetchTradingFeesAsync(const json& params) {
    return fetchAsync("/v5/account/fee-rate", "private", "GET", params);
}

boost::future<json> BybitAsync::fetchFundingRateAsync(const String& symbol, const json& params) {
    json request = {
        {"symbol", this->market_id(symbol)}
    };
    request.update(params);
    return fetchAsync("/v5/market/funding/history", "public", "GET", request);
}

boost::future<json> BybitAsync::fetchFundingRateHistoryAsync(const String& symbol,
                                                            int since, int limit,
                                                            const json& params) {
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
    return fetchAsync("/v5/market/funding/history", "public", "GET", request);
}

boost::future<json> BybitAsync::fetchFundingHistoryAsync(const String& symbol,
                                                        int since, int limit,
                                                        const json& params) {
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
    return fetchAsync("/v5/account/funding-records", "private", "GET", request);
}

boost::future<json> BybitAsync::fetchDepositAddressAsync(const String& code, const json& params) {
    json request = {
        {"coin", code}
    };
    request.update(params);
    return fetchAsync("/v5/asset/deposit/query-address", "private", "GET", request);
}

boost::future<json> BybitAsync::fetchDepositsAsync(const String& code, int since, int limit, const json& params) {
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
    return fetchAsync("/v5/asset/deposit/query-record", "private", "GET", request);
}

boost::future<json> BybitAsync::fetchWithdrawalsAsync(const String& code, int since, int limit, const json& params) {
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
    return fetchAsync("/v5/asset/withdraw/query-record", "private", "GET", request);
}

boost::future<json> BybitAsync::withdrawAsync(const String& code, double amount,
                                            const String& address, const String& tag,
                                            const json& params) {
    json request = {
        {"coin", code},
        {"amount", this->number_to_string(amount)},
        {"address", address}
    };
    if (!tag.empty()) {
        request["tag"] = tag;
    }
    request.update(params);
    return fetchAsync("/v5/asset/withdraw/create", "private", "POST", request);
}

boost::future<json> BybitAsync::fetchTransactionsAsync(const String& code, int since, int limit, const json& params) {
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
    return fetchAsync("/v5/asset/transfer/query-asset-info", "private", "GET", request);
}

} // namespace ccxt
