#include "ccxt/exchanges/ws/bitcoincom_ws.h"
#include "ccxt/base/json.hpp"
#include <chrono>
#include <random>

namespace ccxt {

bitcoincom_ws::bitcoincom_ws() : exchange_ws() {
    this->urls["ws"] = {
        {"public", WS_BASE},
        {"private", WS_BASE}
    };
}

std::string bitcoincom_ws::get_url() const {
    return WS_BASE;
}

void bitcoincom_ws::watch_ticker_impl(const std::string& symbol, const json& params) {
    std::string market_id = this->market_id(symbol);
    subscribe_public("ticker", market_id);
}

void bitcoincom_ws::watch_trades_impl(const std::string& symbol, const json& params) {
    std::string market_id = this->market_id(symbol);
    subscribe_public("trades", market_id);
}

void bitcoincom_ws::watch_ohlcv_impl(const std::string& symbol, const std::string& timeframe, const json& params) {
    std::string market_id = this->market_id(symbol);
    std::string period = this->timeframes[timeframe];
    json request = {
        {"method", "subscribeCandles"},
        {"params", {
            {"symbol", market_id},
            {"period", period}
        }},
        {"id", generate_client_id()}
    };
    this->send(request);
}

void bitcoincom_ws::watch_order_book_impl(const std::string& symbol, const json& params) {
    std::string market_id = this->market_id(symbol);
    subscribe_public("orderbook", market_id);
}

void bitcoincom_ws::watch_balance_impl(const json& params) {
    if (!this->authenticated) {
        throw AuthenticationError("Authentication required for private endpoints");
    }
    subscribe_private("balance");
}

void bitcoincom_ws::watch_orders_impl(const std::string& symbol, const json& params) {
    if (!this->authenticated) {
        throw AuthenticationError("Authentication required for private endpoints");
    }
    std::string market_id = symbol.empty() ? "" : this->market_id(symbol);
    subscribe_private("orders", market_id);
}

void bitcoincom_ws::watch_my_trades_impl(const std::string& symbol, const json& params) {
    if (!this->authenticated) {
        throw AuthenticationError("Authentication required for private endpoints");
    }
    std::string market_id = symbol.empty() ? "" : this->market_id(symbol);
    subscribe_private("trades", market_id);
}

void bitcoincom_ws::authenticate() {
    json request = {
        {"method", "login"},
        {"params", {
            {"algo", "HS256"},
            {"pKey", this->apiKey},
            {"nonce", std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count())},
            {"signature", this->hmac(this->apiKey + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()), this->secret, "sha256")}
        }},
        {"id", generate_client_id()}
    };
    
    this->send(request);
}

void bitcoincom_ws::subscribe_public(const std::string& channel, const std::string& symbol) {
    json request = {
        {"method", "subscribe" + capitalize(channel)},
        {"params", {
            {"symbol", symbol}
        }},
        {"id", generate_client_id()}
    };
    
    this->send(request);
}

void bitcoincom_ws::subscribe_private(const std::string& channel, const std::string& symbol) {
    if (!this->authenticated) {
        this->authenticate();
    }
    
    json params = json::object();
    if (!symbol.empty()) {
        params["symbol"] = symbol;
    }
    
    json request = {
        {"method", "subscribe" + capitalize(channel)},
        {"params", params},
        {"id", generate_client_id()}
    };
    
    this->send(request);
}

void bitcoincom_ws::handle_message(const json& message) {
    if (message.contains("method")) {
        std::string method = message["method"];
        
        if (method == "ticker") {
            handle_ticker(message);
        } else if (method == "snapshotTrades" || method == "updateTrades") {
            handle_trade(message);
        } else if (method == "snapshotCandles" || method == "updateCandles") {
            handle_ohlcv(message);
        } else if (method == "snapshotOrderbook" || method == "updateOrderbook") {
            handle_order_book(message);
        } else if (method == "activeOrders") {
            handle_order(message);
        } else if (method == "balance") {
            handle_balance(message);
        }
    } else if (message.contains("error")) {
        handle_error(message);
    }
}

void bitcoincom_ws::handle_error(const json& message) {
    if (message.contains("error")) {
        json error = message["error"];
        std::string code = this->safe_string(error, "code");
        std::string msg = this->safe_string(error, "message", "Unknown error");
        throw ExchangeError("Bitcoin.com WebSocket error: " + code + " " + msg);
    }
}

void bitcoincom_ws::handle_subscription(const json& message) {
    // Bitcoin.com doesn't send explicit subscription confirmations
    return;
}

void bitcoincom_ws::handle_ticker(const json& message) {
    if (message.contains("params")) {
        json params = message["params"];
        json data = params["data"];
        std::string symbol = this->safe_string(params, "symbol");
        symbol = this->safe_symbol(symbol);
        
        json ticker = {
            {"symbol", symbol},
            {"high", this->safe_string(data, "high")},
            {"low", this->safe_string(data, "low")},
            {"bid", this->safe_string(data, "bid")},
            {"ask", this->safe_string(data, "ask")},
            {"last", this->safe_string(data, "last")},
            {"volume", this->safe_string(data, "volume")},
            {"timestamp", this->safe_integer(data, "timestamp")}
        };
        
        this->emit("ticker", ticker);
    }
}

