#include "ccxt/exchanges/async/bitmart_async.h"

namespace ccxt {

BitmartAsync::BitmartAsync(const boost::asio::io_context& context)
    : ExchangeAsync(context)
    , Bitmart() {}

boost::future<json> BitmartAsync::fetchAsync(const String& path, const String& api,
                                         const String& method, const json& params,
                                         const std::map<String, String>& headers) {
    return ExchangeAsync::fetchAsync(path, api, method, params, headers);
}

boost::future<json> BitmartAsync::fetchMarketsAsync(const json& params) {
    return fetchAsync("/spot/v1/symbols/details", "public", "GET", params);
}

boost::future<json> BitmartAsync::fetchCurrenciesAsync(const json& params) {
    return fetchAsync("/spot/v1/currencies", "public", "GET", params);
}

boost::future<json> BitmartAsync::fetchTickerAsync(const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    return fetchAsync("/spot/v1/ticker?symbol=" + market_id, "public", "GET", params);
}

boost::future<json> BitmartAsync::fetchTickersAsync(const std::vector<String>& symbols, const json& params) {
    return fetchAsync("/spot/v1/ticker", "public", "GET", params);
}

boost::future<json> BitmartAsync::fetchOrderBookAsync(const String& symbol, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (limit > 0) {
        request["size"] = limit;
    }
    return fetchAsync("/spot/v1/symbols/book?symbol=" + market_id, "public", "GET", request);
}

boost::future<json> BitmartAsync::fetchTradesAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/spot/v1/symbols/trades?symbol=" + market_id, "public", "GET", request);
}

boost::future<json> BitmartAsync::fetchOHLCVAsync(const String& symbol, const String& timeframe,
                                               int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"symbol", market_id},
        {"step", timeframe}
    };
    if (since > 0) {
        request["from"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    request.update(params);
    return fetchAsync("/spot/v1/symbols/kline", "public", "GET", request);
}

boost::future<json> BitmartAsync::fetchBalanceAsync(const json& params) {
    return fetchAsync("/spot/v1/wallet", "private", "GET", params);
}

boost::future<json> BitmartAsync::createOrderAsync(const String& symbol, const String& type,
                                               const String& side, double amount,
                                               double price, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"symbol", market_id},
        {"side", side},
        {"type", type},
        {"size", std::to_string(amount)}
    };
    if (price > 0) {
        request["price"] = std::to_string(price);
    }
    request.update(params);
    return fetchAsync("/spot/v1/submit_order", "private", "POST", request);
}

boost::future<json> BitmartAsync::cancelOrderAsync(const String& id, const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"symbol", market_id},
        {"order_id", id}
    };
    request.update(params);
    return fetchAsync("/spot/v2/cancel_order", "private", "POST", request);
}

boost::future<json> BitmartAsync::cancelAllOrdersAsync(const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {{"symbol", market_id}};
    request.update(params);
    return fetchAsync("/spot/v1/cancel_orders", "private", "POST", request);
}

boost::future<json> BitmartAsync::fetchOrderAsync(const String& id, const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"symbol", market_id},
        {"order_id", id}
    };
    request.update(params);
    return fetchAsync("/spot/v1/order_detail", "private", "GET", request);
}

boost::future<json> BitmartAsync::fetchOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {{"symbol", market_id}};
    if (since > 0) {
        request["start_time"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    request.update(params);
    return fetchAsync("/spot/v1/orders", "private", "GET", request);
}

boost::future<json> BitmartAsync::fetchOpenOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {{"symbol", market_id}};
    if (since > 0) {
        request["start_time"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    request.update(params);
    return fetchAsync("/spot/v1/orders/open", "private", "GET", request);
}

boost::future<json> BitmartAsync::fetchClosedOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {{"symbol", market_id}};
    if (since > 0) {
        request["start_time"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    request.update(params);
    return fetchAsync("/spot/v1/orders/history", "private", "GET", request);
}

boost::future<json> BitmartAsync::fetchMyTradesAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {{"symbol", market_id}};
    if (since > 0) {
        request["start_time"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    request.update(params);
    return fetchAsync("/spot/v1/trades", "private", "GET", request);
}

boost::future<json> BitmartAsync::fetchPositionsAsync(const String& symbols, const json& params) {
    json request = params;
    if (!symbols.empty()) {
        request["symbol"] = symbols;
    }
    return fetchAsync("/contract/v1/positions", "private", "GET", request);
}

boost::future<json> BitmartAsync::fetchPositionRiskAsync(const String& symbols, const json& params) {
    json request = params;
    if (!symbols.empty()) {
        request["symbol"] = symbols;
    }
    return fetchAsync("/contract/v1/position/risk", "private", "GET", request);
}

boost::future<json> BitmartAsync::setLeverageAsync(int leverage, const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"symbol", market_id},
        {"leverage", leverage}
    };
    request.update(params);
    return fetchAsync("/contract/v1/position/leverage", "private", "POST", request);
}

boost::future<json> BitmartAsync::setMarginModeAsync(const String& marginMode, const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"symbol", market_id},
        {"margin_mode", marginMode}
    };
    request.update(params);
    return fetchAsync("/contract/v1/position/margin_mode", "private", "POST", request);
}

boost::future<json> BitmartAsync::fetchDepositAddressAsync(const String& code, const json& params) {
    return fetchAsync("/account/v1/deposit/address", "private", "GET", {{"currency", code}});
}

boost::future<json> BitmartAsync::fetchDepositsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["start_time"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/account/v1/deposit/history", "private", "GET", request);
}

boost::future<json> BitmartAsync::fetchWithdrawalsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["start_time"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/account/v1/withdraw/history", "private", "GET", request);
}

boost::future<json> BitmartAsync::withdrawAsync(const String& code, double amount,
                                            const String& address, const String& tag,
                                            const json& params) {
    json request = {
        {"currency", code},
        {"amount", std::to_string(amount)},
        {"destination", "4"}, // 4 for crypto address
        {"address", address}
    };
    if (!tag.empty()) {
        request["tag"] = tag;
    }
    request.update(params);
    return fetchAsync("/account/v1/withdraw/apply", "private", "POST", request);
}

boost::future<json> BitmartAsync::fetchTransactionsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["start_time"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/account/v1/transactions", "private", "GET", request);
}

boost::future<json> BitmartAsync::fetchTransferAsync(const String& id, const json& params) {
    return fetchAsync("/account/v1/transfer/detail", "private", "GET", {{"transfer_id", id}});
}

boost::future<json> BitmartAsync::fetchTransfersAsync(const json& params) {
    return fetchAsync("/account/v1/transfer/history", "private", "GET", params);
}

boost::future<json> BitmartAsync::transferAsync(const String& code, double amount,
                                            const String& fromAccount, const String& toAccount,
                                            const json& params) {
    json request = {
        {"currency", code},
        {"amount", std::to_string(amount)},
        {"from", fromAccount},
        {"to", toAccount}
    };
    request.update(params);
    return fetchAsync("/account/v1/transfer", "private", "POST", request);
}

boost::future<json> BitmartAsync::fetchTradingFeesAsync(const json& params) {
    return fetchAsync("/spot/v1/trade_fee", "private", "GET", params);
}

boost::future<json> BitmartAsync::fetchFundingFeesAsync(const json& params) {
    return fetchAsync("/account/v1/withdraw/charge", "private", "GET", params);
}

} // namespace ccxt
