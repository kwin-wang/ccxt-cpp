#include "ccxt/exchanges/ws/bitflyer_ws.h"
#include "ccxt/base/json.hpp"
#include <chrono>
#include <sstream>
#include <iomanip>

namespace ccxt {

bitflyer_ws::bitflyer_ws() : exchange_ws() {
    this->urls["ws"] = {
        {"public", WS_BASE},
        {"private", WS_PRIVATE}
    };
}

std::string bitflyer_ws::get_url() const {
    return this->authenticated ? WS_PRIVATE : WS_BASE;
}

void bitflyer_ws::watch_ticker_impl(const std::string& symbol, const json& params) {
    std::string market_id = this->market_id(symbol);
    subscribe_public("ticker", market_id);
}

void bitflyer_ws::watch_trades_impl(const std::string& symbol, const json& params) {
    std::string market_id = this->market_id(symbol);
    subscribe_public("executions", market_id);
}

void bitflyer_ws::watch_order_book_impl(const std::string& symbol, const json& params) {
    std::string market_id = this->market_id(symbol);
    subscribe_public("board", market_id);
    subscribe_public("board_snapshot", market_id);
}

void bitflyer_ws::watch_balance_impl(const json& params) {
    if (!this->authenticated) {
        throw AuthenticationError("Authentication required for private endpoints");
    }
    subscribe_private("child_order_events");
}

void bitflyer_ws::watch_orders_impl(const std::string& symbol, const json& params) {
    if (!this->authenticated) {
        throw AuthenticationError("Authentication required for private endpoints");
    }
    std::string market_id = symbol.empty() ? "" : this->market_id(symbol);
    subscribe_private("child_order_events", market_id);
}

void bitflyer_ws::watch_my_trades_impl(const std::string& symbol, const json& params) {
    if (!this->authenticated) {
        throw AuthenticationError("Authentication required for private endpoints");
    }
    std::string market_id = symbol.empty() ? "" : this->market_id(symbol);
    subscribe_private("child_order_events", market_id);
}

void bitflyer_ws::authenticate() {
    long long timestamp = get_timestamp();
    std::string nonce = std::to_string(timestamp);
    std::string auth_payload = nonce + "GET/ws/auth";
    std::string signature = this->hmac(auth_payload, this->secret, "sha256");
    
    json auth_message = {
        {"method", "auth"},
        {"params", {
            {"api_key", this->apiKey},
            {"timestamp", timestamp},
            {"nonce", nonce},
            {"signature", signature}
        }},
        {"id", timestamp}
    };
    
    this->send(auth_message);
}

void bitflyer_ws::subscribe_public(const std::string& channel, const std::string& symbol) {
    json request = {
        {"method", "subscribe"},
        {"params", {
            {"channel", get_channel_name(channel, symbol)}
        }},
        {"id", get_timestamp()}
    };
    
    this->send(request);
}

void bitflyer_ws::subscribe_private(const std::string& channel, const std::string& symbol) {
    if (!this->authenticated) {
        this->authenticate();
    }
    
    json request = {
        {"method", "subscribe"},
        {"params", {
            {"channel", channel}
        }},
        {"id", get_timestamp()}
    };
    
    if (!symbol.empty()) {
        request["params"]["product_code"] = symbol;
    }
    
    this->send(request);
}

void bitflyer_ws::handle_message(const json& message) {
    if (message.contains("method")) {
        std::string method = message["method"];
        
        if (method == "channelMessage") {
            json params = message["params"];
            std::string channel = params["channel"];
            json message_data = params["message"];
            
            if (channel.find("ticker") != std::string::npos) {
                handle_ticker_update(message_data);
            } else if (channel.find("executions") != std::string::npos) {
                handle_trades_update(message_data);
            } else if (channel.find("board") != std::string::npos) {
                if (channel.find("snapshot") != std::string::npos) {
                    handle_order_book_snapshot(message_data);
                } else {
                    handle_order_book_update(message_data);
                }
            } else if (channel == "child_order_events") {
                handle_private_message(message_data);
            }
        }
    } else if (message.contains("error")) {
        handle_error(message);
    }
}

void bitflyer_ws::handle_error(const json& message) {
    if (message.contains("error")) {
        std::string error_msg = message["error"].get<std::string>();
        throw ExchangeError("bitFlyer WebSocket error: " + error_msg);
    }
}

void bitflyer_ws::handle_subscription(const json& message) {
    // bitFlyer doesn't send explicit subscription confirmations
    return;
}

void bitflyer_ws::handle_ticker_update(const json& data) {
    std::string symbol = this->safe_string(data, "product_code");
    symbol = this->safe_symbol(symbol);
    
    json ticker = {
        {"symbol", symbol},
        {"bid", this->safe_string(data, "best_bid")},
        {"ask", this->safe_string(data, "best_ask")},
        {"last", this->safe_string(data, "ltp")},
        {"volume", this->safe_string(data, "volume_by_product")},
        {"timestamp", this->safe_integer(data, "timestamp")}
    };
    
    this->emit("ticker", ticker);
}

void bitflyer_ws::handle_trades_update(const json& data) {
    if (data.is_array()) {
        for (const auto& trade : data) {
            std::string symbol = this->safe_string(trade, "product_code");
            symbol = this->safe_symbol(symbol);
            
            json parsed_trade = {
                {"id", this->safe_string(trade, "id")},
                {"symbol", symbol},
                {"side", this->safe_string_lower(trade, "side")},
                {"amount", this->safe_string(trade, "size")},
                {"price", this->safe_string(trade, "price")},
                {"timestamp", this->safe_integer(trade, "exec_date")},
                {"type", "limit"}
            };
            
            this->emit("trade", parsed_trade);
        }
    }
}

void bitflyer_ws::handle_order_book_snapshot(const json& data) {
    std::string symbol = this->safe_string(data, "product_code");
    symbol = this->safe_symbol(symbol);
    
    json orderbook = {
        {"symbol", symbol},
        {"bids", json::array()},
        {"asks", json::array()},
        {"timestamp", this->safe_integer(data, "timestamp")},
        {"nonce", nullptr}
    };
    
    if (data.contains("bids")) {
        for (const auto& bid : data["bids"]) {
            orderbook["bids"].push_back({
                this->safe_string(bid, "price"),
                this->safe_string(bid, "size")
            });
        }
    }
    
    if (data.contains("asks")) {
        for (const auto& ask : data["asks"]) {
            orderbook["asks"].push_back({
                this->safe_string(ask, "price"),
                this->safe_string(ask, "size")
            });
        }
    }
    
    this->orderbooks[symbol] = orderbook;
    this->emit("orderbook", orderbook);
}

void bitflyer_ws::handle_order_book_update(const json& data) {
    std::string symbol = this->safe_string(data, "product_code");
    symbol = this->safe_symbol(symbol);
    
    if (!this->orderbooks.count(symbol)) {
        initialize_order_book(symbol);
    }
    
    json& orderbook = this->orderbooks[symbol];
    
    if (data.contains("bids")) {
        for (const auto& bid : data["bids"]) {
            update_order_book(symbol, {
                {"side", "bids"},
                {"price", bid["price"]},
                {"amount", bid["size"]}
            });
        }
    }
    
    if (data.contains("asks")) {
        for (const auto& ask : data["asks"]) {
            update_order_book(symbol, {
                {"side", "asks"},
                {"price", ask["price"]},
                {"amount", ask["size"]}
            });
        }
    }
    
    orderbook["timestamp"] = this->safe_integer(data, "timestamp");
    this->emit("orderbook", orderbook);
}

void bitflyer_ws::handle_private_message(const json& data) {
    std::string event_type = this->safe_string(data, "event_type");
    
    if (event_type == "ORDER") {
        handle_order_update(data);
    } else if (event_type == "EXECUTION") {
        handle_trade_update(data);
    } else if (event_type == "BALANCE") {
        handle_balance_update(data);
    }
}

void bitflyer_ws::handle_order_update(const json& data) {
    std::string symbol = this->safe_string(data, "product_code");
    symbol = this->safe_symbol(symbol);
    
    json order = {
        {"id", this->safe_string(data, "child_order_acceptance_id")},
        {"symbol", symbol},
        {"type", this->safe_string_lower(data, "child_order_type")},
        {"side", this->safe_string_lower(data, "side")},
        {"price", this->safe_string(data, "price")},
        {"amount", this->safe_string(data, "size")},
        {"timestamp", this->safe_integer(data, "timestamp")},
        {"status", this->safe_string(data, "child_order_state")}
    };
    
    this->emit("order", order);
}

void bitflyer_ws::handle_trade_update(const json& data) {
    std::string symbol = this->safe_string(data, "product_code");
    symbol = this->safe_symbol(symbol);
    
    json trade = {
        {"id", this->safe_string(data, "child_order_acceptance_id")},
        {"order", this->safe_string(data, "child_order_id")},
        {"symbol", symbol},
        {"type", this->safe_string_lower(data, "child_order_type")},
        {"side", this->safe_string_lower(data, "side")},
        {"price", this->safe_string(data, "price")},
        {"amount", this->safe_string(data, "size")},
        {"timestamp", this->safe_integer(data, "timestamp")},
        {"fee", {
            {"cost", this->safe_string(data, "commission")},
            {"currency", this->safe_string(data, "commission_currency")}
        }}
    };
    
    this->emit("trade", trade);
}

void bitflyer_ws::handle_balance_update(const json& data) {
    json balances = data["balances"];
    json result;
    
    for (const auto& balance : balances) {
        std::string currency = this->safe_string(balance, "currency_code");
        result[currency] = {
            {"free", this->safe_string(balance, "available")},
            {"used", this->safe_string(balance, "reserved")},
            {"total", this->safe_string(balance, "amount")}
        };
    }
    
    this->emit("balance", result);
}

std::string bitflyer_ws::get_channel_name(const std::string& channel, const std::string& symbol) {
    return channel + "." + symbol;
}

void bitflyer_ws::initialize_order_book(const std::string& symbol) {
    this->orderbooks[symbol] = {
        {"symbol", symbol},
        {"bids", json::array()},
        {"asks", json::array()},
        {"timestamp", nullptr},
        {"nonce", nullptr}
    };
}

void bitflyer_ws::update_order_book(const std::string& symbol, const json& delta) {
    json& orderbook = this->orderbooks[symbol];
    std::string side = delta["side"];
    double price = std::stod(delta["price"].get<std::string>());
    double amount = std::stod(delta["amount"].get<std::string>());
    
    auto& side_array = orderbook[side];
    bool found = false;
    
    for (auto it = side_array.begin(); it != side_array.end(); ++it) {
        double entry_price = std::stod((*it)[0].get<std::string>());
        if (entry_price == price) {
            if (amount == 0) {
                side_array.erase(it);
            } else {
                (*it)[1] = std::to_string(amount);
            }
            found = true;
            break;
        } else if ((side == "bids" && entry_price < price) || 
                   (side == "asks" && entry_price > price)) {
            if (amount > 0) {
                side_array.insert(it, {std::to_string(price), std::to_string(amount)});
            }
            found = true;
            break;
        }
    }
    
    if (!found && amount > 0) {
        side_array.push_back({std::to_string(price), std::to_string(amount)});
    }
}

long long bitflyer_ws::get_timestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

} // namespace ccxt
