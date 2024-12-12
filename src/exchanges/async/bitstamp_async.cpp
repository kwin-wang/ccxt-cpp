#include "ccxt/exchanges/async/bitstamp_async.h"

namespace ccxt {

BitstampAsync::BitstampAsync(const boost::asio::io_context& context)
    : ExchangeAsync(context)
    , Bitstamp() {}

boost::future<json> BitstampAsync::fetchAsync(const String& path, const String& api,
                                          const String& method, const json& params,
                                          const std::map<String, String>& headers) {
    return ExchangeAsync::fetchAsync(path, api, method, params, headers);
}

boost::future<json> BitstampAsync::fetchTimeAsync(const json& params) {
    return fetchAsync("/api/v2/time", "public", "GET", params);
}

boost::future<json> BitstampAsync::fetchMarketsAsync(const json& params) {
    return fetchAsync("/api/v2/trading-pairs-info", "public", "GET", params);
}

boost::future<json> BitstampAsync::fetchCurrenciesAsync(const json& params) {
    return fetchAsync("/api/v2/currencies", "public", "GET", params);
}

boost::future<json> BitstampAsync::fetchTickerAsync(const String& symbol, const json& params) {
    auto market = this->market(symbol);
    auto request = this->extend(params, {
        "pair": market["id"]
    });
    return fetchAsync("/api/v2/ticker/" + market["id"], "public", "GET", request)
        .then([this, market, symbol](json response) {
            auto ticker = this->parseTicker(response, market);
            ticker["symbol"] = symbol;
            return ticker;
        });
}

boost::future<json> BitstampAsync::fetchTickersAsync(const std::vector<String>& symbols, const json& params) {
    return fetchAsync("/api/v2/ticker", "public", "GET", params)
        .then([this, symbols](json response) {
            auto result = json::object();
            for (const auto& market : this->markets) {
                auto symbol = market["symbol"].get<String>();
                if (symbols.empty() || std::find(symbols.begin(), symbols.end(), symbol) != symbols.end()) {
                    auto ticker = this->parseTicker(response[market["id"].get<String>()], market);
                    ticker["symbol"] = symbol;
                    result[symbol] = ticker;
                }
            }
            return result;
        });
}

boost::future<json> BitstampAsync::fetchOrderBookAsync(const String& symbol, int limit, const json& params) {
    auto market = this->market(symbol);
    auto request = this->extend(params, {
        "pair": market["id"]
    });
    if (limit) {
        request["limit_orders"] = limit;
    }
    return fetchAsync("/api/v2/order_book/" + market["id"], "public", "GET", request)
        .then([this, symbol](json response) {
            return this->parseOrderBook(response, symbol);
        });
}

boost::future<json> BitstampAsync::fetchTradesAsync(const String& symbol, int since, int limit, const json& params) {
    auto market = this->market(symbol);
    auto request = this->extend(params, {
        "pair": market["id"]
    });
    if (limit) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v2/transactions/" + market["id"], "public", "GET", request)
        .then([this, market, since, limit](json response) {
            return this->parseTrades(response, market, since, limit);
        });
}

boost::future<json> BitstampAsync::fetchOHLCVAsync(const String& symbol, const String& timeframe,
                                                  int since, int limit, const json& params) {
    auto market = this->market(symbol);
    auto request = this->extend(params, {
        "pair": market["id"],
        "step": this->timeframes[timeframe]
    });
    if (since) {
        request["start"] = since;
    }
    if (limit) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v2/ohlc/" + market["id"], "public", "GET", request)
        .then([this, market, timeframe, since, limit](json response) {
            return this->parseOHLCV(response["data"]["ohlc"], market, timeframe, since, limit);
        });
}

boost::future<json> BitstampAsync::fetchBalanceAsync(const json& params) {
    return fetchAsync("/api/v2/balance", "private", "POST", params)
        .then([this](json response) {
            auto result = {"info": response};
            auto codes = this->currencies.keys();
            for (const auto& code : codes) {
                auto currency = this->currency(code);
                auto account = this->account();
                auto currencyId = currency["id"].get<String>();
                auto free = currencyId + "_available";
                auto total = currencyId + "_balance";
                auto used = currencyId + "_reserved";
                if (response.contains(free)) {
                    account["free"] = this->safeString(response, free);
                }
                if (response.contains(total)) {
                    account["total"] = this->safeString(response, total);
                }
                if (response.contains(used)) {
                    account["used"] = this->safeString(response, used);
                }
                result[code] = account;
            }
            return this->parseBalance(result);
        });
}

boost::future<json> BitstampAsync::createOrderAsync(const String& symbol, const String& type,
                                                  const String& side, double amount,
                                                  double price, const json& params) {
    auto market = this->market(symbol);
    auto request = {
        "pair": market["id"],
        "amount": this->amountToPrecision(symbol, amount)
    };
    if (type == "limit") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    String endpoint = "/api/v2/" + side.toLowerCase() + "/" + type;
    return fetchAsync(endpoint, "private", "POST", this->extend(request, params))
        .then([this, market](json response) {
            return this->parseOrder(response, market);
        });
}

boost::future<json> BitstampAsync::cancelOrderAsync(const String& id, const String& symbol, const json& params) {
    return fetchAsync("/api/v2/cancel_order", "private", "POST", {"id": id})
        .then([this](json response) {
            return response;
        });
}

boost::future<json> BitstampAsync::fetchOrderAsync(const String& id, const String& symbol, const json& params) {
    return fetchAsync("/api/v2/order_status", "private", "POST", {"id": id})
        .then([this](json response) {
            return this->parseOrder(response);
        });
}

boost::future<json> BitstampAsync::fetchMyTradesAsync(const String& symbol, int since, int limit, const json& params) {
    auto market = symbol ? this->market(symbol) : nullptr;
    auto request = json::object();
    if (limit) {
        request["limit"] = limit;
    }
    if (market) {
        request["pair"] = market["id"];
    }
    return fetchAsync("/api/v2/user_transactions", "private", "POST", this->extend(request, params))
        .then([this, market, since, limit](json response) {
            return this->parseTrades(response, market, since, limit);
        });
}

boost::future<json> BitstampAsync::fetchTransactionFeesAsync(const json& params) {
    return fetchAsync("/api/v2/balance", "private", "POST", params)
        .then([](json response) {
            return json{
                {"info", response},
                {"withdraw", json::object()},
                {"deposit", json::object()}
            };
        });
}

boost::future<json> BitstampAsync::fetchTradingFeesAsync(const json& params) {
    return fetchAsync("/api/v2/balance", "private", "POST", params)
        .then([this](json response) {
            return json{
                {"info", response},
                {"maker", this->safeNumber(response, "fee")},
                {"taker", this->safeNumber(response, "fee")}
            };
        });
}

boost::future<json> BitstampAsync::withdrawAsync(const String& code, double amount,
                                               const String& address, const String& tag,
                                               const json& params) {
    this->checkAddress(address);
    auto currency = this->currency(code);
    auto request = {
        "address": address,
        "amount": amount,
        "currency": currency["id"]
    };
    if (tag) {
        request["destination_tag"] = tag;
    }
    return fetchAsync("/api/v2/withdrawal", "private", "POST", this->extend(request, params))
        .then([this, currency](json response) {
            return this->parseTransaction(response, currency);
        });
}

boost::future<json> BitstampAsync::fetchAccountsAsync(const json& params) {
    return fetchAsync("/api/v2/balance", "private", "POST", params);
}

boost::future<json> BitstampAsync::fetchFundingFeesAsync(const json& params) {
    return fetchAsync("/api/v2/fees/funding", "private", "POST", params);
}

boost::future<json> BitstampAsync::fetchDepositAddressAsync(const String& code, const json& params) {
    json request = params;
    request["currency"] = code;
    return fetchAsync("/api/v2/deposit-address", "private", "POST", request);
}

boost::future<json> BitstampAsync::fetchDepositsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["time"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v2/user_transactions", "private", "POST", request);
}

boost::future<json> BitstampAsync::fetchWithdrawalsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["time"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v2/user_transactions", "private", "POST", request);
}

boost::future<json> BitstampAsync::fetchTransactionsAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["time"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v2/user_transactions", "private", "POST", request);
}

boost::future<json> BitstampAsync::fetchLedgerAsync(const String& code, int since, int limit, const json& params) {
    json request = params;
    if (!code.empty()) {
        request["currency"] = code;
    }
    if (since > 0) {
        request["time"] = this->iso8601(since);
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/api/v2/user_transactions", "private", "POST", request);
}

} // namespace ccxt
