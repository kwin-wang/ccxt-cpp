#include "exchanges/ws/bitvavo_ws.h"
#include "base/json_helper.h"
#include <chrono>

namespace ccxt {

bitvavo_ws::bitvavo_ws() : exchange_ws() {
    this->urls["ws"] = "wss://ws.bitvavo.com/v2";
    this->urls["api"] = "https://api.bitvavo.com/v2";
    
    this->options["watchOrderBook"]["snapshotDelay"] = 0;
    this->authenticated = false;
    this->window = "24h";  // Default window for authentication
}

Response bitvavo_ws::watchTicker(const std::string& symbol, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "ticker24h:" + market["id"].get<std::string>();
    
    json request = {
        {"action", "subscribe"},
        {"channels", {{"name", "ticker24h", "markets", {market["id"].get<std::string>()}}}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response bitvavo_ws::watchTickers(const std::vector<std::string>& symbols, const Dict& params) {
    std::vector<std::string> marketIds;
    if (symbols.empty()) {
        marketIds = {"ALL"};
    } else {
        for (const auto& symbol : symbols) {
            auto market = this->market(symbol);
            marketIds.push_back(market["id"].get<std::string>());
        }
    }
    
    std::string messageHash = "ticker24h";
    
    json request = {
        {"action", "subscribe"},
        {"channels", {{"name", "ticker24h", "markets", marketIds}}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response bitvavo_ws::watchTrades(const std::string& symbol, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "trades:" + market["id"].get<std::string>();
    
    json request = {
        {"action", "subscribe"},
        {"channels", {{"name", "trades", "markets", {market["id"].get<std::string>()}}}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response bitvavo_ws::watchOrderBook(const std::string& symbol, const int limit, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "book:" + market["id"].get<std::string>();
    
    json request = {
        {"action", "subscribe"},
        {"channels", {{"name", "book", "markets", {market["id"].get<std::string>()}}}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response bitvavo_ws::watchOHLCV(const std::string& symbol, const std::string& timeframe, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "candles:" + timeframe + ":" + market["id"].get<std::string>();
    
    json request = {
        {"action", "subscribe"},
        {"channels", {{"name", "candles", "interval", timeframe, "markets", {market["id"].get<std::string>()}}}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response bitvavo_ws::watchBalance(const Dict& params) {
    if (!this->authenticated) {
        this->authenticate();
    }
    
    std::string messageHash = "account";
    
    json request = {
        {"action", "subscribe"},
        {"channels", {{"name", "account"}}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response bitvavo_ws::watchOrders(const std::string& symbol, const Dict& params) {
    if (!this->authenticated) {
        this->authenticate();
    }
    
    std::string messageHash = symbol.empty() ? "order" : "order:" + this->market(symbol)["id"].get<std::string>();
    
    json request = {
        {"action", "subscribe"},
        {"channels", {{"name", "order"}}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response bitvavo_ws::watchMyTrades(const std::string& symbol, const Dict& params) {
    if (!this->authenticated) {
        this->authenticate();
    }
    
    std::string messageHash = symbol.empty() ? "fill" : "fill:" + this->market(symbol)["id"].get<std::string>();
    
    json request = {
        {"action", "subscribe"},
        {"channels", {{"name", "fill"}}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

void bitvavo_ws::authenticate(const Dict& params) {
    if (this->authenticated) {
        return;
    }
    
    std::string timestamp = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    std::string method = "GET";
    std::string url = "/v2/websocket";
    std::string signature = this->getSignature(timestamp, method, url);
    
    json request = {
        {"action", "authenticate"},
        {"key", this->config_.apiKey},
        {"signature", signature},
        {"timestamp", timestamp},
        {"window", this->window}
    };
    
    this->send(request);
}

std::string bitvavo_ws::getSignature(const std::string& timestamp, const std::string& method, 
                                   const std::string& url, const std::string& body) {
    std::string message = timestamp + method + url;
    if (!body.empty()) {
        message += body;
    }
    return this->hmac(message, this->config_.secret, "sha256");
}

std::string bitvavo_ws::getSymbolId(const std::string& symbol) {
    auto market = this->market(symbol);
    return market["id"].get<std::string>();
}

void bitvavo_ws::handleMessage(const json& message) {
    if (message.contains("event")) {
        std::string event = message["event"].get<std::string>();
        
        if (event == "subscribed") {
            handleSubscriptionStatus(message);
        } else if (event == "authenticate") {
            handleAuthenticationMessage(message);
        } else if (event == "error") {
            handleError(message);
        }
    } else if (message.contains("action")) {
        std::string action = message["action"].get<std::string>();
        
        if (action == "ticker24h") {
            handleTickerMessage(message);
        } else if (action == "trades") {
            handleTradesMessage(message);
        } else if (action == "book") {
            handleOrderBookMessage(message);
        } else if (action == "candles") {
            handleOHLCVMessage(message);
        } else if (action == "account") {
            handleBalanceMessage(message);
        } else if (action == "order") {
            handleOrderMessage(message);
        } else if (action == "fill") {
            handleMyTradesMessage(message);
        }
    }
}

void bitvavo_ws::handleTickerMessage(const json& message) {
    auto data = message["data"];
    std::string market = data["market"].get<std::string>();
    auto symbol = this->marketId(market);
    
    Ticker ticker;
    ticker.symbol = symbol;
    ticker.timestamp = this->safeInteger(data, "timestamp");
    ticker.datetime = this->iso8601(ticker.timestamp);
    ticker.high = this->safeFloat(data, "high");
    ticker.low = this->safeFloat(data, "low");
    ticker.bid = this->safeFloat(data, "bid");
    ticker.ask = this->safeFloat(data, "ask");
    ticker.last = this->safeFloat(data, "last");
    ticker.open = this->safeFloat(data, "open");
    ticker.close = ticker.last;
    ticker.baseVolume = this->safeFloat(data, "volume");
    ticker.quoteVolume = this->safeFloat(data, "volumeQuote");
    ticker.info = data;
    
    this->tickers[symbol] = ticker;
    this->emit("ticker24h:" + market, ticker);
}

void bitvavo_ws::handleTradesMessage(const json& message) {
    auto data = message["data"];
    std::string market = data["market"].get<std::string>();
    auto symbol = this->marketId(market);
    
    Trade trade;
    trade.symbol = symbol;
    trade.id = this->safeString(data, "id");
    trade.timestamp = this->safeInteger(data, "timestamp");
    trade.datetime = this->iso8601(trade.timestamp);
    trade.side = this->safeString(data, "side");
    trade.price = this->safeFloat(data, "price");
    trade.amount = this->safeFloat(data, "amount");
    trade.cost = trade.price * trade.amount;
    trade.info = data;
    
    if (this->trades.find(symbol) == this->trades.end()) {
        this->trades[symbol] = std::vector<Trade>();
    }
    this->trades[symbol].push_back(trade);
    this->emit("trades:" + market, trade);
}

void bitvavo_ws::handleOrderBookMessage(const json& message) {
    auto data = message["data"];
    std::string market = data["market"].get<std::string>();
    auto symbol = this->marketId(market);
    
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
    
    orderbook.timestamp = this->safeInteger(data, "timestamp");
    orderbook.datetime = this->iso8601(orderbook.timestamp);
    orderbook.nonce = this->safeInteger(data, "nonce");
    
    this->emit("book:" + market, orderbook);
}

void bitvavo_ws::handleOHLCVMessage(const json& message) {
    auto data = message["data"];
    std::string market = data["market"].get<std::string>();
    auto symbol = this->marketId(market);
    std::string timeframe = data["interval"].get<std::string>();
    
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
    
    this->emit("candles:" + timeframe + ":" + market, ohlcv);
}

void bitvavo_ws::handleBalanceMessage(const json& message) {
    auto data = message["data"];
    Balance balance;
    balance.timestamp = this->safeInteger(data, "timestamp");
    balance.datetime = this->iso8601(balance.timestamp);
    
    for (const auto& item : data.items()) {
        std::string currency = item.key();
        auto balanceData = item.value();
        
        balance.free[currency] = this->safeFloat(balanceData, "available");
        balance.used[currency] = this->safeFloat(balanceData, "inOrder");
        balance.total[currency] = balance.free[currency] + balance.used[currency];
    }
    
    this->emit("account", balance);
}

void bitvavo_ws::handleOrderMessage(const json& message) {
    auto data = message["data"];
    std::string market = data["market"].get<std::string>();
    auto symbol = this->marketId(market);
    
    Order order;
    order.id = this->safeString(data, "orderId");
    order.clientOrderId = this->safeString(data, "clientOrderId");
    order.timestamp = this->safeInteger(data, "created");
    order.datetime = this->iso8601(order.timestamp);
    order.lastTradeTimestamp = this->safeInteger(data, "updated");
    order.symbol = symbol;
    order.type = this->safeString(data, "orderType");
    order.side = this->safeString(data, "side");
    order.price = this->safeFloat(data, "price");
    order.amount = this->safeFloat(data, "amount");
    order.cost = this->safeFloat(data, "filledAmount") * order.price;
    order.average = this->safeFloat(data, "filledPrice");
    order.filled = this->safeFloat(data, "filledAmount");
    order.remaining = order.amount - order.filled;
    order.status = this->safeString(data, "status");
    order.fee = {
        {"cost", this->safeFloat(data, "feePaid")},
        {"currency", this->safeString(data, "feeCurrency")}
    };
    order.trades = nullptr;
    order.info = data;
    
    this->emit("order:" + market, order);
}

void bitvavo_ws::handleMyTradesMessage(const json& message) {
    auto data = message["data"];
    std::string market = data["market"].get<std::string>();
    auto symbol = this->marketId(market);
    
    Trade trade;
    trade.id = this->safeString(data, "fillId");
    trade.order = this->safeString(data, "orderId");
    trade.timestamp = this->safeInteger(data, "timestamp");
    trade.datetime = this->iso8601(trade.timestamp);
    trade.symbol = symbol;
    trade.type = this->safeString(data, "orderType");
    trade.side = this->safeString(data, "side");
    trade.price = this->safeFloat(data, "price");
    trade.amount = this->safeFloat(data, "amount");
    trade.cost = trade.price * trade.amount;
    trade.fee = {
        {"cost", this->safeFloat(data, "fee")},
        {"currency", this->safeString(data, "feeCurrency")}
    };
    trade.info = data;
    
    this->emit("fill:" + market, trade);
}

void bitvavo_ws::handleAuthenticationMessage(const json& message) {
    if (message["event"].get<std::string>() == "authenticate") {
        if (message.contains("authenticated") && message["authenticated"].get<bool>()) {
            this->authenticated = true;
            this->emit("authenticated", message);
        } else {
            throw ExchangeError("Authentication failed");
        }
    }
}

void bitvavo_ws::handleSubscriptionStatus(const json& message) {
    if (message["event"].get<std::string>() != "subscribed") {
        throw ExchangeError("Subscription failed");
    }
}

void bitvavo_ws::handleError(const json& message) {
    throw ExchangeError(message["error"].get<std::string>());
}

} // namespace ccxt
