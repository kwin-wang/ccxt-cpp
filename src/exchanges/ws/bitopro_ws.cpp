#include "exchanges/ws/bitopro_ws.h"
#include "base/json_helper.h"
#include <chrono>

namespace ccxt {

bitopro_ws::bitopro_ws() : exchange_ws() {
    this->urls["ws"] = "wss://stream.bitopro.com:9443/ws";
    this->urls["wsTest"] = "wss://stream.bitopro.com:9443/ws/test";
    
    this->options["watchOrderBook"]["snapshotDelay"] = 0;
    this->authenticated = false;
}

Response bitopro_ws::watchTicker(const std::string& symbol, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "ticker:" + market["symbol"].get<std::string>();
    
    json request = {
        {"event", "subscribe"},
        {"channel", "TICKER_" + getSymbolId(symbol)}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response bitopro_ws::watchTrades(const std::string& symbol, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "trades:" + market["symbol"].get<std::string>();
    
    json request = {
        {"event", "subscribe"},
        {"channel", "TRADE_" + getSymbolId(symbol)}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response bitopro_ws::watchOrderBook(const std::string& symbol, const int limit, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "orderbook:" + market["symbol"].get<std::string>();
    
    json request = {
        {"event", "subscribe"},
        {"channel", "ORDER_BOOK_" + getSymbolId(symbol)}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response bitopro_ws::watchBalance(const Dict& params) {
    if (!this->authenticated) {
        this->authenticate();
    }
    
    std::string messageHash = "account";
    
    json request = {
        {"event", "subscribe"},
        {"channel", "ACCOUNT_BALANCE"}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response bitopro_ws::watchOrders(const std::string& symbol, const Dict& params) {
    if (!this->authenticated) {
        this->authenticate();
    }
    
    std::string messageHash = symbol.empty() ? "orders" : "orders:" + symbol;
    std::string channel = symbol.empty() ? "ORDER_ALL" : "ORDER_" + getSymbolId(symbol);
    
    json request = {
        {"event", "subscribe"},
        {"channel", channel}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response bitopro_ws::watchMyTrades(const std::string& symbol, const Dict& params) {
    if (!this->authenticated) {
        this->authenticate();
    }
    
    std::string messageHash = symbol.empty() ? "trades" : "trades:" + symbol;
    std::string channel = symbol.empty() ? "TRADE_ALL" : "TRADE_" + getSymbolId(symbol);
    
    json request = {
        {"event", "subscribe"},
        {"channel", channel}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

void bitopro_ws::authenticate(const Dict& params) {
    if (this->authenticated) {
        return;
    }
    
    int64_t timestamp = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
    std::string payload = this->config_.apiKey + std::to_string(timestamp);
    std::string signature = this->hmac(payload, this->config_.secret, "sha384");
    
    json request = {
        {"event", "auth"},
        {"auth", {
            {"apiKey", this->config_.apiKey},
            {"timestamp", timestamp},
            {"signature", signature}
        }}
    };
    
    this->send(request);
}

std::string bitopro_ws::getSymbolId(const std::string& symbol) {
    auto market = this->market(symbol);
    return market["id"].get<std::string>();
}

void bitopro_ws::handleMessage(const json& message) {
    if (message.contains("event")) {
        std::string event = message["event"].get<std::string>();
        
        if (event == "auth") {
            handleAuthenticationMessage(message);
        } else if (event == "subscribe") {
            handleSubscriptionStatus(message);
        }
    }
    
    if (message.contains("channel") && message.contains("data")) {
        std::string channel = message["channel"].get<std::string>();
        
        if (channel.find("TICKER_") == 0) {
            handleTickerMessage(message);
        } else if (channel.find("TRADE_") == 0) {
            handleTradesMessage(message);
        } else if (channel.find("ORDER_BOOK_") == 0) {
            handleOrderBookMessage(message);
        } else if (channel == "ACCOUNT_BALANCE") {
            handleBalanceMessage(message);
        } else if (channel.find("ORDER_") == 0) {
            handleOrderMessage(message);
        }
    }
}

void bitopro_ws::handleTickerMessage(const json& message) {
    auto data = message["data"];
    std::string symbolId = message["channel"].get<std::string>().substr(7);  // Remove "TICKER_" prefix
    auto market = this->marketById(symbolId);
    std::string symbol = market["symbol"].get<std::string>();
    
    Ticker ticker;
    ticker.symbol = symbol;
    ticker.timestamp = this->safeInteger(data, "timestamp");
    ticker.datetime = this->iso8601(ticker.timestamp);
    ticker.high = this->safeFloat(data, "high24hr");
    ticker.low = this->safeFloat(data, "low24hr");
    ticker.bid = this->safeFloat(data, "highestBid");
    ticker.ask = this->safeFloat(data, "lowestAsk");
    ticker.last = this->safeFloat(data, "last");
    ticker.baseVolume = this->safeFloat(data, "volume24hr");
    ticker.info = data;
    
    this->tickers[symbol] = ticker;
    this->emit("ticker:" + symbol, ticker);
}

void bitopro_ws::handleTradesMessage(const json& message) {
    auto data = message["data"];
    std::string symbolId = message["channel"].get<std::string>().substr(6);  // Remove "TRADE_" prefix
    auto market = this->marketById(symbolId);
    std::string symbol = market["symbol"].get<std::string>();
    
    Trade trade;
    trade.symbol = symbol;
    trade.id = this->safeString(data, "tradeId");
    trade.timestamp = this->safeInteger(data, "timestamp");
    trade.datetime = this->iso8601(trade.timestamp);
    trade.price = this->safeFloat(data, "price");
    trade.amount = this->safeFloat(data, "amount");
    trade.side = data["isBuyer"].get<bool>() ? "buy" : "sell";
    trade.info = data;
    
    if (this->trades.find(symbol) == this->trades.end()) {
        this->trades[symbol] = std::vector<Trade>();
    }
    this->trades[symbol].push_back(trade);
    this->emit("trades:" + symbol, trade);
}

void bitopro_ws::handleOrderBookMessage(const json& message) {
    auto data = message["data"];
    std::string symbolId = message["channel"].get<std::string>().substr(11);  // Remove "ORDER_BOOK_" prefix
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
                this->safeFloat(ask, "price"),
                this->safeFloat(ask, "amount")
            });
        }
    }
    
    if (data.contains("bids")) {
        orderbook.bids.clear();
        for (const auto& bid : data["bids"]) {
            orderbook.bids.push_back({
                this->safeFloat(bid, "price"),
                this->safeFloat(bid, "amount")
            });
        }
    }
    
    this->emit("orderbook:" + symbol, orderbook);
}

void bitopro_ws::handleBalanceMessage(const json& message) {
    auto data = message["data"];
    Balance balance;
    balance.timestamp = this->safeInteger(data, "timestamp");
    balance.datetime = this->iso8601(balance.timestamp);
    
    for (const auto& item : data["balances"]) {
        std::string currency = item["currency"].get<std::string>();
        balance.free[currency] = this->safeFloat(item, "available");
        balance.used[currency] = this->safeFloat(item, "locked");
        balance.total[currency] = this->safeFloat(item, "total");
    }
    
    this->emit("account", balance);
}

void bitopro_ws::handleOrderMessage(const json& message) {
    auto data = message["data"];
    std::string symbolId = message["channel"].get<std::string>().substr(6);  // Remove "ORDER_" prefix
    auto market = this->marketById(symbolId);
    std::string symbol = market["symbol"].get<std::string>();
    
    Order order;
    order.id = this->safeString(data, "orderId");
    order.clientOrderId = this->safeString(data, "clientOrderId");
    order.timestamp = this->safeInteger(data, "timestamp");
    order.datetime = this->iso8601(order.timestamp);
    order.symbol = symbol;
    order.type = this->safeString(data, "type");
    order.side = this->safeString(data, "side");
    order.price = this->safeFloat(data, "price");
    order.amount = this->safeFloat(data, "amount");
    order.cost = this->safeFloat(data, "executedValue");
    order.filled = this->safeFloat(data, "executedAmount");
    order.remaining = order.amount - order.filled;
    order.status = this->safeString(data, "status");
    order.fee = {
        {"cost", this->safeFloat(data, "fee")},
        {"currency", this->safeString(data, "feeCurrency")}
    };
    order.info = data;
    
    std::string messageHash = "orders:" + symbol;
    this->emit(messageHash, order);
}

void bitopro_ws::handleAuthenticationMessage(const json& message) {
    if (message["success"].get<bool>()) {
        this->authenticated = true;
        this->emit("authenticated", message);
    } else {
        throw ExchangeError(message["message"].get<std::string>());
    }
}

void bitopro_ws::handleSubscriptionStatus(const json& message) {
    if (!message["success"].get<bool>()) {
        throw ExchangeError(message["message"].get<std::string>());
    }
}

void bitopro_ws::handleError(const json& message) {
    if (message.contains("message")) {
        throw ExchangeError(message["message"].get<std::string>());
    }
}

} // namespace ccxt
