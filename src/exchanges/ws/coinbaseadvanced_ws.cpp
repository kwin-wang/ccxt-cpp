#include "ccxt/exchanges/ws/coinbaseadvanced_ws.h"
#include <boost/format.hpp>
#include <chrono>
#include <openssl/hmac.h>
#include <iomanip>
#include <sstream>

namespace ccxt {

CoinbaseadvancedWS::CoinbaseadvancedWS(const Config& config)
    : WsClient("wss://advanced-trade-ws.coinbase.com", config)
    , coinbaseadvanced(config) {}

void CoinbaseadvancedWS::on_connect() {
    if (!api_key.empty() && !secret.empty()) {
        authenticate();
    }
}

void CoinbaseadvancedWS::authenticate() {
    auto timestamp = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count());

    std::string signature = sign_request(timestamp, "GET", "/ws");

    json auth_message = {
        {"type", "subscribe"},
        {"channel", "user"},
        {"api_key", api_key},
        {"timestamp", timestamp},
        {"signature", signature}
    };

    send(auth_message.dump());
}

std::string CoinbaseadvancedWS::sign_request(const std::string& timestamp, const std::string& method, 
                                           const std::string& path, const std::string& body) {
    std::string message = timestamp + method + path + body;
    
    unsigned char* digest = HMAC(EVP_sha256(), 
                                secret.c_str(), secret.length(),
                                reinterpret_cast<const unsigned char*>(message.c_str()), message.length(),
                                nullptr, nullptr);
    
    std::stringstream ss;
    for(int i = 0; i < 32; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
    }
    
    return ss.str();
}

void CoinbaseadvancedWS::on_message(const json& message) {
    try {
        if (message.contains("type")) {
            std::string type = message["type"].get<std::string>();
            
            if (type == "ticker") {
                handle_ticker_update(message);
            } else if (type == "l2update" || type == "snapshot") {
                handle_level2_update(message);
            } else if (type == "match") {
                handle_trades_update(message);
            } else if (type == "candles") {
                handle_candles_update(message);
            } else if (type == "status") {
                handle_status_update(message);
            } else if (type == "user") {
                handle_user_update(message);
            } else if (type == "orders") {
                handle_orders_update(message);
            } else if (type == "fills") {
                handle_fills_update(message);
            } else if (type == "matches") {
                handle_matches_update(message);
            } else if (type == "error") {
                on_error(message["message"].get<std::string>());
            }
        }
    } catch (const std::exception& e) {
        on_error(std::string("Error processing message: ") + e.what());
    }
}

void CoinbaseadvancedWS::on_error(const std::string& error) {
    // Log error or notify error handler
}

void CoinbaseadvancedWS::on_close() {
    // Clean up resources
    callbacks_.clear();
}

std::string CoinbaseadvancedWS::generate_channel_id(const std::string& channel, const std::string& symbol, const std::string& interval) {
    std::string id = channel;
    if (!symbol.empty()) {
        id += ":" + symbol;
    }
    if (!interval.empty()) {
        id += ":" + interval;
    }
    return id;
}

// Market Data Stream Methods
void CoinbaseadvancedWS::subscribe_ticker(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("ticker", symbol);
    callbacks_[channel_id] = callback;
    
    json params = {
        {"type", "subscribe"},
        {"product_ids", {symbol}},
        {"channel", "ticker"}
    };
    
    send_subscribe_message("ticker", params);
}

void CoinbaseadvancedWS::subscribe_orderbook(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("level2", symbol);
    callbacks_[channel_id] = callback;
    
    json params = {
        {"type", "subscribe"},
        {"product_ids", {symbol}},
        {"channel", "level2"}
    };
    
    send_subscribe_message("level2", params);
}

void CoinbaseadvancedWS::subscribe_trades(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("matches", symbol);
    callbacks_[channel_id] = callback;
    
    json params = {
        {"type", "subscribe"},
        {"product_ids", {symbol}},
        {"channel", "matches"}
    };
    
    send_subscribe_message("matches", params);
}

