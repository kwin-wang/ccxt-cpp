#include "ccxt/exchanges/async/bitget_async.h"

namespace ccxt {

BitgetAsync::BitgetAsync(const boost::asio::io_context& context)
    : ExchangeAsync(context)
    , Bitget() {}

boost::future<json> BitgetAsync::fetchAsync(const String& path, const String& api,
                                         const String& method, const json& params,
                                         const std::map<String, String>& headers) {
    return ExchangeAsync::fetchAsync(path, api, method, params, headers);
}

boost::future<json> BitgetAsync::fetchMarketsAsync(const json& params) {
    return fetchAsync("/api/v2/spot/public/products", "public", "GET", params);
}

boost::future<json> BitgetAsync::fetchTickerAsync(const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    return fetchAsync("/api/v2/spot/market/ticker", "public", "GET", {{"symbol", market_id}});
}

boost::future<json> BitgetAsync::fetchTickersAsync(const std::vector<String>& symbols, const json& params) {
    return fetchAsync("/api/v2/spot/market/tickers", "public", "GET", params);
}

boost::future<json> BitgetAsync::fetchOrderBookAsync(const String& symbol, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {{"symbol", market_id}};
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v2/spot/market/depth", "public", "GET", request);
}

boost::future<json> BitgetAsync::fetchTradesAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {{"symbol", market_id}};
    if (limit > 0) {
        request["limit"] = limit;
    }
    if (since > 0) {
        request["startTime"] = since;
    }
    return fetchAsync("/api/v2/spot/market/fills", "public", "GET", request);
}

boost::future<json> BitgetAsync::fetchOHLCVAsync(const String& symbol, const String& timeframe,
                                               int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"symbol", market_id},
        {"period", timeframe}
    };
    if (limit > 0) {
        request["limit"] = limit;
    }
    if (since > 0) {
        request["startTime"] = since;
    }
    return fetchAsync("/api/v2/spot/market/candles", "public", "GET", request);
}

boost::future<json> BitgetAsync::fetchBalanceAsync(const json& params) {
    return fetchAsync("/api/v2/spot/account/assets", "private", "GET", params);
}

boost::future<json> BitgetAsync::createOrderAsync(const String& symbol, const String& type,
                                               const String& side, double amount,
                                               double price, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"symbol", market_id},
        {"side", side},
        {"orderType", type},
        {"size", std::to_string(amount)}
    };
    if (price > 0) {
        request["price"] = std::to_string(price);
    }
    request.update(params);
    return fetchAsync("/api/v2/spot/trade/place-order", "private", "POST", request);
}

boost::future<json> BitgetAsync::cancelOrderAsync(const String& id, const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"symbol", market_id},
        {"orderId", id}
    };
    request.update(params);
    return fetchAsync("/api/v2/spot/trade/cancel-order", "private", "POST", request);
}

boost::future<json> BitgetAsync::fetchOrderAsync(const String& id, const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"symbol", market_id},
        {"orderId", id}
    };
    request.update(params);
    return fetchAsync("/api/v2/spot/trade/order-info", "private", "GET", request);
}

boost::future<json> BitgetAsync::fetchOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {{"symbol", market_id}};
    if (since > 0) {
        request["startTime"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    request.update(params);
    return fetchAsync("/api/v2/spot/trade/history-orders", "private", "GET", request);
}

boost::future<json> BitgetAsync::fetchOpenOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {{"symbol", market_id}};
    if (since > 0) {
        request["startTime"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    request.update(params);
    return fetchAsync("/api/v2/spot/trade/open-orders", "private", "GET", request);
}

boost::future<json> BitgetAsync::fetchClosedOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"symbol", market_id},
        {"status", "closed"}
    };
    if (since > 0) {
        request["startTime"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    request.update(params);
    return fetchAsync("/api/v2/spot/trade/history-orders", "private", "GET", request);
}

boost::future<json> BitgetAsync::fetchPositionsAsync(const String& symbols, const json& params) {
    json request = params;
    if (!symbols.empty()) {
        request["symbol"] = symbols;
    }
    return fetchAsync("/api/v2/mix/position/positions", "private", "GET", request);
}

boost::future<json> BitgetAsync::fetchPositionRiskAsync(const String& symbols, const json& params) {
    json request = params;
    if (!symbols.empty()) {
        request["symbol"] = symbols;
    }
    return fetchAsync("/api/v2/mix/position/position-risk", "private", "GET", request);
}

boost::future<json> BitgetAsync::setLeverageAsync(int leverage, const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"symbol", market_id},
        {"leverage", leverage}
    };
    request.update(params);
    return fetchAsync("/api/v2/mix/account/set-leverage", "private", "POST", request);
}

