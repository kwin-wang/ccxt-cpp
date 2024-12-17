#include "ccxt/exchanges/ace.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/coroutine2/all.hpp>
#include <iostream>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

namespace ccxt {

void Ace::fetchMarkets() {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.ace.io/v2/market/pairs");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        } else if (httpCode != 200) {
            std::cerr << "HTTP request failed with code: " << httpCode << std::endl;
        } else {
            try {
                auto jsonResponse = nlohmann::json::parse(readBuffer);
                // Process the JSON response (e.g., store market data)
                std::cout << jsonResponse.dump(4) << std::endl; // Pretty print
            } catch (const nlohmann::json::parse_error& e) {
                std::cerr << "JSON parse error: " << e.what() << std::endl;
            }
        }
        curl_easy_cleanup(curl);
    }
}

json Ace::fetchOrderBook(const String &symbol, const long *limit = nullptr,
                      const json &params = json::object()) 
                      {
                        return json::object();
                      }

void Ace::createOrder(const std::string& symbol, const std::string& type, const std::string& side, double amount, double price) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;
    nlohmann::json order;

    // Prepare the JSON order object
    order["symbol"] = symbol;
    order["type"] = type;
    order["side"] = side;
    order["amount"] = amount;
    order["price"] = price;

    std::string jsonData = order.dump();

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.ace.io/v2/orders");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, NULL); // Set appropriate headers if needed
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        } else if (httpCode != 200) {
            std::cerr << "HTTP request failed with code: " << httpCode << std::endl;
        } else {
            try {
                auto jsonResponse = nlohmann::json::parse(readBuffer);
                // Process the JSON response (e.g., check order status)
                std::cout << jsonResponse.dump(4) << std::endl; // Pretty print
            } catch (const nlohmann::json::parse_error& e) {
                std::cerr << "JSON parse error: " << e.what() << std::endl;
            }
        }
        curl_easy_cleanup(curl);
    }
}

void Ace::describe() {
    this->set("id", "ace");
    this->set("name", "ACE");
    this->set("countries", {"TW"}); // Taiwan
    this->set("version", "v2");
    this->set("rateLimit", 100);
    this->set("pro", false);
    
    this->set("has", {
        {"CORS", nullptr},
        {"spot", true},
        {"margin", false},
        {"swap", false},
        {"future", false},
        {"option", false},
        {"cancelAllOrders", false},
        {"cancelOrder", true},
        {"cancelOrders", false},
        {"createOrder", true},
        {"fetchBalance", true},
        {"fetchMarkets", true},
        {"fetchMyTrades", true},
        {"fetchOHLCV", true},
        {"fetchOpenOrders", true},
        {"fetchOrder", true},
        {"fetchOrderBook", true},
        {"fetchTicker", true},
        {"fetchTickers", true}
    });

    this->set("timeframes", {
        {"1m", 1},
        {"5m", 5},
        {"10m", 10},
        {"30m", 30},
        {"1h", 60},
        {"2h", 120},
        {"4h", 240},
        {"6h", 360},
        {"12h", 720},
        {"1d", 1440},
        {"1w", 10080}
    });

    this->set("urls", {
        {"logo", "https://user-images.githubusercontent.com/1294454/2021-02-01/ace.jpg"},
        {"api", {
            {"public", "https://api.ace.io/v2"},
            {"private", "https://api.ace.io/v2"}
        }},
        {"www", "https://ace.io"},
        {"doc", {
            "https://github.com/ace-exchange/ace-official-api-docs/blob/master/api_v2.md"
        }}
    });

    this->set("api", {
        {"public", {
            {"get", {
                {"market/pairs"},
                {"market/{pair}/depth"},
                {"market/{pair}/kline"},
                {"market/{pair}/ticker"},
                {"market/tickers"}
            }}
        }},
        {"private", {
            {"get", {
                {"order/{orderId}"},
                {"order/open"},
                {"order/trades"},
                {"account/balances"}
            }},
            {"post", {
                {"order"}
            }},
            {"delete", {
                {"order/{orderId}"}
            }}
        }}
    });

    this->set("fees", {
        {"trading", {
            {"tierBased", false},
            {"percentage", true},
            {"maker", 0.001},
            {"taker", 0.001}
        }}
    });
}

