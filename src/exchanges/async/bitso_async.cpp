#include "ccxt/exchanges/async/bitso_async.h"

namespace ccxt {

BitsoAsync::BitsoAsync(const boost::asio::io_context& context)
    : ExchangeAsync(context)
    , Bitso() {}

boost::future<json> BitsoAsync::fetchAsync(const String& path, const String& api,
                                       const String& method, const json& params,
                                       const std::map<String, String>& headers) {
    return ExchangeAsync::fetchAsync(path, api, method, params, headers);
}

boost::future<json> BitsoAsync::fetchTimeAsync(const json& params) {
    return fetchAsync("/v3/time", "public", "GET", params);
}

boost::future<json> BitsoAsync::fetchMarketsAsync(const json& params) {
    return fetchAsync("/v3/available_books", "public", "GET", params);
}

boost::future<json> BitsoAsync::fetchCurrenciesAsync(const json& params) {
    return fetchAsync("/v3/currencies", "public", "GET", params);
}

boost::future<json> BitsoAsync::fetchTickerAsync(const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    return fetchAsync("/v3/ticker/?book=" + market_id, "public", "GET", params);
}

boost::future<json> BitsoAsync::fetchTickersAsync(const std::vector<String>& symbols, const json& params) {
    return fetchAsync("/v3/tickers", "public", "GET", params);
}

boost::future<json> BitsoAsync::fetchOrderBookAsync(const String& symbol, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/v3/order_book/?book=" + market_id, "public", "GET", request);
}

boost::future<json> BitsoAsync::fetchTradesAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (limit > 0) {
        request["limit"] = limit;
    }
    if (since > 0) {
        request["marker"] = this->iso8601(since);
    }
    return fetchAsync("/v3/trades/?book=" + market_id, "public", "GET", request);
}

boost::future<json> BitsoAsync::fetchOHLCVAsync(const String& symbol, const String& timeframe,
                                            int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    request["book"] = market_id;
    request["time_bucket"] = timeframe;
    if (since > 0) {
        request["start"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/v3/ohlc", "public", "GET", request);
}

boost::future<json> BitsoAsync::fetchBalanceAsync(const json& params) {
    return fetchAsync("/v3/balance", "private", "GET", params);
}

boost::future<json> BitsoAsync::createOrderAsync(const String& symbol, const String& type,
                                            const String& side, double amount,
                                            double price, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"book", market_id},
        {"side", side},
        {"type", type},
        {"amount", this->amount_to_precision(symbol, amount)}
    };
    if ((type == "limit") && (price > 0)) {
        request["price"] = this->price_to_precision(symbol, price);
    }
    request.update(params);
    return fetchAsync("/v3/orders", "private", "POST", request);
}

boost::future<json> BitsoAsync::cancelOrderAsync(const String& id, const String& symbol, const json& params) {
    return fetchAsync("/v3/orders/" + id, "private", "DELETE", params);
}

boost::future<json> BitsoAsync::cancelAllOrdersAsync(const String& symbol, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["book"] = this->market_id(symbol);
    }
    return fetchAsync("/v3/orders/all", "private", "DELETE", request);
}

boost::future<json> BitsoAsync::fetchOrderAsync(const String& id, const String& symbol, const json& params) {
    return fetchAsync("/v3/orders/" + id, "private", "GET", params);
}

boost::future<json> BitsoAsync::fetchOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["book"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["marker"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/v3/orders", "private", "GET", request);
}

boost::future<json> BitsoAsync::fetchOpenOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["book"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["marker"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/v3/open_orders", "private", "GET", request);
}

boost::future<json> BitsoAsync::fetchClosedOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["book"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["marker"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/v3/user_trades", "private", "GET", request);
}

boost::future<json> BitsoAsync::fetchMyTradesAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["book"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["marker"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/v3/user_trades", "private", "GET", request);
}

boost::future<json> BitsoAsync::fetchAccountsAsync(const json& params) {
    return fetchAsync("/v3/account_status", "private", "GET", params);
}

boost::future<json> BitsoAsync::fetchTradingFeesAsync(const json& params) {
    return fetchAsync("/v3/fees", "private", "GET", params);
}

boost::future<json> BitsoAsync::fetchFundingFeesAsync(const json& params) {
    return fetchAsync("/v3/fees", "private", "GET", params);
}

boost::future<json> BitsoAsync::fetchDepositAddressAsync(const String& code, const json& params) {
    json request = params;
    request["fund_currency"] = code;
    return fetchAsync("/v3/funding_destination", "private", "GET", request);
}

boost::future<json> BitsoAsync::fetchDepositsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["marker"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/v3/fundings", "private", "GET", request);
}

boost::future<json> BitsoAsync::fetchWithdrawalsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["marker"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/v3/withdrawals", "private", "GET", request);
}

boost::future<json> BitsoAsync::withdrawAsync(const String& code, double amount,
                                         const String& address, const String& tag,
                                         const json& params) {
    json request = {
        {"currency", code},
        {"amount", this->number_to_string(amount)},
        {"address", address}
    };
    if (!tag.empty()) {
        request["destination_tag"] = tag;
    }
    request.update(params);
    return fetchAsync("/v3/withdrawals", "private", "POST", request);
}

boost::future<json> BitsoAsync::fetchTransactionsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["marker"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/v3/transfers", "private", "GET", request);
}

boost::future<json> BitsoAsync::fetchTransferAsync(const String& id, const json& params) {
    return fetchAsync("/v3/transfer/" + id, "private", "GET", params);
}

boost::future<json> BitsoAsync::fetchTransfersAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["marker"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/v3/transfers", "private", "GET", request);
}

} // namespace ccxt
