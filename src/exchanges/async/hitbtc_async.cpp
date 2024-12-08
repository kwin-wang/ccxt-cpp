#include "ccxt/exchanges/async/hitbtc_async.h"

namespace ccxt {

HitbtcAsync::HitbtcAsync(const boost::asio::io_context& context)
    : ExchangeAsync(context)
    , Hitbtc() {}

boost::future<json> HitbtcAsync::fetchAsync(const String& path, const String& api,
                                          const String& method, const json& params,
                                          const std::map<String, String>& headers) {
    return ExchangeAsync::fetchAsync(path, api, method, params, headers);
}

boost::future<json> HitbtcAsync::fetchMarketsAsync(const json& params) {
    return fetchAsync("/api/3/public/symbol", "public", "GET", params);
}

boost::future<json> HitbtcAsync::fetchCurrenciesAsync(const json& params) {
    return fetchAsync("/api/3/public/currency", "public", "GET", params);
}

boost::future<json> HitbtcAsync::fetchTickerAsync(const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    return fetchAsync("/api/3/public/ticker/" + market_id, "public", "GET", params);
}

boost::future<json> HitbtcAsync::fetchTickersAsync(const std::vector<String>& symbols, const json& params) {
    return fetchAsync("/api/3/public/ticker", "public", "GET", params);
}

boost::future<json> HitbtcAsync::fetchOrderBookAsync(const String& symbol, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/3/public/orderbook/" + market_id, "public", "GET", request);
}

boost::future<json> HitbtcAsync::fetchTradesAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (limit > 0) {
        request["limit"] = limit;
    }
    if (since > 0) {
        request["from"] = this->iso8601(since);
    }
    return fetchAsync("/api/3/public/trades/" + market_id, "public", "GET", request);
}

boost::future<json> HitbtcAsync::fetchOHLCVAsync(const String& symbol, const String& timeframe,
                                                int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    request["period"] = timeframe;
    if (since > 0) {
        request["from"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/3/public/candles/" + market_id, "public", "GET", request);
}

boost::future<json> HitbtcAsync::fetchBalanceAsync(const json& params) {
    return fetchAsync("/api/3/spot/balance", "private", "GET", params);
}

boost::future<json> HitbtcAsync::createOrderAsync(const String& symbol, const String& type,
                                                const String& side, double amount,
                                                double price, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"symbol", market_id},
        {"side", side},
        {"type", type},
        {"quantity", this->amount_to_precision(symbol, amount)}
    };
    if ((type == "limit") && (price > 0)) {
        request["price"] = this->price_to_precision(symbol, price);
    }
    request.update(params);
    return fetchAsync("/api/3/spot/order", "private", "POST", request);
}

boost::future<json> HitbtcAsync::cancelOrderAsync(const String& id, const String& symbol, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    return fetchAsync("/api/3/spot/order/" + id, "private", "DELETE", request);
}

boost::future<json> HitbtcAsync::cancelAllOrdersAsync(const String& symbol, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    return fetchAsync("/api/3/spot/order", "private", "DELETE", request);
}

boost::future<json> HitbtcAsync::fetchOrderAsync(const String& id, const String& symbol, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    return fetchAsync("/api/3/spot/order/" + id, "private", "GET", request);
}

boost::future<json> HitbtcAsync::fetchOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["from"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/3/spot/history/order", "private", "GET", request);
}

boost::future<json> HitbtcAsync::fetchOpenOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["from"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/3/spot/order", "private", "GET", request);
}

boost::future<json> HitbtcAsync::fetchClosedOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["from"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    request["status"] = "filled,cancelled";
    return fetchAsync("/api/3/spot/history/order", "private", "GET", request);
}

boost::future<json> HitbtcAsync::fetchMyTradesAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["from"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/3/spot/history/trade", "private", "GET", request);
}

boost::future<json> HitbtcAsync::fetchTradingFeesAsync(const json& params) {
    return fetchAsync("/api/3/spot/fee", "private", "GET", params);
}

boost::future<json> HitbtcAsync::fetchFundingFeesAsync(const json& params) {
    return fetchAsync("/api/3/wallet/fee", "private", "GET", params);
}

boost::future<json> HitbtcAsync::fetchTransactionsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["from"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/3/wallet/history/transactions", "private", "GET", request);
}

boost::future<json> HitbtcAsync::fetchDepositsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["from"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/3/wallet/history/deposit", "private", "GET", request);
}

boost::future<json> HitbtcAsync::fetchWithdrawalsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["from"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/3/wallet/history/withdraw", "private", "GET", request);
}

boost::future<json> HitbtcAsync::fetchDepositAddressAsync(const String& code, const json& params) {
    json request = {
        {"currency", code}
    };
    request.update(params);
    return fetchAsync("/api/3/wallet/crypto/address", "private", "GET", request);
}

boost::future<json> HitbtcAsync::withdrawAsync(const String& code, double amount,
                                             const String& address, const String& tag,
                                             const json& params) {
    json request = {
        {"currency", code},
        {"amount", this->number_to_string(amount)},
        {"address", address}
    };
    if (!tag.empty()) {
        request["paymentId"] = tag;
    }
    request.update(params);
    return fetchAsync("/api/3/wallet/crypto/withdraw", "private", "POST", request);
}

boost::future<json> HitbtcAsync::fetchFundingHistoryAsync(const String& symbol,
                                                         int since, int limit,
                                                         const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["from"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/3/margin/history/funding", "private", "GET", request);
}

boost::future<json> HitbtcAsync::fetchPositionsAsync(const std::vector<String>& symbols, const json& params) {
    json request = params;
    if (!symbols.empty()) {
        request["symbols"] = symbols;
    }
    return fetchAsync("/api/3/margin/position", "private", "GET", request);
}

boost::future<json> HitbtcAsync::fetchPositionAsync(const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    return fetchAsync("/api/3/margin/position/" + market_id, "private", "GET", params);
}

boost::future<json> HitbtcAsync::fetchLeverageAsync(const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    return fetchAsync("/api/3/margin/leverage/" + market_id, "private", "GET", params);
}

} // namespace ccxt
