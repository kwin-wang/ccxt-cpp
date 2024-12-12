#include "btcturk.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Btcturk::Btcturk() {
    id = "btcturk";
    name = "BTCTurk";
    version = "v1";
    certified = true;
    pro = false;
    hasPublicAPI = true;
    hasPrivateAPI = true;
    hasFiatAPI = true;
    hasMarginAPI = false;
    hasFuturesAPI = false;

    // Initialize URLs
    baseUrl = "https://api.btcturk.com";
    urls = {
        {"logo", "https://user-images.githubusercontent.com/51840849/87153926-efbef500-c2c0-11ea-9842-05b63612c4b9.jpg"},
        {"api", {
            {"public", "https://api.btcturk.com/api"},
            {"private", "https://api.btcturk.com/api"}
        }},
        {"www", "https://www.btcturk.com"},
        {"doc", {
            "https://github.com/BTCTrader/broker-api-docs",
            "https://docs.btcturk.com/"
        }},
        {"fees", "https://www.btcturk.com/commission-fees"}
    };

    initializeApiEndpoints();
    initializeTimeframes();
    initializeMarketTypes();
    initializeOptions();
    initializeErrorCodes();
    initializeFees();
}

void Btcturk::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "v2/server/exchangeinfo",
                "v2/ticker",
                "v2/orderbook",
                "v2/trades",
                "v2/ohlc",
                "v2/server/time"
            }}
        }},
        {"private", {
            {"GET", {
                "v1/users/balances",
                "v1/openOrders",
                "v1/allOrders",
                "v1/users/transactions/trade",
                "v1/users/transactions/crypto",
                "v1/users/transactions/fiat",
                "v1/users/banks/withdrawal/fiat",
                "v1/users/banks/withdrawal/crypto"
            }},
            {"POST", {
                "v1/order",
                "v1/cancelOrder",
                "v1/users/banks/withdrawal/fiat",
                "v1/users/banks/withdrawal/crypto"
            }}
        }}
    };
}

void Btcturk::initializeTimeframes() {
    timeframes = {
        {"1d", "1d"},
        {"1h", "1h"},
        {"1m", "1m"},
        {"1w", "1w"},
        {"4h", "4h"},
        {"1M", "1M"}
    };
}

