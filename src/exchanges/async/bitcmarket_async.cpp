#include "ccxt/exchanges/async/bitcmarket_async.h"

namespace ccxt {

BitcmarketAsync::BitcmarketAsync(const boost::asio::io_context& context)
    : ExchangeAsync(context)
    , Bitcmarket() {}

boost::future<json> BitcmarketAsync::fetchAsync(const String& path, const String& api,
                                              const String& method, const json& params,
                                              const std::map<String, String>& headers) {
    return ExchangeAsync::fetchAsync(path, api, method, params, headers);
}

boost::future<json> BitcmarketAsync::fetchMarketsAsync(const json& params) {
    return fetchAsync("/api/v1/markets", "public", "GET", params);
}

boost::future<json> BitcmarketAsync::fetchCurrenciesAsync(const json& params) {
    return fetchAsync("/api/v1/currencies", "public", "GET", params);
}

boost::future<json> BitcmarketAsync::fetchTickerAsync(const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    return fetchAsync("/api/v1/ticker/" + market_id, "public", "GET", params);
}

boost::future<json> BitcmarketAsync::fetchTickersAsync(const std::vector<String>& symbols, const json& params) {
    return fetchAsync("/api/v1/tickers", "public", "GET", params);
}

boost::future<json> BitcmarketAsync::fetchOrderBookAsync(const String& symbol, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/orderbook/" + market_id, "public", "GET", request);
}

boost::future<json> BitcmarketAsync::fetchTradesAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (limit > 0) {
        request["limit"] = limit;
    }
    if (since > 0) {
        request["since"] = since;
    }
    return fetchAsync("/api/v1/trades/" + market_id, "public", "GET", request);
}

boost::future<json> BitcmarketAsync::fetchOHLCVAsync(const String& symbol, const String& timeframe,
                                                    int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    request["interval"] = timeframe;
    if (since > 0) {
        request["since"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/klines/" + market_id, "public", "GET", request);
}

boost::future<json> BitcmarketAsync::fetchBalanceAsync(const json& params) {
    return fetchAsync("/api/v1/account/balances", "private", "GET", params);
}

boost::future<json> BitcmarketAsync::createOrderAsync(const String& symbol, const String& type,
                                                    const String& side, double amount,
                                                    double price, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"market", market_id},
        {"side", side},
        {"type", type},
        {"volume", this->amount_to_precision(symbol, amount)}
    };
    if ((type == "limit") && (price > 0)) {
        request["price"] = this->price_to_precision(symbol, price);
    }
    request.update(params);
    return fetchAsync("/api/v1/order", "private", "POST", request);
}

boost::future<json> BitcmarketAsync::cancelOrderAsync(const String& id, const String& symbol, const json& params) {
    json request = {
        {"id", id}
    };
    request.update(params);
    return fetchAsync("/api/v1/order/" + id, "private", "DELETE", request);
}

boost::future<json> BitcmarketAsync::cancelAllOrdersAsync(const String& symbol, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["market"] = this->market_id(symbol);
    }
    return fetchAsync("/api/v1/orders", "private", "DELETE", request);
}

boost::future<json> BitcmarketAsync::fetchOrderAsync(const String& id, const String& symbol, const json& params) {
    return fetchAsync("/api/v1/order/" + id, "private", "GET", params);
}

boost::future<json> BitcmarketAsync::fetchOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["market"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["since"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/orders", "private", "GET", request);
}

boost::future<json> BitcmarketAsync::fetchOpenOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["market"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["since"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/orders/open", "private", "GET", request);
}

boost::future<json> BitcmarketAsync::fetchClosedOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["market"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["since"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/orders/closed", "private", "GET", request);
}

boost::future<json> BitcmarketAsync::fetchMyTradesAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["market"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["since"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/trades/my", "private", "GET", request);
}

boost::future<json> BitcmarketAsync::fetchTradingFeesAsync(const json& params) {
    return fetchAsync("/api/v1/account/trading-fees", "private", "GET", params);
}

boost::future<json> BitcmarketAsync::fetchDepositAddressAsync(const String& code, const json& params) {
    json request = {
        {"currency", code}
    };
    request.update(params);
    return fetchAsync("/api/v1/account/deposit-address", "private", "GET", request);
}

boost::future<json> BitcmarketAsync::fetchDepositsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["since"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/account/deposits", "private", "GET", request);
}

boost::future<json> BitcmarketAsync::fetchWithdrawalsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["since"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/account/withdrawals", "private", "GET", request);
}

boost::future<json> BitcmarketAsync::withdrawAsync(const String& code, double amount,
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
    return fetchAsync("/api/v1/account/withdraw", "private", "POST", request);
}

boost::future<json> BitcmarketAsync::fetchTransactionsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["since"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/account/transactions", "private", "GET", request);
}

} // namespace ccxt