// Market Data API - Sync
json Ace::fetchMarkets(const json& params) {
    auto response = this->fetch("/api/v2/spot/instruments", "GET", params);
    auto data = this->safeValue(response, "data", json::array());
    auto result = json::array();
    
    for (const auto& market : data) {
        result.push_back(this->parseMarket(market));
    }
    
    return result;
}

json Ace::parseMarket(const json& market) {
    json result;
    result["id"] = market["symbol"];
    result["symbol"] = market["symbol"];
    result["base"] = market["base_currency"];
    result["quote"] = market["quote_currency"];
    result["baseId"] = market["base_currency"];
    result["quoteId"] = market["quote_currency"];
    result["active"] = true;
    result["type"] = "spot";
    result["spot"] = true;
    result["margin"] = false;
    result["future"] = false;
    
    result["precision"] = {
        {"amount", market.value("amount_precision", 8)},
        {"price", market.value("price_precision", 8)}
    };
    
    result["limits"] = {
        {"amount", {
            {"min", market.value("min_trade_amount", 0.0)},
            {"max", market.value("max_trade_amount", INFINITY)}
        }},
        {"price", {
            {"min", market.value("min_trade_price", 0.0)},
            {"max", market.value("max_trade_price", INFINITY)}
        }},
        {"cost", {
            {"min", market.value("min_trade_money", 0.0)},
            {"max", market.value("max_trade_money", INFINITY)}
        }}
    };
    
    return result;
}

json Ace::fetchTicker(const String& symbol, const json& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]}
    };
    auto response = this->fetch("/api/v2/spot/ticker", "GET", this->extend(request, params));
    return this->parseTicker(response["data"], market);
}

json Ace::parseTicker(const json& ticker, const Market& market) {
    auto timestamp = this->safeTimestamp(ticker, "timestamp");
    auto symbol = this->safeString(market, "symbol");
    auto last = this->safeString(ticker, "last");
    
    return {
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"high", this->safeString(ticker, "high")},
        {"low", this->safeString(ticker, "low")},
        {"bid", this->safeString(ticker, "bid")},
        {"bidVolume", this->safeString(ticker, "bidSize")},
        {"ask", this->safeString(ticker, "ask")},
        {"askVolume", this->safeString(ticker, "askSize")},
        {"vwap", this->safeString(ticker, "vwap")},
        {"open", this->safeString(ticker, "open")},
        {"close", last},
        {"last", last},
        {"previousClose", nullptr},
        {"change", nullptr},
        {"percentage", nullptr},
        {"average", nullptr},
        {"baseVolume", this->safeString(ticker, "volume")},
        {"quoteVolume", this->safeString(ticker, "quoteVolume")},
        {"info", ticker}
    };
}

json Ace::fetchOrderBook(const String& symbol, const long* limit, const json& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]}
    };
    if (limit) {
        request["limit"] = limit;
    }
    auto response = this->fetch("/api/v2/spot/depth", "GET", this->extend(request, params));
    auto orderbook = response["data"];
    auto timestamp = this->safeTimestamp(orderbook, "timestamp");
    return this->parseOrderBook(orderbook, symbol, timestamp, "bids", "asks");
}

// Trading API - Sync
json Ace::createOrder(const String& symbol, const String& type, const String& side,
                     const double& amount, const double* price, const json& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]},
        {"side", side.toLowerCase()},
        {"type", type.toLowerCase()},
        {"volume", this->amountToPrecision(symbol, amount)}
    };
    
    if (type == "limit") {
        if (!price) {
            throw ArgumentsRequired("createOrder() requires a price argument for limit orders");
        }
        request["price"] = this->priceToPrecision(symbol, *price);
    }
    
    auto response = this->fetch("/api/v2/spot/orders", "POST", this->extend(request, params));
    return this->parseOrder(response["data"], market);
}

json Ace::cancelOrder(const String& id, const String& symbol, const json& params) {
    if (symbol.empty()) {
        throw ArgumentsRequired("cancelOrder() requires a symbol argument");
    }
    
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]},
        {"orderId", id}
    };
    
    auto response = this->fetch("/api/v2/spot/orders/" + id, "DELETE", this->extend(request, params));
    return this->parseOrder(response["data"], market);
}

