#include "ccxt/exchanges/kucoinfutures.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <base64.h>

namespace ccxt {

KuCoinFutures::KuCoinFutures(const ExchangeConfig& config) : Exchange(config) {
    id = "kucoinfutures";
    name = "KuCoin Futures";
    countries = {"SC"}; // Seychelles
    rateLimit = 75;
    version = "v1";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/51840849/87295558-132aaf80-c50e-11ea-9801-a2fb0c57c799.jpg"},
        {"api", {
            {"public", "https://api-futures.kucoin.com"},
            {"private", "https://api-futures.kucoin.com"},
            {"contract", "https://api-futures.kucoin.com"}
        }},
        {"www", "https://futures.kucoin.com"},
        {"doc", {
            "https://docs.kucoin.com/futures",
            "https://docs.kucoin.com"
        }},
        {"fees", "https://futures.kucoin.com/contract/detail"}
    };

    timeframes = {
        {"1m", "1min"},
        {"3m", "3min"},
        {"5m", "5min"},
        {"15m", "15min"},
        {"30m", "30min"},
        {"1h", "1hour"},
        {"2h", "2hour"},
        {"4h", "4hour"},
        {"6h", "6hour"},
        {"8h", "8hour"},
        {"12h", "12hour"},
        {"1d", "1day"},
        {"1w", "1week"}
    };

    initTokens();
}

// Synchronous Methods Implementation

std::vector<Market> KuCoinFutures::fetchMarkets(const Params& params) {
    json response = request("/api/v1/contracts/active", "public", "GET", params);
    std::vector<Market> markets;
    
    for (const auto& market : response["data"]) {
        markets.push_back(parseMarket(market));
    }
    
    return markets;
}

OrderBook KuCoinFutures::fetchOrderBook(const std::string& symbol, int limit, const Params& params) {
    validateSymbol(symbol);
    
    Params requestParams = params;
    if (limit > 0) {
        requestParams["limit"] = std::to_string(limit);
    }
    
    std::string endpoint = "/api/v1/level2/snapshot/" + getKuCoinFuturesSymbol(symbol);
    json response = request(endpoint, "public", "GET", requestParams);
    
    return parseOrderBook(response["data"], symbol);
}

Ticker KuCoinFutures::fetchTicker(const std::string& symbol, const Params& params) {
    validateSymbol(symbol);
    
    std::string endpoint = "/api/v1/ticker/" + getKuCoinFuturesSymbol(symbol);
    json response = request(endpoint, "public", "GET", params);
    
    return parseTicker(response["data"], nullptr);
}

std::vector<Trade> KuCoinFutures::fetchTrades(const std::string& symbol, int since, int limit, const Params& params) {
    validateSymbol(symbol);
    
    Params requestParams = params;
    if (since > 0) {
        requestParams["startAt"] = std::to_string(since);
    }
    if (limit > 0) {
        requestParams["pageSize"] = std::to_string(limit);
    }
    
    std::string endpoint = "/api/v1/trade/history/" + getKuCoinFuturesSymbol(symbol);
    json response = request(endpoint, "public", "GET", requestParams);
    
    std::vector<Trade> trades;
    for (const auto& trade : response["data"]) {
        trades.push_back(parseTrade(trade, nullptr));
    }
    
    return trades;
}

FundingRate KuCoinFutures::fetchFundingRate(const std::string& symbol, const Params& params) {
    validateSymbol(symbol);
    
    std::string endpoint = "/api/v1/funding-rate/" + getKuCoinFuturesSymbol(symbol) + "/current";
    json response = request(endpoint, "public", "GET", params);
    
    return parseFundingRate(response["data"], nullptr);
}

std::map<std::string, FundingRate> KuCoinFutures::fetchFundingRates(const std::vector<std::string>& symbols, const Params& params) {
    std::map<std::string, FundingRate> result;
    
    for (const auto& symbol : symbols) {
        result[symbol] = fetchFundingRate(symbol, params);
    }
    
    return result;
}

Order KuCoinFutures::createOrder(const std::string& symbol, const std::string& type, const std::string& side,
                                double amount, double price, const Params& params) {
    validateSymbol(symbol);
    validateOrder(symbol, type, side, amount, price);
    
    Params requestParams = params;
    requestParams["symbol"] = getKuCoinFuturesSymbol(symbol);
    requestParams["side"] = side;
    requestParams["type"] = type;
    requestParams["size"] = std::to_string(amount);
    
    if (type == "limit") {
        if (price <= 0) {
            throw InvalidOrder("For limit orders, price must be > 0");
        }
        requestParams["price"] = std::to_string(price);
    }
    
    // Add client order id
    requestParams["clientOid"] = getUuid();
    
    json response = request("/api/v1/orders", "private", "POST", requestParams);
    return parseOrder(response["data"], nullptr);
}

