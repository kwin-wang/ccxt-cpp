#include "ccxt/exchanges/async/bitpanda_async.h"

namespace ccxt {

BitpandaAsync::BitpandaAsync(const boost::asio::io_context& context)
    : ExchangeAsync(context)
    , Bitpanda() {}

boost::future<json> BitpandaAsync::fetchAsync(const String& path, const String& api,
                                         const String& method, const json& params,
                                         const std::map<String, String>& headers) {
    return ExchangeAsync::fetchAsync(path, api, method, params, headers);
}

boost::future<json> BitpandaAsync::fetchTimeAsync(const json& params) {
    return fetchAsync("/v1/time", "public", "GET", params);
}

boost::future<json> BitpandaAsync::fetchMarketsAsync(const json& params) {
    return fetchAsync("/v1/markets", "public", "GET", params);
}

boost::future<json> BitpandaAsync::fetchCurrenciesAsync(const json& params) {
    return fetchAsync("/v1/currencies", "public", "GET", params);
}

boost::future<json> BitpandaAsync::fetchTickerAsync(const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    return fetchAsync("/v1/market-ticker/" + market_id, "public", "GET", params);
}

boost::future<json> BitpandaAsync::fetchTickersAsync(const std::vector<String>& symbols, const json& params) {
    return fetchAsync("/v1/market-tickers", "public", "GET", params);
}

boost::future<json> BitpandaAsync::fetchOrderBookAsync(const String& symbol, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (limit > 0) {
        request["level"] = limit;
    }
    return fetchAsync("/v1/order-book/" + market_id, "public", "GET", request);
}

boost::future<json> BitpandaAsync::fetchTradesAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (since > 0) {
        request["from"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/v1/market-trades/" + market_id, "public", "GET", request);
}

boost::future<json> BitpandaAsync::fetchOHLCVAsync(const String& symbol, const String& timeframe,
                                               int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {{"unit", timeframe}};
    if (since > 0) {
        request["from"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    request.update(params);
    return fetchAsync("/v1/candlesticks/" + market_id, "public", "GET", request);
}

boost::future<json> BitpandaAsync::fetchBalanceAsync(const json& params) {
    return fetchAsync("/v1/account/balances", "private", "GET", params);
}

boost::future<json> BitpandaAsync::createOrderAsync(const String& symbol, const String& type,
                                               const String& side, double amount,
                                               double price, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"instrument_code", market_id},
        {"type", type},
        {"side", side},
        {"amount", std::to_string(amount)}
    };
    if (price > 0) {
        request["price"] = std::to_string(price);
    }
    request.update(params);
    return fetchAsync("/v1/account/orders", "private", "POST", request);
}

boost::future<json> BitpandaAsync::cancelOrderAsync(const String& id, const String& symbol, const json& params) {
    return fetchAsync("/v1/account/orders/" + id, "private", "DELETE", params);
}

boost::future<json> BitpandaAsync::cancelAllOrdersAsync(const String& symbol, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["instrument_code"] = this->market_id(symbol);
    }
    return fetchAsync("/v1/account/orders", "private", "DELETE", request);
}

boost::future<json> BitpandaAsync::fetchOrderAsync(const String& id, const String& symbol, const json& params) {
    return fetchAsync("/v1/account/orders/" + id, "private", "GET", params);
}

boost::future<json> BitpandaAsync::fetchOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["instrument_code"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["from"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/v1/account/orders", "private", "GET", request);
}

boost::future<json> BitpandaAsync::fetchOpenOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["instrument_code"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["from"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    request["status"] = "OPEN";
    return fetchAsync("/v1/account/orders", "private", "GET", request);
}

boost::future<json> BitpandaAsync::fetchClosedOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["instrument_code"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["from"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    request["status"] = "FILLED,CANCELLED";
    return fetchAsync("/v1/account/orders", "private", "GET", request);
}

boost::future<json> BitpandaAsync::fetchMyTradesAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["instrument_code"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["from"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/v1/account/trades", "private", "GET", request);
}

boost::future<json> BitpandaAsync::fetchAccountsAsync(const json& params) {
    return fetchAsync("/v1/account", "private", "GET", params);
}

boost::future<json> BitpandaAsync::fetchDepositAddressAsync(const String& code, const json& params) {
    return fetchAsync("/v1/account/deposit/crypto/" + code, "private", "GET", params);
}

boost::future<json> BitpandaAsync::fetchDepositsAsync(const String& code, int since, int limit, const json& params) {
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
    return fetchAsync("/v1/account/deposits", "private", "GET", request);
}

boost::future<json> BitpandaAsync::fetchWithdrawalsAsync(const String& code, int since, int limit, const json& params) {
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
    return fetchAsync("/v1/account/withdrawals", "private", "GET", request);
}

boost::future<json> BitpandaAsync::withdrawAsync(const String& code, double amount,
                                            const String& address, const String& tag,
                                            const json& params) {
    json request = {
        {"currency", code},
        {"amount", std::to_string(amount)},
        {"recipient", {{"address", address}}}
    };
    if (!tag.empty()) {
        request["recipient"]["destination_tag"] = tag;
    }
    request.update(params);
    return fetchAsync("/v1/account/withdraw/crypto", "private", "POST", request);
}

boost::future<json> BitpandaAsync::fetchTransactionsAsync(const String& code, int since, int limit, const json& params) {
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
    return fetchAsync("/v1/account/transactions", "private", "GET", request);
}

boost::future<json> BitpandaAsync::fetchTransfersAsync(const json& params) {
    return fetchAsync("/v1/account/transfers", "private", "GET", params);
}

boost::future<json> BitpandaAsync::transferAsync(const String& code, double amount,
                                            const String& fromAccount, const String& toAccount,
                                            const json& params) {
    json request = {
        {"currency", code},
        {"amount", std::to_string(amount)},
        {"from_account", fromAccount},
        {"to_account", toAccount}
    };
    request.update(params);
    return fetchAsync("/v1/account/transfer", "private", "POST", request);
}

boost::future<json> BitpandaAsync::fetchTradingFeesAsync(const json& params) {
    return fetchAsync("/v1/account/fees/trading", "private", "GET", params);
}

boost::future<json> BitpandaAsync::fetchFundingFeesAsync(const json& params) {
    return fetchAsync("/v1/account/fees/funding", "private", "GET", params);
}

} // namespace ccxt
