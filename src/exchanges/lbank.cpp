#include "lbank.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Lbank::Lbank() {
    id = "lbank";
    name = "LBank";
    version = "v2";
    rateLimit = 1000;
    certified = true;
    pro = false;
    hasPublicAPI = true;
    hasPrivateAPI = true;

    // Initialize API endpoints
    baseUrl = "https://api.lbank.info";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/38063602-9605e28a-3302-11e8-81be-64b1e53c4cfb.jpg"},
        {"api", {
            {"public", "https://api.lbank.info/v2"},
            {"private", "https://api.lbank.info/v2"}
        }},
        {"www", "https://www.lbank.info"},
        {"doc", {
            "https://www.lbank.info/en-US/docs/index.html",
            "https://github.com/LBank-exchange/lbank-official-api-docs"
        }},
        {"fees", "https://www.lbank.info/fees.html"}
    };

    timeframes = {
        {"1m", "minute1"},
        {"5m", "minute5"},
        {"15m", "minute15"},
        {"30m", "minute30"},
        {"1h", "hour1"},
        {"2h", "hour2"},
        {"4h", "hour4"},
        {"6h", "hour6"},
        {"8h", "hour8"},
        {"12h", "hour12"},
        {"1d", "day1"},
        {"1w", "week1"},
        {"1M", "month1"}
    };

    options = {
        {"adjustForTimeDifference", true},
        {"recvWindow", 5000},
        {"timeDifference", 0},
        {"defaultType", "spot"}
    };

    errorCodes = {
        {10000, "Internal error"},
        {10001, "The required parameters can not be empty"},
        {10002, "Validation failed"},
        {10003, "Invalid parameter"},
        {10004, "Request too frequent"},
        {10005, "Secret key does not exist"},
        {10006, "User does not exist"},
        {10007, "Invalid signature"},
        {10008, "Invalid Trading Pair"},
        {10009, "Price and/or Amount are required for limit order"},
        {10010, "Price and/or Amount must be more than 0"},
        {10011, "Market order amount is too large"},
        {10012, "PRICE_PRECISION error"},
        {10013, "AMOUNT_PRECISION error"},
        {10014, "AMOUNT_MIN_ERROR"},
        {10015, "AMOUNT_MAX_ERROR"},
        {10016, "PRICE_MIN_ERROR"},
        {10017, "PRICE_MAX_ERROR"}
    };

    initializeApiEndpoints();
}

void Lbank::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "currencyPairs",
                "ticker/24hr",
                "depth",
                "trades",
                "kline",
                "accuracy",
                "usdToCny",
                "timestamp"
            }}
        }},
        {"private", {
            {"POST", {
                "user_info",
                "create_order",
                "cancel_order",
                "orders_info",
                "orders_info_history",
                "order_transaction_detail",
                "transaction_history",
                "withdraw",
                "withdrawCancel",
                "withdraws",
                "withdrawConfigs",
                "user_trades",
                "deposit_history",
                "withdraw_history",
                "deposit_address"
            }}
        }}
    };
}

json Lbank::fetchMarkets(const json& params) {
    json response = fetch("/currencyPairs", "public", "GET", params);
    json result = json::array();
    
    for (const auto& pair : response) {
        String id = pair.get<String>();
        std::vector<String> parts;
        std::stringstream ss(id);
        String part;
        while (std::getline(ss, part, '_')) {
            parts.push_back(part);
        }
        
        String baseId = parts[0];
        String quoteId = parts[1];
        String base = this->safeCurrencyCode(baseId);
        String quote = this->safeCurrencyCode(quoteId);
        String symbol = base + "/" + quote;
        
        json accuracy = fetch("/accuracy", "public", "GET", {{"symbol", id}});
        
        result.push_back({
            {"id", id},
            {"symbol", symbol},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", true},
            {"type", "spot"},
            {"spot", true},
            {"margin", false},
            {"future", false},
            {"option", false},
            {"contract", false},
            {"precision", {
                {"amount", accuracy["quantityAccuracy"]},
                {"price", accuracy["priceAccuracy"]}
            }},
            {"limits", {
                {"amount", {
                    {"min", accuracy["minTranQua"]},
                    {"max", nullptr}
                }},
                {"price", {
                    {"min", nullptr},
                    {"max", nullptr}
                }},
                {"cost", {
                    {"min", accuracy["minTranQua"]},
                    {"max", nullptr}
                }}
            }},
            {"info", accuracy}
        });
    }
    
    return result;
}