Order KuCoinFutures::cancelOrder(const std::string& id, const std::string& symbol, const Params& params) {
    validateSymbol(symbol);
    
    std::string endpoint = "/api/v1/orders/" + id;
    json response = request(endpoint, "private", "DELETE", params);
    
    return parseOrder(response["data"], nullptr);
}

std::vector<Order> KuCoinFutures::cancelAllOrders(const std::string& symbol, const Params& params) {
    Params requestParams = params;
    if (!symbol.empty()) {
        requestParams["symbol"] = getKuCoinFuturesSymbol(symbol);
    }
    
    json response = request("/api/v1/orders", "private", "DELETE", requestParams);
    
    std::vector<Order> orders;
    for (const auto& order : response["data"]) {
        orders.push_back(parseOrder(order, nullptr));
    }
    
    return orders;
}

Balance KuCoinFutures::fetchBalance(const Params& params) {
    json response = request("/api/v1/account-overview", "private", "GET", params);
    return parseBalance(response["data"]);
}

std::vector<Position> KuCoinFutures::fetchPositions(const std::vector<std::string>& symbols, const Params& params) {
    Params requestParams = params;
    
    json response = request("/api/v1/positions", "private", "GET", requestParams);
    
    std::vector<Position> positions;
    for (const auto& position : response["data"]) {
        positions.push_back(parsePosition(position, nullptr));
    }
    
    return positions;
}

MarginModification KuCoinFutures::setLeverage(int leverage, const std::string& symbol, const Params& params) {
    validateSymbol(symbol);
    
    Params requestParams = params;
    requestParams["symbol"] = getKuCoinFuturesSymbol(symbol);
    requestParams["leverage"] = std::to_string(leverage);
    
    json response = request("/api/v1/position/risk-limit-level/change", "private", "POST", requestParams);
    
    MarginModification result;
    result.leverage = leverage;
    result.symbol = symbol;
    result.status = "ok";
    
    return result;
}

// Asynchronous Methods Implementation

std::future<std::vector<Market>> KuCoinFutures::fetchMarketsAsync(const Params& params) {
    return std::async(std::launch::async, [this, params]() {
        return fetchMarkets(params);
    });
}

std::future<OrderBook> KuCoinFutures::fetchOrderBookAsync(const std::string& symbol, int limit, const Params& params) {
    return std::async(std::launch::async, [this, symbol, limit, params]() {
        return fetchOrderBook(symbol, limit, params);
    });
}

std::future<Ticker> KuCoinFutures::fetchTickerAsync(const std::string& symbol, const Params& params) {
    return std::async(std::launch::async, [this, symbol, params]() {
        return fetchTicker(symbol, params);
    });
}

std::future<std::vector<Trade>> KuCoinFutures::fetchTradesAsync(const std::string& symbol, int since, int limit, const Params& params) {
    return std::async(std::launch::async, [this, symbol, since, limit, params]() {
        return fetchTrades(symbol, since, limit, params);
    });
}

std::future<FundingRate> KuCoinFutures::fetchFundingRateAsync(const std::string& symbol, const Params& params) {
    return std::async(std::launch::async, [this, symbol, params]() {
        return fetchFundingRate(symbol, params);
    });
}

std::future<Order> KuCoinFutures::createOrderAsync(const std::string& symbol, const std::string& type, const std::string& side,
                                                  double amount, double price, const Params& params) {
    return std::async(std::launch::async, [this, symbol, type, side, amount, price, params]() {
        return createOrder(symbol, type, side, amount, price, params);
    });
}

std::future<Balance> KuCoinFutures::fetchBalanceAsync(const Params& params) {
    return std::async(std::launch::async, [this, params]() {
        return fetchBalance(params);
    });
}

// Helper Methods Implementation

std::string KuCoinFutures::sign(const std::string& path, const std::string& api, const std::string& method,
                               const Params& params, const std::string& body, const std::map<std::string, std::string>& headers) {
    std::string timestamp = getTimestamp();
    std::string signature = createSignature(timestamp, method, path, body);
    
    auto authHeaders = getAuthHeaders(method, path, body);
    for (const auto& [key, value] : authHeaders) {
        addHeader(key, value);
    }
    
    return path;
}

// Parsing Methods Implementation

Market KuCoinFutures::parseMarket(const json& market) {
    Market result;
    result.id = market["symbol"].get<std::string>();
    result.symbol = market["baseCurrency"].get<std::string>() + "/" + market["quoteCurrency"].get<std::string>();
    result.base = market["baseCurrency"].get<std::string>();
    result.quote = market["quoteCurrency"].get<std::string>();
    result.baseId = market["baseCurrency"].get<std::string>();
    result.quoteId = market["quoteCurrency"].get<std::string>();
    result.active = market["enableTrading"].get<bool>();
    result.contract = true;
    result.inverse = market["isInverse"].get<bool>();
    result.linear = !result.inverse;
    result.contractSize = std::stod(market["multiplier"].get<std::string>());
    
    result.precision.amount = std::stod(market["lotSize"].get<std::string>());
    result.precision.price = std::stod(market["tickSize"].get<std::string>());
    
    result.limits.amount.min = std::stod(market["minOrderQty"].get<std::string>());
    result.limits.amount.max = std::stod(market["maxOrderQty"].get<std::string>());
    result.limits.price.min = std::stod(market["tickSize"].get<std::string>());
    result.limits.price.max = std::stod(market["maxPrice"].get<std::string>());
    result.limits.cost.min = result.limits.amount.min * result.limits.price.min;
    
    return result;
}

