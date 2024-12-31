#include "mexc.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Mexc::Mexc() {
    id = "mexc";
    name = "MEXC Global";
    version = "v3";
    rateLimit = 50;
    testnet = false;

    // Initialize API endpoints
    baseUrl = testnet ? "https://api.testnet.mexc.com" : "https://api.mexc.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/137283979-8b2a818d-8633-461b-bfca-de89e8c446b2.jpg"},
        {"api", {
            {"public", baseUrl},
            {"private", baseUrl},
            {"spot", baseUrl + "/api/v3"},
            {"futures", baseUrl + "/api/futures/v1"}
        }},
        {"www", "https://www.mexc.com"},
        {"doc", {
            "https://mxcdevelop.github.io/APIDoc/",
            "https://mxcdevelop.github.io/apidocs/spot_v3_en/"
        }},
        {"fees", "https://www.mexc.com/fee"}
    };

    timeframes = {
        {"1m", "1m"},
        {"3m", "3m"},
        {"5m", "5m"},
        {"15m", "15m"},
        {"30m", "30m"},
        {"1h", "1h"},
        {"2h", "2h"},
        {"4h", "4h"},
        {"6h", "6h"},
        {"8h", "8h"},
        {"12h", "12h"},
        {"1d", "1d"},
        {"3d", "3d"},
        {"1w", "1w"},
        {"1M", "1M"}
    };

    options = {
        {"defaultType", "spot"},
        {"adjustForTimeDifference", true},
        {"recvWindow", "5000"}
    };

    initializeApiEndpoints();
}

void Mexc::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "api/v3/exchangeInfo",
                "api/v3/ticker/24hr",
                "api/v3/ticker/price",
                "api/v3/ticker/bookTicker",
                "api/v3/depth",
                "api/v3/trades",
                "api/v3/historicalTrades",
                "api/v3/klines",
                "api/v3/avgPrice",
                "api/v3/time"
            }}
        }},
        {"private", {
            {"GET", {
                "api/v3/account",
                "api/v3/openOrders",
                "api/v3/allOrders",
                "api/v3/myTrades",
                "api/v3/order",
                "api/v3/capital/config/getall",
                "api/v3/capital/deposit/hisrec",
                "api/v3/capital/withdraw/history",
                "api/v3/capital/deposit/address",
                "api/v3/capital/transfer"
            }},
            {"POST", {
                "api/v3/order",
                "api/v3/order/test",
                "api/v3/capital/withdraw/apply",
                "api/v3/capital/transfer"
            }},
            {"DELETE", {
                "api/v3/order",
                "api/v3/openOrders"
            }}
        }},
        {"futures", {
            {"GET", {
                "api/futures/v1/account/positions",
                "api/futures/v1/account/risk",
                "api/futures/v1/funding/rate",
                "api/futures/v1/funding/history"
            }},
            {"POST", {
                "api/futures/v1/position/leverage",
                "api/futures/v1/position/margin"
            }}
        }}
    };
}

