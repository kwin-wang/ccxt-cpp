#include "exchanges/ws/bitrue_ws.h"
#include "base/json_helper.h"
#include <chrono>

namespace ccxt {

bitrue_ws::bitrue_ws() : exchange_ws() {
    this->urls["ws"] = "wss://ws.bitrue.com/ws";
    this->urls["wsPublic"] = "wss://ws.bitrue.com/ws/stream";
    this->urls["wsPrivate"] = "wss://ws.bitrue.com/ws/user";
    
    this->options["watchOrderBook"]["snapshotDelay"] = 0;
    this->authenticated = false;
    this->listenKey = 0;
}

Response bitrue_ws::watchTicker(const std::string& symbol, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "ticker:" + market["symbol"].get<std::string>();
    
    json request = {
        {"method", "SUBSCRIBE"},
        {"params", {getSymbolId(symbol) + "@ticker"}},
        {"id", this->requestId++}
    };
    
    return this->watch(this->urls["wsPublic"], messageHash, request, messageHash);
}

Response bitrue_ws::watchTickers(const std::vector<std::string>& symbols, const Dict& params) {
    std::vector<std::string> messageHashes;
    std::vector<std::string> streamParams;
    
    for (const auto& symbol : symbols) {
        auto market = this->market(symbol);
        messageHashes.push_back("ticker:" + market["symbol"].get<std::string>());
        streamParams.push_back(getSymbolId(symbol) + "@ticker");
    }
    
    json request = {
        {"method", "SUBSCRIBE"},
        {"params", streamParams},
        {"id", this->requestId++}
    };
    
    return this->watchMultiple(this->urls["wsPublic"], messageHashes, request, messageHashes);
}

Response bitrue_ws::watchTrades(const std::string& symbol, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "trades:" + market["symbol"].get<std::string>();
    
    json request = {
        {"method", "SUBSCRIBE"},
        {"params", {getSymbolId(symbol) + "@trade"}},
        {"id", this->requestId++}
    };
    
    return this->watch(this->urls["wsPublic"], messageHash, request, messageHash);
}

Response bitrue_ws::watchOrderBook(const std::string& symbol, const int limit, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "orderbook:" + market["symbol"].get<std::string>();
    
    std::string channel = "@depth";
    if (limit == 5 || limit == 10 || limit == 20) {
        channel = "@depth" + std::to_string(limit);
    }
    
    json request = {
        {"method", "SUBSCRIBE"},
        {"params", {getSymbolId(symbol) + channel}},
        {"id", this->requestId++}
    };
    
    return this->watch(this->urls["wsPublic"], messageHash, request, messageHash);
}

Response bitrue_ws::watchOHLCV(const std::string& symbol, const std::string& timeframe, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "kline:" + market["symbol"].get<std::string>() + ":" + timeframe;
    
    json request = {
        {"method", "SUBSCRIBE"},
        {"params", {getSymbolId(symbol) + "@kline_" + timeframe}},
        {"id", this->requestId++}
    };
    
    return this->watch(this->urls["wsPublic"], messageHash, request, messageHash);
}

Response bitrue_ws::watchBalance(const Dict& params) {
    if (!this->authenticated) {
        this->authenticate();
    }
    
    std::string messageHash = "balance";
    
    json request = {
        {"method", "SUBSCRIBE"},
        {"params", {"balance"}},
        {"id", this->requestId++}
    };
    
    return this->watch(this->urls["wsPrivate"], messageHash, request, messageHash);
}

Response bitrue_ws::watchOrders(const std::string& symbol, const Dict& params) {
    if (!this->authenticated) {
        this->authenticate();
    }
    
    std::string messageHash = symbol.empty() ? "orders" : "orders:" + symbol;
    
    json request = {
        {"method", "SUBSCRIBE"},
        {"params", {"orders"}},
        {"id", this->requestId++}
    };
    
    return this->watch(this->urls["wsPrivate"], messageHash, request, messageHash);
}

Response bitrue_ws::watchMyTrades(const std::string& symbol, const Dict& params) {
    if (!this->authenticated) {
        this->authenticate();
    }
    
    std::string messageHash = symbol.empty() ? "myTrades" : "myTrades:" + symbol;
    
    json request = {
        {"method", "SUBSCRIBE"},
        {"params", {"trades"}},
        {"id", this->requestId++}
    };
    
    return this->watch(this->urls["wsPrivate"], messageHash, request, messageHash);
}

void bitrue_ws::authenticate(const Dict& params) {
    if (this->authenticated) {
        return;
    }
    
    int64_t timestamp = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
    std::string signature = this->hmac(std::to_string(timestamp), this->config_.secret, "sha256");
    
    json request = {
        {"method", "LOGIN"},
        {"params", {
            {"apiKey", this->config_.apiKey},
            {"timestamp", timestamp},
            {"signature", signature}
        }},
        {"id", this->requestId++}
    };
    
    this->send(request);
}

std::string bitrue_ws::getSymbolId(const std::string& symbol) {
    auto market = this->market(symbol);
    return market["id"].get<std::string>();
}

void bitrue_ws::handleMessage(const json& message) {
    if (message.contains("method")) {
        std::string method = message["method"].get<std::string>();
        
        if (method == "LOGIN") {
            handleAuthenticationMessage(message);
            return;
        }
    }
    
    if (message.contains("stream")) {
        std::string stream = message["stream"].get<std::string>();
        
        if (stream.find("@ticker") != std::string::npos) {
            handleTickerMessage(message);
        } else if (stream.find("@trade") != std::string::npos) {
            handleTradesMessage(message);
        } else if (stream.find("@depth") != std::string::npos) {
            handleOrderBookMessage(message);
        } else if (stream.find("@kline") != std::string::npos) {
            handleOHLCVMessage(message);
        }
    } else if (message.contains("e")) {
        std::string event = message["e"].get<std::string>();
        
        if (event == "outboundAccountPosition") {
            handleBalanceMessage(message);
        } else if (event == "executionReport") {
            handleOrderMessage(message);
        } else if (event == "trade") {
            handleMyTradesMessage(message);
        }
    }
}

void bitrue_ws::handleTickerMessage(const json& message) {
    auto data = message["data"];
    std::string symbolId = data["s"].get<std::string>();
    auto market = this->marketById(symbolId);
    std::string symbol = market["symbol"].get<std::string>();
    
    Ticker ticker;
    ticker.symbol = symbol;
    ticker.timestamp = data["E"].get<int64_t>();
    ticker.datetime = this->iso8601(ticker.timestamp);
    ticker.high = this->safeFloat(data, "h");
    ticker.low = this->safeFloat(data, "l");
    ticker.bid = this->safeFloat(data, "b");
    ticker.ask = this->safeFloat(data, "a");
    ticker.last = this->safeFloat(data, "c");
    ticker.open = this->safeFloat(data, "o");
    ticker.baseVolume = this->safeFloat(data, "v");
    ticker.quoteVolume = this->safeFloat(data, "q");
    ticker.info = data;
    
    this->tickers[symbol] = ticker;
    this->emit("ticker:" + symbol, ticker);
}

void bitrue_ws::handleTradesMessage(const json& message) {
    auto data = message["data"];
    std::string symbolId = data["s"].get<std::string>();
    auto market = this->marketById(symbolId);
    std::string symbol = market["symbol"].get<std::string>();
    
    Trade trade;
    trade.symbol = symbol;
    trade.id = this->safeString(data, "t");
    trade.timestamp = data["T"].get<int64_t>();
    trade.datetime = this->iso8601(trade.timestamp);
    trade.price = this->safeFloat(data, "p");
    trade.amount = this->safeFloat(data, "q");
    trade.side = data["m"].get<bool>() ? "sell" : "buy";
    trade.info = data;
    
    if (this->trades.find(symbol) == this->trades.end()) {
        this->trades[symbol] = std::vector<Trade>();
    }
    this->trades[symbol].push_back(trade);
    this->emit("trades:" + symbol, trade);
}

void bitrue_ws::handleOrderBookMessage(const json& message) {
    auto data = message["data"];
    std::string symbolId = data["s"].get<std::string>();
    auto market = this->marketById(symbolId);
    std::string symbol = market["symbol"].get<std::string>();
    
    if (this->orderbooks.find(symbol) == this->orderbooks.end()) {
        this->orderbooks[symbol] = OrderBook();
    }
    
    OrderBook& orderbook = this->orderbooks[symbol];
    orderbook.symbol = symbol;
    orderbook.timestamp = data["T"].get<int64_t>();
    orderbook.datetime = this->iso8601(orderbook.timestamp);
    
    if (data.contains("asks")) {
        orderbook.asks.clear();
        for (const auto& ask : data["asks"]) {
            orderbook.asks.push_back({
                this->safeFloat(ask, 0),  // price
                this->safeFloat(ask, 1)   // amount
            });
        }
    }
    
    if (data.contains("bids")) {
        orderbook.bids.clear();
        for (const auto& bid : data["bids"]) {
            orderbook.bids.push_back({
                this->safeFloat(bid, 0),  // price
                this->safeFloat(bid, 1)   // amount
            });
        }
    }
    
    this->emit("orderbook:" + symbol, orderbook);
}

void bitrue_ws::handleOHLCVMessage(const json& message) {
    auto data = message["data"];
    auto k = data["k"];
    std::string symbolId = k["s"].get<std::string>();
    auto market = this->marketById(symbolId);
    std::string symbol = market["symbol"].get<std::string>();
    std::string timeframe = k["i"].get<std::string>();
    
    OHLCV ohlcv;
    ohlcv.timestamp = k["t"].get<int64_t>();
    ohlcv.open = this->safeFloat(k, "o");
    ohlcv.high = this->safeFloat(k, "h");
    ohlcv.low = this->safeFloat(k, "l");
    ohlcv.close = this->safeFloat(k, "c");
    ohlcv.volume = this->safeFloat(k, "v");
    
    std::string key = symbol + ":" + timeframe;
    if (this->ohlcvs.find(key) == this->ohlcvs.end()) {
        this->ohlcvs[key] = std::vector<OHLCV>();
    }
    this->ohlcvs[key].push_back(ohlcv);
    
    this->emit("kline:" + symbol + ":" + timeframe, ohlcv);
}

void bitrue_ws::handleBalanceMessage(const json& message) {
    Balance balance;
    balance.timestamp = message["E"].get<int64_t>();
    balance.datetime = this->iso8601(balance.timestamp);
    
    auto balances = message["B"];
    for (const auto& item : balances) {
        std::string currency = item["a"].get<std::string>();
        balance.free[currency] = this->safeFloat(item, "f");
        balance.used[currency] = this->safeFloat(item, "l");
        balance.total[currency] = this->safeFloat(item, "b");
    }
    
    this->emit("balance", balance);
}

void bitrue_ws::handleOrderMessage(const json& message) {
    std::string symbolId = message["s"].get<std::string>();
    auto market = this->marketById(symbolId);
    std::string symbol = market["symbol"].get<std::string>();
    
    Order order;
    order.id = this->safeString(message, "i");
    order.clientOrderId = this->safeString(message, "c");
    order.timestamp = message["T"].get<int64_t>();
    order.datetime = this->iso8601(order.timestamp);
    order.lastTradeTimestamp = nullptr;
    order.symbol = symbol;
    order.type = this->safeString(message, "o");
    order.side = this->safeString(message, "S");
    order.price = this->safeFloat(message, "p");
    order.amount = this->safeFloat(message, "q");
    order.cost = this->safeFloat(message, "Z");
    order.average = this->safeFloat(message, "ap");
    order.filled = this->safeFloat(message, "z");
    order.remaining = this->safeFloat(message, "q") - order.filled;
    order.status = this->safeString(message, "X");
    order.fee = {
        {"cost", this->safeFloat(message, "n")},
        {"currency", this->safeString(message, "N")}
    };
    order.trades = nullptr;
    order.info = message;
    
    std::string messageHash = "orders:" + symbol;
    this->emit(messageHash, order);
}

void bitrue_ws::handleMyTradesMessage(const json& message) {
    std::string symbolId = message["s"].get<std::string>();
    auto market = this->marketById(symbolId);
    std::string symbol = market["symbol"].get<std::string>();
    
    Trade trade;
    trade.id = this->safeString(message, "t");
    trade.order = this->safeString(message, "i");
    trade.timestamp = message["T"].get<int64_t>();
    trade.datetime = this->iso8601(trade.timestamp);
    trade.symbol = symbol;
    trade.type = this->safeString(message, "o");
    trade.side = this->safeString(message, "S");
    trade.price = this->safeFloat(message, "L");
    trade.amount = this->safeFloat(message, "q");
    trade.cost = trade.price * trade.amount;
    trade.fee = {
        {"cost", this->safeFloat(message, "n")},
        {"currency", this->safeString(message, "N")}
    };
    trade.info = message;
    
    std::string messageHash = "myTrades:" + symbol;
    this->emit(messageHash, trade);
}

void bitrue_ws::handleAuthenticationMessage(const json& message) {
    if (message.contains("result") && message["result"].get<bool>()) {
        this->authenticated = true;
        this->emit("authenticated", message);
    } else {
        throw ExchangeError(message["msg"].get<std::string>());
    }
}

void bitrue_ws::handleSubscriptionStatus(const json& message) {
    if (message.contains("result") && !message["result"].get<bool>()) {
        throw ExchangeError(message["msg"].get<std::string>());
    }
}

void bitrue_ws::handleError(const json& message) {
    if (message.contains("msg")) {
        throw ExchangeError(message["msg"].get<std::string>());
    }
}

} // namespace ccxt