json Lbank::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/user_info", "private", "POST", params);
    return parseBalance(response["info"]["funds"]);
}

json Lbank::parseBalance(const json& response) {
    json result = {{"info", response}};
    json free = response["free"];
    json freezed = response["freezed"];
    
    for (const auto& [currency, balance] : free.items()) {
        String code = this->safeCurrencyCode(currency);
        String account = {
            {"free", this->safeFloat(free, currency)},
            {"used", this->safeFloat(freezed, currency)},
            {"total", this->safeFloat(free, currency) + this->safeFloat(freezed, currency)}
        };
        result[code] = account;
    }
    
    return result;
}

json Lbank::createOrder(const String& symbol, const String& type,
                       const String& side, double amount,
                       double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"symbol", market["id"]},
        {"type", type},
        {"side", side},
        {"amount", this->amountToPrecision(symbol, amount)}
    };
    
    if (type == "limit") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    json response = fetch("/create_order", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response, market);
}

String Lbank::sign(const String& path, const String& api,
                   const String& method, const json& params,
                   const std::map<String, String>& headers,
                   const json& body) {
    String url = this->urls["api"][api];
    url += "/" + this->implodeParams(path, params);
    
    if (api == "private") {
        this->checkRequiredCredentials();
        String timestamp = std::to_string(this->milliseconds());
        String query = this->urlencode(this->keysort(params));
        String auth = query + "&secret_key=" + this->config_.secret;
        String signature = this->hash(this->encode(auth), "md5");
        
        if (method == "GET") {
            if (!params.empty()) {
                url += "?" + query;
            }
        } else {
            body = this->extend(params, {
                {"api_key", this->config_.apiKey},
                {"sign", signature},
                {"timestamp", timestamp}
            });
            const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/x-www-form-urlencoded";
        }
    } else if (!params.empty()) {
        url += "?" + this->urlencode(params);
    }
    
    return url;
}

String Lbank::getNonce() {
    return std::to_string(this->milliseconds());
}

json Lbank::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "order_id");
    String timestamp = this->safeString(order, "create_time");
    String status = this->parseOrderStatus(this->safeString(order, "status"));
    String symbol = nullptr;
    
    if (!market.empty()) {
        symbol = market["symbol"];
    }
    
    String type = this->safeString(order, "type");
    String side = this->safeString(order, "side");
    
    return {
        {"id", id},
        {"clientOrderId", nullptr},
        {"datetime", this->iso8601(timestamp)},
        {"timestamp", this->parse8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"type", type},
        {"timeInForce", nullptr},
        {"postOnly", nullptr},
        {"status", status},
        {"symbol", symbol},
        {"side", side},
        {"price", this->safeFloat(order, "price")},
        {"stopPrice", nullptr},
        {"cost", nullptr},
        {"amount", this->safeFloat(order, "amount")},
        {"filled", this->safeFloat(order, "deal_amount")},
        {"remaining", this->safeFloat(order, "remain_amount")},
        {"trades", nullptr},
        {"fee", {
            {"currency", market["quote"]},
            {"cost", this->safeFloat(order, "deal_fee")},
            {"rate", nullptr}
        }},
        {"info", order}
    };
}

String Lbank::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"0", "open"},
        {"1", "closed"},
        {"2", "canceled"},
        {"3", "open"},
        {"4", "canceled"}
    };
    
    return this->safeString(statuses, status, status);
}

// Market Data API Implementation
json Lbank::fetchTicker(const String& symbol, const json& params) {
    auto market = loadMarket(symbol);
    auto request = {
        {"symbol", market["id"]}
    };
    auto response = fetch("/ticker/24hr", "public", "GET", extend(request, params));
    return parseTicker(response, market);
}