void bitcoincom_ws::handle_trade(const json& message) {
    if (message.contains("params")) {
        json params = message["params"];
        json data = params["data"];
        std::string symbol = this->safe_string(params, "symbol");
        symbol = this->safe_symbol(symbol);
        
        for (const auto& trade : data) {
            json parsed_trade = {
                {"id", this->safe_string(trade, "id")},
                {"symbol", symbol},
                {"price", this->safe_string(trade, "price")},
                {"amount", this->safe_string(trade, "quantity")},
                {"side", this->safe_string_lower(trade, "side")},
                {"timestamp", this->safe_integer(trade, "timestamp")}
            };
            
            this->emit("trade", parsed_trade);
        }
    }
}

void bitcoincom_ws::handle_ohlcv(const json& message) {
    if (message.contains("params")) {
        json params = message["params"];
        json data = params["data"];
        std::string symbol = this->safe_string(params, "symbol");
        symbol = this->safe_symbol(symbol);
        
        for (const auto& candle : data) {
            json parsed_candle = {
                {"timestamp", this->safe_integer(candle, "timestamp")},
                {"open", this->safe_string(candle, "open")},
                {"high", this->safe_string(candle, "high")},
                {"low", this->safe_string(candle, "low")},
                {"close", this->safe_string(candle, "close")},
                {"volume", this->safe_string(candle, "volume")}
            };
            
            this->emit("ohlcv", parsed_candle);
        }
    }
}

void bitcoincom_ws::handle_order_book(const json& message) {
    if (message.contains("params")) {
        json params = message["params"];
        json data = params["data"];
        std::string symbol = this->safe_string(params, "symbol");
        symbol = this->safe_symbol(symbol);
        
        json result;
        if (message["method"] == "snapshotOrderbook") {
            parse_ob_snapshot(data, result);
        } else {
            parse_ob_update(data, result);
        }
        
        result["symbol"] = symbol;
        result["timestamp"] = this->safe_integer(data, "timestamp");
        result["nonce"] = this->safe_integer(data, "sequence");
        
        this->emit("orderbook", result);
    }
}

void bitcoincom_ws::parse_ob_snapshot(const json& data, json& result) {
    result["bids"] = json::array();
    result["asks"] = json::array();
    
    if (data.contains("bid")) {
        for (const auto& bid : data["bid"]) {
            result["bids"].push_back({
                this->safe_string(bid, "price"),
                this->safe_string(bid, "size")
            });
        }
    }
    
    if (data.contains("ask")) {
        for (const auto& ask : data["ask"]) {
            result["asks"].push_back({
                this->safe_string(ask, "price"),
                this->safe_string(ask, "size")
            });
        }
    }
}

void bitcoincom_ws::parse_ob_update(const json& data, json& result) {
    result["bids"] = json::array();
    result["asks"] = json::array();
    
    if (data.contains("bid")) {
        for (const auto& bid : data["bid"]) {
            result["bids"].push_back({
                this->safe_string(bid, "price"),
                this->safe_string(bid, "size")
            });
        }
    }
    
    if (data.contains("ask")) {
        for (const auto& ask : data["ask"]) {
            result["asks"].push_back({
                this->safe_string(ask, "price"),
                this->safe_string(ask, "size")
            });
        }
    }
}

void bitcoincom_ws::handle_balance(const json& message) {
    if (message.contains("params")) {
        json params = message["params"];
        json data = params["data"];
        json result;
        
        for (const auto& balance : data) {
            std::string currency = this->safe_string(balance, "currency");
            result[currency] = {
                {"free", this->safe_string(balance, "available")},
                {"used", this->safe_string(balance, "reserved")},
                {"total", this->safe_string(balance, "total")}
            };
        }
        
        this->emit("balance", result);
    }
}

void bitcoincom_ws::handle_order(const json& message) {
    if (message.contains("params")) {
        json params = message["params"];
        json data = params["data"];
        
        for (const auto& order : data) {
            std::string symbol = this->safe_string(order, "symbol");
            symbol = this->safe_symbol(symbol);
            
            json parsed_order = {
                {"id", this->safe_string(order, "id")},
                {"clientOrderId", this->safe_string(order, "clientOrderId")},
                {"symbol", symbol},
                {"type", this->safe_string_lower(order, "type")},
                {"side", this->safe_string_lower(order, "side")},
                {"price", this->safe_string(order, "price")},
                {"amount", this->safe_string(order, "quantity")},
                {"filled", this->safe_string(order, "cumQuantity")},
                {"status", this->safe_string_lower(order, "status")},
                {"timestamp", this->safe_integer(order, "createdAt")}
            };
            
            this->emit("order", parsed_order);
        }
    }
}

std::string bitcoincom_ws::generate_client_id() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1, 1000000);
    return std::to_string(dis(gen));
}

} // namespace ccxt
