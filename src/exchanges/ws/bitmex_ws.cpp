#include "exchanges/ws/bitmex_ws.h"
#include "base/json_helper.h"
#include <chrono>

namespace ccxt {

bitmex_ws::bitmex_ws() : exchange_ws() {
    this->urls["ws"] = "wss://ws.bitmex.com/realtime";
    this->urls["wsTest"] = "wss://ws.testnet.bitmex.com/realtime";
    
    this->options["watchOrderBook"]["snapshotDelay"] = 0;
    this->authenticated = false;
    this->expires = 0;
}

Response bitmex_ws::watchTicker(const std::string& symbol, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "instrument:" + market["symbol"].get<std::string>();
    
    json request = {
        {"op", "subscribe"},
        {"args", {"instrument:" + getSymbolId(symbol)}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response bitmex_ws::watchTickers(const std::vector<std::string>& symbols, const Dict& params) {
    std::vector<std::string> messageHashes;
    std::vector<std::string> args;
    
    for (const auto& symbol : symbols) {
        auto market = this->market(symbol);
        messageHashes.push_back("instrument:" + market["symbol"].get<std::string>());
        args.push_back("instrument:" + getSymbolId(symbol));
    }
    
    json request = {
        {"op", "subscribe"},
        {"args", args}
    };
    
    return this->watchMultiple(this->urls["ws"], messageHashes, request, messageHashes);
}

Response bitmex_ws::watchTrades(const std::string& symbol, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "trade:" + market["symbol"].get<std::string>();
    
    json request = {
        {"op", "subscribe"},
        {"args", {"trade:" + getSymbolId(symbol)}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response bitmex_ws::watchOrderBook(const std::string& symbol, const int limit, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "orderBook:" + market["symbol"].get<std::string>();
    
    json request = {
        {"op", "subscribe"},
        {"args", {"orderBook10:" + getSymbolId(symbol)}}  // Using L10 (10 levels) orderbook
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response bitmex_ws::watchOHLCV(const std::string& symbol, const std::string& timeframe, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "tradeBin" + timeframe + ":" + market["symbol"].get<std::string>();
    
    json request = {
        {"op", "subscribe"},
        {"args", {"tradeBin" + timeframe + ":" + getSymbolId(symbol)}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response bitmex_ws::watchBalance(const Dict& params) {
    if (!this->authenticated) {
        this->authenticate();
    }
    
    std::string messageHash = "margin";
    
    json request = {
        {"op", "subscribe"},
        {"args", {"margin"}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response bitmex_ws::watchOrders(const std::string& symbol, const Dict& params) {
    if (!this->authenticated) {
        this->authenticate();
    }
    
    std::string messageHash = symbol.empty() ? "order" : "order:" + symbol;
    std::string channel = "order";
    if (!symbol.empty()) {
        channel = "order:" + getSymbolId(symbol);
    }
    
    json request = {
        {"op", "subscribe"},
        {"args", {channel}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response bitmex_ws::watchMyTrades(const std::string& symbol, const Dict& params) {
    if (!this->authenticated) {
        this->authenticate();
    }
    
    std::string messageHash = symbol.empty() ? "execution" : "execution:" + symbol;
    std::string channel = "execution";
    if (!symbol.empty()) {
        channel = "execution:" + getSymbolId(symbol);
    }
    
    json request = {
        {"op", "subscribe"},
        {"args", {channel}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response bitmex_ws::watchPositions(const std::string& symbol, const Dict& params) {
    if (!this->authenticated) {
        this->authenticate();
    }
    
    std::string messageHash = symbol.empty() ? "position" : "position:" + symbol;
    std::string channel = "position";
    if (!symbol.empty()) {
        channel = "position:" + getSymbolId(symbol);
    }
    
    json request = {
        {"op", "subscribe"},
        {"args", {channel}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

void bitmex_ws::authenticate(const Dict& params) {
    if (this->authenticated) {
        return;
    }
    
    this->expires = std::chrono::system_clock::now().time_since_epoch().count() / 1000000 + 10000;  // Expires in 10 seconds
    std::string signature = this->hmac("GET/realtime" + std::to_string(this->expires), this->config_.secret, "sha256");
    
    json request = {
        {"op", "authKeyExpires"},
        {"args", {
            this->config_.apiKey,
            this->expires,
            signature
        }}
    };
    
    this->send(request);
}

std::string bitmex_ws::getSymbolId(const std::string& symbol) {
    auto market = this->market(symbol);
    return market["id"].get<std::string>();
}

void bitmex_ws::handleMessage(const json& message) {
    if (message.contains("table")) {
        std::string table = message["table"].get<std::string>();
        
        if (table == "instrument") {
            handleTickerMessage(message);
        } else if (table == "trade") {
            handleTradesMessage(message);
        } else if (table == "orderBook10") {
            handleOrderBookMessage(message);
        } else if (table.find("tradeBin") == 0) {
            handleOHLCVMessage(message);
        } else if (table == "margin") {
            handleBalanceMessage(message);
        } else if (table == "order") {
            handleOrderMessage(message);
        } else if (table == "execution") {
            handleMyTradesMessage(message);
        } else if (table == "position") {
            handlePositionMessage(message);
        }
    } else if (message.contains("success")) {
        if (message.contains("request") && message["request"].contains("op")) {
            std::string op = message["request"]["op"].get<std::string>();
            if (op == "authKeyExpires") {
                handleAuthenticationMessage(message);
            } else {
                handleSubscriptionStatus(message);
            }
        }
    }
}

void bitmex_ws::handleTickerMessage(const json& message) {
    auto data = message["data"];
    for (const auto& ticker : data) {
        std::string symbolId = ticker["symbol"].get<std::string>();
        auto market = this->marketById(symbolId);
        std::string symbol = market["symbol"].get<std::string>();
        
        Ticker tickerObj;
        tickerObj.symbol = symbol;
        tickerObj.timestamp = this->parse8601(ticker["timestamp"].get<std::string>());
        tickerObj.datetime = this->iso8601(tickerObj.timestamp);
        tickerObj.high = this->safeFloat(ticker, "highPrice");
        tickerObj.low = this->safeFloat(ticker, "lowPrice");
        tickerObj.bid = this->safeFloat(ticker, "bidPrice");
        tickerObj.ask = this->safeFloat(ticker, "askPrice");
        tickerObj.last = this->safeFloat(ticker, "lastPrice");
        tickerObj.open = this->safeFloat(ticker, "prevClosePrice");
        tickerObj.baseVolume = this->safeFloat(ticker, "volume");
        tickerObj.info = ticker;
        
        this->tickers[symbol] = tickerObj;
        this->emit("instrument:" + symbol, tickerObj);
    }
}

void bitmex_ws::handleTradesMessage(const json& message) {
    auto data = message["data"];
    for (const auto& trade : data) {
        std::string symbolId = trade["symbol"].get<std::string>();
        auto market = this->marketById(symbolId);
        std::string symbol = market["symbol"].get<std::string>();
        
        Trade tradeObj;
        tradeObj.symbol = symbol;
        tradeObj.id = this->safeString(trade, "trdMatchID");
        tradeObj.timestamp = this->parse8601(trade["timestamp"].get<std::string>());
        tradeObj.datetime = this->iso8601(tradeObj.timestamp);
        tradeObj.price = this->safeFloat(trade, "price");
        tradeObj.amount = this->safeFloat(trade, "size");
        tradeObj.side = trade["side"].get<std::string>();
        tradeObj.info = trade;
        
        if (this->trades.find(symbol) == this->trades.end()) {
            this->trades[symbol] = std::vector<Trade>();
        }
        this->trades[symbol].push_back(tradeObj);
        this->emit("trade:" + symbol, tradeObj);
    }
}

void bitmex_ws::handleOrderBookMessage(const json& message) {
    auto data = message["data"][0];
    std::string symbolId = data["symbol"].get<std::string>();
    auto market = this->marketById(symbolId);
    std::string symbol = market["symbol"].get<std::string>();
    
    if (this->orderbooks.find(symbol) == this->orderbooks.end()) {
        this->orderbooks[symbol] = OrderBook();
    }
    
    OrderBook& orderbook = this->orderbooks[symbol];
    orderbook.symbol = symbol;
    orderbook.timestamp = this->parse8601(data["timestamp"].get<std::string>());
    orderbook.datetime = this->iso8601(orderbook.timestamp);
    
    orderbook.bids.clear();
    orderbook.asks.clear();
    
    for (const auto& bid : data["bids"]) {
        orderbook.bids.push_back({
            this->safeFloat(bid, 0),  // price
            this->safeFloat(bid, 1)   // amount
        });
    }
    
    for (const auto& ask : data["asks"]) {
        orderbook.asks.push_back({
            this->safeFloat(ask, 0),  // price
            this->safeFloat(ask, 1)   // amount
        });
    }
    
    this->emit("orderBook:" + symbol, orderbook);
}

void bitmex_ws::handleOHLCVMessage(const json& message) {
    auto data = message["data"];
    for (const auto& candle : data) {
        std::string symbolId = candle["symbol"].get<std::string>();
        auto market = this->marketById(symbolId);
        std::string symbol = market["symbol"].get<std::string>();
        std::string timeframe = message["table"].get<std::string>().substr(8);  // Remove "tradeBin" prefix
        
        OHLCV ohlcv;
        ohlcv.timestamp = this->parse8601(candle["timestamp"].get<std::string>());
        ohlcv.open = this->safeFloat(candle, "open");
        ohlcv.high = this->safeFloat(candle, "high");
        ohlcv.low = this->safeFloat(candle, "low");
        ohlcv.close = this->safeFloat(candle, "close");
        ohlcv.volume = this->safeFloat(candle, "volume");
        
        std::string key = symbol + ":" + timeframe;
        if (this->ohlcvs.find(key) == this->ohlcvs.end()) {
            this->ohlcvs[key] = std::vector<OHLCV>();
        }
        this->ohlcvs[key].push_back(ohlcv);
        
        this->emit("tradeBin" + timeframe + ":" + symbol, ohlcv);
    }
}

void bitmex_ws::handleBalanceMessage(const json& message) {
    auto data = message["data"][0];
    Balance balance;
    balance.timestamp = this->parse8601(data["timestamp"].get<std::string>());
    balance.datetime = this->iso8601(balance.timestamp);
    
    std::string currency = "XBt";  // BitMEX uses Satoshis
    balance.free[currency] = this->safeFloat(data, "availableMargin") / 100000000.0;  // Convert Satoshis to BTC
    balance.used[currency] = this->safeFloat(data, "marginBalance") / 100000000.0 - balance.free[currency];
    balance.total[currency] = this->safeFloat(data, "marginBalance") / 100000000.0;
    
    this->emit("margin", balance);
}

void bitmex_ws::handleOrderMessage(const json& message) {
    auto data = message["data"];
    for (const auto& order : data) {
        std::string symbolId = order["symbol"].get<std::string>();
        auto market = this->marketById(symbolId);
        std::string symbol = market["symbol"].get<std::string>();
        
        Order orderObj;
        orderObj.id = this->safeString(order, "orderID");
        orderObj.clientOrderId = this->safeString(order, "clOrdID");
        orderObj.timestamp = this->parse8601(order["timestamp"].get<std::string>());
        orderObj.datetime = this->iso8601(orderObj.timestamp);
        orderObj.lastTradeTimestamp = nullptr;
        orderObj.symbol = symbol;
        orderObj.type = this->safeString(order, "ordType");
        orderObj.side = this->safeString(order, "side");
        orderObj.price = this->safeFloat(order, "price");
        orderObj.amount = this->safeFloat(order, "orderQty");
        orderObj.cost = nullptr;
        orderObj.average = this->safeFloat(order, "avgPx");
        orderObj.filled = this->safeFloat(order, "cumQty");
        orderObj.remaining = this->safeFloat(order, "leavesQty");
        orderObj.status = this->safeString(order, "ordStatus");
        orderObj.fee = nullptr;
        orderObj.trades = nullptr;
        orderObj.info = order;
        
        std::string messageHash = "order:" + symbol;
        this->emit(messageHash, orderObj);
    }
}

void bitmex_ws::handleMyTradesMessage(const json& message) {
    auto data = message["data"];
    for (const auto& trade : data) {
        std::string symbolId = trade["symbol"].get<std::string>();
        auto market = this->marketById(symbolId);
        std::string symbol = market["symbol"].get<std::string>();
        
        Trade tradeObj;
        tradeObj.id = this->safeString(trade, "execID");
        tradeObj.order = this->safeString(trade, "orderID");
        tradeObj.timestamp = this->parse8601(trade["timestamp"].get<std::string>());
        tradeObj.datetime = this->iso8601(tradeObj.timestamp);
        tradeObj.symbol = symbol;
        tradeObj.type = this->safeString(trade, "ordType");
        tradeObj.side = this->safeString(trade, "side");
        tradeObj.price = this->safeFloat(trade, "price");
        tradeObj.amount = this->safeFloat(trade, "lastQty");
        tradeObj.cost = this->safeFloat(trade, "lastQty") * this->safeFloat(trade, "price");
        tradeObj.fee = {
            {"cost", this->safeFloat(trade, "commission")},
            {"currency", "XBt"}
        };
        tradeObj.info = trade;
        
        std::string messageHash = "execution:" + symbol;
        this->emit(messageHash, tradeObj);
    }
}

void bitmex_ws::handlePositionMessage(const json& message) {
    auto data = message["data"];
    for (const auto& pos : data) {
        std::string symbolId = pos["symbol"].get<std::string>();
        auto market = this->marketById(symbolId);
        std::string symbol = market["symbol"].get<std::string>();
        
        Position position;
        position.info = pos;
        position.symbol = symbol;
        position.timestamp = this->parse8601(pos["timestamp"].get<std::string>());
        position.datetime = this->iso8601(position.timestamp);
        position.contracts = this->safeFloat(pos, "currentQty");
        position.contractSize = this->safeFloat(pos, "multiplier");
        position.entryPrice = this->safeFloat(pos, "avgEntryPrice");
        position.unrealizedPnl = this->safeFloat(pos, "unrealisedPnl") / 100000000.0;  // Convert from Satoshis
        position.leverage = this->safeFloat(pos, "leverage");
        position.liquidationPrice = this->safeFloat(pos, "liquidationPrice");
        position.collateral = this->safeFloat(pos, "maintMargin") / 100000000.0;  // Convert from Satoshis
        position.notional = this->safeFloat(pos, "homeNotional");
        position.percentage = this->safeFloat(pos, "unrealisedPnlPcnt") * 100;
        
        this->positions[symbol] = position;
        std::string messageHash = "position:" + symbol;
        this->emit(messageHash, position);
    }
}

void bitmex_ws::handleAuthenticationMessage(const json& message) {
    if (message["success"].get<bool>()) {
        this->authenticated = true;
        this->emit("authenticated", message);
    } else {
        throw ExchangeError(message["error"].get<std::string>());
    }
}

void bitmex_ws::handleSubscriptionStatus(const json& message) {
    if (!message["success"].get<bool>()) {
        throw ExchangeError(message["error"].get<std::string>());
    }
}

void bitmex_ws::handleError(const json& message) {
    if (message.contains("error")) {
        throw ExchangeError(message["error"].get<std::string>());
    }
}

} // namespace ccxt