json Ace::fetchOrder(const String& id, const String& symbol, const json& params) {
    if (symbol.empty()) {
        throw ArgumentsRequired("fetchOrder() requires a symbol argument");
    }
    
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]},
        {"orderId", id}
    };
    
    auto response = this->fetch("/api/v2/spot/orders/" + id, "GET", this->extend(request, params));
    return this->parseOrder(response["data"], market);
}

json Ace::fetchOpenOrders(const String& symbol, const long* since,
                         const long* limit, const json& params) {
    if (symbol.empty()) {
        throw ArgumentsRequired("fetchOpenOrders() requires a symbol argument");
    }
    
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]}
    };
    
    if (limit) {
        request["limit"] = limit;
    }
    if (since) {
        request["startTime"] = since;
    }
    
    auto response = this->fetch("/api/v2/spot/openOrders", "GET", this->extend(request, params));
    return this->parseOrders(response["data"], market, since, limit);
}

json Ace::fetchClosedOrders(const String& symbol, const long* since,
                          const long* limit, const json& params) {
    if (symbol.empty()) {
        throw ArgumentsRequired("fetchClosedOrders() requires a symbol argument");
    }
    
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = json{
        {"symbol", market["id"]},
        {"status", "2"}  // 2 represents closed orders in ACE API
    };
    
    if (limit) {
        request["limit"] = limit;
    }
    if (since) {
        request["startTime"] = since;
    }
    
    auto response = this->fetch("/api/v2/spot/orders", "GET", this->extend(request, params));
    return this->parseOrders(response["data"], market, since, limit);
}

json Ace::fetchTickers(const std::vector<String>& symbols, const json& params) {
    this->loadMarkets();
    auto response = this->fetch("/api/v2/spot/tickers", "GET", params);
    auto data = this->safeValue(response, "data", json::array());
    auto result = {};
    
    for (const auto& ticker : data) {
        auto marketId = this->safeString(ticker, "symbol");
        auto market = this->safeMarket(marketId);
        auto symbol = market["symbol"];
        result[symbol] = this->parseTicker(ticker, market);
    }
    
    return this->filterByArray(result, "symbol", symbols);
}

String Ace::sign(const String& path, const String& api,
                const String& method, const json& params,
                const std::map<String, String>& headers,
                const json& body) {
    auto url = this->urls["api"]["rest"] + path;
    auto query = params;
    
    if (api == "private") {
        this->checkRequiredCredentials();
        auto nonce = std::to_string(this->nonce());
        auto auth = nonce + method + path;
        
        if (method == "GET") {
            if (!params.empty()) {
                url += "?" + this->urlencode(params);
                auth += "?" + this->urlencode(params);
            }
        } else {
            if (!params.empty()) {
                body = params;
                auth += this->json(params);
            }
        }
        
        auto signature = this->hmac(auth, this->secret, "sha256", "hex");
        headers["ACE-ACCESS-KEY"] = this->apiKey;
        headers["ACE-ACCESS-SIGN"] = signature;
        headers["ACE-ACCESS-TIMESTAMP"] = nonce;
    } else {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    }
    
    return url;
}

String Ace::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"NEW", "open"},
        {"FILLED", "closed"},
        {"PARTIALLY_FILLED", "open"},
        {"CANCELED", "canceled"},
        {"PENDING_CANCEL", "canceling"},
        {"REJECTED", "rejected"}
    };
    
    return this->safeString(statuses, status, status);
}

json Ace::parseOrder(const json& order, const Market& market) {
    auto id = this->safeString(order, "orderId");
    auto timestamp = this->safeTimestamp(order, "time");
    auto symbol = market["symbol"];
    auto type = this->safeStringLower(order, "type");
    auto side = this->safeStringLower(order, "side");
    
    auto price = this->safeString(order, "price");
    auto amount = this->safeString(order, "origQty");
    auto filled = this->safeString(order, "executedQty");
    auto remaining = this->safeString(order, "remainingQty");
    auto status = this->parseOrderStatus(this->safeString(order, "status"));
    auto cost = this->safeString(order, "cummulativeQuoteQty");
    
    return {
        {"id", id},
        {"clientOrderId", this->safeString(order, "clientOrderId")},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"symbol", symbol},
        {"type", type},
        {"side", side},
        {"price", price},
        {"amount", amount},
        {"cost", cost},
        {"average", nullptr},
        {"filled", filled},
        {"remaining", remaining},
        {"status", status},
        {"fee", nullptr},
        {"trades", nullptr},
        {"info", order}
    };
}