json Lbank::fetchTickers(const std::vector<String>& symbols, const json& params) {
    loadMarkets();
    auto response = fetch("/ticker/24hr", "public", "GET", params);
    auto tickers = json::array();
    for (const auto& ticker : response) {
        if (!symbols.empty()) {
            auto marketId = ticker["symbol"].get<String>();
            if (marketIds.find(marketId) != marketIds.end()) {
                auto market = markets_by_id[marketId];
                if (std::find(symbols.begin(), symbols.end(), market["symbol"]) != symbols.end()) {
                    tickers.push_back(parseTicker(ticker, market));
                }
            }
        } else {
            tickers.push_back(parseTicker(ticker));
        }
    }
    return tickers;
}

json Lbank::fetchOrderBook(const String& symbol, int limit, const json& params) {
    auto market = loadMarket(symbol);
    auto request = {
        {"symbol", market["id"]},
        {"size", limit ? limit : 60}
    };
    auto response = fetch("/depth", "public", "GET", extend(request, params));
    return parseOrderBook(response, market["symbol"], undefined, "bids", "asks", 0, 1);
}

json Lbank::fetchTrades(const String& symbol, int since, int limit, const json& params) {
    auto market = loadMarket(symbol);
    auto request = {
        {"symbol", market["id"]},
        {"size", limit ? limit : 100}
    };
    auto response = fetch("/trades", "public", "GET", extend(request, params));
    return parseTrades(response, market, since, limit);
}

json Lbank::fetchOHLCV(const String& symbol, const String& timeframe, int since, int limit, const json& params) {
    auto market = loadMarket(symbol);
    auto request = {
        {"symbol", market["id"]},
        {"type", timeframes[timeframe]},
        {"size", limit ? limit : 100}
    };
    if (since) {
        request["time"] = since;
    }
    auto response = fetch("/kline", "public", "GET", extend(request, params));
    return parseOHLCVs(response, market, timeframe, since, limit);
}

// Async Market Data API Implementation
AsyncPullType Lbank::asyncFetchMarkets(const json& params) {
    return std::async(std::launch::async, [this, params]() {
        return fetchMarkets(params);
    });
}

AsyncPullType Lbank::asyncFetchTicker(const String& symbol, const json& params) {
    return std::async(std::launch::async, [this, symbol, params]() {
        return fetchTicker(symbol, params);
    });
}

AsyncPullType Lbank::asyncFetchTickers(const std::vector<String>& symbols, const json& params) {
    return std::async(std::launch::async, [this, symbols, params]() {
        return fetchTickers(symbols, params);
    });
}

AsyncPullType Lbank::asyncFetchOrderBook(const String& symbol, int limit, const json& params) {
    return std::async(std::launch::async, [this, symbol, limit, params]() {
        return fetchOrderBook(symbol, limit, params);
    });
}

AsyncPullType Lbank::asyncFetchTrades(const String& symbol, int since, int limit, const json& params) {
    return std::async(std::launch::async, [this, symbol, since, limit, params]() {
        return fetchTrades(symbol, since, limit, params);
    });
}

AsyncPullType Lbank::asyncFetchOHLCV(const String& symbol, const String& timeframe, int since, int limit, const json& params) {
    return std::async(std::launch::async, [this, symbol, timeframe, since, limit, params]() {
        return fetchOHLCV(symbol, timeframe, since, limit, params);
    });
}

// Trading API Implementation
json Lbank::cancelOrder(const String& id, const String& symbol, const json& params) {
    if (symbol.empty()) {
        throw ArgumentsRequired("cancelOrder() requires a symbol argument");
    }
    auto market = loadMarket(symbol);
    auto request = {
        {"symbol", market["id"]},
        {"orderId", id}
    };
    return fetch("/cancel_order", "private", "POST", extend(request, params));
}

json Lbank::fetchOrder(const String& id, const String& symbol, const json& params) {
    if (symbol.empty()) {
        throw ArgumentsRequired("fetchOrder() requires a symbol argument");
    }
    auto market = loadMarket(symbol);
    auto request = {
        {"symbol", market["id"]},
        {"orderId", id}
    };
    auto response = fetch("/orders_info", "private", "POST", extend(request, params));
    return parseOrder(response, market);
}