json Btcturk::fetchMarkets(const json& params) {
    json response = fetch("/v2/server/exchangeinfo", "public", "GET", params);
    json markets = response["data"]["symbols"];
    json result = json::array();
    
    for (const auto& market : markets) {
        String id = market["id"].get<String>();
        String baseId = market["numerator"].get<String>();
        String quoteId = market["denominator"].get<String>();
        String base = this->safeCurrencyCode(baseId);
        String quote = this->safeCurrencyCode(quoteId);
        String symbol = base + "/" + quote;
        
        result.push_back({
            {"id", id},
            {"symbol", symbol},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", market["status"].get<String>() == "TRADING"},
            {"precision", {
                {"amount", market["numeratorScale"].get<int>()},
                {"price", market["denominatorScale"].get<int>()}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeFloat(market, "minExchangeValue")},
                    {"max", this->safeFloat(market, "maxExchangeValue")}
                }},
                {"price", {
                    {"min", this->safeFloat(market, "minPrice")},
                    {"max", this->safeFloat(market, "maxPrice")}
                }},
                {"cost", {
                    {"min", this->safeFloat(market, "minTotal")},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return result;
}

json Btcturk::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = this->privateGetV1UsersBalances(params);
    json balances = this->safeValue(response, "data");
    return this->parseBalance(balances);
}

json Btcturk::parseBalance(const json& response) {
    json result = {{"info", response}};
    
    for (const auto& balance : response) {
        String currencyId = balance["asset"].get<String>();
        String code = this->safeCurrencyCode(currencyId);
        String account = {
            {"free", this->safeFloat(balance, "free")},
            {"used", this->safeFloat(balance, "locked")},
            {"total", this->safeFloat(balance, "balance")}
        };
        result[code] = account;
    }
    
    return result;
}

json Btcturk::createOrder(const String& symbol, const String& type,
                         const String& side, double amount,
                         double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    String orderType = type.substr(0, 1).toUpperCase() + type.substr(1);
    String orderMethod = side.substr(0, 1).toUpperCase() + side.substr(1);
    json request = {
        {"pairSymbol", market["id"]},
        {"orderType", orderType},
        {"orderMethod", orderMethod},
        {"quantity", this->amountToPrecision(symbol, amount)}
    };
    if (type == "limit") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    json response = this->privatePostV1Order(this->extend(request, params));
    json data = this->safeValue(response, "data");
    return this->parseOrder(data, market);
}

String Btcturk::sign(const String& path, const String& api,
                     const String& method, const json& params,
                     const std::map<String, String>& headers,
                     const json& body) {
    String url = this->urls["api"][api] + path;
    
    if (api == "private") {
        this->checkRequiredCredentials();
        String timestamp = std::to_string(this->milliseconds());
        String nonce = this->uuid();
        
        String auth = this->apiKey + timestamp;
        String signature = this->hmac(auth, this->encode(this->secret),
                                    "sha256", "base64");
        
        const_cast<std::map<String, String>&>(headers)["X-PCK"] = this->apiKey;
        const_cast<std::map<String, String>&>(headers)["X-Stamp"] = timestamp;
        const_cast<std::map<String, String>&>(headers)["X-Signature"] = signature;
        
        if (method == "POST") {
            const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/json";
            body = this->json(this->extend(params, {}));
        } else if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    } else if (!params.empty()) {
        url += "?" + this->urlencode(params);
    }
    
    return url;
}

String Btcturk::getNonce() {
    return this->uuid();
}

json Btcturk::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "id");
    String timestamp = this->safeString(order, "datetime");
    String status = this->parseOrderStatus(this->safeString(order, "status"));
    String symbol = nullptr;
    
    if (!market.empty()) {
        symbol = market["symbol"];
    } else {
        String marketId = this->safeString(order, "pairSymbol");
        if (marketId != nullptr) {
            if (this->markets_by_id.contains(marketId)) {
                market = this->markets_by_id[marketId];
                symbol = market["symbol"];
            } else {
                symbol = marketId;
            }
        }
    }
    
    String type = this->safeStringLower(order, "type");
    String side = this->safeStringLower(order, "orderMethod");
    
    return {
        {"id", id},
        {"clientOrderId", nullptr},
        {"timestamp", this->parse8601(timestamp)},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"type", type},
        {"timeInForce", nullptr},
        {"postOnly", nullptr},
        {"status", status},
        {"symbol", symbol},
        {"side", side},
        {"price", this->safeFloat(order, "price")},
        {"amount", this->safeFloat(order, "quantity")},
        {"filled", this->safeFloat(order, "executedQuantity")},
        {"remaining", this->safeFloat(order, "leftQuantity")},
        {"cost", this->safeFloat(order, "executedValue")},
        {"trades", nullptr},
        {"fee", nullptr},
        {"info", order}
    };
}

String Btcturk::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"Untouched", "open"},
        {"Partial", "open"},
        {"Canceled", "canceled"},
        {"Closed", "closed"}
    };
    
    return this->safeString(statuses, status, status);
}

json Btcturk::fetchTicker(const String& symbol, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {
        {"pairSymbol", market["id"]}
    };
    json response = this->publicGetV2Ticker(this->extend(request, params));
    json ticker = this->safeValue(response, "data");
    return this->parseTicker(ticker, market);
}

json Btcturk::fetchTickers(const std::vector<String>& symbols, const json& params) {
    this->loadMarkets();
    json response = this->publicGetV2Ticker(params);
    json data = this->safeValue(response, "data");
    json result = json::object();
    for (const auto& ticker : data) {
        json market = this->safeMarket(ticker["pair"]);
        String symbol = market["symbol"];
        if (symbols.empty() || std::find(symbols.begin(), symbols.end(), symbol) != symbols.end()) {
            result[symbol] = this->parseTicker(ticker, market);
        }
    }
    return result;
}

json Btcturk::fetchOrderBook(const String& symbol, int limit, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {
        {"pairSymbol", market["id"]}
    };
    if (limit != 0) {
        request["limit"] = limit;
    }
    json response = this->publicGetV2Orderbook(this->extend(request, params));
    json orderbook = this->safeValue(response, "data");
    double timestamp = this->safeTimestamp(response, "timestamp");
    return this->parseOrderBook(orderbook, symbol, timestamp);
}

json Btcturk::fetchTrades(const String& symbol, int since, int limit, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {
        {"pairSymbol", market["id"]}
    };
    if (limit != 0) {
        request["limit"] = limit;
    }
    json response = this->publicGetV2Trades(this->extend(request, params));
    json trades = this->safeValue(response, "data");
    return this->parseTrades(trades, market, since, limit);
}

json Btcturk::fetchOHLCV(const String& symbol, const String& timeframe, int since, int limit, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {
        {"pairSymbol", market["id"]},
        {"resolution", this->timeframes[timeframe]}
    };
    if (since != 0) {
        request["from"] = this->iso8601(since);
    }
    if (limit != 0) {
        request["limit"] = limit;
    }
    json response = this->publicGetV2Ohlc(this->extend(request, params));
    json ohlcv = this->safeValue(response, "data");
    return this->parseOHLCVs(ohlcv, market, timeframe, since, limit);
}