json Ace::parseOHLCV(const json& ohlcv, const Market& market) {
    auto dateTime = this->safeString(ohlcv, "createTime");
    auto timestamp = this->parse8601(dateTime);
    if (timestamp) {
        timestamp -= 28800000;  // 8 hours
    }
    
    return json::array({
        timestamp,
        this->safeNumber(ohlcv, "openPrice"),
        this->safeNumber(ohlcv, "highPrice"),
        this->safeNumber(ohlcv, "lowPrice"),
        this->safeNumber(ohlcv, "closePrice"),
        this->safeNumber(ohlcv, "volume")
    });
}

json Ace::fetchOHLCV(const String& symbol, const String& timeframe,
                    const long* since, const long* limit,
                    const json& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    
    json request = {
        {"symbol", market["id"]},
        {"interval", this->timeframes[timeframe]}
    };
    
    if (limit) {
        request["limit"] = limit;
    }
    if (since) {
        request["startTime"] = since;
    }
    
    auto response = this->fetch("/api/v2/spot/klines", "GET", this->extend(request, params));
    auto data = this->safeValue(response, "data", json::array());
    return this->parseOHLCVs(data, market, timeframe, since, limit);
}

json Ace::parseBalance(const json& response) {
    auto balances = this->safeValue(response, "balances", json::array());
    auto result = {
        {"info", response}
    };
    
    for (const auto& balance : balances) {
        auto currencyId = this->safeString(balance, "asset");
        auto code = this->safeCurrencyCode(currencyId);
        auto account = this->account();
        
        account["free"] = this->safeString(balance, "free");
        account["used"] = this->safeString(balance, "locked");
        result[code] = account;
    }
    
    return result;
}

json Ace::fetchBalance(const json& params) {
    this->loadMarkets();
    auto response = this->fetch("/api/v2/spot/account", "GET", params);
    return this->parseBalance(response["data"]);
}

json Ace::parseTrade(const json& trade, const Market& market) {
    auto id = this->safeString(trade, "id");
    auto orderId = this->safeString(trade, "orderNo");
    auto timestamp = this->safeTimestamp(trade, "time");
    if (!timestamp) {
        auto dateTime = this->safeString(trade, "tradeTime");
        if (dateTime) {
            timestamp = this->parse8601(dateTime) - 28800000;  // 8 hours
        }
    }
    
    auto symbol = market["symbol"];
    auto side = this->safeString(trade, "side");
    if (side == "1") {
        side = "buy";
    } else if (side == "2") {
        side = "sell";
    }
    
    auto price = this->safeString(trade, "price");
    auto amount = this->safeString(trade, "quantity");
    auto cost = this->safeString(trade, "amount");
    
    auto fee = json::object();
    auto feeCost = this->safeNumber(trade, "fee");
    if (!feeCost.empty()) {
        auto feeCurrency = this->safeString(trade, "feeCurrency");
        fee = {
            {"cost", feeCost},
            {"currency", this->safeCurrencyCode(feeCurrency)}
        };
    }
    
    return {
        {"id", id},
        {"info", trade},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"symbol", symbol},
        {"order", orderId},
        {"type", "limit"},
        {"side", side},
        {"takerOrMaker", nullptr},
        {"price", price},
        {"amount", amount},
        {"cost", cost},
        {"fee", fee}
    };
}

json Ace::fetchMyTrades(const String& symbol, const long* since,
                       const long* limit, const json& params) {
    if (symbol.empty()) {
        throw ArgumentsRequired("fetchMyTrades() requires a symbol argument");
    }
    
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]}
    };
    
    if (limit) {
        request["limit"] = limit;
    }
    if (since) {
        request["startTime"] = since;
    }
    
    auto response = this->fetch("/api/v2/spot/myTrades", "GET", this->extend(request, params));
    return this->parseTrades(response["data"], market, since, limit);
}