json Lbank::fetchOrders(const String& symbol, int since, int limit, const json& params) {
    if (symbol.empty()) {
        throw ArgumentsRequired("fetchOrders() requires a symbol argument");
    }
    auto market = loadMarket(symbol);
    auto request = {
        {"symbol", market["id"]}
    };
    if (since) {
        request["startTime"] = since;
    }
    if (limit) {
        request["size"] = limit;
    }
    auto response = fetch("/orders_info_history", "private", "POST", extend(request, params));
    return parseOrders(response, market, since, limit);
}

json Lbank::fetchOpenOrders(const String& symbol, int since, int limit, const json& params) {
    if (symbol.empty()) {
        throw ArgumentsRequired("fetchOpenOrders() requires a symbol argument");
    }
    auto market = loadMarket(symbol);
    auto request = {
        {"symbol", market["id"]}
    };
    if (since) {
        request["startTime"] = since;
    }
    if (limit) {
        request["size"] = limit;
    }
    auto response = fetch("/orders_info_no_deal", "private", "POST", extend(request, params));
    return parseOrders(response, market, since, limit);
}

json Lbank::fetchClosedOrders(const String& symbol, int since, int limit, const json& params) {
    if (symbol.empty()) {
        throw ArgumentsRequired("fetchClosedOrders() requires a symbol argument");
    }
    auto market = loadMarket(symbol);
    auto request = {
        {"symbol", market["id"]}
    };
    if (since) {
        request["startTime"] = since;
    }
    if (limit) {
        request["size"] = limit;
    }
    auto response = fetch("/orders_info_history", "private", "POST", extend(request, params));
    return parseOrders(response, market, since, limit);
}

// Async Trading API Implementation
AsyncPullType Lbank::asyncFetchBalance(const json& params) {
    return std::async(std::launch::async, [this, params]() {
        return fetchBalance(params);
    });
}

AsyncPullType Lbank::asyncCreateOrder(const String& symbol, const String& type, const String& side,
                                      double amount, double price, const json& params) {
    return std::async(std::launch::async, [this, symbol, type, side, amount, price, params]() {
        return createOrder(symbol, type, side, amount, price, params);
    });
}

AsyncPullType Lbank::asyncCancelOrder(const String& id, const String& symbol, const json& params) {
    return std::async(std::launch::async, [this, id, symbol, params]() {
        return cancelOrder(id, symbol, params);
    });
}

AsyncPullType Lbank::asyncFetchOrder(const String& id, const String& symbol, const json& params) {
    return std::async(std::launch::async, [this, id, symbol, params]() {
        return fetchOrder(id, symbol, params);
    });
}

AsyncPullType Lbank::asyncFetchOrders(const String& symbol, int since, int limit, const json& params) {
    return std::async(std::launch::async, [this, symbol, since, limit, params]() {
        return fetchOrders(symbol, since, limit, params);
    });
}

AsyncPullType Lbank::asyncFetchOpenOrders(const String& symbol, int since, int limit, const json& params) {
    return std::async(std::launch::async, [this, symbol, since, limit, params]() {
        return fetchOpenOrders(symbol, since, limit, params);
    });
}

AsyncPullType Lbank::asyncFetchClosedOrders(const String& symbol, int since, int limit, const json& params) {
    return std::async(std::launch::async, [this, symbol, since, limit, params]() {
        return fetchClosedOrders(symbol, since, limit, params);
    });
}

// Account API Implementation
json Lbank::fetchMyTrades(const String& symbol, int since, int limit, const json& params) {
    if (symbol.empty()) {
        throw ArgumentsRequired("fetchMyTrades() requires a symbol argument");
    }
    auto market = loadMarket(symbol);
    auto request = {
        {"symbol", market["id"]}
    };
    if (since) {
        request["startTime"] = since;
    }
    if (limit) {
        request["size"] = limit;
    }
    auto response = fetch("/user_trades", "private", "POST", extend(request, params));
    return parseTrades(response, market, since, limit);
}

json Lbank::fetchDeposits(const String& code, int since, int limit, const json& params) {
    if (code.empty()) {
        throw ArgumentsRequired("fetchDeposits() requires a currency code argument");
    }
    auto currency = loadCurrency(code);
    auto request = {
        {"asset", currency["id"]}
    };
    if (since) {
        request["startTime"] = since;
    }
    if (limit) {
        request["size"] = limit;
    }
    auto response = fetch("/deposit_history", "private", "POST", extend(request, params));
    return parseTransactions(response, currency, since, limit, "deposit");
}

