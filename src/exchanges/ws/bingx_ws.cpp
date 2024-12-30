#include "ccxt/exchanges/ws/bingx_ws.h"
#include "ccxt/base/json.hpp"
#include "ccxt/base/functions.hpp"
#include <chrono>
#include <sstream>
#include <iomanip>

namespace ccxt {

bingx_ws::bingx_ws() : exchange_ws() {
    this->urls["ws"] = {
        {"public", WS_BASE},
        {"private", WS_PRIVATE}
    };
}

std::string bingx_ws::get_url() const {
    return WS_BASE;
}

void bingx_ws::watch_ticker_impl(const std::string& symbol, const json& params) {
    std::string market_id = this->market_id(symbol);
    subscribe_public("ticker", market_id);
}

void bingx_ws::watch_trades_impl(const std::string& symbol, const json& params) {
    std::string market_id = this->market_id(symbol);
    subscribe_public("trade", market_id);
}

void bingx_ws::watch_ohlcv_impl(const std::string& symbol, const std::string& timeframe, const json& params) {
    std::string market_id = this->market_id(symbol);
    std::string interval = this->timeframes[timeframe];
    json request = {
        {"symbol", market_id},
        {"interval", interval}
    };
    subscribe_public("kline", market_id);
}

void bingx_ws::watch_order_book_impl(const std::string& symbol, const json& params) {
    std::string market_id = this->market_id(symbol);
    subscribe_public("depth", market_id);
}

void bingx_ws::watch_bids_asks_impl(const std::string& symbol, const json& params) {
    std::string market_id = this->market_id(symbol);
    subscribe_public("bookTicker", market_id);
}

void bingx_ws::watch_balance_impl(const json& params) {
    if (!this->authenticated) {
        throw AuthenticationError("Authentication required for private endpoints");
    }
    subscribe_private("account");
}

void bingx_ws::watch_orders_impl(const std::string& symbol, const json& params) {
    if (!this->authenticated) {
        throw AuthenticationError("Authentication required for private endpoints");
    }
    std::string market_id = symbol.empty() ? "" : this->market_id(symbol);
    subscribe_private("order", market_id);
}

void bingx_ws::watch_my_trades_impl(const std::string& symbol, const json& params) {
    if (!this->authenticated) {
        throw AuthenticationError("Authentication required for private endpoints");
    }
    std::string market_id = symbol.empty() ? "" : this->market_id(symbol);
    subscribe_private("trade", market_id);
}

void bingx_ws::watch_positions_impl(const json& params) {
    if (!this->authenticated) {
        throw AuthenticationError("Authentication required for private endpoints");
    }
    subscribe_private("position");
}

void bingx_ws::authenticate() {
    long long timestamp = get_timestamp();
    std::string request_id = generate_request_id();
    
    json auth_params = {
        {"apiKey", this->config_.apiKey},
        {"timestamp", timestamp},
        {"requestId", request_id}
    };
    
    std::string signature = sign_request(auth_params);
    auth_params["signature"] = signature;
    
    json request = {
        {"method", "login"},
        {"params", auth_params},
        {"id", request_id}
    };
    
    this->send(request);
}

void bingx_ws::subscribe_public(const std::string& channel, const std::string& symbol) {
    std::string request_id = generate_request_id();
    json params = {{"symbol", symbol}};
    
    json request = {
        {"method", "subscribe"},
        {"params", {channel}},
        {"id", request_id}
    };
    
    if (!symbol.empty()) {
        request["params"] = {channel + "@" + symbol};
    }
    
    this->send(request);
}

void bingx_ws::subscribe_private(const std::string& channel, const std::string& symbol) {
    if (!this->authenticated) {
        this->authenticate();
    }
    
    std::string request_id = generate_request_id();
    json params = symbol.empty() ? json::array({channel}) : json::array({channel + "@" + symbol});
    
    json request = {
        {"method", "subscribe"},
        {"params", params},
        {"id", request_id}
    };
    
    this->send(request);
}

void bingx_ws::handle_message(const json& message) {
    if (message.contains("method")) {
        std::string method = message["method"];
        
        if (method == "push") {
            if (message.contains("params")) {
                json params = message["params"];
                if (params.contains("channel")) {
                    std::string channel = params["channel"];
                    
                    if (channel == "ticker") {
                        handle_ticker(params);
                    } else if (channel == "trade") {
                        handle_trade(params);
                    } else if (channel == "kline") {
                        handle_ohlcv(params);
                    } else if (channel == "depth") {
                        handle_order_book(params);
                    } else if (channel == "account") {
                        handle_balance(params);
                    } else if (channel == "order") {
                        handle_order(params);
                    } else if (channel == "position") {
                        handle_position(params);
                    }
                }
            }
        }
    }
}

void bingx_ws::handle_error(const json& message) {
    if (message.contains("code") && message.contains("msg")) {
        int code = message["code"];
        std::string msg = message["msg"];
        throw ExchangeError("BingX WebSocket error: " + std::to_string(code) + " " + msg);
    }
}

void bingx_ws::handle_subscription(const json& message) {
    if (message.contains("result") && message["result"] == true) {
        // Subscription successful
        return;
    }
}

void bingx_ws::handle_ticker(const json& message) {
    if (message.contains("data")) {
        json data = message["data"];
        std::string symbol = this->safe_string(data, "symbol");
        symbol = this->safe_symbol(symbol);
        
        json ticker = {
            {"symbol", symbol},
            {"high", this->safe_string(data, "high24h")},
            {"low", this->safe_string(data, "low24h")},
            {"bid", this->safe_string(data, "bestBid")},
            {"ask", this->safe_string(data, "bestAsk")},
            {"last", this->safe_string(data, "lastPrice")},
            {"volume", this->safe_string(data, "volume24h")},
            {"timestamp", this->safe_integer(data, "timestamp")}
        };
        
        this->emit("ticker", ticker);
    }
}

void bingx_ws::handle_trade(const json& message) {
    if (message.contains("data")) {
        json data = message["data"];
        std::string symbol = this->safe_string(data, "symbol");
        symbol = this->safe_symbol(symbol);
        
        json trade = {
            {"symbol", symbol},
            {"id", this->safe_string(data, "tradeId")},
            {"price", this->safe_string(data, "price")},
            {"amount", this->safe_string(data, "quantity")},
            {"side", this->safe_string_lower(data, "side")},
            {"timestamp", this->safe_integer(data, "timestamp")}
        };
        
        this->emit("trade", trade);
    }
}

void bingx_ws::handle_ohlcv(const json& message) {
    if (message.contains("data")) {
        json data = message["data"];
        std::string symbol = this->safe_string(data, "symbol");
        symbol = this->safe_symbol(symbol);
        
        json kline = {
            {"timestamp", this->safe_integer(data, "timestamp")},
            {"open", this->safe_string(data, "open")},
            {"high", this->safe_string(data, "high")},
            {"low", this->safe_string(data, "low")},
            {"close", this->safe_string(data, "close")},
            {"volume", this->safe_string(data, "volume")}
        };
        
        this->emit("ohlcv", kline);
    }
}

void bingx_ws::handle_order_book(const json& message) {
    if (message.contains("data")) {
        json data = message["data"];
        std::string symbol = this->safe_string(data, "symbol");
        symbol = this->safe_symbol(symbol);
        
        json orderbook = {
            {"symbol", symbol},
            {"bids", this->safe_value(data, "bids", json::array())},
            {"asks", this->safe_value(data, "asks", json::array())},
            {"timestamp", this->safe_integer(data, "timestamp")},
            {"nonce", this->safe_integer(data, "lastUpdateId")}
        };
        
        this->emit("orderbook", orderbook);
    }
}

void bingx_ws::handle_balance(const json& message) {
    if (message.contains("data")) {
        json data = message["data"];
        json balances = data["balances"];
        json result;
        
        for (const auto& balance : balances) {
            std::string currency = this->safe_string(balance, "asset");
            result[currency] = {
                {"free", this->safe_string(balance, "free")},
                {"used", this->safe_string(balance, "locked")},
                {"total", this->safe_string(balance, "total")}
            };
        }
        
        this->emit("balance", result);
    }
}

void bingx_ws::handle_order(const json& message) {
    if (message.contains("data")) {
        json data = message["data"];
        std::string symbol = this->safe_string(data, "symbol");
        symbol = this->safe_symbol(symbol);
        
        json order = {
            {"id", this->safe_string(data, "orderId")},
            {"clientOrderId", this->safe_string(data, "clientOrderId")},
            {"symbol", symbol},
            {"type", this->safe_string_lower(data, "type")},
            {"side", this->safe_string_lower(data, "side")},
            {"price", this->safe_string(data, "price")},
            {"amount", this->safe_string(data, "origQty")},
            {"filled", this->safe_string(data, "executedQty")},
            {"status", this->safe_string_lower(data, "status")},
            {"timestamp", this->safe_integer(data, "time")}
        };
        
        this->emit("order", order);
    }
}

void bingx_ws::handle_position(const json& message) {
    if (message.contains("data")) {
        json data = message["data"];
        std::string symbol = this->safe_string(data, "symbol");
        symbol = this->safe_symbol(symbol);
        
        json position = {
            {"symbol", symbol},
            {"size", this->safe_string(data, "positionAmt")},
            {"side", this->safe_string_lower(data, "positionSide")},
            {"notional", this->safe_string(data, "notional")},
            {"leverage", this->safe_string(data, "leverage")},
            {"unrealizedPnl", this->safe_string(data, "unrealizedProfit")},
            {"timestamp", this->safe_integer(data, "updateTime")}
        };
        
        this->emit("position", position);
    }
}

std::string bingx_ws::sign_request(const json& request) {
    std::string query_string;
    for (auto it = request.begin(); it != request.end(); ++it) {
        if (!query_string.empty()) {
            query_string += "&";
        }
        query_string += it.key() + "=" + it.value().dump();
    }
    
    return hmac(query_string, this->config_.secret, "sha256");
}

long long bingx_ws::get_timestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

std::string bingx_ws::generate_request_id() {
    static long long request_id = 0;
    return std::to_string(++request_id);
}

} // namespace ccxt
