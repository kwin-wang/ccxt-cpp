#include "ccxt/exchanges/async/bybit_async.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/strand.hpp>

namespace ccxt {

BybitAsync::BybitAsync(const boost::asio::io_context& context)
    : ExchangeAsync(context), Bybit() {}

boost::future<json> BybitAsync::fetchAsync(const String& path, const String& api,
                                         const String& method, const json& params,
                                         const std::map<String, String>& headers) {
    return ExchangeAsync::fetchAsync(path, api, method, params, headers);
}

// Market Data API
boost::future<json> BybitAsync::fetchMarketsAsync(const json& params) {
    return fetchAsync("/v5/market/instruments-info", "public", "GET", params, {});
}

boost::future<json> BybitAsync::fetchTickerAsync(const String& symbol, const json& params) {
    json request = {{"symbol", getBybitSymbol(symbol)}};
    return fetchAsync("/v5/market/tickers", "public", "GET", this->extend(request, params), {});
}

boost::future<json> BybitAsync::fetchTickersAsync(const std::vector<String>& symbols, const json& params) {
    json request = json::object();
    if (!symbols.empty()) {
        std::vector<String> bybitSymbols;
        for (const auto& symbol : symbols) {
            bybitSymbols.push_back(getBybitSymbol(symbol));
        }
        request["symbols"] = bybitSymbols;
    }
    return fetchAsync("/v5/market/tickers", "public", "GET", this->extend(request, params), {});
}

boost::future<json> BybitAsync::fetchOrderBookAsync(const String& symbol, int limit, const json& params) {
    json request = {
        {"symbol", getBybitSymbol(symbol)}
    };
    if (limit != 0) {
        request["limit"] = limit;
    }
    return fetchAsync("/v5/market/orderbook", "public", "GET", this->extend(request, params), {});
}

boost::future<json> BybitAsync::fetchTradesAsync(const String& symbol, int since, int limit, const json& params) {
    json request = {
        {"symbol", getBybitSymbol(symbol)}
    };
    if (limit != 0) {
        request["limit"] = limit;
    }
    if (since != 0) {
        request["startTime"] = since;
    }
    return fetchAsync("/v5/market/trades", "public", "GET", this->extend(request, params), {});
}

boost::future<json> BybitAsync::fetchOHLCVAsync(const String& symbol, const String& timeframe,
                                               int since, int limit, const json& params) {
    json request = {
        {"symbol", getBybitSymbol(symbol)},
        {"interval", timeframes[timeframe]}
    };
    if (limit != 0) {
        request["limit"] = limit;
    }
    if (since != 0) {
        request["startTime"] = since;
    }
    return fetchAsync("/v5/market/kline", "public", "GET", this->extend(request, params), {});
}

// Trading API
boost::future<json> BybitAsync::fetchBalanceAsync(const json& params) {
    return fetchAsync("/v5/account/wallet-balance", "private", "GET", params, {});
}

boost::future<json> BybitAsync::createOrderAsync(const String& symbol, const String& type,
                                               const String& side, double amount,
                                               double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    String uppercaseType = type.toUpperCase();
    
    json request = {
        {"symbol", market["id"]},
        {"side", side.toUpperCase()},
        {"orderType", uppercaseType},
        {"qty", this->amountToPrecision(symbol, amount)}
    };
    
    if (uppercaseType == "LIMIT") {
        if (price == 0) {
            throw ExchangeError("createOrder requires price for LIMIT orders");
        }
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    return fetchAsync("/v5/order/create", "private", "POST", this->extend(request, params), {});
}

boost::future<json> BybitAsync::cancelOrderAsync(const String& id, const String& symbol,
                                               const json& params) {
    if (symbol.empty()) {
        throw ExchangeError("cancelOrder requires a symbol argument");
    }
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {
        {"symbol", market["id"]},
        {"orderId", id}
    };
    return fetchAsync("/v5/order/cancel", "private", "POST", this->extend(request, params), {});
}

boost::future<json> BybitAsync::fetchOrderAsync(const String& id, const String& symbol,
                                              const json& params) {
    if (symbol.empty()) {
        throw ExchangeError("fetchOrder requires a symbol argument");
    }
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {
        {"symbol", market["id"]},
        {"orderId", id}
    };
    return fetchAsync("/v5/order/realtime", "private", "GET", this->extend(request, params), {});
}

boost::future<json> BybitAsync::fetchOrdersAsync(const String& symbol, int since,
                                               int limit, const json& params) {
    if (symbol.empty()) {
        throw ExchangeError("fetchOrders requires a symbol argument");
    }
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {
        {"symbol", market["id"]}
    };
    if (limit != 0) {
        request["limit"] = limit;
    }
    if (since != 0) {
        request["startTime"] = since;
    }
    return fetchAsync("/v5/order/history", "private", "GET", this->extend(request, params), {});
}

boost::future<json> BybitAsync::fetchOpenOrdersAsync(const String& symbol, int since,
                                                   int limit, const json& params) {
    json request = this->extend({
        {"orderStatus", "NEW"}
    }, params);
    return fetchOrdersAsync(symbol, since, limit, request);
}

boost::future<json> BybitAsync::fetchClosedOrdersAsync(const String& symbol, int since,
                                                     int limit, const json& params) {
    json request = this->extend({
        {"orderStatus", "FILLED"}
    }, params);
    return fetchOrdersAsync(symbol, since, limit, request);
}

boost::future<json> BybitAsync::fetchPositionsAsync(const std::vector<String>& symbols,
                                                  const json& params) {
    this->loadMarkets();
    json request = json::object();
    if (!symbols.empty()) {
        std::vector<String> marketIds;
        for (const auto& symbol : symbols) {
            Market market = this->market(symbol);
            marketIds.push_back(market["id"]);
        }
        request["symbol"] = marketIds;
    }
    return fetchAsync("/v5/position/list", "private", "GET", this->extend(request, params), {});
}

boost::future<json> BybitAsync::setLeverageAsync(double leverage, const String& symbol,
                                               const json& params) {
    this->loadMarkets();
    if (symbol.empty()) {
        throw ExchangeError("setLeverage requires a symbol argument");
    }
    Market market = this->market(symbol);
    json request = {
        {"symbol", market["id"]},
        {"leverage", leverage}
    };
    return fetchAsync("/v5/position/set-leverage", "private", "POST", this->extend(request, params), {});
}

boost::future<json> BybitAsync::fetchFundingRateAsync(const String& symbol,
                                                    const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {
        {"symbol", market["id"]}
    };
    return fetchAsync("/v5/market/funding/history", "public", "GET", this->extend(request, params), {});
}

} // namespace ccxt