Trade KuCoinFutures::parseTrade(const json& trade, const Market* market) {
    Trade result;
    result.id = trade["tradeId"].get<std::string>();
    result.timestamp = std::stoll(trade["ts"].get<std::string>());
    result.datetime = iso8601(result.timestamp);
    result.symbol = market ? market->symbol : trade["symbol"].get<std::string>();
    result.side = trade["side"].get<std::string>();
    result.price = std::stod(trade["price"].get<std::string>());
    result.amount = std::stod(trade["size"].get<std::string>());
    result.cost = result.price * result.amount;
    
    return result;
}

Order KuCoinFutures::parseOrder(const json& order, const Market* market) {
    Order result;
    result.id = order["orderId"].get<std::string>();
    result.clientOrderId = order["clientOid"].get<std::string>();
    result.timestamp = std::stoll(order["createdAt"].get<std::string>());
    result.datetime = iso8601(result.timestamp);
    result.symbol = market ? market->symbol : order["symbol"].get<std::string>();
    result.type = order["type"].get<std::string>();
    result.side = order["side"].get<std::string>();
    result.price = std::stod(order["price"].get<std::string>());
    result.amount = std::stod(order["size"].get<std::string>());
    result.filled = std::stod(order["dealSize"].get<std::string>());
    result.remaining = result.amount - result.filled;
    result.status = parseOrderStatus(order["status"].get<std::string>());
    
    return result;
}

Position KuCoinFutures::parsePosition(const json& position, const Market* market) {
    Position result;
    result.id = position["id"].get<std::string>();
    result.symbol = market ? market->symbol : position["symbol"].get<std::string>();
    result.timestamp = std::stoll(position["timestamp"].get<std::string>());
    result.datetime = iso8601(result.timestamp);
    result.side = position["side"].get<std::string>();
    result.contracts = std::stod(position["currentQty"].get<std::string>());
    result.contractSize = std::stod(position["multiplier"].get<std::string>());
    result.entryPrice = std::stod(position["avgEntryPrice"].get<std::string>());
    result.leverage = std::stoi(position["leverage"].get<std::string>());
    result.liquidationPrice = std::stod(position["liquidationPrice"].get<std::string>());
    result.margin = std::stod(position["maintMargin"].get<std::string>());
    result.notional = std::stod(position["positionValue"].get<std::string>());
    
    return result;
}

FundingRate KuCoinFutures::parseFundingRate(const json& fundingRate, const Market* market) {
    FundingRate result;
    result.symbol = market ? market->symbol : fundingRate["symbol"].get<std::string>();
    result.timestamp = std::stoll(fundingRate["timestamp"].get<std::string>());
    result.datetime = iso8601(result.timestamp);
    result.rate = std::stod(fundingRate["value"].get<std::string>());
    result.next = std::stoll(fundingRate["predictedValue"].get<std::string>());
    
    return result;
}

Balance KuCoinFutures::parseBalance(const json& balance) {
    Balance result;
    result.free = std::stod(balance["availableBalance"].get<std::string>());
    result.used = std::stod(balance["frozenBalance"].get<std::string>());
    result.total = result.free + result.used;
    
    return result;
}

std::string KuCoinFutures::parseOrderStatus(const std::string& status) const {
    static const std::map<std::string, std::string> statuses = {
        {"open", "open"},
        {"done", "closed"},
        {"match", "closed"},
        {"cancel", "canceled"}
    };
    
    auto it = statuses.find(status);
    return it != statuses.end() ? it->second : status;
}

void KuCoinFutures::handleErrors(const json& response) {
    if (!response.contains("code")) {
        return;
    }
    
    auto code = response["code"].get<std::string>();
    auto message = response.contains("msg") ? response["msg"].get<std::string>() : "";
    
    if (code == "200000") {
        return;
    }
    
    if (code == "400100") {
        throw AuthenticationError(message);
    } else if (code == "400200") {
        throw InvalidOrder(message);
    } else if (code == "400500") {
        throw InvalidOrder(message);
    } else if (code == "400600") {
        throw InvalidOrder(message);
    } else if (code == "400700") {
        throw InvalidOrder(message);
    } else if (code == "400800") {
        throw InsufficientFunds(message);
    } else if (code == "411100") {
        throw OrderNotFound(message);
    } else {
        throw ExchangeError(message);
    }
}

} // namespace ccxt