json Ace::fetchOrderTrades(const String& id, const String& symbol,
                         const long* since, const long* limit,
                         const json& params) {
    if (symbol.empty()) {
        throw ArgumentsRequired("fetchOrderTrades() requires a symbol argument");
    }
    
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]},
        {"orderNo", id}
    };
    
    if (limit) {
        request["limit"] = limit;
    }
    
    auto response = this->fetch("/api/v2/spot/orderTrades", "GET", this->extend(request, params));
    return this->parseTrades(response["data"], market, since, limit);
}

// Async Methods using Boost.Coroutine2
AsyncPullType Ace::fetchMarketsAsync(const json& params) {
    return AsyncPullType(
        [this, params](boost::coroutines2::coroutine<json>::push_type& yield) {
            try {
                auto result = this->fetchMarkets(params);
                yield(result);
            } catch (const std::exception& e) {
                throw;
            }
        });
}

AsyncPullType Ace::fetchTickerAsync(const String& symbol, const json& params) {
    return AsyncPullType(
        [this, symbol, params](boost::coroutines2::coroutine<json>::push_type& yield) {
            try {
                auto result = this->fetchTicker(symbol, params);
                yield(result);
            } catch (const std::exception& e) {
                throw;
            }
        });
}

AsyncPullType Ace::fetchTickersAsync(
    const std::vector<String>& symbols, const json& params) {
    return AsyncPullType(
        [this, symbols, params](boost::coroutines2::coroutine<json>::push_type& yield) {
            try {
                auto result = this->fetchTickers(symbols, params);
                yield(result);
            } catch (const std::exception& e) {
                throw;
            }
        });
}

AsyncPullType Ace::fetchTickersAsync(
    const std::vector<String>& symbols, const json& params) {
    return AsyncPullType(
        [&](boost::coroutines2::coroutine<json>::push_type& yield) {
            this->loadMarkets();
            auto response = this->fetch("/api/v2/spot/tickers", "GET", params);
            auto data = this->safeValue(response, "data", json::array());
            auto result = json::object();
            
            for (const auto& ticker : data) {
                auto marketId = this->safeString(ticker, "symbol");
                auto market = this->safeMarket(marketId);
                auto symbol = market["symbol"];
                result[symbol] = this->parseTicker(ticker, market);
            }
            
            yield(this->filterByArray(result, "symbol", symbols));
        }
    );
}

AsyncPullType Ace::fetchMyTradesAsync(const String& symbol,
                                                                      const long* since,
                                                                      const long* limit,
                                                                      const json& params) {
    return AsyncPullType(
        [&](boost::coroutines2::coroutine<json>::push_type& yield) {
            if (symbol.empty()) {
                throw ArgumentsRequired("fetchMyTradesAsync() requires a symbol argument");
            }
            
            this->loadMarkets();
            auto market = this->market(symbol);
            auto request = json{
                {"symbol", market["id"]}
            };
            
            if (limit) {
                request["limit"] = limit;
            }
            if (since) {
                request["startTime"] = since;
            }
            
            auto response = this->fetch("/api/v2/spot/myTrades", "GET", this->extend(request, params));
            yield(this->parseTrades(response["data"], market, since, limit));
        }
    );
}

AsyncPullType Ace::fetchOrderTradesAsync(const String& id,
                                                                         const String& symbol,
                                                                         const long* since,
                                                                         const long* limit,
                                                                         const json& params) {
    return AsyncPullType(
        [&](boost::coroutines2::coroutine<json>::push_type& yield) {
            if (symbol.empty()) {
                throw ArgumentsRequired("fetchOrderTradesAsync() requires a symbol argument");
            }
            
            this->loadMarkets();
            auto market = this->market(symbol);
            auto request = json{
                {"symbol", market["id"]},
                {"orderNo", id}
            };
            
            if (limit) {
                request["limit"] = limit;
            }
            
            auto response = this->fetch("/api/v2/spot/orderTrades", "GET", this->extend(request, params));
            yield(this->parseTrades(response["data"], market, since, limit));
        }
    );
}

