#include "exchanges/ws/cex_ws.h"
#include "base/json_helper.h"
#include <chrono>

namespace ccxt {

cex_ws::cex_ws() : exchange_ws() {
    this->urls["ws"] = "wss://ws.cex.io/ws";
    this->urls["api"] = "https://cex.io/api";
    
    this->options["watchOrderBook"]["snapshotDelay"] = 0;
    this->authenticated = false;
    this->pingInterval = 10000;  // 10 seconds
    this->lastPingTimestamp = 0;
}

Response cex_ws::watchTicker(const std::string& symbol, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "ticker:" + market["id"].get<std::string>();
    
    json request = {
        {"e", "subscribe"},
        {"rooms", {
            {"pair", {market["baseId"].get<std::string>() + "/" + market["quoteId"].get<std::string>()}},
            {"data", {"tickers"}}
        }}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response cex_ws::watchTrades(const std::string& symbol, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "trades:" + market["id"].get<std::string>();
    
    json request = {
        {"e", "subscribe"},
        {"rooms", {
            {"pair", {market["baseId"].get<std::string>() + "/" + market["quoteId"].get<std::string>()}},
            {"data", {"trades"}}
        }}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response cex_ws::watchOrderBook(const std::string& symbol, const int limit, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "orderbook:" + market["id"].get<std::string>();
    
    json request = {
        {"e", "subscribe"},
        {"rooms", {
            {"pair", {market["baseId"].get<std::string>() + "/" + market["quoteId"].get<std::string>()}},
            {"data", {"pair"}}
        }}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response cex_ws::watchOHLCV(const std::string& symbol, const std::string& timeframe, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "ohlcv:" + timeframe + ":" + market["id"].get<std::string>();
    
    json request = {
        {"e", "subscribe"},
        {"rooms", {
            {"pair", {market["baseId"].get<std::string>() + "/" + market["quoteId"].get<std::string>()}},
            {"data", {"ohlcv1m"}}  // CEX.IO only supports 1-minute candles
        }}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response cex_ws::watchBalance(const Dict& params) {
    if (!this->authenticated) {
        this->authenticate();
    }
    
    json request = {
        {"e", "subscribe"},
        {"rooms", {"balance"}}
    };
    
    return this->watch(this->urls["ws"], "balance", request, "balance");
}

Response cex_ws::watchOrders(const std::string& symbol, const Dict& params) {
    if (!this->authenticated) {
        this->authenticate();
    }
    
    std::string messageHash = symbol.empty() ? "orders" : "orders:" + this->market(symbol)["id"].get<std::string>();
    
    json request = {
        {"e", "subscribe"},
        {"rooms", {"orders"}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response cex_ws::watchMyTrades(const std::string& symbol, const Dict& params) {
    if (!this->authenticated) {
        this->authenticate();
    }
    
    std::string messageHash = symbol.empty() ? "trades" : "trades:" + this->market(symbol)["id"].get<std::string>();
    
    json request = {
        {"e", "subscribe"},
        {"rooms", {"trades"}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

void cex_ws::authenticate(const Dict& params) {
    if (this->authenticated) {
        return;
    }
    
    std::string timestamp = std::to_string(std::chrono::system_clock::now().time_since_epoch().count() / 1000000);
    std::string nonce = timestamp;
    std::string signature = this->getSignature(timestamp, nonce);
    
    json request = {
        {"e", "auth"},
        {"auth", {
            {"key", this->apiKey},
            {"signature", signature},
            {"timestamp", timestamp}
        }}
    };
    
    this->send(request);
}

std::string cex_ws::getSignature(const std::string& timestamp, const std::string& nonce) {
    std::string message = timestamp + this->apiKey + nonce;
    return this->hmac(message, this->secret, "sha256");
}

std::string cex_ws::getSymbolId(const std::string& symbol) {
    auto market = this->market(symbol);
    return market["id"].get<std::string>();
}

void cex_ws::handleMessage(const json& message) {
    // Handle ping/pong
    int64_t currentTime = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
    if (currentTime - this->lastPingTimestamp > this->pingInterval) {
        json pingMessage = {{"e", "ping"}};
        this->send(pingMessage);
        this->lastPingTimestamp = currentTime;
    }
    
    if (message.contains("e")) {
        std::string event = message["e"].get<std::string>();
        
        if (event == "ping") {
            json pongMessage = {{"e", "pong"}};
            this->send(pongMessage);
        } else if (event == "auth") {
            handleAuthenticationMessage(message);
        } else if (event == "subscribe") {
            handleSubscriptionStatus(message);
        } else if (event == "tick") {
            handleTickerMessage(message);
        } else if (event == "trade") {
            handleTradesMessage(message);
        } else if (event == "md") {
            handleOrderBookMessage(message);
        } else if (event == "ohlcv") {
            handleOHLCVMessage(message);
        } else if (event == "balance") {
            handleBalanceMessage(message);
        } else if (event == "order") {
            handleOrderMessage(message);
        } else if (event == "tx") {
            handleMyTradesMessage(message);
        } else if (event == "error") {
            handleError(message);
        }
    }
}

void cex_ws::handleTickerMessage(const json& message) {
    auto data = message["data"];
    std::string pair = data["pair"].get<std::string>();
    
    Ticker ticker;
    ticker.symbol = this->marketId(pair);
    ticker.timestamp = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
    ticker.datetime = this->iso8601(ticker.timestamp);
    ticker.high = this->safeFloat(data, "high");
    ticker.low = this->safeFloat(data, "low");
    ticker.bid = this->safeFloat(data, "bid");
    ticker.ask = this->safeFloat(data, "ask");
    ticker.last = this->safeFloat(data, "last");
    ticker.open = this->safeFloat(data, "open24");
    ticker.close = ticker.last;
    ticker.baseVolume = this->safeFloat(data, "volume");
    ticker.quoteVolume = this->safeFloat(data, "volume30d");
    ticker.info = data;
    
    this->tickers[ticker.symbol] = ticker;
    this->emit("ticker:" + pair, ticker);
}

void cex_ws::handleTradesMessage(const json& message) {
    auto data = message["data"];
    std::string pair = data["pair"].get<std::string>();
    auto symbol = this->marketId(pair);
    
    Trade trade;
    trade.symbol = symbol;
    trade.id = this->safeString(data, "id");
    trade.timestamp = this->safeInteger(data, "time");
    trade.datetime = this->iso8601(trade.timestamp);
    trade.side = this->safeString(data, "type");
    trade.price = this->safeFloat(data, "price");
    trade.amount = this->safeFloat(data, "amount");
    trade.cost = trade.price * trade.amount;
    trade.info = data;
    
    if (this->trades.find(symbol) == this->trades.end()) {
        this->trades[symbol] = std::vector<Trade>();
    }
    this->trades[symbol].push_back(trade);
    this->emit("trades:" + pair, trade);
}

void cex_ws::handleOrderBookMessage(const json& message) {
    auto data = message["data"];
    std::string pair = data["pair"].get<std::string>();
    auto symbol = this->marketId(pair);
    
    if (this->orderbooks.find(symbol) == this->orderbooks.end()) {
        this->orderbooks[symbol] = OrderBook();
    }
    
    OrderBook& orderbook = this->orderbooks[symbol];
    orderbook.symbol = symbol;
    orderbook.timestamp = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
    orderbook.datetime = this->iso8601(orderbook.timestamp);
    
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
    
    this->emit("orderbook:" + pair, orderbook);
}

void cex_ws::handleOHLCVMessage(const json& message) {
    auto data = message["data"];
    std::string pair = data["pair"].get<std::string>();
    auto symbol = this->marketId(pair);
    
    OHLCV ohlcv;
    ohlcv.timestamp = this->safeInteger(data, "timestamp");
    ohlcv.open = this->safeFloat(data, "open");
    ohlcv.high = this->safeFloat(data, "high");
    ohlcv.low = this->safeFloat(data, "low");
    ohlcv.close = this->safeFloat(data, "close");
    ohlcv.volume = this->safeFloat(data, "volume");
    
    std::string key = symbol + ":1m";  // CEX.IO only supports 1-minute candles
    if (this->ohlcvs.find(key) == this->ohlcvs.end()) {
        this->ohlcvs[key] = std::vector<OHLCV>();
    }
    this->ohlcvs[key].push_back(ohlcv);
    
    this->emit("ohlcv:1m:" + pair, ohlcv);
}

void cex_ws::handleBalanceMessage(const json& message) {
    auto data = message["data"];
    Balance balance;
    balance.timestamp = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
    balance.datetime = this->iso8601(balance.timestamp);
    
    for (const auto& item : data.items()) {
        std::string currency = item.key();
        auto balanceData = item.value();
        
        balance.free[currency] = this->safeFloat(balanceData, "available");
        balance.used[currency] = this->safeFloat(balanceData, "orders");
        balance.total[currency] = this->safeFloat(balanceData, "balance");
    }
    
    this->emit("balance", balance);
}

void cex_ws::handleOrderMessage(const json& message) {
    auto data = message["data"];
    std::string pair = data["pair"].get<std::string>();
    auto symbol = this->marketId(pair);
    
    Order order;
    order.id = this->safeString(data, "id");
    order.clientOrderId = this->safeString(data, "client_order_id");
    order.timestamp = this->safeInteger(data, "time");
    order.datetime = this->iso8601(order.timestamp);
    order.lastTradeTimestamp = nullptr;
    order.symbol = symbol;
    order.type = this->safeString(data, "type");
    order.side = this->safeString(data, "side");
    order.price = this->safeFloat(data, "price");
    order.amount = this->safeFloat(data, "amount");
    order.cost = this->safeFloat(data, "total");
    order.average = this->safeFloat(data, "fa:vwap");
    order.filled = this->safeFloat(data, "fa:executed");
    order.remaining = order.amount - order.filled;
    order.status = this->safeString(data, "status");
    order.fee = {
        {"cost", this->safeFloat(data, "fa:fee")},
        {"currency", this->safeString(data, "fa:fee_currency")}
    };
    order.trades = nullptr;
    order.info = data;
    
    this->emit("orders:" + pair, order);
}

void cex_ws::handleMyTradesMessage(const json& message) {
    auto data = message["data"];
    std::string pair = data["pair"].get<std::string>();
    auto symbol = this->marketId(pair);
    
    Trade trade;
    trade.id = this->safeString(data, "id");
    trade.order = this->safeString(data, "order_id");
    trade.timestamp = this->safeInteger(data, "time");
    trade.datetime = this->iso8601(trade.timestamp);
    trade.symbol = symbol;
    trade.type = "limit";
    trade.side = this->safeString(data, "type");
    trade.price = this->safeFloat(data, "price");
    trade.amount = this->safeFloat(data, "amount");
    trade.cost = trade.price * trade.amount;
    trade.fee = {
        {"cost", this->safeFloat(data, "fee")},
        {"currency", this->safeString(data, "fee_currency")}
    };
    trade.info = data;
    
    this->emit("trades:" + pair, trade);
}

void cex_ws::handleAuthenticationMessage(const json& message) {
    if (message.contains("ok") && message["ok"].get<bool>()) {
        this->authenticated = true;
        this->emit("authenticated", message);
    } else {
        throw ExchangeError(message["data"]["error"].get<std::string>());
    }
}

void cex_ws::handleSubscriptionStatus(const json& message) {
    if (!message.contains("ok") || !message["ok"].get<bool>()) {
        throw ExchangeError("Subscription failed");
    }
}

void cex_ws::handleError(const json& message) {
    throw ExchangeError(message["data"]["error"].get<std::string>());
}

} // namespace ccxt