json Mexc::fetchMarkets(const json& params) {
    json response = fetch("/api/v3/exchangeInfo", "public", "GET", params);
    json markets = json::array();
    
    for (const auto& market : response["symbols"]) {
        std::string id = market["symbol"];
        std::string baseId = market["baseAsset"];
        std::string quoteId = market["quoteAsset"];
        std::string base = this->commonCurrencyCode(baseId);
        std::string quote = this->commonCurrencyCode(quoteId);
        bool active = market["status"] == "TRADING";
        
        markets.push_back({
            {"id", id},
            {"symbol", base + "/" + quote},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", active},
            {"type", "spot"},
            {"spot", true},
            {"future", false},
            {"swap", false},
            {"option", false},
            {"contract", false},
            {"linear", false},
            {"inverse", false},
            {"precision", {
                {"amount", this->precisionFromstd::string(market["baseAssetPrecision"])},
                {"price", this->precisionFromstd::string(market["quotePrecision"])}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeFloat(market, "minQty")},
                    {"max", this->safeFloat(market, "maxQty")}
                }},
                {"price", {
                    {"min", this->safeFloat(market, "minPrice")},
                    {"max", this->safeFloat(market, "maxPrice")}
                }},
                {"cost", {
                    {"min", this->safeFloat(market, "minNotional")},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return markets;
}

json Mexc::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/api/v3/account", "private", "GET", params);
    json balances = response["balances"];
    json result = {"info", response};
    
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

json Mexc::createOrder(const std::string& symbol, const std::string& type,
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
        request["timeInForce"] = this->safeString(params, "timeInForce", "GTC");
    }
    
    json response = fetch("/api/v3/order", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response, market);
}

std::string Mexc::sign(const std::string& path, const std::string& api,
                  const std::string& method, const json& params,
                  const std::map<std::string, std::string>& headers,
                  const json& body) {
    std::string url = this->urls["api"][api] + "/" + path;
    std::string timestamp = std::to_string(this->milliseconds());
    
    if (api == "public") {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    } else {
        this->checkRequiredCredentials();
        
        std::string query = "timestamp=" + timestamp;
        
        if (!params.empty()) {
            query += "&" + this->urlencode(this->keysort(params));
        }
        
        std::string signature = this->hmac(query, this->config_.secret, "sha256", "hex");
        url += "?" + query + "&signature=" + signature;
        
        const_cast<std::map<std::string, std::string>&>(headers)["X-MEXC-APIKEY"] = this->config_.apiKey;
        
        if (method == "POST") {
            const_cast<std::map<std::string, std::string>&>(headers)["Content-Type"] = "application/json";
            if (!params.empty()) {
                body = this->json(params);
            }
        }
    }
    
    return url;
}

json Mexc::parseOrder(const json& order, const Market& market) {
    std::string id = this->safeString(order, "orderId");
    std::string timestamp = this->safeInteger(order, "time");
    std::string status = this->parseOrderStatus(this->safeString(order, "status"));
    std::string symbol = market.symbol;
    std::string type = this->safeStringLower(order, "type");
    std::string side = this->safeStringLower(order, "side");
    
    return {
        {"id", id},
        {"clientOrderId", this->safeString(order, "clientOrderId")},
        {"timestamp", timestamp},
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
        {"fee", nullptr},
        {"info", order}
    };
}

json Mexc::parseOrderStatus(const std::string& status) {
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

json Mexc::fetchTicker(const std::string& symbol, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {{"symbol", market.id}};
    json response = fetch("/api/v3/ticker/24hr", "public", "GET", this->extend(request, params));
    return this->parseTicker(response, market);
}

json Mexc::fetchTickers(const std::vector<std::string>& symbols, const json& params) {
    this->loadMarkets();
    json response = fetch("/api/v3/ticker/24hr", "public", "GET", params);
    json result = json::object();
    
    for (const auto& ticker : response) {
        std::string marketId = this->safeString(ticker, "symbol");
        if (this->markets_by_id.contains(marketId)) {
            Market market = this->markets_by_id[marketId];
            std::string symbol = market.symbol;
            if (symbols.empty() || std::find(symbols.begin(), symbols.end(), symbol) != symbols.end()) {
                result[symbol] = this->parseTicker(ticker, market);
            }
        }
    }
    
    return result;
}

json Mexc::fetchOrderBook(const std::string& symbol, int limit, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {{"symbol", market.id}};
    
    if (limit) {
        request["limit"] = limit;
    }
    
    json response = fetch("/api/v3/depth", "public", "GET", this->extend(request, params));
    return this->parseOrderBook(response, symbol);
}

json Mexc::fetchTrades(const std::string& symbol, int since, int limit, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {{"symbol", market.id}};
    
    if (limit) {
        request["limit"] = limit;
    }
    
    json response = fetch("/api/v3/trades", "public", "GET", this->extend(request, params));
    return this->parseTrades(response, market, since, limit);
}

json Mexc::fetchOHLCV(const std::string& symbol, const std::string& timeframe,
                     int since, int limit, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {
        {"symbol", market.id},
        {"interval", this->timeframes[timeframe]}
    };
    
    if (since) {
        request["startTime"] = since;
    }
    
    if (limit) {
        request["limit"] = limit;
    }
    
    json response = fetch("/api/v3/klines", "public", "GET", this->extend(request, params));
    return this->parseOHLCVs(response, market, timeframe, since, limit);
}

json Mexc::cancelOrder(const std::string& id, const std::string& symbol, const json& params) {
    if (symbol.empty()) {
        throw ArgumentsRequired("cancelOrder requires a symbol argument");
    }
    
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {
        {"symbol", market.id},
        {"orderId", id}
    };
    
    return fetch("/api/v3/order", "private", "DELETE", this->extend(request, params));
}

json Mexc::fetchOrder(const std::string& id, const std::string& symbol, const json& params) {
    if (symbol.empty()) {
        throw ArgumentsRequired("fetchOrder requires a symbol argument");
    }
    
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {
        {"symbol", market.id},
        {"orderId", id}
    };
    
    json response = fetch("/api/v3/order", "private", "GET", this->extend(request, params));
    return this->parseOrder(response, market);
}

json Mexc::fetchOrders(const std::string& symbol, int since, int limit, const json& params) {
    if (symbol.empty()) {
        throw ArgumentsRequired("fetchOrders requires a symbol argument");
    }
    
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {{"symbol", market.id}};
    
    if (since) {
        request["startTime"] = since;
    }
    
    if (limit) {
        request["limit"] = limit;
    }
    
    json response = fetch("/api/v3/allOrders", "private", "GET", this->extend(request, params));
    return this->parseOrders(response, market, since, limit);
}

json Mexc::fetchOpenOrders(const std::string& symbol, int since, int limit, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {{"symbol", market.id}};
    
    if (since) {
        request["startTime"] = since;
    }
    
    if (limit) {
        request["limit"] = limit;
    }
    
    json response = fetch("/api/v3/openOrders", "private", "GET", this->extend(request, params));
    return this->parseOrders(response, market, since, limit);
}

json Mexc::fetchClosedOrders(const std::string& symbol, int since, int limit, const json& params) {
    json orders = this->fetchOrders(symbol, since, limit, params);
    return this->filterBy(orders, "status", "closed");
}

json Mexc::fetchPositions(const std::string& symbols, const json& params) {
    this->loadMarkets();
    json response = fetch("/api/futures/v1/account/positions", "futures", "GET", params);
    return this->parsePositions(response);
}

json Mexc::fetchPositionRisk(const std::string& symbols, const json& params) {
    this->loadMarkets();
    json response = fetch("/api/futures/v1/account/risk", "futures", "GET", params);
    return this->parsePositionRisk(response);
}

json Mexc::setLeverage(int leverage, const std::string& symbol, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {
        {"symbol", market.id},
        {"leverage", leverage}
    };
    
    return fetch("/api/futures/v1/position/leverage", "futures", "POST", this->extend(request, params));
}

json Mexc::setMarginMode(const std::string& marginMode, const std::string& symbol, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {
        {"symbol", market.id},
        {"marginMode", marginMode}
    };
    
    return fetch("/api/futures/v1/position/margin", "futures", "POST", this->extend(request, params));
}

json Mexc::fetchFundingRate(const std::string& symbol, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {{"symbol", market.id}};
    
    json response = fetch("/api/futures/v1/funding/rate", "futures", "GET", this->extend(request, params));
    return this->parseFundingRate(response);
}

json Mexc::fetchFundingRates(const std::vector<std::string>& symbols, const json& params) {
    this->loadMarkets();
    json response = fetch("/api/futures/v1/funding/rate", "futures", "GET", params);
    return this->parseFundingRates(response);
}

json Mexc::fetchFundingHistory(const std::string& symbol, int since, int limit, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {{"symbol", market.id}};
    
    if (since) {
        request["startTime"] = since;
    }
    
    if (limit) {
        request["limit"] = limit;
    }
    
    json response = fetch("/api/futures/v1/funding/history", "futures", "GET", this->extend(request, params));
    return this->parseFundingHistory(response);
}

std::string Mexc::getTimestamp() {
    return std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

std::string Mexc::createSignature(const std::string& timestamp, const std::string& method,
                           const std::string& path, const std::string& body) {
    std::string message = timestamp + method + path;
    if (!body.empty()) {
        message += body;
    }
    
    unsigned char* digest = HMAC(EVP_sha256(), this->config_.secret.c_str(), this->config_.secret.length(),
                                (unsigned char*)message.c_str(), message.length(), NULL, NULL);
    
    std::stringstream ss;
    for(int i = 0; i < 32; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
    }
    return ss.str();
}

std::string Mexc::getMexcSymbol(const std::string& symbol) {
    return symbol.find('/') != std::string::npos ? symbol.substr(0, symbol.find('/')) + symbol.substr(symbol.find('/') + 1) : symbol;
}

std::string Mexc::getCommonSymbol(const std::string& mexcSymbol) {
    return mexcSymbol.substr(0, 3) + "/" + mexcSymbol.substr(3);
}

json Mexc::parsePosition(const json& position, const Market& market) {
    std::string symbol = this->safeString(position, "symbol");
    std::string timestamp = this->safeInteger(position, "timestamp");
    
    return {
        {"info", position},
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"initialMargin", this->safeFloat(position, "initialMargin")},
        {"initialMarginPercentage", this->safeFloat(position, "initialMarginPercentage")},
        {"maintenanceMargin", this->safeFloat(position, "maintenanceMargin")},
        {"maintenanceMarginPercentage", this->safeFloat(position, "maintenanceMarginPercentage")},
        {"entryPrice", this->safeFloat(position, "entryPrice")},
        {"notional", this->safeFloat(position, "notional")},
        {"leverage", this->safeFloat(position, "leverage")},
        {"unrealizedPnl", this->safeFloat(position, "unrealizedPnl")},
        {"contracts", this->safeFloat(position, "contracts")},
        {"contractSize", this->safeFloat(position, "contractSize")},
        {"marginRatio", this->safeFloat(position, "marginRatio")},
        {"liquidationPrice", this->safeFloat(position, "liquidationPrice")},
        {"markPrice", this->safeFloat(position, "markPrice")},
        {"collateral", this->safeFloat(position, "collateral")},
        {"marginType", this->safeString(position, "marginType")},
        {"side", this->safeString(position, "side")},
        {"percentage", this->safeFloat(position, "percentage")}
    };
}

json Mexc::parseLedgerEntryType(const std::string& type) {
    static const std::map<std::string, std::string> types = {
        {"deposit", "transaction"},
        {"withdrawal", "transaction"},
        {"trade", "trade"},
        {"fee", "fee"}
    };
    
    return types.contains(type) ? types.at(type) : type;
}

} // namespace ccxt
