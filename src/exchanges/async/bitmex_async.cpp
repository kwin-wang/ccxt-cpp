#include "ccxt/exchanges/async/bitmex_async.h"

namespace ccxt {

BitmexAsync::BitmexAsync(const boost::asio::io_context& context)
    : ExchangeAsync(context)
    , Bitmex() {}

boost::future<json> BitmexAsync::fetchAsync(const String& path, const String& api,
                                         const String& method, const json& params,
                                         const std::map<String, String>& headers) {
    return ExchangeAsync::fetchAsync(path, api, method, params, headers);
}

boost::future<json> BitmexAsync::fetchMarketsAsync(const json& params) {
    return fetchAsync("/api/v1/instrument/active", "public", "GET", params);
}

boost::future<json> BitmexAsync::fetchCurrenciesAsync(const json& params) {
    return fetchAsync("/api/v1/user/wallet/currency", "private", "GET", params);
}

boost::future<json> BitmexAsync::fetchTickerAsync(const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    return fetchAsync("/api/v1/instrument/ticker", "public", "GET", {{"symbol", market_id}});
}

boost::future<json> BitmexAsync::fetchTickersAsync(const std::vector<String>& symbols, const json& params) {
    return fetchAsync("/api/v1/instrument/ticker", "public", "GET", params);
}

boost::future<json> BitmexAsync::fetchOrderBookAsync(const String& symbol, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {{"symbol", market_id}};
    if (limit > 0) {
        request["depth"] = limit;
    }
    return fetchAsync("/api/v1/orderBook/L2", "public", "GET", request);
}

boost::future<json> BitmexAsync::fetchTradesAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {{"symbol", market_id}};
    if (since > 0) {
        request["startTime"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    return fetchAsync("/api/v1/trade", "public", "GET", request);
}

boost::future<json> BitmexAsync::fetchOHLCVAsync(const String& symbol, const String& timeframe,
                                               int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"symbol", market_id},
        {"binSize", timeframe}
    };
    if (since > 0) {
        request["startTime"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    request.update(params);
    return fetchAsync("/api/v1/trade/bucketed", "public", "GET", request);
}

boost::future<json> BitmexAsync::fetchBalanceAsync(const json& params) {
    return fetchAsync("/api/v1/user/margin", "private", "GET", params);
}

boost::future<json> BitmexAsync::createOrderAsync(const String& symbol, const String& type,
                                               const String& side, double amount,
                                               double price, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"symbol", market_id},
        {"side", side},
        {"ordType", type},
        {"orderQty", amount}
    };
    if (price > 0) {
        request["price"] = price;
    }
    request.update(params);
    return fetchAsync("/api/v1/order", "private", "POST", request);
}

boost::future<json> BitmexAsync::editOrderAsync(const String& id, const String& symbol,
                                             const String& type, const String& side,
                                             double amount, double price,
                                             const json& params) {
    json request = {{"orderID", id}};
    if (amount > 0) {
        request["orderQty"] = amount;
    }
    if (price > 0) {
        request["price"] = price;
    }
    request.update(params);
    return fetchAsync("/api/v1/order", "private", "PUT", request);
}

boost::future<json> BitmexAsync::cancelOrderAsync(const String& id, const String& symbol, const json& params) {
    return fetchAsync("/api/v1/order", "private", "DELETE", {{"orderID", id}});
}

boost::future<json> BitmexAsync::cancelAllOrdersAsync(const String& symbol, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    return fetchAsync("/api/v1/order/all", "private", "DELETE", request);
}

boost::future<json> BitmexAsync::fetchOrderAsync(const String& id, const String& symbol, const json& params) {
    json request = {{"orderID", id}};
    request.update(params);
    return fetchAsync("/api/v1/order", "private", "GET", request);
}

boost::future<json> BitmexAsync::fetchOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["startTime"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    return fetchAsync("/api/v1/order", "private", "GET", request);
}

boost::future<json> BitmexAsync::fetchOpenOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    request["filter"] = {{"open", true}};
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["startTime"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    return fetchAsync("/api/v1/order", "private", "GET", request);
}

boost::future<json> BitmexAsync::fetchClosedOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    request["filter"] = {{"open", false}};
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["startTime"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    return fetchAsync("/api/v1/order", "private", "GET", request);
}

boost::future<json> BitmexAsync::fetchMyTradesAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["startTime"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    return fetchAsync("/api/v1/execution/tradeHistory", "private", "GET", request);
}

boost::future<json> BitmexAsync::fetchPositionsAsync(const String& symbols, const json& params) {
    json request = params;
    if (!symbols.empty()) {
        request["symbol"] = symbols;
    }
    return fetchAsync("/api/v1/position", "private", "GET", request);
}