AsyncPullType Ace::fetchOrderBookAsync(
    const String& symbol, const long* limit, const json& params) {
    return AsyncPullType(
        [this, symbol, limit, params](boost::coroutines2::coroutine<json>::push_type& yield) {
            try {
                auto result = this->fetchOrderBook(symbol, limit, params);
                yield(result);
            } catch (const std::exception& e) {
                throw;
            }
        });
}

AsyncPullType Ace::fetchOHLCVAsync(
    const String& symbol, const String& timeframe,
    const long* since, const long* limit,
    const json& params) {
    return AsyncPullType(
        [this, symbol, timeframe, since, limit, params](boost::coroutines2::coroutine<json>::push_type& yield) {
            try {
                auto result = this->fetchOHLCV(symbol, timeframe, since, limit, params);
                yield(result);
            } catch (const std::exception& e) {
                throw;
            }
        });
}

AsyncPullType Ace::createOrderAsync(
    const String& symbol, const String& type, const String& side,
    const double& amount, const double* price,
    const json& params) {
    return AsyncPullType(
        [this, symbol, type, side, amount, price, params](boost::coroutines2::coroutine<json>::push_type& yield) {
            try {
                auto result = this->createOrder(symbol, type, side, amount, price, params);
                yield(result);
            } catch (const std::exception& e) {
                throw;
            }
        });
}

AsyncPullType Ace::cancelOrderAsync(
    const String& id, const String& symbol, const json& params) {
    return AsyncPullType(
        [this, id, symbol, params](boost::coroutines2::coroutine<json>::push_type& yield) {
            try {
                auto result = this->cancelOrder(id, symbol, params);
                yield(result);
            } catch (const std::exception& e) {
                throw;
            }
        });
}

AsyncPullType Ace::fetchOrderAsync(
    const String& id, const String& symbol, const json& params) {
    return AsyncPullType(
        [this, id, symbol, params](boost::coroutines2::coroutine<json>::push_type& yield) {
            try {
                auto result = this->fetchOrder(id, symbol, params);
                yield(result);
            } catch (const std::exception& e) {
                throw;
            }
        });
}

AsyncPullType Ace::fetchOpenOrdersAsync(
    const String& symbol, const long* since,
    const long* limit, const json& params) {
    return AsyncPullType(
        [this, symbol, since, limit, params](boost::coroutines2::coroutine<json>::push_type& yield) {
            try {
                auto result = this->fetchOpenOrders(symbol, since, limit, params);
                yield(result);
            } catch (const std::exception& e) {
                throw;
            }
        });
}

AsyncPullType Ace::fetchMyTradesAsync(
    const String& symbol, const long* since,
    const long* limit, const json& params) {
    return AsyncPullType(
        [this, symbol, since, limit, params](boost::coroutines2::coroutine<json>::push_type& yield) {
            try {
                auto result = this->fetchMyTrades(symbol, since, limit, params);
                yield(result);
            } catch (const std::exception& e) {
                throw;
            }
        });
}

AsyncPullType Ace::fetchBalanceAsync(const json& params) {
    return AsyncPullType(
        [this, params](boost::coroutines2::coroutine<json>::push_type& yield) {
            try {
                auto result = this->fetchBalance(params);
                yield(result);
            } catch (const std::exception& e) {
                throw;
            }
        });
}

AsyncPullType Ace::fetchClosedOrdersAsync(
    const String& symbol, const long* since,
    const long* limit, const json& params) {
    return AsyncPullType(
        [&](boost::coroutines2::coroutine<json>::push_type& yield) {
            if (symbol.empty()) {
                throw ArgumentsRequired("fetchClosedOrdersAsync() requires a symbol argument");
            }
            
            this->loadMarkets();
            auto market = this->market(symbol);
            auto request = json{
                {"symbol", market["id"]},
                {"status", "2"}  // 2 represents closed orders in ACE API
            };
            
            if (limit) {
                request["limit"] = limit;
            }
            if (since) {
                request["startTime"] = since;
            }
            
            auto response = this->fetch("/api/v2/spot/orders", "GET", this->extend(request, params));
            yield(this->parseOrders(response["data"], market, since, limit));
        }
    );
}

