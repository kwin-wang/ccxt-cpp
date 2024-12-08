#include "exchanges/ws/blofin_ws.h"
#include "base/json_helper.h"
#include <chrono>

namespace ccxt {

blofin_ws::blofin_ws() : exchange_ws() {
    this->urls["ws"] = "wss://ws.blofin.com/ws/v1";
    this->urls["api"] = "https://api.blofin.com";
    
    this->options["watchOrderBook"]["snapshotDelay"] = 0;
    this->authenticated = false;
    this->pingInterval = 20000;  // 20 seconds
    this->lastPingTimestamp = 0;
}

Response blofin_ws::watchTicker(const std::string& symbol, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "tickers:" + market["id"].get<std::string>();
    
    json request = {
        {"op", "subscribe"},
        {"args", {{
            {"channel", "tickers"},
            {"instId", market["id"].get<std::string>()}
        }}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response blofin_ws::watchTickers(const std::vector<std::string>& symbols, const Dict& params) {
    std::vector<json> args;
    if (symbols.empty()) {
        args.push_back({
            {"channel", "tickers"},
            {"instId", "all"}
        });
    } else {
        for (const auto& symbol : symbols) {
            auto market = this->market(symbol);
            args.push_back({
                {"channel", "tickers"},
                {"instId", market["id"].get<std::string>()}
            });
        }
    }
    
    json request = {
        {"op", "subscribe"},
        {"args", args}
    };
    
    return this->watch(this->urls["ws"], "tickers", request, "tickers");
}

Response blofin_ws::watchTrades(const std::string& symbol, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "trades:" + market["id"].get<std::string>();
    
    json request = {
        {"op", "subscribe"},
        {"args", {{
            {"channel", "trades"},
            {"instId", market["id"].get<std::string>()}
        }}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response blofin_ws::watchOrderBook(const std::string& symbol, const int limit, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "books:" + market["id"].get<std::string>();
    
    json request = {
        {"op", "subscribe"},
        {"args", {{
            {"channel", "books"},
            {"instId", market["id"].get<std::string>()}
        }}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response blofin_ws::watchOHLCV(const std::string& symbol, const std::string& timeframe, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "candle" + timeframe + ":" + market["id"].get<std::string>();
    
    json request = {
        {"op", "subscribe"},
        {"args", {{
            {"channel", "candle" + timeframe},
            {"instId", market["id"].get<std::string>()}
        }}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response blofin_ws::watchBalance(const Dict& params) {
    if (!this->authenticated) {
        this->authenticate();
    }
    
    json request = {
        {"op", "subscribe"},
        {"args", {{
            {"channel", "account"}
        }}}
    };
    
    return this->watch(this->urls["ws"], "account", request, "account");
}

Response blofin_ws::watchOrders(const std::string& symbol, const Dict& params) {
    if (!this->authenticated) {
        this->authenticate();
    }
    
    std::string messageHash = symbol.empty() ? "orders" : "orders:" + this->market(symbol)["id"].get<std::string>();
    
    json request = {
        {"op", "subscribe"},
        {"args", {{
            {"channel", "orders"},
            {"instId", symbol.empty() ? "all" : this->market(symbol)["id"].get<std::string>()}
        }}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response blofin_ws::watchMyTrades(const std::string& symbol, const Dict& params) {
    if (!this->authenticated) {
        this->authenticate();
    }
    
    std::string messageHash = symbol.empty() ? "trades" : "trades:" + this->market(symbol)["id"].get<std::string>();
    
    json request = {
        {"op", "subscribe"},
        {"args", {{
            {"channel", "trades"},
            {"instId", symbol.empty() ? "all" : this->market(symbol)["id"].get<std::string>()}
        }}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response blofin_ws::watchPositions(const Dict& params) {
    if (!this->authenticated) {
        this->authenticate();
    }
    
    json request = {
        {"op", "subscribe"},
        {"args", {{
            {"channel", "positions"}
        }}}
    };
    
    return this->watch(this->urls["ws"], "positions", request, "positions");
}

void blofin_ws::authenticate(const Dict& params) {
    if (this->authenticated) {
        return;
    }
    
    std::string timestamp = std::to_string(std::chrono::system_clock::now().time_since_epoch().count() / 1000000);
    std::string method = "GET";
    std::string requestPath = "/ws/v1";
    std::string signature = this->getSignature(timestamp, method, requestPath);
    
    json request = {
        {"op", "login"},
        {"args", {{
            {"apiKey", this->apiKey},
            {"passphrase", this->password},
            {"timestamp", timestamp},
            {"sign", signature}
        }}}
    };
    
    this->send(request);
}

std::string blofin_ws::getSignature(const std::string& timestamp, const std::string& method, 
                                  const std::string& requestPath, const std::string& body) {
    std::string message = timestamp + method + requestPath;
    if (!body.empty()) {
        message += body;
    }
    return this->hmac(message, this->secret, "sha256");
}

std::string blofin_ws::getSymbolId(const std::string& symbol) {
    auto market = this->market(symbol);
    return market["id"].get<std::string>();
}

void blofin_ws::handleMessage(const json& message) {
    if (message.contains("event")) {
        std::string event = message["event"].get<std::string>();
        
        if (event == "subscribe") {
            handleSubscriptionStatus(message);
        } else if (event == "login") {
            handleAuthenticationMessage(message);
        } else if (event == "error") {
            handleError(message);
        }
    } else if (message.contains("arg") && message.contains("data")) {
        std::string channel = message["arg"]["channel"].get<std::string>();
        
        if (channel == "tickers") {
            handleTickerMessage(message);
        } else if (channel == "trades") {
            handleTradesMessage(message);
        } else if (channel == "books") {
            handleOrderBookMessage(message);
        } else if (channel.find("candle") == 0) {
            handleOHLCVMessage(message);
        } else if (channel == "account") {
            handleBalanceMessage(message);
        } else if (channel == "orders") {
            handleOrderMessage(message);
        } else if (channel == "trades") {
            handleMyTradesMessage(message);
        } else if (channel == "positions") {
            handlePositionMessage(message);
        }
    }
    
    // Handle ping/pong
    int64_t currentTime = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
    if (currentTime - this->lastPingTimestamp > this->pingInterval) {
        json pingMessage = {{"op", "ping"}};
        this->send(pingMessage);
        this->lastPingTimestamp = currentTime;
    }
}

void blofin_ws::handleTickerMessage(const json& message) {
    auto data = message["data"];
    std::string instId = message["arg"]["instId"].get<std::string>();
    auto symbol = this->marketId(instId);
    
    Ticker ticker;
    ticker.symbol = symbol;
    ticker.timestamp = this->safeInteger(data, "ts");
    ticker.datetime = this->iso8601(ticker.timestamp);
    ticker.high = this->safeFloat(data, "high24h");
    ticker.low = this->safeFloat(data, "low24h");
    ticker.bid = this->safeFloat(data, "bidPx");
    ticker.ask = this->safeFloat(data, "askPx");
    ticker.last = this->safeFloat(data, "last");
    ticker.open = this->safeFloat(data, "open24h");
    ticker.close = ticker.last;
    ticker.baseVolume = this->safeFloat(data, "vol24h");
    ticker.quoteVolume = this->safeFloat(data, "volCcy24h");
    ticker.info = data;
    
    this->tickers[symbol] = ticker;
    this->emit("tickers:" + instId, ticker);
}

void blofin_ws::handleTradesMessage(const json& message) {
    auto data = message["data"];
    std::string instId = message["arg"]["instId"].get<std::string>();
    auto symbol = this->marketId(instId);
    
    for (const auto& trade : data) {
        Trade tradeObj;
        tradeObj.symbol = symbol;
        tradeObj.id = this->safeString(trade, "tradeId");
        tradeObj.timestamp = this->safeInteger(trade, "ts");
        tradeObj.datetime = this->iso8601(tradeObj.timestamp);
        tradeObj.side = this->safeString(trade, "side");
        tradeObj.price = this->safeFloat(trade, "px");
        tradeObj.amount = this->safeFloat(trade, "sz");
        tradeObj.cost = tradeObj.price * tradeObj.amount;
        tradeObj.info = trade;
        
        if (this->trades.find(symbol) == this->trades.end()) {
            this->trades[symbol] = std::vector<Trade>();
        }
        this->trades[symbol].push_back(tradeObj);
        this->emit("trades:" + instId, tradeObj);
    }
}

void blofin_ws::handleOrderBookMessage(const json& message) {
    auto data = message["data"];
    std::string instId = message["arg"]["instId"].get<std::string>();
    auto symbol = this->marketId(instId);
    
    if (this->orderbooks.find(symbol) == this->orderbooks.end()) {
        this->orderbooks[symbol] = OrderBook();
    }
    
    OrderBook& orderbook = this->orderbooks[symbol];
    orderbook.symbol = symbol;
    
    if (data.contains("bids")) {
        for (const auto& bid : data["bids"]) {
            float price = this->safeFloat(bid, 0);
            float amount = this->safeFloat(bid, 1);
            orderbook.bids[price] = amount;
        }
    }
    
    if (data.contains("asks")) {
        for (const auto& ask : data["asks"]) {
            float price = this->safeFloat(ask, 0);
            float amount = this->safeFloat(ask, 1);
            orderbook.asks[price] = amount;
        }
    }
    
    orderbook.timestamp = this->safeInteger(data, "ts");
    orderbook.datetime = this->iso8601(orderbook.timestamp);
    
    this->emit("books:" + instId, orderbook);
}

void blofin_ws::handleOHLCVMessage(const json& message) {
    auto data = message["data"];
    std::string instId = message["arg"]["instId"].get<std::string>();
    auto symbol = this->marketId(instId);
    std::string channel = message["arg"]["channel"].get<std::string>();
    std::string timeframe = channel.substr(6);  // Remove "candle" prefix
    
    for (const auto& candle : data) {
        OHLCV ohlcv;
        ohlcv.timestamp = this->safeInteger(candle, 0);
        ohlcv.open = this->safeFloat(candle, 1);
        ohlcv.high = this->safeFloat(candle, 2);
        ohlcv.low = this->safeFloat(candle, 3);
        ohlcv.close = this->safeFloat(candle, 4);
        ohlcv.volume = this->safeFloat(candle, 5);
        
        std::string key = symbol + ":" + timeframe;
        if (this->ohlcvs.find(key) == this->ohlcvs.end()) {
            this->ohlcvs[key] = std::vector<OHLCV>();
        }
        this->ohlcvs[key].push_back(ohlcv);
        
        this->emit("candle" + timeframe + ":" + instId, ohlcv);
    }
}

void blofin_ws::handleBalanceMessage(const json& message) {
    auto data = message["data"];
    Balance balance;
    balance.timestamp = this->safeInteger(data, "uTime");
    balance.datetime = this->iso8601(balance.timestamp);
    
    for (const auto& item : data["details"]) {
        std::string currency = this->safeString(item, "ccy");
        balance.free[currency] = this->safeFloat(item, "availBal");
        balance.used[currency] = this->safeFloat(item, "frozenBal");
        balance.total[currency] = this->safeFloat(item, "totalEq");
    }
    
    this->emit("account", balance);
}

void blofin_ws::handleOrderMessage(const json& message) {
    auto data = message["data"];
    std::string instId = message["arg"]["instId"].get<std::string>();
    auto symbol = this->marketId(instId);
    
    Order order;
    order.id = this->safeString(data, "ordId");
    order.clientOrderId = this->safeString(data, "clOrdId");
    order.timestamp = this->safeInteger(data, "cTime");
    order.datetime = this->iso8601(order.timestamp);
    order.lastTradeTimestamp = this->safeInteger(data, "uTime");
    order.symbol = symbol;
    order.type = this->safeString(data, "ordType");
    order.side = this->safeString(data, "side");
    order.price = this->safeFloat(data, "px");
    order.amount = this->safeFloat(data, "sz");
    order.cost = this->safeFloat(data, "fillSz") * this->safeFloat(data, "avgPx");
    order.average = this->safeFloat(data, "avgPx");
    order.filled = this->safeFloat(data, "fillSz");
    order.remaining = order.amount - order.filled;
    order.status = this->safeString(data, "state");
    order.fee = {
        {"cost", this->safeFloat(data, "fee")},
        {"currency", this->safeString(data, "feeCcy")}
    };
    order.trades = nullptr;
    order.info = data;
    
    this->emit("orders:" + instId, order);
}

void blofin_ws::handleMyTradesMessage(const json& message) {
    auto data = message["data"];
    std::string instId = message["arg"]["instId"].get<std::string>();
    auto symbol = this->marketId(instId);
    
    Trade trade;
    trade.id = this->safeString(data, "tradeId");
    trade.order = this->safeString(data, "ordId");
    trade.timestamp = this->safeInteger(data, "ts");
    trade.datetime = this->iso8601(trade.timestamp);
    trade.symbol = symbol;
    trade.type = this->safeString(data, "ordType");
    trade.side = this->safeString(data, "side");
    trade.price = this->safeFloat(data, "fillPx");
    trade.amount = this->safeFloat(data, "fillSz");
    trade.cost = trade.price * trade.amount;
    trade.fee = {
        {"cost", this->safeFloat(data, "fee")},
        {"currency", this->safeString(data, "feeCcy")}
    };
    trade.info = data;
    
    this->emit("trades:" + instId, trade);
}

void blofin_ws::handlePositionMessage(const json& message) {
    auto data = message["data"];
    
    for (const auto& pos : data) {
        std::string instId = this->safeString(pos, "instId");
        auto symbol = this->marketId(instId);
        
        Position position;
        position.info = pos;
        position.symbol = symbol;
        position.timestamp = this->safeInteger(pos, "uTime");
        position.datetime = this->iso8601(position.timestamp);
        position.contracts = this->safeFloat(pos, "pos");
        position.contractSize = this->safeFloat(pos, "lotSz");
        position.unrealizedPnl = this->safeFloat(pos, "upl");
        position.leverage = this->safeFloat(pos, "lever");
        position.collateral = this->safeFloat(pos, "margin");
        position.notional = this->safeFloat(pos, "notionalUsd");
        position.markPrice = this->safeFloat(pos, "markPx");
        position.liquidationPrice = this->safeFloat(pos, "liqPx");
        position.marginMode = this->safeString(pos, "mgnMode");
        position.side = this->safeString(pos, "posSide");
        
        this->positions[symbol] = position;
        this->emit("positions", position);
    }
}

void blofin_ws::handleAuthenticationMessage(const json& message) {
    if (message["event"].get<std::string>() == "login") {
        if (message.contains("code") && message["code"].get<std::string>() == "0") {
            this->authenticated = true;
            this->emit("authenticated", message);
        } else {
            throw ExchangeError(message["msg"].get<std::string>());
        }
    }
}

void blofin_ws::handleSubscriptionStatus(const json& message) {
    if (message["event"].get<std::string>() != "subscribe") {
        throw ExchangeError("Subscription failed");
    }
}

void blofin_ws::handleError(const json& message) {
    throw ExchangeError(message["msg"].get<std::string>());
}

} // namespace ccxt
