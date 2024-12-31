#include "wazirx.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

WazirX::WazirX() {
    id = "wazirx";
    name = "WazirX";
    countries = {"IN"}; // India
    version = "v2";
    rateLimit = 1000;
    certified = true;
    pro = false;
    has = {
        {"cancelOrder", true},
        {"createOrder", true},
        {"fetchBalance", true},
        {"fetchClosedOrders", true},
        {"fetchCurrencies", true},
        {"fetchDeposits", true},
        {"fetchMarkets", true},
        {"fetchMyTrades", true},
        {"fetchOHLCV", true},
        {"fetchOpenOrders", true},
        {"fetchOrder", true},
        {"fetchOrderBook", true},
        {"fetchOrders", true},
        {"fetchStatus", true},
        {"fetchTicker", true},
        {"fetchTickers", true},
        {"fetchTime", true},
        {"fetchTrades", true},
        {"fetchWithdrawals", true}
    };

    // URLs
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/148647666-c109c20b-f8ac-472f-91c3-5f658cb90f49.jpeg"},
        {"api", {
            {"public", "https://api.wazirx.com/api/v2"},
            {"private", "https://api.wazirx.com/api/v2"}
        }},
        {"www", "https://wazirx.com"},
        {"doc", {
            "https://docs.wazirx.com/#public-rest-api-for-wazirx",
            "https://docs.wazirx.com/"
        }},
        {"fees", "https://wazirx.com/fees"}
    };

    timeframes = {
        {"1m", "1m"},
        {"5m", "5m"},
        {"15m", "15m"},
        {"30m", "30m"},
        {"1h", "1h"},
        {"2h", "2h"},
        {"4h", "4h"},
        {"6h", "6h"},
        {"12h", "12h"},
        {"1d", "1d"},
        {"3d", "3d"},
        {"1w", "1w"}
    };

    initializeApiEndpoints();
}