boost::future<json> BitmexAsync::fetchPositionRiskAsync(const String& symbols, const json& params) {
    json request = params;
    if (!symbols.empty()) {
        request["symbol"] = symbols;
    }
    return fetchAsync("/api/v1/position/risk", "private", "GET", request);
}

boost::future<json> BitmexAsync::setLeverageAsync(int leverage, const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"symbol", market_id},
        {"leverage", leverage}
    };
    request.update(params);
    return fetchAsync("/api/v1/position/leverage", "private", "POST", request);
}

boost::future<json> BitmexAsync::setMarginModeAsync(const String& marginMode, const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"symbol", market_id},
        {"marginMode", marginMode}
    };
    request.update(params);
    return fetchAsync("/api/v1/position/margin", "private", "POST", request);
}

boost::future<json> BitmexAsync::fetchLeverageAsync(const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    return fetchAsync("/api/v1/position/leverage", "private", "GET", {{"symbol", market_id}});
}

boost::future<json> BitmexAsync::fetchLiquidationsAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {{"symbol", market_id}};
    if (since > 0) {
        request["startTime"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    request.update(params);
    return fetchAsync("/api/v1/liquidation", "public", "GET", request);
}

boost::future<json> BitmexAsync::fetchFundingRateAsync(const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    return fetchAsync("/api/v1/funding", "public", "GET", {{"symbol", market_id}});
}

boost::future<json> BitmexAsync::fetchFundingRatesAsync(const std::vector<String>& symbols, const json& params) {
    return fetchAsync("/api/v1/funding", "public", "GET", params);
}

boost::future<json> BitmexAsync::fetchFundingHistoryAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {{"symbol", market_id}};
    if (since > 0) {
        request["startTime"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    request.update(params);
    return fetchAsync("/api/v1/funding", "public", "GET", request);
}

boost::future<json> BitmexAsync::fetchIndexOHLCVAsync(const String& symbol, const String& timeframe,
                                                   int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"symbol", market_id},
        {"binSize", timeframe}
    };
    if (since > 0) {
        request["startTime"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    request.update(params);
    return fetchAsync("/api/v1/indexComposites", "public", "GET", request);
}

boost::future<json> BitmexAsync::fetchMarkOHLCVAsync(const String& symbol, const String& timeframe,
                                                  int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"symbol", market_id},
        {"binSize", timeframe}
    };
    if (since > 0) {
        request["startTime"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    request.update(params);
    return fetchAsync("/api/v1/markPrice", "public", "GET", request);
}

boost::future<json> BitmexAsync::fetchPremiumIndexOHLCVAsync(const String& symbol, const String& timeframe,
                                                          int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"symbol", market_id},
        {"binSize", timeframe}
    };
    if (since > 0) {
        request["startTime"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    request.update(params);
    return fetchAsync("/api/v1/premiumIndex", "public", "GET", request);
}

boost::future<json> BitmexAsync::fetchDepositAddressAsync(const String& code, const json& params) {
    return fetchAsync("/api/v1/user/depositAddress", "private", "GET", {{"currency", code}});
}

boost::future<json> BitmexAsync::fetchDepositsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["startTime"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    return fetchAsync("/api/v1/user/depositHistory", "private", "GET", request);
}

boost::future<json> BitmexAsync::fetchWithdrawalsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["startTime"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    return fetchAsync("/api/v1/user/withdrawalHistory", "private", "GET", request);
}

boost::future<json> BitmexAsync::withdrawAsync(const String& code, double amount,
                                            const String& address, const String& tag,
                                            const json& params) {
    json request = {
        {"currency", code},
        {"amount", amount},
        {"address", address}
    };
    if (!tag.empty()) {
        request["tag"] = tag;
    }
    request.update(params);
    return fetchAsync("/api/v1/user/requestWithdrawal", "private", "POST", request);
}

boost::future<json> BitmexAsync::fetchTransactionsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["startTime"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    return fetchAsync("/api/v1/user/walletHistory", "private", "GET", request);
}

boost::future<json> BitmexAsync::fetchWalletHistoryAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["startTime"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    return fetchAsync("/api/v1/user/walletSummary", "private", "GET", request);
}

boost::future<json> BitmexAsync::fetchTradingFeesAsync(const json& params) {
    return fetchAsync("/api/v1/user/commission", "private", "GET", params);
}

boost::future<json> BitmexAsync::fetchFundingFeesAsync(const json& params) {
    return fetchAsync("/api/v1/user/margin", "private", "GET", params);
}

} // namespace ccxt