json Ace::parseTransaction(const json& transaction, const String& currency) {
    auto id = this->safeString(transaction, "id");
    auto currencyId = this->safeString(transaction, "currency");
    auto code = this->safeCurrencyCode(currencyId);
    auto timestamp = this->safeTimestamp(transaction, "createTime");
    auto updated = this->safeTimestamp(transaction, "updateTime");
    auto status = this->parseTransactionStatus(this->safeString(transaction, "status"));
    auto amount = this->safeNumber(transaction, "amount");
    auto addressFrom = this->safeString(transaction, "fromAddress");
    auto addressTo = this->safeString(transaction, "toAddress");
    auto txid = this->safeString(transaction, "txHash");
    auto tag = this->safeString(transaction, "memo");
    auto fee = nullptr;
    auto feeCost = this->safeNumber(transaction, "fee");
    
    if (feeCost != nullptr) {
        fee = {
            {"currency", code},
            {"cost", feeCost}
        };
    }
    
    return {
        {"info", transaction},
        {"id", id},
        {"txid", txid},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"network", nullptr},
        {"address", addressTo},
        {"addressTo", addressTo},
        {"addressFrom", addressFrom},
        {"tag", tag},
        {"tagTo", tag},
        {"tagFrom", nullptr},
        {"type", "deposit"},
        {"amount", amount},
        {"currency", code},
        {"status", status},
        {"updated", updated},
        {"internal", false},
        {"fee", fee}
    };
}

String Ace::parseTransactionStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"1", "pending"},     // Processing
        {"2", "ok"},         // Success
        {"3", "failed"},     // Failed
        {"4", "canceled"}    // Canceled
    };
    
    return this->safeString(statuses, status, status);
}

json Ace::fetchDeposits(const String& code, const long* since,
                       const long* limit, const json& params) {
    if (code.empty()) {
        throw ArgumentsRequired("fetchDeposits() requires a code argument");
    }
    
    this->loadMarkets();
    auto currency = this->currency(code);
    auto request = json{
        {"currency", currency["id"]}
    };
    
    if (limit) {
        request["limit"] = limit;
    }
    if (since) {
        request["startTime"] = since;
    }
    
    auto response = this->fetch("/api/v2/account/deposits", "GET", this->extend(request, params));
    auto data = this->safeValue(response, "data", json::array());
    return this->parseTransactions(data, currency["code"], since, limit);
}

AsyncPullType Ace::fetchDepositsAsync(
    const String& code, const long* since,
    const long* limit, const json& params) {
    return AsyncPullType(
        [&](boost::coroutines2::coroutine<json>::push_type& yield) {
            if (code.empty()) {
                throw ArgumentsRequired("fetchDepositsAsync() requires a code argument");
            }
            
            this->loadMarkets();
            auto currency = this->currency(code);
            auto request = json{
                {"currency", currency["id"]}
            };
            
            if (limit) {
                request["limit"] = limit;
            }
            if (since) {
                request["startTime"] = since;
            }
            
            auto response = this->fetch("/api/v2/account/deposits", "GET", this->extend(request, params));
            auto data = this->safeValue(response, "data", json::array());
            yield(this->parseTransactions(data, currency["code"], since, limit));
        }
    );
}

json Ace::parseCurrency(const json& currency) {
    auto id = this->safeString(currency, "currency");
    auto code = this->safeCurrencyCode(id);
    auto name = this->safeString(currency, "name");
    auto active = this->safeInteger(currency, "status") == 1;
    auto fee = this->safeNumber(currency, "withdrawFee");
    auto precision = this->safeInteger(currency, "precision");
    
    auto limits = json::object();
    limits["withdraw"] = {
        {"min", this->safeNumber(currency, "minWithdraw")},
        {"max", this->safeNumber(currency, "maxWithdraw")}
    };
    limits["deposit"] = {
        {"min", this->safeNumber(currency, "minDeposit")},
        {"max", nullptr}
    };
    
    return {
        {"id", id},
        {"code", code},
        {"name", name},
        {"active", active},
        {"deposit", active},
        {"withdraw", active},
        {"fee", fee},
        {"precision", precision},
        {"limits", limits},
        {"info", currency}
    };
}

json Ace::fetchCurrencies(const json& params) {
    auto response = this->fetch("/api/v2/spot/currencies", "GET", params);
    auto data = this->safeValue(response, "data", json::array());
    auto result = json::object();
    
    for (const auto& currency : data) {
        auto parsed = this->parseCurrency(currency);
        auto code = this->safeString(parsed, "code");
        result[code] = parsed;
    }
    
    return result;
}