json Btcturk::parseTicker(const json& ticker, const Market& market) {
    double timestamp = this->safeTimestamp(ticker, "timestamp");
    String symbol = this->safeString(market, "symbol");
    return {
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"high", this->safeFloat(ticker, "high")},
        {"low", this->safeFloat(ticker, "low")},
        {"bid", this->safeFloat(ticker, "bid")},
        {"bidVolume", this->safeFloat(ticker, "bidSize")},
        {"ask", this->safeFloat(ticker, "ask")},
        {"askVolume", this->safeFloat(ticker, "askSize")},
        {"vwap", this->safeFloat(ticker, "average")},
        {"open", this->safeFloat(ticker, "open")},
        {"close", this->safeFloat(ticker, "last")},
        {"last", this->safeFloat(ticker, "last")},
        {"previousClose", this->safeFloat(ticker, "previousClose")},
        {"change", this->safeFloat(ticker, "daily")},
        {"percentage", this->safeFloat(ticker, "dailyPercent")},
        {"baseVolume", this->safeFloat(ticker, "volume")},
        {"quoteVolume", this->safeFloat(ticker, "quoteVolume")},
        {"info", ticker}
    };
}

json Btcturk::parseTrade(const json& trade, const Market& market) {
    String id = this->safeString(trade, "id");
    double timestamp = this->safeTimestamp(trade, "timestamp");
    double price = this->safeFloat(trade, "price");
    double amount = this->safeFloat(trade, "amount");
    String side = this->safeString(trade, "side");
    return {
        {"id", id},
        {"info", trade},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"symbol", market["symbol"]},
        {"type", "limit"},
        {"side", side},
        {"order", this->safeString(trade, "orderId")},
        {"takerOrMaker", this->safeString(trade, "takerOrMaker")},
        {"price", price},
        {"amount", amount},
        {"cost", price * amount},
        {"fee", this->parseTradeFee(trade)}
    };
}

json Btcturk::parseOHLCV(const json& ohlcv, const Market& market, const String& timeframe) {
    return {
        this->safeTimestamp(ohlcv, "time"),
        this->safeFloat(ohlcv, "open"),
        this->safeFloat(ohlcv, "high"),
        this->safeFloat(ohlcv, "low"),
        this->safeFloat(ohlcv, "close"),
        this->safeFloat(ohlcv, "volume")
    };
}

json Btcturk::cancelOrder(const String& id, const String& symbol, const json& params) {
    if (symbol.empty()) {
        throw ExchangeError("symbol is required for cancelOrder");
    }
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {
        {"id", id},
        {"pairSymbol", market["id"]}
    };
    return this->privatePostV1CancelOrder(this->extend(request, params));
}

json Btcturk::fetchOrder(const String& id, const String& symbol, const json& params) {
    if (symbol.empty()) {
        throw ExchangeError("symbol is required for fetchOrder");
    }
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {
        {"id", id},
        {"pairSymbol", market["id"]}
    };
    json response = this->privateGetV1Order(this->extend(request, params));
    json data = this->safeValue(response, "data");
    return this->parseOrder(data, market);
}

json Btcturk::fetchOrders(const String& symbol, int since, int limit, const json& params) {
    if (symbol.empty()) {
        throw ExchangeError("symbol is required for fetchOrders");
    }
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {
        {"pairSymbol", market["id"]}
    };
    if (limit != 0) {
        request["limit"] = limit;
    }
    if (since != 0) {
        request["startTime"] = this->iso8601(since);
    }
    json response = this->privateGetV1AllOrders(this->extend(request, params));
    json data = this->safeValue(response, "data");
    return this->parseOrders(data, market, since, limit);
}

json Btcturk::fetchOpenOrders(const String& symbol, int since, int limit, const json& params) {
    if (symbol.empty()) {
        throw ExchangeError("symbol is required for fetchOpenOrders");
    }
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {
        {"pairSymbol", market["id"]}
    };
    if (limit != 0) {
        request["limit"] = limit;
    }
    json response = this->privateGetV1OpenOrders(this->extend(request, params));
    json data = this->safeValue(response, "data");
    return this->parseOrders(data, market, since, limit);
}

json Btcturk::fetchClosedOrders(const String& symbol, int since, int limit, const json& params) {
    json request = this->extend({
        {"status": "closed"}
    }, params);
    return this->fetchOrders(symbol, since, limit, request);
}

} // namespace ccxt
