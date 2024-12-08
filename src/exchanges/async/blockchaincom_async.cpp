#include "ccxt/exchanges/async/blockchaincom_async.h"

namespace ccxt {

BlockchaincomAsync::BlockchaincomAsync(const boost::asio::io_context& context)
    : ExchangeAsync(context)
    , Blockchaincom() {}

boost::future<json> BlockchaincomAsync::fetchAsync(const String& path, const String& api,
                                               const String& method, const json& params,
                                               const std::map<String, String>& headers) {
    return ExchangeAsync::fetchAsync(path, api, method, params, headers);
}

boost::future<json> BlockchaincomAsync::fetchTimeAsync(const json& params) {
    return fetchAsync("/time", "public", "GET", params);
}

boost::future<json> BlockchaincomAsync::fetchMarketsAsync(const json& params) {
    return fetchAsync("/markets", "public", "GET", params);
}

boost::future<json> BlockchaincomAsync::fetchCurrenciesAsync(const json& params) {
    return fetchAsync("/symbols", "public", "GET", params);
}

boost::future<json> BlockchaincomAsync::fetchTickerAsync(const String& symbol, const json& params) {
    String market_id = this->market_id(symbol);
    return fetchAsync("/tickers/" + market_id, "public", "GET", params);
}

boost::future<json> BlockchaincomAsync::fetchTickersAsync(const std::vector<String>& symbols, const json& params) {
    return fetchAsync("/tickers", "public", "GET", params);
}

boost::future<json> BlockchaincomAsync::fetchOrderBookAsync(const String& symbol, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (limit > 0) {
        request["depth"] = limit;
    }
    return fetchAsync("/l2/" + market_id, "public", "GET", request);
}

boost::future<json> BlockchaincomAsync::fetchTradesAsync(const String& symbol, int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    if (limit > 0) {
        request["limit"] = limit;
    }
    if (since > 0) {
        request["before"] = since;
    }
    return fetchAsync("/trades/" + market_id, "public", "GET", request);
}

boost::future<json> BlockchaincomAsync::fetchOHLCVAsync(const String& symbol, const String& timeframe,
                                                    int since, int limit, const json& params) {
    String market_id = this->market_id(symbol);
    json request = params;
    request["timeframe"] = timeframe;
    if (since > 0) {
        request["start_time"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/candlesticks/" + market_id, "public", "GET", request);
}

boost::future<json> BlockchaincomAsync::fetchBalanceAsync(const json& params) {
    return fetchAsync("/accounts", "private", "GET", params);
}

boost::future<json> BlockchaincomAsync::createOrderAsync(const String& symbol, const String& type,
                                                    const String& side, double amount,
                                                    double price, const json& params) {
    String market_id = this->market_id(symbol);
    json request = {
        {"symbol", market_id},
        {"side", side},
        {"orderType", type},
        {"quantity", this->amount_to_precision(symbol, amount)}
    };
    if ((type == "LIMIT") && (price > 0)) {
        request["price"] = this->price_to_precision(symbol, price);
    }
    request.update(params);
    return fetchAsync("/orders", "private", "POST", request);
}

boost::future<json> BlockchaincomAsync::cancelOrderAsync(const String& id, const String& symbol, const json& params) {
    return fetchAsync("/orders/" + id, "private", "DELETE", params);
}

boost::future<json> BlockchaincomAsync::cancelAllOrdersAsync(const String& symbol, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    return fetchAsync("/orders", "private", "DELETE", request);
}

boost::future<json> BlockchaincomAsync::fetchOrderAsync(const String& id, const String& symbol, const json& params) {
    return fetchAsync("/orders/" + id, "private", "GET", params);
}

boost::future<json> BlockchaincomAsync::fetchOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["from"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/orders", "private", "GET", request);
}

boost::future<json> BlockchaincomAsync::fetchOpenOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["from"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    request["status"] = "OPEN";
    return fetchAsync("/orders", "private", "GET", request);
}

boost::future<json> BlockchaincomAsync::fetchClosedOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["from"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    request["status"] = "FILLED,CANCELED,REJECTED,EXPIRED";
    return fetchAsync("/orders", "private", "GET", request);
}

boost::future<json> BlockchaincomAsync::fetchMyTradesAsync(const String& symbol, int since, int limit, const json& params) {
    json request = params;
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    if (since > 0) {
        request["from"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/trades", "private", "GET", request);
}

boost::future<json> BlockchaincomAsync::fetchAccountsAsync(const json& params) {
    return fetchAsync("/accounts", "private", "GET", params);
}

boost::future<json> BlockchaincomAsync::fetchTradingFeesAsync(const json& params) {
    return fetchAsync("/fees/trading", "private", "GET", params);
}

boost::future<json> BlockchaincomAsync::fetchFundingFeesAsync(const json& params) {
    return fetchAsync("/fees/funding", "private", "GET", params);
}

boost::future<json> BlockchaincomAsync::fetchDepositAddressAsync(const String& code, const json& params) {
    json request = params;
    request["currency"] = code;
    return fetchAsync("/deposits/address", "private", "GET", request);
}

boost::future<json> BlockchaincomAsync::fetchDepositsAsync(const String& code, int since, int limit, const json& params) {
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
    return fetchAsync("/deposits", "private", "GET", request);
}

boost::future<json> BlockchaincomAsync::fetchWithdrawalsAsync(const String& code, int since, int limit, const json& params) {
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
    return fetchAsync("/withdrawals", "private", "GET", request);
}

boost::future<json> BlockchaincomAsync::withdrawAsync(const String& code, double amount,
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
    return fetchAsync("/withdrawals", "private", "POST", request);
}

boost::future<json> BlockchaincomAsync::fetchTransactionsAsync(const String& code, int since, int limit, const json& params) {
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
    return fetchAsync("/transactions", "private", "GET", request);
}

} // namespace ccxt
