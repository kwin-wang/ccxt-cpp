#include "ccxt/exchanges/async/bitteam_async.h"

namespace ccxt {

BitteamAsync::BitteamAsync(const boost::asio::io_context& context)
    : ExchangeAsync(context)
    , Bitteam() {}

boost::future<json> BitteamAsync::fetchAsync(const String& path, const String& api,
                                         const String& method, const json& params,
                                         const std::map<String, String>& headers) {
    return ExchangeAsync::fetchAsync(path, api, method, params, headers);
}

boost::future<json> BitteamAsync::fetchTimeAsync(const json& params) {
    return fetchAsync("/api/v2/time", "public", "GET", params);
}

boost::future<json> BitteamAsync::fetchMarketsAsync(const json& params) {
    return fetchAsync("/api/v2/public/symbols", "public", "GET", params);
}

boost::future<json> BitteamAsync::fetchCurrenciesAsync(const json& params) {
    return fetchAsync("/api/v2/public/currencies", "public", "GET", params);
}

boost::future<json> BitteamAsync::fetchTickerAsync(const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    return fetchAsync("/api/v2/public/ticker/" + market_id, "public", "GET", params);
}

boost::future<json> BitteamAsync::fetchTickersAsync(const std::vector<String>& symbols, const json& params) {
    return fetchAsync("/api/v2/public/tickers", "public", "GET", params);
}

boost::future<json> BitteamAsync::fetchOrderBookAsync(const String& symbol, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v2/public/depth/" + market_id, "public", "GET", request);
}

boost::future<json> BitteamAsync::fetchTradesAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (limit > 0) {
        request["limit"] = limit;
    }
    if (since > 0) {
        request["from"] = since;
    }
    return fetchAsync("/api/v2/public/trades/" + market_id, "public", "GET", request);
}

boost::future<json> BitteamAsync::fetchOHLCVAsync(const String& symbol, const String& timeframe,
                                              int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    request["interval"] = timeframe;
    if (since > 0) {
        request["from"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v2/public/klines/" + market_id, "public", "GET", request);
}

boost::future<json> BitteamAsync::fetchBalanceAsync(const json& params) {
    return fetchAsync("/api/v2/private/account/balances", "private", "GET", params);
}

boost::future<json> BitteamAsync::createOrderAsync(const String& symbol, const String& type,
                                              const String& side, double amount,
                                              double price, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"market", market_id},
        {"side", side},
        {"type", type},
        {"amount", this->amount_to_precision(symbol, amount)}
    };
    if ((type == "limit") && (price > 0)) {
        request["price"] = this->price_to_precision(symbol, price);
    }
    request.update(params);
    return fetchAsync("/api/v2/private/order/new", "private", "POST", request);
}

boost::future<json> BitteamAsync::cancelOrderAsync(const String& id, const String& symbol, const json& params) {
    json request = params;
    request["orderId"] = id;
    if (!symbol.empty()) {
        request["market"] = this->market_id(symbol);
    }
    return fetchAsync("/api/v2/private/order/cancel", "private", "POST", request);
}

boost::future<json> BitteamAsync::cancelAllOrdersAsync(const String& symbol, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["market"] = this->market_id(symbol);
    }
    return fetchAsync("/api/v2/private/order/cancel/all", "private", "POST", request);
}

boost::future<json> BitteamAsync::fetchOrderAsync(const String& id, const String& symbol, const json& params) {
    json request = params;
    request["orderId"] = id;
    if (!symbol.empty()) {
        request["market"] = this->market_id(symbol);
    }
    return fetchAsync("/api/v2/private/order", "private", "GET", request);
}

boost::future<json> BitteamAsync::fetchOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["market"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["from"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v2/private/orders", "private", "GET", request);
}

boost::future<json> BitteamAsync::fetchOpenOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["market"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["from"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v2/private/orders/open", "private", "GET", request);
}

boost::future<json> BitteamAsync::fetchClosedOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["market"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["from"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v2/private/orders/closed", "private", "GET", request);
}

boost::future<json> BitteamAsync::fetchMyTradesAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["market"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["from"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v2/private/trades", "private", "GET", request);
}

boost::future<json> BitteamAsync::fetchAccountsAsync(const json& params) {
    return fetchAsync("/api/v2/private/account", "private", "GET", params);
}

boost::future<json> BitteamAsync::fetchTradingFeesAsync(const json& params) {
    return fetchAsync("/api/v2/private/account/fees/trading", "private", "GET", params);
}

boost::future<json> BitteamAsync::fetchFundingFeesAsync(const json& params) {
    return fetchAsync("/api/v2/private/account/fees/funding", "private", "GET", params);
}

boost::future<json> BitteamAsync::fetchDepositAddressAsync(const String& code, const json& params) {
    json request = params;
    request["currency"] = code;
    return fetchAsync("/api/v2/private/account/deposit/address", "private", "GET", request);
}

boost::future<json> BitteamAsync::fetchDepositsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["from"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v2/private/account/deposits", "private", "GET", request);
}

boost::future<json> BitteamAsync::fetchWithdrawalsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["from"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v2/private/account/withdrawals", "private", "GET", request);
}

boost::future<json> BitteamAsync::withdrawAsync(const String& code, double amount,
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
    return fetchAsync("/api/v2/private/account/withdraw", "private", "POST", request);
}

boost::future<json> BitteamAsync::fetchTransactionsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["from"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v2/private/account/transactions", "private", "GET", request);
}

} // namespace ccxt
