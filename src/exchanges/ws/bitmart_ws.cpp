#include "exchanges/ws/bitmart_ws.h"
#include "base/json_helper.h"
#include <chrono>

namespace ccxt {

bitmart_ws::bitmart_ws() : exchange_ws() {
    this->urls["ws"] = "wss://ws-manager-compress.bitmart.com/api?protocol=1.1";
    this->urls["wsPublic"] = "wss://ws-manager-compress.bitmart.com/api?protocol=1.1";
    this->urls["wsPrivate"] = "wss://ws-manager-compress.bitmart.com/user?protocol=1.1";
    
    this->options["watchOrderBook"]["snapshotDelay"] = 0;
    this->authenticated = false;
}

Response bitmart_ws::watchTicker(const std::string& symbol, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "spot/ticker:" + market["symbol"].get<std::string>();
    
    json request = {
        {"op", "subscribe"},
        {"args", {{"channel", "spot/ticker"}, {"instId", getSymbolId(symbol)}}}
    };
    
    return this->watch(this->urls["wsPublic"], messageHash, request, messageHash);
}

Response bitmart_ws::watchTickers(const std::vector<std::string>& symbols, const Dict& params) {
    std::vector<std::string> messageHashes;
    std::vector<json> args;
    
    for (const auto& symbol : symbols) {
        auto market = this->market(symbol);
        messageHashes.push_back("spot/ticker:" + market["symbol"].get<std::string>());
        args.push_back({{"channel", "spot/ticker"}, {"instId", getSymbolId(symbol)}});
    }
    
    json request = {
        {"op", "subscribe"},
        {"args", args}
    };
    
    return this->watchMultiple(this->urls["wsPublic"], messageHashes, request, messageHashes);
}

Response bitmart_ws::watchTrades(const std::string& symbol, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "spot/trade:" + market["symbol"].get<std::string>();
    
    json request = {
        {"op", "subscribe"},
        {"args", {{"channel", "spot/trade"}, {"instId", getSymbolId(symbol)}}}
    };
    
    return this->watch(this->urls["wsPublic"], messageHash, request, messageHash);
}

Response bitmart_ws::watchOrderBook(const std::string& symbol, const int limit, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "spot/depth:" + market["symbol"].get<std::string>();
    
    json request = {
        {"op", "subscribe"},
        {"args", {{"channel", "spot/depth"}, {"instId", getSymbolId(symbol)}}}
    };
    
    return this->watch(this->urls["wsPublic"], messageHash, request, messageHash);
}

Response bitmart_ws::watchOHLCV(const std::string& symbol, const std::string& timeframe, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "spot/kline:" + market["symbol"].get<std::string>() + ":" + timeframe;
    
    json request = {
        {"op", "subscribe"},
        {"args", {{"channel", "spot/kline"}, {"instId", getSymbolId(symbol)}, {"period", timeframe}}}
    };
    
    return this->watch(this->urls["wsPublic"], messageHash, request, messageHash);
}

Response bitmart_ws::watchBalance(const Dict& params) {
    if (!this->authenticated) {
        this->authenticate();
    }
    
    std::string messageHash = "spot/wallet";
    
    json request = {
        {"op", "subscribe"},
        {"args", {{"channel", "spot/wallet"}}}
    };
    
    return this->watch(this->urls["wsPrivate"], messageHash, request, messageHash);
}

Response bitmart_ws::watchOrders(const std::string& symbol, const Dict& params) {
    if (!this->authenticated) {
        this->authenticate();
    }
    
    std::string messageHash = symbol.empty() ? "spot/order" : "spot/order:" + symbol;
    
    json args = {{"channel", "spot/order"}};
    if (!symbol.empty()) {
        args["instId"] = getSymbolId(symbol);
    }
    
    json request = {
        {"op", "subscribe"},
        {"args", {args}}
    };
    
    return this->watch(this->urls["wsPrivate"], messageHash, request, messageHash);
}

Response bitmart_ws::watchMyTrades(const std::string& symbol, const Dict& params) {
    if (!this->authenticated) {
        this->authenticate();
    }
    
    std::string messageHash = symbol.empty() ? "spot/trade" : "spot/trade:" + symbol;
    
    json args = {{"channel", "spot/trade"}};
    if (!symbol.empty()) {
        args["instId"] = getSymbolId(symbol);
    }
    
    json request = {
        {"op", "subscribe"},
        {"args", {args}}
    };
    
    return this->watch(this->urls["wsPrivate"], messageHash, request, messageHash);
}

void bitmart_ws::authenticate(const Dict& params) {
    if (this->authenticated) {
        return;
    }
    
    std::string timestamp = std::to_string(std::chrono::system_clock::now().time_since_epoch().count() / 1000000);
    std::string message = timestamp + "#" + this->apiKey + "#" + this->secret;
    std::string signature = this->hmac(message, this->secret, "sha256");
    
    json request = {
        {"op", "login"},
        {"args", {
            {"apiKey", this->apiKey},
            {"timestamp", timestamp},
            {"sign", signature}
        }}
    };
    
    this->send(request);
}

std::string bitmart_ws::getSymbolId(const std::string& symbol) {
    auto market = this->market(symbol);
    return market["id"].get<std::string>();
}

void bitmart_ws::handleMessage(const json& message) {
    if (message.contains("event")) {
        std::string event = message["event"].get<std::string>();
        
        if (event == "login") {
            handleAuthenticationMessage(message);
            return;
        } else if (event == "subscribe") {
            handleSubscriptionStatus(message);
            return;
        }
    }
    
    if (message.contains("data") && message.contains("table")) {
        std::string table = message["table"].get<std::string>();
        
        if (table == "spot/ticker") {
            handleTickerMessage(message);
        } else if (table == "spot/trade") {
            handleTradesMessage(message);
        } else if (table == "spot/depth") {
            handleOrderBookMessage(message);
        } else if (table == "spot/kline") {
            handleOHLCVMessage(message);
        } else if (table == "spot/wallet") {
            handleBalanceMessage(message);
        } else if (table == "spot/order") {
            handleOrderMessage(message);
        }
    }
}

void bitmart_ws::handleTickerMessage(const json& message) {
    auto data = message["data"][0];
    std::string symbolId = data["instId"].get<std::string>();
    auto market = this->marketById(symbolId);
    std::string symbol = market["symbol"].get<std::string>();
    
    Ticker ticker;
    ticker.symbol = symbol;
    ticker.timestamp = this->safeInteger(data, "timestamp");
    ticker.datetime = this->iso8601(ticker.timestamp);
    ticker.high = this->safeFloat(data, "high_24h");
    ticker.low = this->safeFloat(data, "low_24h");
    ticker.bid = this->safeFloat(data, "best_bid");
    ticker.ask = this->safeFloat(data, "best_ask");
    ticker.last = this->safeFloat(data, "last");
    ticker.open = this->safeFloat(data, "open_24h");
    ticker.baseVolume = this->safeFloat(data, "base_volume_24h");
    ticker.quoteVolume = this->safeFloat(data, "quote_volume_24h");
    ticker.info = data;
    
    this->tickers[symbol] = ticker;
    this->emit("spot/ticker:" + symbol, ticker);
}

void bitmart_ws::handleTradesMessage(const json& message) {
    auto data = message["data"];
    for (const auto& trade : data) {
        std::string symbolId = trade["instId"].get<std::string>();
        auto market = this->marketById(symbolId);
        std::string symbol = market["symbol"].get<std::string>();
        
        Trade tradeObj;
        tradeObj.symbol = symbol;
        tradeObj.id = this->safeString(trade, "tradeId");
        tradeObj.timestamp = this->safeInteger(trade, "timestamp");
        tradeObj.datetime = this->iso8601(tradeObj.timestamp);
        tradeObj.price = this->safeFloat(trade, "price");
        tradeObj.amount = this->safeFloat(trade, "size");
        tradeObj.side = trade["side"].get<std::string>();
        tradeObj.info = trade;
        
        if (this->trades.find(symbol) == this->trades.end()) {
            this->trades[symbol] = std::vector<Trade>();
        }
        this->trades[symbol].push_back(tradeObj);
        this->emit("spot/trade:" + symbol, tradeObj);
    }
}

void bitmart_ws::handleOrderBookMessage(const json& message) {
    auto data = message["data"][0];
    std::string symbolId = data["instId"].get<std::string>();
    auto market = this->marketById(symbolId);
    std::string symbol = market["symbol"].get<std::string>();
    
    if (this->orderbooks.find(symbol) == this->orderbooks.end()) {
        this->orderbooks[symbol] = OrderBook();
    }
    
    OrderBook& orderbook = this->orderbooks[symbol];
    orderbook.symbol = symbol;
    orderbook.timestamp = this->safeInteger(data, "timestamp");
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
    
    this->emit("spot/depth:" + symbol, orderbook);
}

void bitmart_ws::handleOHLCVMessage(const json& message) {
    auto data = message["data"][0];
    std::string symbolId = data["instId"].get<std::string>();
    auto market = this->marketById(symbolId);
    std::string symbol = market["symbol"].get<std::string>();
    std::string timeframe = data["period"].get<std::string>();
    
    OHLCV ohlcv;
    ohlcv.timestamp = this->safeInteger(data, "timestamp");
    ohlcv.open = this->safeFloat(data, "open");
    ohlcv.high = this->safeFloat(data, "high");
    ohlcv.low = this->safeFloat(data, "low");
    ohlcv.close = this->safeFloat(data, "close");
    ohlcv.volume = this->safeFloat(data, "volume");
    
    std::string key = symbol + ":" + timeframe;
    if (this->ohlcvs.find(key) == this->ohlcvs.end()) {
        this->ohlcvs[key] = std::vector<OHLCV>();
    }
    this->ohlcvs[key].push_back(ohlcv);
    
    this->emit("spot/kline:" + symbol + ":" + timeframe, ohlcv);
}

void bitmart_ws::handleBalanceMessage(const json& message) {
    auto data = message["data"];
    Balance balance;
    balance.timestamp = this->safeInteger(data, "timestamp");
    balance.datetime = this->iso8601(balance.timestamp);
    
    for (const auto& item : data["balances"]) {
        std::string currency = item["currency"].get<std::string>();
        balance.free[currency] = this->safeFloat(item, "available");
        balance.used[currency] = this->safeFloat(item, "hold");
        balance.total[currency] = this->safeFloat(item, "total");
    }
    
    this->emit("spot/wallet", balance);
}

void bitmart_ws::handleOrderMessage(const json& message) {
    auto data = message["data"];
    for (const auto& order : data) {
        std::string symbolId = order["instId"].get<std::string>();
        auto market = this->marketById(symbolId);
        std::string symbol = market["symbol"].get<std::string>();
        
        Order orderObj;
        orderObj.id = this->safeString(order, "orderId");
        orderObj.clientOrderId = this->safeString(order, "clOrdId");
        orderObj.timestamp = this->safeInteger(order, "timestamp");
        orderObj.datetime = this->iso8601(orderObj.timestamp);
        orderObj.lastTradeTimestamp = nullptr;
        orderObj.symbol = symbol;
        orderObj.type = this->safeString(order, "ordType");
        orderObj.side = this->safeString(order, "side");
        orderObj.price = this->safeFloat(order, "price");
        orderObj.amount = this->safeFloat(order, "size");
        orderObj.cost = this->safeFloat(order, "notional");
        orderObj.average = this->safeFloat(order, "avgPrice");
        orderObj.filled = this->safeFloat(order, "accFillSz");
        orderObj.remaining = this->safeFloat(order, "remainSize");
        orderObj.status = this->safeString(order, "state");
        orderObj.fee = nullptr;
        orderObj.trades = nullptr;
        orderObj.info = order;
        
        std::string messageHash = "spot/order:" + symbol;
        this->emit(messageHash, orderObj);
    }
}

void bitmart_ws::handleAuthenticationMessage(const json& message) {
    if (message["code"].get<int>() == 0) {
        this->authenticated = true;
        this->emit("authenticated", message);
    } else {
        throw ExchangeError(message["msg"].get<std::string>());
    }
}

void bitmart_ws::handleSubscriptionStatus(const json& message) {
    // Handle subscription confirmation messages
    if (message["code"].get<int>() != 0) {
        throw ExchangeError(message["msg"].get<std::string>());
    }
}

void bitmart_ws::handleError(const json& message) {
    if (message.contains("code") && message["code"].get<int>() != 0) {
        throw ExchangeError(message["msg"].get<std::string>());
    }
}

} // namespace ccxt
