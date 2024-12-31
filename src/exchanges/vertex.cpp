#include "ccxt/exchanges/vertex.h"
#include <jwt-cpp/jwt.h>
#include <sstream>
#include <iomanip>

namespace ccxt {

Vertex::Vertex() {
    id = "vertex";
    name = "Vertex";
    countries = {"BVI"};  // British Virgin Islands
    version = "v1";
    rateLimit = 100;
    certified = true;
    pro = true;
    has = {
        {"cancelAllOrders", true},
        {"cancelOrder", true},
        {"createOrder", true},
        {"fetchBalance", true},
        {"fetchClosedOrders", true},
        {"fetchCurrencies", true},
        {"fetchDeposits", true},
        {"fetchFundingRate", true},
        {"fetchFundingRateHistory", true},
        {"fetchIndexOHLCV", true},
        {"fetchLeverage", true},
        {"fetchMarkets", true},
        {"fetchMarkOHLCV", true},
        {"fetchMyTrades", true},
        {"fetchOHLCV", true},
        {"fetchOpenOrders", true},
        {"fetchOrder", true},
        {"fetchOrderBook", true},
        {"fetchOrders", true},
        {"fetchPositions", true},
        {"fetchTicker", true},
        {"fetchTickers", true},
        {"fetchTrades", true},
        {"fetchWithdrawals", true},
        {"setLeverage", true}
    };

    // URLs
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/158227251-3a92a220-9222-453c-9277-977c6677fe71.jpg"},
        {"api", {
            {"public", "https://prod.vertexprotocol.com/v1"},
            {"private", "https://prod.vertexprotocol.com/v1"}
        }},
        {"www", "https://vertex.fi"},
        {"doc", {
            "https://docs.vertex.fi/docs/api/overview",
            "https://vertex.fi/docs/api"
        }},
        {"fees", "https://vertex.fi/fees"}
    };

    timeframes = {
        {"1m", "1m"},
        {"5m", "5m"},
        {"15m", "15m"},
        {"30m", "30m"},
        {"1h", "1h"},
        {"4h", "4h"},
        {"12h", "12h"},
        {"1d", "1d"},
        {"1w", "1w"},
        {"1M", "1M"}
    };

    initializeApiEndpoints();
}

void Vertex::initializeApiEndpoints() {
    api = {
        {"public", {
            {"get", {
                "markets",
                "orderbook/{symbol}",
                "trades/{symbol}",
                "tickers",
                "ticker/{symbol}",
                "candles/{symbol}",
                "index/candles/{symbol}",
                "mark/candles/{symbol}",
                "funding_rate/{symbol}",
                "funding_rate_history/{symbol}"
            }}
        }},
        {"private", {
            {"get", {
                "balances",
                "positions",
                "orders",
                "order/{orderId}",
                "trades",
                "leverage/{symbol}"
            }},
            {"post", {
                "order",
                "orders/cancel",
                "leverage"
            }},
            {"delete", {
                "order/{orderId}"
            }}
        }}
    };

    fees = {
        {"trading", {
            {"tierBased", true},
            {"percentage", true},
            {"maker", 0.0002},
            {"taker", 0.0005}
        }},
        {"funding", {
            {"tierBased", false},
            {"percentage", true},
            {"withdraw", {}},
            {"deposit", {}}
        }}
    };

    requiredCredentials = {
        {"apiKey", true},
        {"secret", true}
    };

    precisionMode = TICK_SIZE;
}