json Lbank::fetchWithdrawals(const String& code, int since, int limit, const json& params) {
    if (code.empty()) {
        throw ArgumentsRequired("fetchWithdrawals() requires a currency code argument");
    }
    auto currency = loadCurrency(code);
    auto request = {
        {"asset", currency["id"]}
    };
    if (since) {
        request["startTime"] = since;
    }
    if (limit) {
        request["size"] = limit;
    }
    auto response = fetch("/withdraw_history", "private", "POST", extend(request, params));
    return parseTransactions(response, currency, since, limit, "withdrawal");
}

json Lbank::fetchDepositAddress(const String& code, const json& params) {
    if (code.empty()) {
        throw ArgumentsRequired("fetchDepositAddress() requires a currency code argument");
    }
    auto currency = loadCurrency(code);
    auto request = {
        {"asset", currency["id"]}
    };
    auto response = fetch("/deposit_address", "private", "POST", extend(request, params));
    return parseDepositAddress(response, currency);
}

json Lbank::withdraw(const String& code, double amount, const String& address, const String& tag, const json& params) {
    if (code.empty()) {
        throw ArgumentsRequired("withdraw() requires a currency code argument");
    }
    auto currency = loadCurrency(code);
    auto request = {
        {"asset", currency["id"]},
        {"amount", amount},
        {"address", address}
    };
    if (!tag.empty()) {
        request["memo"] = tag;
    }
    auto response = fetch("/withdraw", "private", "POST", extend(request, params));
    return parseTransaction(response, currency);
}

// Async Account API Implementation
AsyncPullType Lbank::asyncFetchMyTrades(const String& symbol, int since, int limit, const json& params) {
    return std::async(std::launch::async, [this, symbol, since, limit, params]() {
        return fetchMyTrades(symbol, since, limit, params);
    });
}

AsyncPullType Lbank::asyncFetchDeposits(const String& code, int since, int limit, const json& params) {
    return std::async(std::launch::async, [this, code, since, limit, params]() {
        return fetchDeposits(code, since, limit, params);
    });
}

AsyncPullType Lbank::asyncFetchWithdrawals(const String& code, int since, int limit, const json& params) {
    return std::async(std::launch::async, [this, code, since, limit, params]() {
        return fetchWithdrawals(code, since, limit, params);
    });
}

AsyncPullType Lbank::asyncFetchDepositAddress(const String& code, const json& params) {
    return std::async(std::launch::async, [this, code, params]() {
        return fetchDepositAddress(code, params);
    });
}

AsyncPullType Lbank::asyncWithdraw(const String& code, double amount, const String& address, const String& tag, const json& params) {
    return std::async(std::launch::async, [this, code, amount, address, tag, params]() {
        return withdraw(code, amount, address, tag, params);
    });
}

// Additional Features Implementation
json Lbank::fetchCurrencies(const json& params) {
    auto response = fetch("/accuracy", "public", "GET", params);
    return parseCurrencies(response);
}

json Lbank::fetchTradingFees(const json& params) {
    loadMarkets();
    auto response = fetch("/trading_fees", "private", "POST", params);
    return parseTradingFees(response);
}

json Lbank::fetchFundingFees(const json& params) {
    loadMarkets();
    auto response = fetch("/withdrawConfigs", "private", "POST", params);
    return parseFundingFees(response);
}

json Lbank::fetchTransactionFees(const json& params) {
    loadMarkets();
    auto response = fetch("/withdrawConfigs", "private", "POST", params);
    return parseTransactionFees(response);
}

json Lbank::fetchSystemStatus(const json& params) {
    return fetch("/system_status", "public", "GET", params);
}

json Lbank::fetchTime(const json& params) {
    auto response = fetch("/timestamp", "public", "GET", params);
    return response["timestamp"].get<int64_t>();
}

// Async Additional Features Implementation
AsyncPullType Lbank::asyncFetchCurrencies(const json& params) {
    return std::async(std::launch::async, [this, params]() {
        return fetchCurrencies(params);
    });
}