boost::future<json> BitgetAsync::setMarginModeAsync(const String& marginMode, const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"symbol", market_id},
        {"marginMode", marginMode}
    };
    request.update(params);
    return fetchAsync("/api/v2/mix/account/set-margin-mode", "private", "POST", request);
}

boost::future<json> BitgetAsync::fetchFundingRateAsync(const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    return fetchAsync("/api/v2/mix/market/current-funding-rate", "public", "GET", {{"symbol", market_id}});
}

boost::future<json> BitgetAsync::fetchFundingRatesAsync(const std::vector<String>& symbols, const json& params) {
    return fetchAsync("/api/v2/mix/market/funding-rates", "public", "GET", params);
}

boost::future<json> BitgetAsync::fetchFundingHistoryAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {{"symbol", market_id}};
    if (since > 0) {
        request["startTime"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    request.update(params);
    return fetchAsync("/api/v2/mix/market/funding-history", "public", "GET", request);
}

boost::future<json> BitgetAsync::fetchMyTradesAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {{"symbol", market_id}};
    if (since > 0) {
        request["startTime"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    request.update(params);
    return fetchAsync("/api/v2/spot/trade/fills", "private", "GET", request);
}

boost::future<json> BitgetAsync::fetchDepositsAsync(const String& code, int since, int limit, const json& params) {
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
    return fetchAsync("/api/v2/spot/wallet/deposit-history", "private", "GET", request);
}

boost::future<json> BitgetAsync::fetchWithdrawalsAsync(const String& code, int since, int limit, const json& params) {
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
    return fetchAsync("/api/v2/spot/wallet/withdrawal-history", "private", "GET", request);
}

boost::future<json> BitgetAsync::fetchDepositAddressAsync(const String& code, const json& params) {
    return fetchAsync("/api/v2/spot/wallet/deposit-address", "private", "GET", {{"coin", code}});
}

boost::future<json> BitgetAsync::withdrawAsync(const String& code, double amount,
                                            const String& address, const String& tag,
                                            const json& params) {
    json request = {
        {"coin", code},
        {"amount", std::to_string(amount)},
        {"address", address}
    };
    if (!tag.empty()) {
        request["tag"] = tag;
    }
    request.update(params);
    return fetchAsync("/api/v2/spot/wallet/withdrawal", "private", "POST", request);
}

boost::future<json> BitgetAsync::fetchTransactionsAsync(const String& code, int since, int limit, const json& params) {
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
    return fetchAsync("/api/v2/spot/wallet/bills", "private", "GET", request);
}

boost::future<json> BitgetAsync::fetchLedgerAsync(const String& code, int since, int limit, const json& params) {
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
    return fetchAsync("/api/v2/spot/account/bills", "private", "GET", request);
}

boost::future<json> BitgetAsync::fetchTradingFeesAsync(const json& params) {
    return fetchAsync("/api/v2/spot/account/trading-fees", "private", "GET", params);
}

boost::future<json> BitgetAsync::fetchFundingFeesAsync(const json& params) {
    return fetchAsync("/api/v2/spot/wallet/withdrawal-fees", "private", "GET", params);
}

} // namespace ccxt
