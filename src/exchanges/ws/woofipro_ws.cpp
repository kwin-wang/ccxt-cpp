#include "ccxt/exchanges/ws/woofipro_ws.h"
#include <boost/format.hpp>
#include <chrono>
#include <openssl/hmac.h>
#include <iomanip>
#include <sstream>

namespace ccxt {

WoofiproWS::WoofiproWS(const Config& config)
    : WsClient("wss://wss.woo.org/ws/stream", config)
    , woofipro(config) {}

void WoofiproWS::on_connect() {
    if (!api_key.empty() && !secret.empty()) {
        authenticate();
    }
}

void WoofiproWS::authenticate() {
    auto timestamp = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count());

    std::string signature = sign_request(timestamp, "GET", "/auth");

    json auth_message = {
        {"event", "auth"},
        {"params", {
            {"apikey", api_key},
            {"sign", signature},
            {"timestamp", timestamp}
        }}
    };

    send(auth_message.dump());
}

std::string WoofiproWS::sign_request(const std::string& timestamp, const std::string& method, const std::string& path) {
    std::string message = timestamp + method + path;
    
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

void WoofiproWS::on_message(const json& message) {
    try {
        if (message.contains("status") && message["status"] != "success") {
            on_error(message["message"].get<std::string>());
            return;
        }

        if (message.contains("topic")) {
            std::string topic = message["topic"].get<std::string>();
            
            if (topic.find("market.") == 0) {
                if (topic.find(".ticker") != std::string::npos) {
                    handle_ticker_update(message["data"]);
                } else if (topic.find(".orderbook") != std::string::npos) {
                    handle_orderbook_update(message["data"]);
                } else if (topic.find(".trade") != std::string::npos) {
                    handle_trades_update(message["data"]);
                } else if (topic.find(".kline") != std::string::npos) {
                    handle_kline_update(message["data"]);
                } else if (topic.find(".summary") != std::string::npos) {
                    handle_market_summary_update(message["data"]);
                }
            } else if (topic.find("private.") == 0) {
                if (topic == "private.order") {
                    handle_orders_update(message["data"]);
                } else if (topic == "private.trade") {
                    handle_trades_history_update(message["data"]);
                } else if (topic == "private.balance") {
                    handle_balance_update(message["data"]);
                } else if (topic == "private.position") {
                    handle_positions_update(message["data"]);
                }
            }
        }
    } catch (const std::exception& e) {
        on_error(std::string("Error processing message: ") + e.what());
    }
}

void WoofiproWS::on_error(const std::string& error) {
    // Log error or notify error handler
}

void WoofiproWS::on_close() {
    // Clean up resources
    callbacks_.clear();
}

std::string WoofiproWS::generate_channel_id(const std::string& channel, const std::string& symbol, const std::string& interval) {
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
void WoofiproWS::subscribe_ticker(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("ticker", symbol);
    callbacks_[channel_id] = callback;
    
    json params = {
        {"symbol", symbol}
    };
    
    send_subscribe_message("market." + symbol + ".ticker", params);
}

void WoofiproWS::subscribe_orderbook(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("orderbook", symbol);
    callbacks_[channel_id] = callback;
    
    json params = {
        {"symbol", symbol}
    };
    
    send_subscribe_message("market." + symbol + ".orderbook", params);
}

void WoofiproWS::subscribe_trades(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("trades", symbol);
    callbacks_[channel_id] = callback;
    
    json params = {
        {"symbol", symbol}
    };
    
    send_subscribe_message("market." + symbol + ".trade", params);
}

void WoofiproWS::subscribe_kline(const std::string& symbol, const std::string& interval, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("kline", symbol, interval);
    callbacks_[channel_id] = callback;
    
    json params = {
        {"symbol", symbol},
        {"interval", interval}
    };
    
    send_subscribe_message("market." + symbol + ".kline." + interval, params);
}

void WoofiproWS::subscribe_market_summary(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("summary", symbol);
    callbacks_[channel_id] = callback;
    
    json params = {
        {"symbol", symbol}
    };
    
    send_subscribe_message("market." + symbol + ".summary", params);
}

// Private Data Stream Methods
void WoofiproWS::subscribe_orders(std::function<void(const json&)> callback) {
    callbacks_["orders"] = callback;
    send_authenticated_request("private.order", {});
}

void WoofiproWS::subscribe_trades_history(std::function<void(const json&)> callback) {
    callbacks_["trades"] = callback;
    send_authenticated_request("private.trade", {});
}

void WoofiproWS::subscribe_balance(std::function<void(const json&)> callback) {
    callbacks_["balance"] = callback;
    send_authenticated_request("private.balance", {});
}

void WoofiproWS::subscribe_positions(std::function<void(const json&)> callback) {
    callbacks_["positions"] = callback;
    send_authenticated_request("private.position", {});
}

// Trading Operations
void WoofiproWS::place_order(const std::string& symbol, const std::string& side, const std::string& type,
                            double quantity, double price, const std::map<std::string, std::string>& params) {
    json order_params = {
        {"symbol", symbol},
        {"side", side},
        {"type", type},
        {"size", quantity}
    };

    if (price > 0) {
        order_params["price"] = price;
    }

    for (const auto& [key, value] : params) {
        order_params[key] = value;
    }

    send_authenticated_request("order.place", order_params);
}

void WoofiproWS::cancel_order(const std::string& order_id) {
    json params = {
        {"order_id", order_id}
    };

    send_authenticated_request("order.cancel", params);
}

void WoofiproWS::cancel_all_orders(const std::string& symbol) {
    json params = {};
    if (!symbol.empty()) {
        params["symbol"] = symbol;
    }

    send_authenticated_request("order.cancel_all", params);
}

void WoofiproWS::modify_order(const std::string& order_id, const std::string& symbol,
                             double quantity, double price) {
    json params = {
        {"order_id", order_id},
        {"symbol", symbol},
        {"size", quantity},
        {"price", price}
    };

    send_authenticated_request("order.modify", params);
}

// Unsubscribe Methods
void WoofiproWS::unsubscribe_ticker(const std::string& symbol) {
    std::string channel_id = generate_channel_id("ticker", symbol);
    callbacks_.erase(channel_id);
    
    json params = {
        {"symbol", symbol}
    };
    
    send_unsubscribe_message("market." + symbol + ".ticker", params);
}

void WoofiproWS::unsubscribe_orderbook(const std::string& symbol) {
    std::string channel_id = generate_channel_id("orderbook", symbol);
    callbacks_.erase(channel_id);
    
    json params = {
        {"symbol", symbol}
    };
    
    send_unsubscribe_message("market." + symbol + ".orderbook", params);
}

void WoofiproWS::unsubscribe_trades(const std::string& symbol) {
    std::string channel_id = generate_channel_id("trades", symbol);
    callbacks_.erase(channel_id);
    
    json params = {
        {"symbol", symbol}
    };
    
    send_unsubscribe_message("market." + symbol + ".trade", params);
}

void WoofiproWS::unsubscribe_kline(const std::string& symbol, const std::string& interval) {
    std::string channel_id = generate_channel_id("kline", symbol, interval);
    callbacks_.erase(channel_id);
    
    json params = {
        {"symbol", symbol},
        {"interval", interval}
    };
    
    send_unsubscribe_message("market." + symbol + ".kline." + interval, params);
}

void WoofiproWS::unsubscribe_market_summary(const std::string& symbol) {
    std::string channel_id = generate_channel_id("summary", symbol);
    callbacks_.erase(channel_id);
    
    json params = {
        {"symbol", symbol}
    };
    
    send_unsubscribe_message("market." + symbol + ".summary", params);
}

void WoofiproWS::unsubscribe_orders() {
    callbacks_.erase("orders");
    send_authenticated_request("private.order.unsubscribe", {});
}

void WoofiproWS::unsubscribe_trades_history() {
    callbacks_.erase("trades");
    send_authenticated_request("private.trade.unsubscribe", {});
}

void WoofiproWS::unsubscribe_balance() {
    callbacks_.erase("balance");
    send_authenticated_request("private.balance.unsubscribe", {});
}

void WoofiproWS::unsubscribe_positions() {
    callbacks_.erase("positions");
    send_authenticated_request("private.position.unsubscribe", {});
}

// Helper Methods
void WoofiproWS::send_subscribe_message(const std::string& topic, const json& params) {
    json subscribe_msg = {
        {"event", "subscribe"},
        {"topic", topic},
        {"params", params}
    };

    send(subscribe_msg.dump());
}

void WoofiproWS::send_unsubscribe_message(const std::string& topic, const json& params) {
    json unsubscribe_msg = {
        {"event", "unsubscribe"},
        {"topic", topic},
        {"params", params}
    };

    send(unsubscribe_msg.dump());
}

void WoofiproWS::send_authenticated_request(const std::string& topic, const json& params) {
    if (api_key.empty() || secret.empty()) {
        throw std::runtime_error("API key and secret are required for authenticated requests");
    }

    auto timestamp = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count());

    std::string signature = sign_request(timestamp, "GET", "/" + topic);

    json request = {
        {"event", topic},
        {"params", params},
        {"apikey", api_key},
        {"sign", signature},
        {"timestamp", timestamp}
    };

    send(request.dump());
}

// Update Handlers
void WoofiproWS::handle_ticker_update(const json& data) {
    std::string symbol = data["symbol"].get<std::string>();
    std::string channel_id = generate_channel_id("ticker", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void WoofiproWS::handle_orderbook_update(const json& data) {
    std::string symbol = data["symbol"].get<std::string>();
    std::string channel_id = generate_channel_id("orderbook", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void WoofiproWS::handle_trades_update(const json& data) {
    std::string symbol = data["symbol"].get<std::string>();
    std::string channel_id = generate_channel_id("trades", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void WoofiproWS::handle_kline_update(const json& data) {
    std::string symbol = data["symbol"].get<std::string>();
    std::string interval = data["interval"].get<std::string>();
    std::string channel_id = generate_channel_id("kline", symbol, interval);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void WoofiproWS::handle_market_summary_update(const json& data) {
    std::string symbol = data["symbol"].get<std::string>();
    std::string channel_id = generate_channel_id("summary", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void WoofiproWS::handle_orders_update(const json& data) {
    if (callbacks_.find("orders") != callbacks_.end()) {
        callbacks_["orders"](data);
    }
}

void WoofiproWS::handle_trades_history_update(const json& data) {
    if (callbacks_.find("trades") != callbacks_.end()) {
        callbacks_["trades"](data);
    }
}

void WoofiproWS::handle_balance_update(const json& data) {
    if (callbacks_.find("balance") != callbacks_.end()) {
        callbacks_["balance"](data);
    }
}

void WoofiproWS::handle_positions_update(const json& data) {
    if (callbacks_.find("positions") != callbacks_.end()) {
        callbacks_["positions"](data);
    }
}

} // namespace ccxt