void WazirX::initializeApiEndpoints() {
    api = {
        {"public", {
            {"get", {
                "tickers",
                "ticker/{symbol}",
                "depth/{symbol}",
                "trades/{symbol}",
                "klines",
                "system/status",
                "time",
                "markets"
            }}
        }},
        {"private", {
            {"get", {
                "funds",
                "order",
                "order/history",
                "order/trade_history"
            }},
            {"post", {
                "order",
                "order/cancel"
            }}
        }}
    };

    fees = {
        {"trading", {
            {"tierBased", false},
            {"percentage", true},
            {"taker", 0.002},
            {"maker", 0.002}
        }},
        {"funding", {
            {"tierBased", false},
            {"percentage", false},
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
json WazirX::fetchMarkets(const json& params) {
    auto response = fetch("/markets", "public", "GET", params);
    json result = json::array();

    for (const auto& market : response["markets"]) {
        std::string id = market["symbol"].get<std::string>();
        std::string baseId = market["baseAsset"].get<std::string>();
        std::string quoteId = market["quoteAsset"].get<std::string>();
        std::string base = commonCurrencyCode(baseId);
        std::string quote = commonCurrencyCode(quoteId);
        std::string symbol = base + "/" + quote;

        result.push_back({
            {"id", id},
            {"symbol", symbol},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", market["status"].get<std::string>() == "trading"},
            {"type", "spot"},
            {"spot", true},
            {"margin", false},
            {"future", false},
            {"swap", false},
            {"option", false},
            {"contract", false},
            {"precision", {
                {"amount", market["baseAssetPrecision"].get<int>()},
                {"price", market["quoteAssetPrecision"].get<int>()}
            }},
            {"limits", {
                {"amount", {
                    {"min", market["minTradeSize"].get<double>()},
                    {"max", market["maxTradeSize"].get<double>()}
                }},
                {"price", {
                    {"min", market["tickSize"].get<double>()},
                    {"max", nullptr}
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

json WazirX::fetchTicker(const std::string& symbol, const json& params) {
    auto market = loadMarket(symbol);
    auto response = fetch("/ticker/" + market["id"].get<std::string>(), "public", "GET", params);
    return parseTicker(response, market);
}

json WazirX::fetchTickers(const std::vector<std::string>& symbols, const json& params) {
    auto response = fetch("/tickers", "public", "GET", params);
    json result = json::object();

    for (const auto& [marketId, ticker] : response.items()) {
        if (ticker.is_object()) {
            auto market = loadMarketById(marketId);
            result[market["symbol"].get<std::string>()] = parseTicker(ticker, market);
        }
    }

    return filterByArray(result, "symbol", symbols);
}

json WazirX::fetchOrderBook(const std::string& symbol, int limit, const json& params) {
    auto market = loadMarket(symbol);
    json requestParams = params;
    if (limit > 0) {
        requestParams["limit"] = limit;
    }

    auto response = fetch("/depth/" + market["id"].get<std::string>(), "public", "GET", requestParams);
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

json WazirX::fetchOHLCV(const std::string& symbol, const std::string& timeframe,
                       int since, int limit, const json& params) {
    auto market = loadMarket(symbol);
    json requestParams = {
        {"symbol", market["id"]},
        {"interval", timeframes[timeframe]}
    };

    if (since > 0) {
        requestParams["startTime"] = since;
    }
    if (limit > 0) {
        requestParams["limit"] = limit;
    }
    requestParams.update(params);

    auto response = fetch("/klines", "public", "GET", requestParams);
    return parseOHLCVs(response, market, timeframe, since, limit);
}

json WazirX::fetchTrades(const std::string& symbol, int since, int limit, const json& params) {
    auto market = loadMarket(symbol);
    json requestParams = params;
    if (limit > 0) {
        requestParams["limit"] = limit;
    }

    auto response = fetch("/trades/" + market["id"].get<std::string>(), "public", "GET", requestParams);
    return parseTrades(response, market, since, limit);
}

// Helper methods for parsing market data
json WazirX::parseTicker(const json& ticker, const Market& market) {
    long long timestamp = ticker["at"].get<long long>() * 1000;
    std::string symbol = market.symbol;

    return {
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", iso8601(timestamp)},
        {"high", ticker["high"].get<double>()},
        {"low", ticker["low"].get<double>()},
        {"bid", ticker["buy"].get<double>()},
        {"bidVolume", nullptr},
        {"ask", ticker["sell"].get<double>()},
        {"askVolume", nullptr},
        {"vwap", nullptr},
        {"open", ticker["open"].get<double>()},
        {"close", ticker["last"].get<double>()},
        {"last", ticker["last"].get<double>()},
        {"previousClose", nullptr},
        {"change", nullptr},
        {"percentage", nullptr},
        {"average", nullptr},
        {"baseVolume", ticker["volume"].get<double>()},
        {"quoteVolume", ticker["quoteVolume"].get<double>()},
        {"info", ticker}
    };
}

json WazirX::parseOHLCV(const json& ohlcv, const Market& market) {
    return json::array({
        ohlcv[0].get<long long>(), // timestamp
        ohlcv[1].get<double>(),    // open
        ohlcv[2].get<double>(),    // high
        ohlcv[3].get<double>(),    // low
        ohlcv[4].get<double>(),    // close
        ohlcv[5].get<double>()     // volume
    });
}

json WazirX::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/account", "private", "GET", params);
    return parseBalance(response);
}

json WazirX::parseBalance(const json& response) {
    json result = {"info", response};
    json balances = response["balances"];
    
    for (const auto& balance : balances) {
        std::string currencyId = balance["asset"];
        std::string code = this->commonCurrencyCode(currencyId);
        
        result[code] = {
            {"free", this->safeFloat(balance, "free")},
            {"used", this->safeFloat(balance, "locked")},
            {"total", this->safeFloat(balance, "free") + this->safeFloat(balance, "locked")}
        };
    }
    
    return result;
}

json WazirX::createOrder(const std::string& symbol, const std::string& type,
                        const std::string& side, double amount,
                        double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"symbol", market.id},
        {"side", side.upper()},
        {"type", type.upper()},
        {"quantity", this->amountToPrecision(symbol, amount)}
    };
    
    if (type == "limit") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    json response = fetch("/order", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response, market);
}

std::string WazirX::sign(const std::string& path, const std::string& api,
                    const std::string& method, const json& params,
                    const std::map<std::string, std::string>& headers,
                    const json& body) {
    std::string url = this->urls["api"][api] + path;
    std::string timestamp = std::to_string(this->milliseconds());
    
    if (api == "public") {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    } else {
        this->checkRequiredCredentials();
        
        json request = this->extend({
            "timestamp": timestamp,
            "recvWindow": this->options["recvWindow"]
        }, params);
        
        std::string signature = this->createSignature(timestamp, method, path,
                                               request.dump());
        request["signature"] = signature;
        
        if (method == "GET") {
            url += "?" + this->urlencode(request);
        } else {
            body = request;
            const_cast<std::map<std::string, std::string>&>(headers)["Content-Type"] = "application/json";
        }
        
        const_cast<std::map<std::string, std::string>&>(headers)["X-API-KEY"] = this->config_.apiKey;
    }
    
    return url;
}

std::string WazirX::createSignature(const std::string& timestamp, const std::string& method,
                              const std::string& path, const std::string& body) {
    std::string message = timestamp + method + path + body;
    return this->hmac(message, this->encode(this->config_.secret),
                     "sha256", "hex");
}

json WazirX::parseOrder(const json& order, const Market& market) {
    std::string id = this->safeString(order, "orderId");
    std::string timestamp = this->safeString(order, "time");
    std::string status = this->parseOrderStatus(this->safeString(order, "status"));
    std::string symbol = market.symbol;
    std::string type = this->safeStringLower(order, "type");
    std::string side = this->safeStringLower(order, "side");
    
    return {
        {"id", id},
        {"clientOrderId", this->safeString(order, "clientOrderId")},
        {"timestamp", this->safeInteger(order, "time")},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"status", status},
        {"symbol", symbol},
        {"type", type},
        {"side", side},
        {"price", this->safeFloat(order, "price")},
        {"amount", this->safeFloat(order, "origQty")},
        {"filled", this->safeFloat(order, "executedQty")},
        {"remaining", this->safeFloat(order, "origQty") - this->safeFloat(order, "executedQty")},
        {"cost", this->safeFloat(order, "cummulativeQuoteQty")},
        {"trades", nullptr},
        {"fee", {
            {"currency", market.quote},
            {"cost", this->safeFloat(order, "commission")},
            {"rate", this->safeFloat(order, "commissionRate")}
        }},
        {"info", order}
    };
}

json WazirX::parseOrderStatus(const std::string& status) {
    static const std::map<std::string, std::string> statuses = {
        {"NEW", "open"},
        {"PARTIALLY_FILLED", "open"},
        {"FILLED", "closed"},
        {"CANCELED", "canceled"},
        {"PENDING_CANCEL", "canceling"},
        {"REJECTED", "rejected"},
        {"EXPIRED", "expired"}
    };
    
    return statuses.contains(status) ? statuses.at(status) : status;
}

} // namespace ccxt