AsyncPullType Ace::fetchCurrenciesAsync(const json& params) {
    return AsyncPullType(
        [&](boost::coroutines2::coroutine<json>::push_type& yield) {
            auto response = this->fetch("/api/v2/spot/currencies", "GET", params);
            auto data = this->safeValue(response, "data", json::array());
            auto result = json::object();
            
            for (const auto& currency : data) {
                auto parsed = this->parseCurrency(currency);
                auto code = this->safeString(parsed, "code");
                result[code] = parsed;
            }
            
            yield(result);
        }
    );
}

json Ace::fetchWithdrawals(const String& code, const long* since,
                          const long* limit, const json& params) {
    if (code.empty()) {
        throw ArgumentsRequired("fetchWithdrawals() requires a code argument");
    }
    
    this->loadMarkets();
    auto currency = this->currency(code);
    auto request = json{
        {"currency", currency["id"]}
    };
    
    if (limit) {
        request["limit"] = limit;
    }
    if (since) {
        request["startTime"] = since;
    }
    
    auto response = this->fetch("/api/v2/account/withdrawals", "GET", this->extend(request, params));
    auto data = this->safeValue(response, "data", json::array());
    return this->parseTransactions(data, currency["code"], since, limit, {
        {"type", "withdrawal"}
    });
}

AsyncPullType Ace::fetchWithdrawalsAsync(
    const String& code, const long* since,
    const long* limit, const json& params) {
    return AsyncPullType(
        [&](boost::coroutines2::coroutine<json>::push_type& yield) {
            if (code.empty()) {
                throw ArgumentsRequired("fetchWithdrawalsAsync() requires a code argument");
            }
            
            this->loadMarkets();
            auto currency = this->currency(code);
            auto request = json{
                {"currency", currency["id"]}
            };
            
            if (limit) {
                request["limit"] = limit;
            }
            if (since) {
                request["startTime"] = since;
            }
            
            auto response = this->fetch("/api/v2/account/withdrawals", "GET", this->extend(request, params));
            auto data = this->safeValue(response, "data", json::array());
            yield(this->parseTransactions(data, currency["code"], since, limit, {
                {"type", "withdrawal"}
            }));
        }
    );
}

json Ace::withdraw(const String& code, const double& amount, const String& address,
                  const String& tag, const json& params) {
    if (code.empty()) {
        throw ArgumentsRequired("withdraw() requires a code argument");
    }
    if (amount <= 0) {
        throw ArgumentsRequired("withdraw() amount must be greater than zero");
    }
    if (address.empty()) {
        throw ArgumentsRequired("withdraw() requires an address argument");
    }
    
    this->loadMarkets();
    auto currency = this->currency(code);
    auto request = json{
        {"currency", currency["id"]},
        {"amount", this->numberToString(amount)},
        {"address", address}
    };
    
    if (!tag.empty()) {
        request["memo"] = tag;
    }
    
    auto response = this->fetch("/api/v2/account/withdraw", "POST", this->extend(request, params));
    auto data = this->safeValue(response, "data", json::object());
    return this->parseTransaction(data, currency["code"]);
}

 {
    return AsyncPullType(
        [&](boost::coroutines2::coroutine<json>::push_type& yield) {
            if (code.empty()) {
                throw ArgumentsRequired("withdrawAsync() requires a code argument");
            }
            if (amount <= 0) {
                throw ArgumentsRequired("withdrawAsync() amount must be greater than zero");
            }
            if (address.empty()) {
                throw ArgumentsRequired("withdrawAsync() requires an address argument");
            }
            
            this->loadMarkets();
            auto currency = this->currency(code);
            auto request = json{
                {"currency", currency["id"]},
                {"amount", this->numberToString(amount)},
                {"address", address}
            };
            
            if (!tag.empty()) {
                request["memo"] = tag;
            }
            
            auto response = this->fetch("/api/v2/account/withdraw", "POST", this->extend(request, params));
            auto data = this->safeValue(response, "data", json::object());
            yield(this->parseTransaction(data, currency["code"]));
        }
    );
}

} // namespace ccxt
