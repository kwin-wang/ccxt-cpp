#include "ccxt/exchanges/async/bl3p_async.h"

namespace ccxt {

BL3PAsync::BL3PAsync(const boost::asio::io_context& context)
    : ExchangeAsync(context)
    , BL3P() {}

boost::future<json> BL3PAsync::fetchAsync(const String& path, const String& api,
                                      const String& method, const json& params,
                                      const std::map<String, String>& headers) {
    return ExchangeAsync::fetchAsync(path, api, method, params, headers);
}

boost::future<json> BL3PAsync::fetchTimeAsync(const json& params) {
    return fetchAsync("/1/time", "public", "GET", params);
}

boost::future<json> BL3PAsync::fetchMarketsAsync(const json& params) {
    return fetchAsync("/1/markets", "public", "GET", params);
}

boost::future<json> BL3PAsync::fetchCurrenciesAsync(const json& params) {
    return fetchAsync("/1/currencies", "public", "GET", params);
}

boost::future<json> BL3PAsync::fetchTickerAsync(const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    return fetchAsync("/1/" + market_id + "/ticker", "public", "GET", params);
}

boost::future<json> BL3PAsync::fetchTickersAsync(const std::vector<String>& symbols, const json& params) {
    return fetchAsync("/1/tickers", "public", "GET", params);
}

boost::future<json> BL3PAsync::fetchOrderBookAsync(const String& symbol, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (limit > 0) {
        request["count"] = limit;
    }
    return fetchAsync("/1/" + market_id + "/orderbook", "public", "GET", request);
}

boost::future<json> BL3PAsync::fetchTradesAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (limit > 0) {
        request["count"] = limit;
    }
    if (since > 0) {
        request["since"] = since;
    }
    return fetchAsync("/1/" + market_id + "/trades", "public", "GET", request);
}

boost::future<json> BL3PAsync::fetchOHLCVAsync(const String& symbol, const String& timeframe,
                                           int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    request["timeframe"] = timeframe;
    if (since > 0) {
        request["since"] = since;
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    return fetchAsync("/1/" + market_id + "/candles", "public", "GET", request);
}

boost::future<json> BL3PAsync::fetchBalanceAsync(const json& params) {
    return fetchAsync("/1/wallet", "private", "GET", params);
}

boost::future<json> BL3PAsync::createOrderAsync(const String& symbol, const String& type,
                                           const String& side, double amount,
                                           double price, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"type", type},
        {"amount_int", this->amount_to_precision(symbol, amount)},
        {"fee_currency", "BTC"}
    };
    if ((type == "limit") && (price > 0)) {
        request["price_int"] = this->price_to_precision(symbol, price);
    }
    request.update(params);
    String path = "/1/" + market_id + "/" + side;
    return fetchAsync(path, "private", "POST", request);
}

boost::future<json> BL3PAsync::cancelOrderAsync(const String& id, const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    request["order_id"] = id;
    return fetchAsync("/1/" + market_id + "/cancel", "private", "POST", request);
}

boost::future<json> BL3PAsync::fetchOrderAsync(const String& id, const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    request["order_id"] = id;
    return fetchAsync("/1/" + market_id + "/order", "private", "GET", request);
}

boost::future<json> BL3PAsync::fetchOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (since > 0) {
        request["since"] = since;
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    return fetchAsync("/1/" + market_id + "/orders", "private", "GET", request);
}

boost::future<json> BL3PAsync::fetchOpenOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (since > 0) {
        request["since"] = since;
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    return fetchAsync("/1/" + market_id + "/orders/open", "private", "GET", request);
}

boost::future<json> BL3PAsync::fetchClosedOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (since > 0) {
        request["since"] = since;
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    return fetchAsync("/1/" + market_id + "/orders/history", "private", "GET", request);
}

boost::future<json> BL3PAsync::fetchMyTradesAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (since > 0) {
        request["since"] = since;
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    return fetchAsync("/1/" + market_id + "/trades/history", "private", "GET", request);
}

boost::future<json> BL3PAsync::fetchBalancesAsync(const json& params) {
    return fetchAsync("/1/wallet/balances", "private", "GET", params);
}

boost::future<json> BL3PAsync::fetchTradingFeesAsync(const json& params) {
    return fetchAsync("/1/fees/trading", "private", "GET", params);
}

boost::future<json> BL3PAsync::fetchDepositAddressAsync(const String& code, const json& params) {
    json request = params;
    request["currency"] = code;
    return fetchAsync("/1/deposit/address", "private", "GET", request);
}

boost::future<json> BL3PAsync::fetchDepositsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["since"] = since;
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    return fetchAsync("/1/deposit/history", "private", "GET", request);
}

boost::future<json> BL3PAsync::fetchWithdrawalsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["since"] = since;
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    return fetchAsync("/1/withdrawal/history", "private", "GET", request);
}

boost::future<json> BL3PAsync::withdrawAsync(const String& code, double amount,
                                        const String& address, const String& tag,
                                        const json& params) {
    json request = {
        {"currency", code},
        {"amount_int", this->amount_to_precision(code, amount)},
        {"address", address}
    };
    if (!tag.empty()) {
        request["address_tag"] = tag;
    }
    request.update(params);
    return fetchAsync("/1/withdrawal", "private", "POST", request);
}

boost::future<json> BL3PAsync::fetchTransactionsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["since"] = since;
    }
    if (limit > 0) {
        request["count"] = limit;
    }
    return fetchAsync("/1/wallet/history", "private", "GET", request);
}

} // namespace ccxt