// Market Data Methods
json Vertex::fetchMarkets(const json& params) {
    auto response = fetch("/markets", "public", "GET", params);
    json result = json::array();

    for (const auto& market : response["markets"]) {
        std::string id = market["symbol"].get<std::string>();
        std::string baseId = market["baseAsset"].get<std::string>();
        std::string quoteId = market["quoteAsset"].get<std::string>();
        std::string base = commonCurrencyCode(baseId);
        std::string quote = commonCurrencyCode(quoteId);
        std::string type = market["type"].get<std::string>();
        std::string symbol = base + "/" + quote + ":" + quote;
        bool linear = (type == "linear");

        result.push_back({
            {"id", id},
            {"symbol", symbol},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", market["active"].get<bool>()},
            {"type", type},
            {"linear", linear},
            {"inverse", !linear},
            {"spot", false},
            {"swap", true},
            {"future", false},
            {"option", false},
            {"contract", true},
            {"contractSize", market["contractSize"].get<double>()},
            {"precision", {
                {"amount", market["amountPrecision"].get<int>()},
                {"price", market["pricePrecision"].get<int>()}
            }},
            {"limits", {
                {"leverage", {
                    {"min", market["minLeverage"].get<double>()},
                    {"max", market["maxLeverage"].get<double>()}
                }},
                {"amount", {
                    {"min", market["minOrderSize"].get<double>()},
                    {"max", market["maxOrderSize"].get<double>()}
                }},
                {"price", {
                    {"min", market["minPrice"].get<double>()},
                    {"max", market["maxPrice"].get<double>()}
                }},
                {"cost", {
                    {"min", market["minNotional"].get<double>()},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }

    return result;
}

json Vertex::fetchTicker(const std::string& symbol, const json& params) {
    auto market = loadMarket(symbol);
    json requestParams = params;
    requestParams["symbol"] = market["id"];

    auto response = fetch("/ticker/" + market["id"].get<std::string>(), "public", "GET", requestParams);
    return parseTicker(response, market);
}

json Vertex::fetchTickers(const std::vector<std::string>& symbols, const json& params) {
    auto response = fetch("/tickers", "public", "GET", params);
    json result = json::object();

    for (const auto& ticker : response["tickers"]) {
        std::string marketId = ticker["symbol"].get<std::string>();
        auto market = loadMarketById(marketId);
        std::string symbol = market["symbol"].get<std::string>();
        result[symbol] = parseTicker(ticker, market);
    }

    return filterByArray(result, "symbol", symbols);
}

json Vertex::fetchOrderBook(const std::string& symbol, int limit, const json& params) {
    auto market = loadMarket(symbol);
    json requestParams = params;
    if (limit > 0) {
        requestParams["depth"] = limit;
    }

    auto response = fetch("/orderbook/" + market["id"].get<std::string>(), "public", "GET", requestParams);
    long long timestamp = response["timestamp"].get<long long>();

    return {
        {"symbol", symbol},
        {"bids", response["bids"]},
        {"asks", response["asks"]},
        {"timestamp", timestamp},
        {"datetime", iso8601(timestamp)},
        {"nonce", nullptr}
    };
}

json Vertex::fetchOHLCV(const std::string& symbol, const std::string& timeframe,
                       int since, int limit, const json& params) {
    auto market = loadMarket(symbol);
    json requestParams = params;
    requestParams["symbol"] = market["id"];
    requestParams["interval"] = timeframes[timeframe];

    if (since > 0) {
        requestParams["startTime"] = since;
    }
    if (limit > 0) {
        requestParams["limit"] = limit;
    }

    auto response = fetch("/candles/" + market["id"].get<std::string>(), "public", "GET", requestParams);
    return parseOHLCVs(response["candles"], market, timeframe, since, limit);
}

json Vertex::fetchTrades(const std::string& symbol, int since, int limit, const json& params) {
    auto market = loadMarket(symbol);
    json requestParams = params;
    
    if (since > 0) {
        requestParams["startTime"] = since;
    }
    if (limit > 0) {
        requestParams["limit"] = limit;
    }

    auto response = fetch("/trades/" + market["id"].get<std::string>(), "public", "GET", requestParams);
    return parseTrades(response["trades"], market, since, limit);
}

// Helper methods for parsing market data
json Vertex::parseTicker(const json& ticker, const Market& market) {
    long long timestamp = ticker["timestamp"].get<long long>();
    std::string symbol = market.symbol;

    return {
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", iso8601(timestamp)},
        {"high", ticker["high24h"].get<double>()},
        {"low", ticker["low24h"].get<double>()},
        {"bid", ticker["bestBid"].get<double>()},
        {"bidVolume", ticker["bestBidSize"].get<double>()},
        {"ask", ticker["bestAsk"].get<double>()},
        {"askVolume", ticker["bestAskSize"].get<double>()},
        {"vwap", ticker["vwap"].get<double>()},
        {"open", ticker["open24h"].get<double>()},
        {"close", ticker["lastPrice"].get<double>()},
        {"last", ticker["lastPrice"].get<double>()},
        {"previousClose", nullptr},
        {"change", nullptr},
        {"percentage", ticker["priceChange24h"].get<double>()},
        {"average", nullptr},
        {"baseVolume", ticker["volume24h"].get<double>()},
        {"quoteVolume", ticker["quoteVolume24h"].get<double>()},
        {"info", ticker}
    };
}

json Vertex::parseOHLCV(const json& ohlcv, const Market& market) {
    return json::array({
        ohlcv["timestamp"].get<long long>(),
        ohlcv["open"].get<double>(),
        ohlcv["high"].get<double>(),
        ohlcv["low"].get<double>(),
        ohlcv["close"].get<double>(),
        ohlcv["volume"].get<double>()
    });
}

} // namespace ccxt
