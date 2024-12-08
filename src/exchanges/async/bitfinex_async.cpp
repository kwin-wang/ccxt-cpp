#include "ccxt/exchanges/async/bitfinex_async.h"

namespace ccxt {

BitfinexAsync::BitfinexAsync(const boost::asio::io_context& context)
    : ExchangeAsync(context)
    , Bitfinex() {}

boost::future<json> BitfinexAsync::fetchAsync(const String& path, const String& api,
                                           const String& method, const json& params,
                                           const std::map<String, String>& headers) {
    return ExchangeAsync::fetchAsync(path, api, method, params, headers);
}

boost::future<json> BitfinexAsync::fetchMarketsAsync(const json& params) {
    return fetchAsync("/v2/conf/pub:info:pair", "public", "GET", params);
}

boost::future<json> BitfinexAsync::fetchTickerAsync(const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    return fetchAsync("/v2/ticker/t" + market_id, "public", "GET", params);
}

boost::future<json> BitfinexAsync::fetchTickersAsync(const std::vector<String>& symbols, const json& params) {
    return fetchAsync("/v2/tickers", "public", "GET", params);
}

boost::future<json> BitfinexAsync::fetchOrderBookAsync(const String& symbol, int limit, const json& params) {
    json request = params;
    if (limit > 0) {
        request["limit"] = limit;
    }
    String market_id = this->market_id(symbol);
    return fetchAsync("/v2/book/t" + market_id + "/P0", "public", "GET", request);
}

boost::future<json> BitfinexAsync::fetchTradesAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (since > 0) {
        request["start"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    String market_id = this->market_id(symbol);
    return fetchAsync("/v2/trades/t" + market_id + "/hist", "public", "GET", request);
}

boost::future<json> BitfinexAsync::fetchOHLCVAsync(const String& symbol, const String& timeframe,
                                                 int since, int limit, const json& params) {
    json request = params;
    if (since > 0) {
        request["start"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    String market_id = this->market_id(symbol);
    return fetchAsync("/v2/candles/trade:" + timeframe + ":t" + market_id + "/hist", "public", "GET", request);
}

boost::future<json> BitfinexAsync::fetchBalanceAsync(const json& params) {
    return fetchAsync("/v2/auth/r/wallets", "private", "POST", params);
}

boost::future<json> BitfinexAsync::createOrderAsync(const String& symbol, const String& type, const String& side,
                                                  double amount, double price, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"symbol", "t" + market_id},
        {"type", type},
        {"side", side},
        {"amount", std::to_string(amount)}
    };
    if (price > 0) {
        request["price"] = std::to_string(price);
    }
    request.update(params);
    return fetchAsync("/v2/auth/w/order/submit", "private", "POST", request);
}

boost::future<json> BitfinexAsync::cancelOrderAsync(const String& id, const String& symbol, const json& params) {
    json request = {{"id", id}};
    request.update(params);
    return fetchAsync("/v2/auth/w/order/cancel", "private", "POST", request);
}

boost::future<json> BitfinexAsync::fetchOrderAsync(const String& id, const String& symbol, const json& params) {
    json request = {{"id", id}};
    request.update(params);
    return fetchAsync("/v2/auth/r/order/" + id, "private", "POST", request);
}

boost::future<json> BitfinexAsync::fetchOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (since > 0) {
        request["start"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    if (!symbol.empty()) {
        String market_id = this->market_id(symbol);
        request["symbol"] = "t" + market_id;
    }
    return fetchAsync("/v2/auth/r/orders/hist", "private", "POST", request);
}

boost::future<json> BitfinexAsync::fetchOpenOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        String market_id = this->market_id(symbol);
        request["symbol"] = "t" + market_id;
    }
    return fetchAsync("/v2/auth/r/orders", "private", "POST", request);
}

boost::future<json> BitfinexAsync::fetchClosedOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    return fetchOrdersAsync(symbol, since, limit, params);
}

boost::future<json> BitfinexAsync::fetchPositionsAsync(const String& symbol, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        String market_id = this->market_id(symbol);
        request["symbol"] = "t" + market_id;
    }
    return fetchAsync("/v2/auth/r/positions", "private", "POST", request);
}

boost::future<json> BitfinexAsync::fetchMyTradesAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (since > 0) {
        request["start"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    if (!symbol.empty()) {
        String market_id = this->market_id(symbol);
        request["symbol"] = "t" + market_id;
    }
    return fetchAsync("/v2/auth/r/trades/hist", "private", "POST", request);
}

boost::future<json> BitfinexAsync::fetchLedgerAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (since > 0) {
        request["start"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    if (!code.empty()) {
        String currency = this->currency(code);
        request["currency"] = currency;
    }
    return fetchAsync("/v2/auth/r/ledgers/hist", "private", "POST", request);
}

boost::future<json> BitfinexAsync::fetchFundingRatesAsync(const std::vector<String>& symbols, const json& params) {
    json request = params;
    if (!symbols.empty()) {
        std::vector<String> market_ids;
        for (const auto& symbol : symbols) {
            market_ids.push_back("f" + this->market_id(symbol));
        }
        request["symbols"] = market_ids;
    }
    return fetchAsync("/v2/calc/trade/avg", "public", "POST", request);
}

boost::future<json> BitfinexAsync::setLeverageAsync(const String& symbol, double leverage, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"symbol", "t" + market_id},
        {"leverage", std::to_string(leverage)}
    };
    request.update(params);
    return fetchAsync("/v2/auth/w/margin/set", "private", "POST", request);
}

boost::future<json> BitfinexAsync::fetchDepositsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (since > 0) {
        request["start"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    if (!code.empty()) {
        String currency = this->currency(code);
        request["currency"] = currency;
    }
    return fetchAsync("/v2/auth/r/movements/hist", "private", "POST", request);
}

boost::future<json> BitfinexAsync::fetchWithdrawalsAsync(const String& code, int since, int limit, const json& params) {
    return fetchDepositsAsync(code, since, limit, params);
}

boost::future<json> BitfinexAsync::fetchDepositAddressAsync(const String& code, const json& params) {
    String currency = this->currency(code);
    json request = {{"currency", currency}};
    request.update(params);
    return fetchAsync("/v2/auth/r/deposit/address", "private", "POST", request);
}

boost::future<json> BitfinexAsync::transferAsync(const String& code, double amount, const String& fromAccount,
                                               const String& toAccount, const json& params) {
    String currency = this->currency(code);
    json request = {
        {"currency", currency},
        {"amount", std::to_string(amount)},
        {"from", fromAccount},
        {"to", toAccount}
    };
    request.update(params);
    return fetchAsync("/v2/auth/w/transfer", "private", "POST", request);
}

} // namespace ccxt
