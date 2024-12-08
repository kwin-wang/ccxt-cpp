#include "ccxt/exchanges/async/btcalpha_async.h"

namespace ccxt {

BtcalphaAsync::BtcalphaAsync(const boost::asio::io_context& context)
    : ExchangeAsync(context)
    , Btcalpha() {}

boost::future<json> BtcalphaAsync::fetchAsync(const String& path, const String& api,
                                            const String& method, const json& params,
                                            const std::map<String, String>& headers) {
    return ExchangeAsync::fetchAsync(path, api, method, params, headers);
}

boost::future<json> BtcalphaAsync::fetchMarketsAsync(const json& params) {
    return fetchAsync("/api/v1/pairs", "public", "GET", params);
}

boost::future<json> BtcalphaAsync::fetchCurrenciesAsync(const json& params) {
    return fetchAsync("/api/v1/currencies", "public", "GET", params);
}

boost::future<json> BtcalphaAsync::fetchTickerAsync(const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    return fetchAsync("/api/v1/ticker/" + market_id, "public", "GET", params);
}

boost::future<json> BtcalphaAsync::fetchTickersAsync(const std::vector<String>& symbols, const json& params) {
    return fetchAsync("/api/v1/ticker", "public", "GET", params);
}

boost::future<json> BtcalphaAsync::fetchOrderBookAsync(const String& symbol, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/orderbook/" + market_id, "public", "GET", request);
}

boost::future<json> BtcalphaAsync::fetchTradesAsync(const String& symbol, int since, int limit, const json& params) {
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

boost::future<json> BtcalphaAsync::fetchOHLCVAsync(const String& symbol, const String& timeframe,
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
    return fetchAsync("/api/v1/charts/" + market_id, "public", "GET", request);
}

boost::future<json> BtcalphaAsync::fetchBalanceAsync(const json& params) {
    return fetchAsync("/api/v1/wallets", "private", "GET", params);
}

boost::future<json> BtcalphaAsync::createOrderAsync(const String& symbol, const String& type,
                                                  const String& side, double amount,
                                                  double price, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"pair", market_id},
        {"type", side},
        {"amount", this->amount_to_precision(symbol, amount)}
    };
    if ((type == "limit") && (price > 0)) {
        request["price"] = this->price_to_precision(symbol, price);
    }
    request.update(params);
    return fetchAsync("/api/v1/order/new", "private", "POST", request);
}

boost::future<json> BtcalphaAsync::cancelOrderAsync(const String& id, const String& symbol, const json& params) {
    json request = {
        {"order", id}
    };
    request.update(params);
    return fetchAsync("/api/v1/order/cancel", "private", "POST", request);
}

boost::future<json> BtcalphaAsync::cancelAllOrdersAsync(const String& symbol, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["pair"] = this->market_id(symbol);
    }
    return fetchAsync("/api/v1/order/cancel/all", "private", "POST", request);
}

boost::future<json> BtcalphaAsync::fetchOrderAsync(const String& id, const String& symbol, const json& params) {
    json request = {
        {"order", id}
    };
    request.update(params);
    return fetchAsync("/api/v1/order", "private", "GET", request);
}

boost::future<json> BtcalphaAsync::fetchOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["pair"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["since"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/orders", "private", "GET", request);
}

boost::future<json> BtcalphaAsync::fetchOpenOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["pair"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["since"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/orders/open", "private", "GET", request);
}

boost::future<json> BtcalphaAsync::fetchClosedOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["pair"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["since"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/orders/closed", "private", "GET", request);
}

boost::future<json> BtcalphaAsync::fetchMyTradesAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["pair"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["since"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v1/trades/own", "private", "GET", request);
}

boost::future<json> BtcalphaAsync::fetchTradingFeesAsync(const json& params) {
    return fetchAsync("/api/v1/commission", "private", "GET", params);
}

boost::future<json> BtcalphaAsync::fetchDepositAddressAsync(const String& code, const json& params) {
    json request = {
        {"currency", code}
    };
    request.update(params);
    return fetchAsync("/api/v1/deposit/address", "private", "GET", request);
}

boost::future<json> BtcalphaAsync::fetchDepositsAsync(const String& code, int since, int limit, const json& params) {
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
    return fetchAsync("/api/v1/deposits", "private", "GET", request);
}

boost::future<json> BtcalphaAsync::fetchWithdrawalsAsync(const String& code, int since, int limit, const json& params) {
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
    return fetchAsync("/api/v1/withdrawals", "private", "GET", request);
}

boost::future<json> BtcalphaAsync::withdrawAsync(const String& code, double amount,
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
    return fetchAsync("/api/v1/withdraw", "private", "POST", request);
}

boost::future<json> BtcalphaAsync::fetchTransactionsAsync(const String& code, int since, int limit, const json& params) {
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
    return fetchAsync("/api/v1/transactions", "private", "GET", request);
}

} // namespace ccxt