AsyncPullType Lbank::asyncFetchTradingFees(const json& params) {
    return std::async(std::launch::async, [this, params]() {
        return fetchTradingFees(params);
    });
}

AsyncPullType Lbank::asyncFetchFundingFees(const json& params) {
    return std::async(std::launch::async, [this, params]() {
        return fetchFundingFees(params);
    });
}

AsyncPullType Lbank::asyncFetchTransactionFees(const json& params) {
    return std::async(std::launch::async, [this, params]() {
        return fetchTransactionFees(params);
    });
}

AsyncPullType Lbank::asyncFetchSystemStatus(const json& params) {
    return std::async(std::launch::async, [this, params]() {
        return fetchSystemStatus(params);
    });
}

AsyncPullType Lbank::asyncFetchTime(const json& params) {
    return std::async(std::launch::async, [this, params]() {
        return fetchTime(params);
    });
}

// Parse Methods Implementation
json Lbank::parseTicker(const json& ticker, const Market& market) {
    auto timestamp = ticker["timestamp"].get<int64_t>();
    return {
        {"symbol", market["symbol"]},
        {"timestamp", timestamp},
        {"datetime", iso8601(timestamp)},
        {"high", safeFloat(ticker, "high")},
        {"low", safeFloat(ticker, "low")},
        {"bid", safeFloat(ticker, "bid")},
        {"bidVolume", safeFloat(ticker, "bidVolume")},
        {"ask", safeFloat(ticker, "ask")},
        {"askVolume", safeFloat(ticker, "askVolume")},
        {"vwap", safeFloat(ticker, "vwap")},
        {"open", safeFloat(ticker, "open")},
        {"close", safeFloat(ticker, "close")},
        {"last", safeFloat(ticker, "close")},
        {"previousClose", safeFloat(ticker, "previousClose")},
        {"change", safeFloat(ticker, "change")},
        {"percentage", safeFloat(ticker, "percentage")},
        {"average", safeFloat(ticker, "average")},
        {"baseVolume", safeFloat(ticker, "baseVolume")},
        {"quoteVolume", safeFloat(ticker, "quoteVolume")},
        {"info", ticker}
    };
}

json Lbank::parseTrade(const json& trade, const Market& market) {
    auto timestamp = trade["timestamp"].get<int64_t>();
    return {
        {"id", safeString(trade, "id")},
        {"info", trade},
        {"timestamp", timestamp},
        {"datetime", iso8601(timestamp)},
        {"symbol", market["symbol"]},
        {"type", safeString(trade, "type")},
        {"side", safeString(trade, "side")},
        {"order", safeString(trade, "orderId")},
        {"takerOrMaker", safeString(trade, "takerOrMaker")},
        {"price", safeFloat(trade, "price")},
        {"amount", safeFloat(trade, "amount")},
        {"cost", safeFloat(trade, "cost")},
        {"fee", parseFee(trade)}
    };
}

json Lbank::parseTransaction(const json& transaction, const String& currency) {
    auto timestamp = transaction["timestamp"].get<int64_t>();
    return {
        {"id", safeString(transaction, "id")},
        {"info", transaction},
        {"timestamp", timestamp},
        {"datetime", iso8601(timestamp)},
        {"currency", currency},
        {"address", safeString(transaction, "address")},
        {"tag", safeString(transaction, "tag")},
        {"type", safeString(transaction, "type")},
        {"amount", safeFloat(transaction, "amount")},
        {"status", safeString(transaction, "status")},
        {"fee", parseFee(transaction)},
        {"txid", safeString(transaction, "txid")}
    };
}

json Lbank::parseDepositAddress(const json& depositAddress, const String& currency) {
    return {
        {"currency", currency},
        {"address", safeString(depositAddress, "address")},
        {"tag", safeString(depositAddress, "tag")},
        {"network", safeString(depositAddress, "network")},
        {"info", depositAddress}
    };
}

json Lbank::parseFee(const json& fee, const Market& market) {
    return {
        {"currency", market["quote"]},
        {"cost", safeFloat(fee, "cost")},
        {"rate", safeFloat(fee, "rate")}
    };
}

} // namespace ccxt
