#include "ccxt/exchanges/async/bitopro_async.h"

namespace ccxt {

BitoproAsync::BitoproAsync(const boost::asio::io_context& context)
    : ExchangeAsync(context)
    , Bitopro() {}

boost::future<json> BitoproAsync::fetchAsync(const String& path, const String& api,
                                         const String& method, const json& params,
                                         const std::map<String, String>& headers) {
    return ExchangeAsync::fetchAsync(path, api, method, params, headers);
}

boost::future<json> BitoproAsync::fetchMarketsAsync(const json& params) {
    return fetchAsync("/v3/provisioning/trading-pairs", "public", "GET", params);
}

boost::future<json> BitoproAsync::fetchCurrenciesAsync(const json& params) {
    return fetchAsync("/v3/provisioning/currencies", "public", "GET", params);
}

boost::future<json> BitoproAsync::fetchTickerAsync(const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    return fetchAsync("/v3/tickers/" + market_id, "public", "GET", params);
}

boost::future<json> BitoproAsync::fetchTickersAsync(const std::vector<String>& symbols, const json& params) {
    return fetchAsync("/v3/tickers", "public", "GET", params);
}

boost::future<json> BitoproAsync::fetchOrderBookAsync(const String& symbol, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/v3/order-book/" + market_id, "public", "GET", request);
}

boost::future<json> BitoproAsync::fetchTradesAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/v3/trades/" + market_id, "public", "GET", request);
}

boost::future<json> BitoproAsync::fetchOHLCVAsync(const String& symbol, const String& timeframe,
                                               int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"resolution", timeframe}
    };
    if (since > 0) {
        request["from"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    request.update(params);
    return fetchAsync("/v3/trading-history/" + market_id, "public", "GET", request);
}

boost::future<json> BitoproAsync::fetchBalanceAsync(const json& params) {
    return fetchAsync("/v3/accounts/balance", "private", "GET", params);
}

boost::future<json> BitoproAsync::createOrderAsync(const String& symbol, const String& type,
                                               const String& side, double amount,
                                               double price, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"pair", market_id},
        {"action", side},
        {"type", type},
        {"amount", std::to_string(amount)}
    };
    if (price > 0) {
        request["price"] = std::to_string(price);
    }
    request.update(params);
    return fetchAsync("/v3/orders", "private", "POST", request);
}

boost::future<json> BitoproAsync::cancelOrderAsync(const String& id, const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    return fetchAsync("/v3/orders/" + market_id + "/" + id, "private", "DELETE", params);
}

boost::future<json> BitoproAsync::cancelAllOrdersAsync(const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    return fetchAsync("/v3/orders/" + market_id, "private", "DELETE", params);
}

boost::future<json> BitoproAsync::fetchOrderAsync(const String& id, const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    return fetchAsync("/v3/orders/" + market_id + "/" + id, "private", "GET", params);
}

boost::future<json> BitoproAsync::fetchOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (since > 0) {
        request["startTimestamp"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/v3/orders/all/" + market_id, "private", "GET", request);
}

boost::future<json> BitoproAsync::fetchOpenOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (since > 0) {
        request["startTimestamp"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/v3/orders/open/" + market_id, "private", "GET", request);
}

boost::future<json> BitoproAsync::fetchClosedOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (since > 0) {
        request["startTimestamp"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/v3/orders/history/" + market_id, "private", "GET", request);
}

boost::future<json> BitoproAsync::fetchMyTradesAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (since > 0) {
        request["startTimestamp"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/v3/orders/trades/" + market_id, "private", "GET", request);
}

boost::future<json> BitoproAsync::fetchDepositAddressAsync(const String& code, const json& params) {
    return fetchAsync("/v3/wallet/deposit/addresses/" + code, "private", "GET", params);
}

boost::future<json> BitoproAsync::fetchDepositsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["startTimestamp"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/v3/wallet/deposits", "private", "GET", request);
}

boost::future<json> BitoproAsync::fetchWithdrawalsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["startTimestamp"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/v3/wallet/withdrawals", "private", "GET", request);
}

boost::future<json> BitoproAsync::withdrawAsync(const String& code, double amount,
                                            const String& address, const String& tag,
                                            const json& params) {
    json request = {
        {"currency", code},
        {"amount", std::to_string(amount)},
        {"address", address}
    };
    if (!tag.empty()) {
        request["message"] = tag;
    }
    request.update(params);
    return fetchAsync("/v3/wallet/withdrawals", "private", "POST", request);
}

boost::future<json> BitoproAsync::fetchTransactionsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["startTimestamp"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/v3/wallet/transactions", "private", "GET", request);
}

boost::future<json> BitoproAsync::fetchLedgerAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["startTimestamp"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/v3/wallet/ledger", "private", "GET", request);
}

boost::future<json> BitoproAsync::fetchTradingFeesAsync(const json& params) {
    return fetchAsync("/v3/accounts/trading-fees", "private", "GET", params);
}

boost::future<json> BitoproAsync::fetchFundingFeesAsync(const json& params) {
    return fetchAsync("/v3/wallet/fees", "private", "GET", params);
}

} // namespace ccxt