void CoinbaseadvancedWS::subscribe_candles(const std::string& symbol, const std::string& interval, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("candles", symbol, interval);
    callbacks_[channel_id] = callback;
    
    json params = {
        {"type", "subscribe"},
        {"product_ids", {symbol}},
        {"channel", "candles"},
        {"granularity", interval}
    };
    
    send_subscribe_message("candles", params);
}

void CoinbaseadvancedWS::subscribe_level2(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("level2", symbol);
    callbacks_[channel_id] = callback;
    
    json params = {
        {"type", "subscribe"},
        {"product_ids", {symbol}},
        {"channel", "level2"}
    };
    
    send_subscribe_message("level2", params);
}

void CoinbaseadvancedWS::subscribe_status(std::function<void(const json&)> callback) {
    callbacks_["status"] = callback;
    
    json params = {
        {"type", "subscribe"},
        {"channel", "status"}
    };
    
    send_subscribe_message("status", params);
}

// Private Data Stream Methods
void CoinbaseadvancedWS::subscribe_user(std::function<void(const json&)> callback) {
    callbacks_["user"] = callback;
    send_authenticated_request("user", {});
}

void CoinbaseadvancedWS::subscribe_orders(std::function<void(const json&)> callback) {
    callbacks_["orders"] = callback;
    send_authenticated_request("orders", {});
}

void CoinbaseadvancedWS::subscribe_fills(std::function<void(const json&)> callback) {
    callbacks_["fills"] = callback;
    send_authenticated_request("fills", {});
}

void CoinbaseadvancedWS::subscribe_matches(std::function<void(const json&)> callback) {
    callbacks_["matches"] = callback;
    send_authenticated_request("matches", {});
}

// Trading Operations
void CoinbaseadvancedWS::place_order(const std::string& symbol, const std::string& side, const std::string& type,
                                    double quantity, double price, const std::map<std::string, std::string>& params) {
    json order_params = {
        {"product_id", symbol},
        {"side", side},
        {"order_type", type},
        {"size", quantity}
    };

    if (price > 0) {
        order_params["price"] = price;
    }

    for (const auto& [key, value] : params) {
        order_params[key] = value;
    }

    send_authenticated_request("place_order", order_params);
}

void CoinbaseadvancedWS::cancel_order(const std::string& order_id) {
    json params = {
        {"order_id", order_id}
    };

    send_authenticated_request("cancel_order", params);
}

void CoinbaseadvancedWS::cancel_all_orders(const std::string& symbol) {
    json params = {};
    if (!symbol.empty()) {
        params["product_id"] = symbol;
    }

    send_authenticated_request("cancel_all", params);
}

void CoinbaseadvancedWS::modify_order(const std::string& order_id, const std::string& symbol,
                                     double quantity, double price) {
    json params = {
        {"order_id", order_id},
        {"product_id", symbol},
        {"size", quantity},
        {"price", price}
    };

    send_authenticated_request("modify_order", params);
}

// Unsubscribe Methods
void CoinbaseadvancedWS::unsubscribe_ticker(const std::string& symbol) {
    std::string channel_id = generate_channel_id("ticker", symbol);
    callbacks_.erase(channel_id);
    
    json params = {
        {"type", "unsubscribe"},
        {"product_ids", {symbol}},
        {"channel", "ticker"}
    };
    
    send_unsubscribe_message("ticker", params);
}

void CoinbaseadvancedWS::unsubscribe_orderbook(const std::string& symbol) {
    std::string channel_id = generate_channel_id("level2", symbol);
    callbacks_.erase(channel_id);
    
    json params = {
        {"type", "unsubscribe"},
        {"product_ids", {symbol}},
        {"channel", "level2"}
    };
    
    send_unsubscribe_message("level2", params);
}

void CoinbaseadvancedWS::unsubscribe_trades(const std::string& symbol) {
    std::string channel_id = generate_channel_id("matches", symbol);
    callbacks_.erase(channel_id);
    
    json params = {
        {"type", "unsubscribe"},
        {"product_ids", {symbol}},
        {"channel", "matches"}
    };
    
    send_unsubscribe_message("matches", params);
}

