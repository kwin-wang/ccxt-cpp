#include "exchanges/ws/bithumb_ws.h"
#include "base/json_helper.h"

namespace ccxt {

bithumb_ws::bithumb_ws() : exchange_ws() {
    this->urls["ws"] = "wss://pubwss.bithumb.com/pub/ws";
    this->options["watchOrderBook"]["snapshotDelay"] = 0;
}

Response bithumb_ws::watchTicker(const std::string& symbol, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "ticker:" + market["symbol"].get<std::string>();
    
    json request = {
        {"type", "ticker"},
        {"symbols", {getSymbolId(symbol)}},
        {"tickTypes", {get_value(params, "tickTypes", "24H")}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response bithumb_ws::watchTickers(const std::vector<std::string>& symbols, const Dict& params) {
    std::vector<std::string> symbolIds;
    std::vector<std::string> messageHashes;
    
    for (const auto& symbol : symbols) {
        auto market = this->market(symbol);
        symbolIds.push_back(getSymbolId(symbol));
        messageHashes.push_back("ticker:" + market["symbol"].get<std::string>());
    }
    
    json request = {
        {"type", "ticker"},
        {"symbols", symbolIds},
        {"tickTypes", {get_value(params, "tickTypes", "24H")}}
    };
    
    return this->watchMultiple(this->urls["ws"], messageHashes, request, messageHashes);
}

Response bithumb_ws::watchTrades(const std::string& symbol, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "trades:" + market["symbol"].get<std::string>();
    
    json request = {
        {"type", "transaction"},
        {"symbols", {getSymbolId(symbol)}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response bithumb_ws::watchOrderBook(const std::string& symbol, const int limit, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "orderbook:" + market["symbol"].get<std::string>();
    
    json request = {
        {"type", "orderbookdepth"},
        {"symbols", {getSymbolId(symbol)}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

std::string bithumb_ws::getSymbolId(const std::string& symbol) {
    auto market = this->market(symbol);
    return market["base"].get<std::string>() + "_" + market["quote"].get<std::string>();
}

void bithumb_ws::handleMessage(const json& message) {
    if (message.contains("type")) {
        std::string type = message["type"].get<std::string>();
        
        if (type == "ticker") {
            handleTickerMessage(message);
        } else if (type == "transaction") {
            handleTradesMessage(message);
        } else if (type == "orderbookdepth") {
            handleOrderBookMessage(message);
        }
    }
}

void bithumb_ws::handleTickerMessage(const json& message) {
    if (!message.contains("content")) {
        return;
    }
    
    auto content = message["content"];
    std::string symbolId = content["symbol"].get<std::string>();
    auto market = this->marketBySymbolId(symbolId);
    std::string symbol = market["symbol"].get<std::string>();
    
    Ticker ticker;
    ticker.symbol = symbol;
    ticker.timestamp = this->parse8601(content["date"].get<std::string>() + " " + content["time"].get<std::string>());
    ticker.datetime = this->iso8601(ticker.timestamp);
    ticker.high = this->safeFloat(content, "highPrice");
    ticker.low = this->safeFloat(content, "lowPrice");
    ticker.bid = this->safeFloat(content, "buyPrice");
    ticker.ask = this->safeFloat(content, "sellPrice");
    ticker.last = this->safeFloat(content, "closePrice");
    ticker.open = this->safeFloat(content, "openPrice");
    ticker.baseVolume = this->safeFloat(content, "volume");
    ticker.info = content;
    
    this->tickers[symbol] = ticker;
    this->emit("ticker:" + symbol, ticker);
}

void bithumb_ws::handleTradesMessage(const json& message) {
    if (!message.contains("content")) {
        return;
    }
    
    auto content = message["content"];
    std::string symbolId = content["symbol"].get<std::string>();
    auto market = this->marketBySymbolId(symbolId);
    std::string symbol = market["symbol"].get<std::string>();
    
    Trade trade;
    trade.symbol = symbol;
    trade.timestamp = this->parse8601(content["date"].get<std::string>() + " " + content["time"].get<std::string>());
    trade.datetime = this->iso8601(trade.timestamp);
    trade.price = this->safeFloat(content, "price");
    trade.amount = this->safeFloat(content, "quantity");
    trade.side = content["buySellGb"].get<std::string>() == "1" ? "buy" : "sell";
    trade.info = content;
    
    if (this->trades.find(symbol) == this->trades.end()) {
        this->trades[symbol] = std::vector<Trade>();
    }
    this->trades[symbol].push_back(trade);
    this->emit("trades:" + symbol, trade);
}

void bithumb_ws::handleOrderBookMessage(const json& message) {
    if (!message.contains("content")) {
        return;
    }
    
    auto content = message["content"];
    std::string symbolId = content["symbol"].get<std::string>();
    auto market = this->marketBySymbolId(symbolId);
    std::string symbol = market["symbol"].get<std::string>();
    
    if (this->orderbooks.find(symbol) == this->orderbooks.end()) {
        this->orderbooks[symbol] = OrderBook();
    }
    
    OrderBook& orderbook = this->orderbooks[symbol];
    orderbook.symbol = symbol;
    orderbook.timestamp = this->parse8601(content["date"].get<std::string>() + " " + content["time"].get<std::string>());
    orderbook.datetime = this->iso8601(orderbook.timestamp);
    
    if (content.contains("asks")) {
        for (const auto& ask : content["asks"]) {
            orderbook.asks.push_back({
                this->safeFloat(ask, "price"),
                this->safeFloat(ask, "quantity")
            });
        }
    }
    
    if (content.contains("bids")) {
        for (const auto& bid : content["bids"]) {
            orderbook.bids.push_back({
                this->safeFloat(bid, "price"),
                this->safeFloat(bid, "quantity")
            });
        }
    }
    
    this->emit("orderbook:" + symbol, orderbook);
}

void bithumb_ws::handleError(const json& message) {
    // Handle any error messages from the WebSocket connection
    if (message.contains("message")) {
        throw ExchangeError(message["message"].get<std::string>());
    }
}

} // namespace ccxt
