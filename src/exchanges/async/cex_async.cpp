#include "ccxt/exchanges/async/cex_async.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace ccxt {

CexAsync::CexAsync(const boost::asio::io_context& context)
    : ExchangeAsync(context), cex() {}

boost::future<json> CexAsync::fetchAsync(const String& path, const String& api,
                                       const String& method, const json& params,
                                       const std::map<String, String>& headers) {
    return ExchangeAsync::fetchAsync(path, api, method, params, headers);
}

// Market Data API
boost::future<json> CexAsync::fetchMarketsAsync(const json& params) {
    return fetchAsync("/currency_profile", "public", "GET", params, {});
}

boost::future<json> CexAsync::fetchTickerAsync(const String& symbol, const json& params) {
    String endpoint = "/ticker/" + this->marketId(symbol);
    return fetchAsync(endpoint, "public", "GET", params, {});
}

boost::future<json> CexAsync::fetchTickersAsync(const std::vector<String>& symbols, const json& params) {
    if (symbols.empty()) {
        throw ArgumentsRequired("fetchTickers requires symbols argument");
    }
    String symbolsStr;
    for (size_t i = 0; i < symbols.size(); ++i) {
        symbolsStr += this->marketId(symbols[i]);
        if (i < symbols.size() - 1) {
            symbolsStr += ",";
        }
    }
    String endpoint = "/tickers/" + symbolsStr;
    return fetchAsync(endpoint, "public", "GET", params, {});
}

boost::future<json> CexAsync::fetchOrderBookAsync(const String& symbol, int limit, const json& params) {
    String endpoint = "/order_book/" + this->marketId(symbol);
    json request = params;
    if (limit != 0) {
        request["depth"] = limit;
    }
    return fetchAsync(endpoint, "public", "GET", request, {});
}

boost::future<json> CexAsync::fetchTradesAsync(const String& symbol, int since, int limit, const json& params) {
    String endpoint = "/trade_history/" + this->marketId(symbol);
    json request = params;
    if (limit != 0) {
        request["limit"] = limit;
    }
    if (since != 0) {
        request["since"] = since;
    }
    return fetchAsync(endpoint, "public", "GET", request, {});
}

boost::future<json> CexAsync::fetchOHLCVAsync(const String& symbol, const String& timeframe,
                                            int since, int limit, const json& params) {
    String endpoint = "/ohlcv/hd/" + this->marketId(symbol);
    json request = this->extend({
        {"timeframe": timeframe}
    }, params);
    if (limit != 0) {
        request["limit"] = limit;
    }
    if (since != 0) {
        request["since"] = since;
    }
    return fetchAsync(endpoint, "public", "GET", request, {});
}

// Trading API
boost::future<json> CexAsync::fetchBalanceAsync(const json& params) {
    return fetchAsync("/balance/", "private", "POST", params, {});
}

boost::future<json> CexAsync::createOrderAsync(const String& symbol, const String& type,
                                             const String& side, double amount,
                                             double price, const json& params) {
    this->loadMarkets();
    String endpoint = "/place_order/" + this->marketId(symbol);
    json request = {
        {"type", type},
        {"side", side},
        {"amount", this->amountToPrecision(symbol, amount)}
    };
    
    if (type == "limit") {
        if (price == 0) {
            throw ExchangeError("createOrder requires price for limit orders");
        }
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    return fetchAsync(endpoint, "private", "POST", this->extend(request, params), {});
}

boost::future<json> CexAsync::cancelOrderAsync(const String& id, const String& symbol,
                                             const json& params) {
    json request = {
        {"id", id}
    };
    if (!symbol.empty()) {
        request["symbol"] = this->marketId(symbol);
    }
    return fetchAsync("/cancel_order", "private", "POST", this->extend(request, params), {});
}

boost::future<json> CexAsync::fetchOrderAsync(const String& id, const String& symbol,
                                            const json& params) {
    json request = {
        {"id", id}
    };
    if (!symbol.empty()) {
        request["symbol"] = this->marketId(symbol);
    }
    return fetchAsync("/get_order", "private", "POST", this->extend(request, params), {});
}

boost::future<json> CexAsync::fetchOpenOrdersAsync(const String& symbol, int since,
                                                 int limit, const json& params) {
    json request = params;
    String endpoint = symbol.empty() ? "/open_orders" : "/open_orders/" + this->marketId(symbol);
    if (limit != 0) {
        request["limit"] = limit;
    }
    if (since != 0) {
        request["since"] = since;
    }
    return fetchAsync(endpoint, "private", "POST", request, {});
}

boost::future<json> CexAsync::fetchClosedOrdersAsync(const String& symbol, int since,
                                                   int limit, const json& params) {
    if (symbol.empty()) {
        throw ArgumentsRequired("fetchClosedOrders requires a symbol argument");
    }
    String endpoint = "/archived_orders/" + this->marketId(symbol);
    json request = params;
    if (limit != 0) {
        request["limit"] = limit;
    }
    if (since != 0) {
        request["since"] = since;
    }
    return fetchAsync(endpoint, "private", "POST", request, {});
}

boost::future<json> CexAsync::fetchDepositAddressAsync(const String& code,
                                                     const json& params) {
    this->loadMarkets();
    Currency currency = this->currency(code);
    json request = {
        {"currency", currency["id"]}
    };
    return fetchAsync("/get_address", "private", "POST", this->extend(request, params), {});
}

boost::future<json> CexAsync::fetchTransactionsAsync(const String& code, int since,
                                                   int limit, const json& params) {
    this->loadMarkets();
    json request = params;
    if (!code.empty()) {
        Currency currency = this->currency(code);
        request["currency"] = currency["id"];
    }
    if (since != 0) {
        request["since"] = since;
    }
    if (limit != 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/archived_orders/get_crypto_transactions", "private", "POST", request, {});
}

} // namespace ccxt