void CoinbaseadvancedWS::unsubscribe_candles(const std::string& symbol, const std::string& interval) {
    std::string channel_id = generate_channel_id("candles", symbol, interval);
    callbacks_.erase(channel_id);
    
    json params = {
        {"type", "unsubscribe"},
        {"product_ids", {symbol}},
        {"channel", "candles"},
        {"granularity", interval}
    };
    
    send_unsubscribe_message("candles", params);
}

void CoinbaseadvancedWS::unsubscribe_level2(const std::string& symbol) {
    std::string channel_id = generate_channel_id("level2", symbol);
    callbacks_.erase(channel_id);
    
    json params = {
        {"type", "unsubscribe"},
        {"product_ids", {symbol}},
        {"channel", "level2"}
    };
    
    send_unsubscribe_message("level2", params);
}

void CoinbaseadvancedWS::unsubscribe_status() {
    callbacks_.erase("status");
    
    json params = {
        {"type", "unsubscribe"},
        {"channel", "status"}
    };
    
    send_unsubscribe_message("status", params);
}

void CoinbaseadvancedWS::unsubscribe_user() {
    callbacks_.erase("user");
    send_authenticated_request("unsubscribe_user", {});
}

void CoinbaseadvancedWS::unsubscribe_orders() {
    callbacks_.erase("orders");
    send_authenticated_request("unsubscribe_orders", {});
}

void CoinbaseadvancedWS::unsubscribe_fills() {
    callbacks_.erase("fills");
    send_authenticated_request("unsubscribe_fills", {});
}

void CoinbaseadvancedWS::unsubscribe_matches() {
    callbacks_.erase("matches");
    send_authenticated_request("unsubscribe_matches", {});
}

// Helper Methods
void CoinbaseadvancedWS::send_subscribe_message(const std::string& channel, const json& params) {
    send(params.dump());
}

void CoinbaseadvancedWS::send_unsubscribe_message(const std::string& channel, const json& params) {
    send(params.dump());
}

void CoinbaseadvancedWS::send_authenticated_request(const std::string& type, const json& params) {
    if (api_key.empty() || secret.empty()) {
        throw std::runtime_error("API key and secret are required for authenticated requests");
    }

    auto timestamp = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count());

    json request = params;
    request["type"] = type;
    request["api_key"] = api_key;
    request["timestamp"] = timestamp;
    
    std::string body = request.dump();
    request["signature"] = sign_request(timestamp, "GET", "/ws", body);

    send(request.dump());
}

// Update Handlers
void CoinbaseadvancedWS::handle_ticker_update(const json& data) {
    std::string symbol = data["product_id"].get<std::string>();
    std::string channel_id = generate_channel_id("ticker", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void CoinbaseadvancedWS::handle_level2_update(const json& data) {
    std::string symbol = data["product_id"].get<std::string>();
    std::string channel_id = generate_channel_id("level2", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void CoinbaseadvancedWS::handle_trades_update(const json& data) {
    std::string symbol = data["product_id"].get<std::string>();
    std::string channel_id = generate_channel_id("matches", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void CoinbaseadvancedWS::handle_candles_update(const json& data) {
    std::string symbol = data["product_id"].get<std::string>();
    std::string interval = data["granularity"].get<std::string>();
    std::string channel_id = generate_channel_id("candles", symbol, interval);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void CoinbaseadvancedWS::handle_status_update(const json& data) {
    if (callbacks_.find("status") != callbacks_.end()) {
        callbacks_["status"](data);
    }
}

void CoinbaseadvancedWS::handle_user_update(const json& data) {
    if (callbacks_.find("user") != callbacks_.end()) {
        callbacks_["user"](data);
    }
}

void CoinbaseadvancedWS::handle_orders_update(const json& data) {
    if (callbacks_.find("orders") != callbacks_.end()) {
        callbacks_["orders"](data);
    }
}

void CoinbaseadvancedWS::handle_fills_update(const json& data) {
    if (callbacks_.find("fills") != callbacks_.end()) {
        callbacks_["fills"](data);
    }
}

void CoinbaseadvancedWS::handle_matches_update(const json& data) {
    if (callbacks_.find("matches") != callbacks_.end()) {
        callbacks_["matches"](data);
    }
}

} // namespace ccxt
