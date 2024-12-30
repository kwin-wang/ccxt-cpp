#include "exchanges/ws/bitstamp_ws.h"
#include "base/json_helper.h"
#include <chrono>

namespace ccxt {

bitstamp_ws::bitstamp_ws() : exchange_ws() {
    this->urls["ws"] = "wss://ws.bitstamp.net";
    
    this->options["watchOrderBook"]["snapshotDelay"] = 0;
    this->authenticated = false;
}

Response bitstamp_ws::watchTicker(const std::string& symbol, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "live_trades_" + getSymbolId(symbol);
    
    json request = {
        {"event", "subscribe"},
        {"channel", messageHash},
        {"data", {}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response bitstamp_ws::watchTrades(const std::string& symbol, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "live_trades_" + getSymbolId(symbol);
    
    json request = {
        {"event", "subscribe"},
        {"channel", messageHash},
        {"data", {}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response bitstamp_ws::watchOrderBook(const std::string& symbol, const int limit, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "order_book_" + getSymbolId(symbol);
    
    json request = {
        {"event", "subscribe"},
        {"channel", messageHash},
        {"data", {}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response bitstamp_ws::watchOHLCV(const std::string& symbol, const std::string& timeframe, const Dict& params) {
    auto market = this->market(symbol);
    std::string messageHash = "ohlc_" + getSymbolId(symbol);
    
    json request = {
        {"event", "subscribe"},
        {"channel", messageHash},
        {"data", {{"timeframe", timeframe}}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response bitstamp_ws::watchBalance(const Dict& params) {
    if (!this->authenticated) {
        this->authenticate();
    }
    
    std::string messageHash = "private_balance";
    
    json request = {
        {"event", "subscribe"},
        {"channel", messageHash},
        {"data", {}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response bitstamp_ws::watchOrders(const std::string& symbol, const Dict& params) {
    if (!this->authenticated) {
        this->authenticate();
    }
    
    std::string messageHash = symbol.empty() ? "private_orders_all" : "private_orders_" + getSymbolId(symbol);
    
    json request = {
        {"event", "subscribe"},
        {"channel", messageHash},
        {"data", {}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

Response bitstamp_ws::watchMyTrades(const std::string& symbol, const Dict& params) {
    if (!this->authenticated) {
        this->authenticate();
    }
    
    std::string messageHash = symbol.empty() ? "private_trades_all" : "private_trades_" + getSymbolId(symbol);
    
    json request = {
        {"event", "subscribe"},
        {"channel", messageHash},
        {"data", {}}
    };
    
    return this->watch(this->urls["ws"], messageHash, request, messageHash);
}

void bitstamp_ws::authenticate(const Dict& params) {
    if (this->authenticated) {
        return;
    }
    
    std::string nonce = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    std::string signature = this->hmac(nonce + this->config_.apiKey, this->config_.secret, "sha256");
    
    json request = {
        {"event", "bts:subscribe"},
        {"data", {
            {"key", this->config_.apiKey},
            {"signature", signature},
            {"nonce", nonce}
        }}
    };
    
    this->send(request);
}

std::string bitstamp_ws::getSymbolId(const std::string& symbol) {
    auto market = this->market(symbol);
    return market["id"].get<std::string>();
}

void bitstamp_ws::handleMessage(const json& message) {
    if (message.contains("event")) {
        std::string event = message["event"].get<std::string>();
        
        if (event == "bts:subscription_succeeded") {
            handleSubscriptionStatus(message);
        } else if (event == "bts:error") {
            handleError(message);
        } else if (event == "trade") {
            handleTradesMessage(message);
        } else if (event == "data") {
            std::string channel = message["channel"].get<std::string>();
            
            if (channel.find("live_trades_") == 0) {
                handleTickerMessage(message);
            } else if (channel.find("order_book_") == 0) {
                handleOrderBookMessage(message);
            } else if (channel.find("ohlc_") == 0) {
                handleOHLCVMessage(message);
            } else if (channel == "private_balance") {
                handleBalanceMessage(message);
            } else if (channel.find("private_orders_") == 0) {
                handleOrderMessage(message);
            } else if (channel.find("private_trades_") == 0) {
                handleMyTradesMessage(message);
            }
        }
    }
}

void bitstamp_ws::handleTickerMessage(const json& message) {
    auto data = message["data"];
    std::string channel = message["channel"].get<std::string>();
    std::string symbolId = channel.substr(12);  // Remove "live_trades_" prefix
    auto market = this->marketById(symbolId);
    std::string symbol = market["symbol"].get<std::string>();
    
    Ticker ticker;
    ticker.symbol = symbol;
    ticker.timestamp = this->safeInteger(data, "timestamp") * 1000;  // Convert to milliseconds
    ticker.datetime = this->iso8601(ticker.timestamp);
    ticker.last = this->safeFloat(data, "price");
    ticker.baseVolume = this->safeFloat(data, "amount");
    ticker.info = data;
    
    this->tickers[symbol] = ticker;
    this->emit("live_trades_" + symbolId, ticker);
}

void bitstamp_ws::handleTradesMessage(const json& message) {
    auto data = message["data"];
    std::string channel = message["channel"].get<std::string>();
    std::string symbolId = channel.substr(12);  // Remove "live_trades_" prefix
    auto market = this->marketById(symbolId);
    std::string symbol = market["symbol"].get<std::string>();
    
    Trade trade;
    trade.symbol = symbol;
    trade.id = this->safeString(data, "id");
    trade.timestamp = this->safeInteger(data, "timestamp") * 1000;  // Convert to milliseconds
    trade.datetime = this->iso8601(trade.timestamp);
    trade.price = this->safeFloat(data, "price");
    trade.amount = this->safeFloat(data, "amount");
    trade.side = data["type"].get<int>() == 0 ? "buy" : "sell";
    trade.info = data;
    
    if (this->trades.find(symbol) == this->trades.end()) {
        this->trades[symbol] = std::vector<Trade>();
    }
    this->trades[symbol].push_back(trade);
    this->emit("live_trades_" + symbolId, trade);
}

void bitstamp_ws::handleOrderBookMessage(const json& message) {
    auto data = message["data"];
    std::string channel = message["channel"].get<std::string>();
    std::string symbolId = channel.substr(11);  // Remove "order_book_" prefix
    auto market = this->marketById(symbolId);
    std::string symbol = market["symbol"].get<std::string>();
    
    if (this->orderbooks.find(symbol) == this->orderbooks.end()) {
        this->orderbooks[symbol] = OrderBook();
    }
    
    OrderBook& orderbook = this->orderbooks[symbol];
    orderbook.symbol = symbol;
    orderbook.timestamp = this->safeInteger(data, "timestamp") * 1000;  // Convert to milliseconds
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
    
    this->emit("order_book_" + symbolId, orderbook);
}

void bitstamp_ws::handleOHLCVMessage(const json& message) {
    auto data = message["data"];
    std::string channel = message["channel"].get<std::string>();
    std::string symbolId = channel.substr(5);  // Remove "ohlc_" prefix
    auto market = this->marketById(symbolId);
    std::string symbol = market["symbol"].get<std::string>();
    std::string timeframe = data["timeframe"].get<std::string>();
    
    OHLCV ohlcv;
    ohlcv.timestamp = this->safeInteger(data, "timestamp") * 1000;  // Convert to milliseconds
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
    
    this->emit("ohlc_" + symbolId, ohlcv);
}

void bitstamp_ws::handleBalanceMessage(const json& message) {
    auto data = message["data"];
    Balance balance;
    balance.timestamp = this->safeInteger(data, "timestamp") * 1000;  // Convert to milliseconds
    balance.datetime = this->iso8601(balance.timestamp);
    
    for (const auto& item : data.items()) {
        std::string currency = item.key();
        if (currency.find("_balance") != std::string::npos) {
            currency = currency.substr(0, currency.find("_balance"));
            balance.total[currency] = this->safeFloat(data, currency + "_balance");
            balance.free[currency] = this->safeFloat(data, currency + "_available");
            balance.used[currency] = balance.total[currency] - balance.free[currency];
        }
    }
    
    this->emit("private_balance", balance);
}

void bitstamp_ws::handleOrderMessage(const json& message) {
    auto data = message["data"];
    std::string channel = message["channel"].get<std::string>();
    std::string symbolId = channel.substr(14);  // Remove "private_orders_" prefix
    auto market = this->marketById(symbolId);
    std::string symbol = market["symbol"].get<std::string>();
    
    Order order;
    order.id = this->safeString(data, "id");
    order.clientOrderId = this->safeString(data, "client_order_id");
    order.timestamp = this->safeInteger(data, "datetime") * 1000;  // Convert to milliseconds
    order.datetime = this->iso8601(order.timestamp);
    order.lastTradeTimestamp = nullptr;
    order.symbol = symbol;
    order.type = this->safeString(data, "order_type");
    order.side = this->safeString(data, "order_side");
    order.price = this->safeFloat(data, "price");
    order.amount = this->safeFloat(data, "amount");
    order.cost = this->safeFloat(data, "value");
    order.average = nullptr;
    order.filled = this->safeFloat(data, "amount_at_create") - this->safeFloat(data, "amount");
    order.remaining = this->safeFloat(data, "amount");
    order.status = this->safeString(data, "status");
    order.fee = nullptr;
    order.trades = nullptr;
    order.info = data;
    
    this->emit("private_orders_" + symbolId, order);
}

void bitstamp_ws::handleMyTradesMessage(const json& message) {
    auto data = message["data"];
    std::string channel = message["channel"].get<std::string>();
    std::string symbolId = channel.substr(14);  // Remove "private_trades_" prefix
    auto market = this->marketById(symbolId);
    std::string symbol = market["symbol"].get<std::string>();
    
    Trade trade;
    trade.id = this->safeString(data, "id");
    trade.order = this->safeString(data, "order_id");
    trade.timestamp = this->safeInteger(data, "datetime") * 1000;  // Convert to milliseconds
    trade.datetime = this->iso8601(trade.timestamp);
    trade.symbol = symbol;
    trade.type = "limit";
    trade.side = this->safeString(data, "type") == "0" ? "buy" : "sell";
    trade.price = this->safeFloat(data, "price");
    trade.amount = this->safeFloat(data, "amount");
    trade.cost = trade.price * trade.amount;
    trade.fee = {
        {"cost", this->safeFloat(data, "fee")},
        {"currency", this->safeString(data, "fee_currency")}
    };
    trade.info = data;
    
    this->emit("private_trades_" + symbolId, trade);
}

void bitstamp_ws::handleAuthenticationMessage(const json& message) {
    if (message["event"].get<std::string>() == "bts:subscription_succeeded") {
        this->authenticated = true;
        this->emit("authenticated", message);
    } else {
        throw ExchangeError(message["data"]["message"].get<std::string>());
    }
}

void bitstamp_ws::handleSubscriptionStatus(const json& message) {
    if (message["event"].get<std::string>() != "bts:subscription_succeeded") {
        throw ExchangeError("Subscription failed");
    }
}

void bitstamp_ws::handleError(const json& message) {
    if (message.contains("data") && message["data"].contains("message")) {
        throw ExchangeError(message["data"]["message"].get<std::string>());
    }
}

} // namespace ccxt
