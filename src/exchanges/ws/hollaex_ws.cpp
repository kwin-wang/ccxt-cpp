#include "ccxt/exchanges/ws/hollaex_ws.h"
#include <boost/format.hpp>
#include <chrono>
#include <openssl/hmac.h>
#include <iomanip>
#include <sstream>

namespace ccxt {

HollaexWS::HollaexWS(const Config& config)
    : WsClient("wss://api.hollaex.com/stream", config)
    , hollaex(config) {}

void HollaexWS::on_connect() {
    if (!api_key.empty() && !secret.empty()) {
        authenticate();
    }
}

void HollaexWS::authenticate() {
    auto timestamp = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count());

    std::string signature = sign_request(timestamp, "GET", "/stream");

    json auth_message = {
        {"event", "auth"},
        {"data", {
            {"apiKey", api_key},
            {"timestamp", timestamp},
            {"signature", signature}
        }}
    };

    send(auth_message.dump());
}

std::string HollaexWS::sign_request(const std::string& timestamp, const std::string& method, const std::string& path) {
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

void HollaexWS::on_message(const json& message) {
    try {
        if (message.contains("error")) {
            on_error(message["error"]["message"].get<std::string>());
            return;
        }

        if (message.contains("event")) {
            std::string event = message["event"].get<std::string>();
            
            if (event == "ticker") {
                handle_ticker_update(message["data"]);
            } else if (event == "orderbook") {
                handle_orderbook_update(message["data"]);
            } else if (event == "trades") {
                handle_trades_update(message["data"]);
            } else if (event == "chart") {
                handle_chart_update(message["data"]);
            } else if (event == "market") {
                handle_market_update(message["data"]);
            } else if (event == "user") {
                handle_user_update(message["data"]);
            } else if (event == "orders") {
                handle_orders_update(message["data"]);
            } else if (event == "trades_history") {
                handle_trades_history_update(message["data"]);
            } else if (event == "balance") {
                handle_balance_update(message["data"]);
            } else if (event == "wallet") {
                handle_wallet_update(message["data"]);
            }
        }
    } catch (const std::exception& e) {
        on_error(std::string("Error processing message: ") + e.what());
    }
}

void HollaexWS::on_error(const std::string& error) {
    // Log error or notify error handler
}

void HollaexWS::on_close() {
    // Clean up resources
    callbacks_.clear();
}

std::string HollaexWS::generate_channel_id(const std::string& channel, const std::string& symbol, const std::string& interval) {
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
void HollaexWS::subscribe_ticker(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("ticker", symbol);
    callbacks_[channel_id] = callback;
    
    json data = {
        {"symbol", symbol}
    };
    
    send_subscribe_message("subscribe", {
        {"topic", "ticker"},
        {"data", data}
    });
}

void HollaexWS::subscribe_orderbook(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("orderbook", symbol);
    callbacks_[channel_id] = callback;
    
    json data = {
        {"symbol", symbol}
    };
    
    send_subscribe_message("subscribe", {
        {"topic", "orderbook"},
        {"data", data}
    });
}

void HollaexWS::subscribe_trades(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("trades", symbol);
    callbacks_[channel_id] = callback;
    
    json data = {
        {"symbol", symbol}
    };
    
    send_subscribe_message("subscribe", {
        {"topic", "trades"},
        {"data", data}
    });
}

void HollaexWS::subscribe_chart(const std::string& symbol, const std::string& interval, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("chart", symbol, interval);
    callbacks_[channel_id] = callback;
    
    json data = {
        {"symbol", symbol},
        {"interval", interval}
    };
    
    send_subscribe_message("subscribe", {
        {"topic", "chart"},
        {"data", data}
    });
}

void HollaexWS::subscribe_market(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("market", symbol);
    callbacks_[channel_id] = callback;
    
    json data = {
        {"symbol", symbol}
    };
    
    send_subscribe_message("subscribe", {
        {"topic", "market"},
        {"data", data}
    });
}

// Private Data Stream Methods
void HollaexWS::subscribe_user(std::function<void(const json&)> callback) {
    callbacks_["user"] = callback;
    send_authenticated_request("subscribe", {{"topic", "user"}});
}

void HollaexWS::subscribe_orders(std::function<void(const json&)> callback) {
    callbacks_["orders"] = callback;
    send_authenticated_request("subscribe", {{"topic", "orders"}});
}

void HollaexWS::subscribe_trades_history(std::function<void(const json&)> callback) {
    callbacks_["trades_history"] = callback;
    send_authenticated_request("subscribe", {{"topic", "trades_history"}});
}

void HollaexWS::subscribe_balance(std::function<void(const json&)> callback) {
    callbacks_["balance"] = callback;
    send_authenticated_request("subscribe", {{"topic", "balance"}});
}

void HollaexWS::subscribe_wallet(std::function<void(const json&)> callback) {
    callbacks_["wallet"] = callback;
    send_authenticated_request("subscribe", {{"topic", "wallet"}});
}

// Trading Operations
void HollaexWS::place_order(const std::string& symbol, const std::string& side, const std::string& type,
                           double quantity, double price, const std::map<std::string, std::string>& params) {
    json order_data = {
        {"symbol", symbol},
        {"side", side},
        {"type", type},
        {"size", quantity}
    };

    if (price > 0) {
        order_data["price"] = price;
    }

    for (const auto& [key, value] : params) {
        order_data[key] = value;
    }

    send_authenticated_request("order", order_data);
}

void HollaexWS::cancel_order(const std::string& order_id, const std::string& symbol) {
    json data = {
        {"order_id", order_id},
        {"symbol", symbol}
    };

    send_authenticated_request("cancel_order", data);
}

void HollaexWS::cancel_all_orders(const std::string& symbol) {
    json data = {};
    if (!symbol.empty()) {
        data["symbol"] = symbol;
    }

    send_authenticated_request("cancel_all_orders", data);
}

void HollaexWS::modify_order(const std::string& order_id, const std::string& symbol,
                            double quantity, double price) {
    json data = {
        {"order_id", order_id},
        {"symbol", symbol},
        {"size", quantity},
        {"price", price}
    };

    send_authenticated_request("modify_order", data);
}

// Unsubscribe Methods
void HollaexWS::unsubscribe_ticker(const std::string& symbol) {
    std::string channel_id = generate_channel_id("ticker", symbol);
    callbacks_.erase(channel_id);
    
    json data = {
        {"symbol", symbol}
    };
    
    send_unsubscribe_message("unsubscribe", {
        {"topic", "ticker"},
        {"data", data}
    });
}

void HollaexWS::unsubscribe_orderbook(const std::string& symbol) {
    std::string channel_id = generate_channel_id("orderbook", symbol);
    callbacks_.erase(channel_id);
    
    json data = {
        {"symbol", symbol}
    };
    
    send_unsubscribe_message("unsubscribe", {
        {"topic", "orderbook"},
        {"data", data}
    });
}

void HollaexWS::unsubscribe_trades(const std::string& symbol) {
    std::string channel_id = generate_channel_id("trades", symbol);
    callbacks_.erase(channel_id);
    
    json data = {
        {"symbol", symbol}
    };
    
    send_unsubscribe_message("unsubscribe", {
        {"topic", "trades"},
        {"data", data}
    });
}

void HollaexWS::unsubscribe_chart(const std::string& symbol, const std::string& interval) {
    std::string channel_id = generate_channel_id("chart", symbol, interval);
    callbacks_.erase(channel_id);
    
    json data = {
        {"symbol", symbol},
        {"interval", interval}
    };
    
    send_unsubscribe_message("unsubscribe", {
        {"topic", "chart"},
        {"data", data}
    });
}

void HollaexWS::unsubscribe_market(const std::string& symbol) {
    std::string channel_id = generate_channel_id("market", symbol);
    callbacks_.erase(channel_id);
    
    json data = {
        {"symbol", symbol}
    };
    
    send_unsubscribe_message("unsubscribe", {
        {"topic", "market"},
        {"data", data}
    });
}

void HollaexWS::unsubscribe_user() {
    callbacks_.erase("user");
    send_authenticated_request("unsubscribe", {{"topic", "user"}});
}

void HollaexWS::unsubscribe_orders() {
    callbacks_.erase("orders");
    send_authenticated_request("unsubscribe", {{"topic", "orders"}});
}

void HollaexWS::unsubscribe_trades_history() {
    callbacks_.erase("trades_history");
    send_authenticated_request("unsubscribe", {{"topic", "trades_history"}});
}

void HollaexWS::unsubscribe_balance() {
    callbacks_.erase("balance");
    send_authenticated_request("unsubscribe", {{"topic", "balance"}});
}

void HollaexWS::unsubscribe_wallet() {
    callbacks_.erase("wallet");
    send_authenticated_request("unsubscribe", {{"topic", "wallet"}});
}

// Helper Methods
void HollaexWS::send_subscribe_message(const std::string& event, const json& data) {
    json message = {
        {"event", event},
        {"data", data}
    };
    send(message.dump());
}

void HollaexWS::send_unsubscribe_message(const std::string& event, const json& data) {
    json message = {
        {"event", event},
        {"data", data}
    };
    send(message.dump());
}

void HollaexWS::send_authenticated_request(const std::string& event, const json& data) {
    if (api_key.empty() || secret.empty()) {
        throw std::runtime_error("API key and secret are required for authenticated requests");
    }

    auto timestamp = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count());

    json request = {
        {"event", event},
        {"data", data},
        {"timestamp", timestamp}
    };
    
    request["signature"] = sign_request(timestamp, "GET", "/stream");

    send(request.dump());
}

// Update Handlers
void HollaexWS::handle_ticker_update(const json& data) {
    std::string symbol = data["symbol"].get<std::string>();
    std::string channel_id = generate_channel_id("ticker", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void HollaexWS::handle_orderbook_update(const json& data) {
    std::string symbol = data["symbol"].get<std::string>();
    std::string channel_id = generate_channel_id("orderbook", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void HollaexWS::handle_trades_update(const json& data) {
    std::string symbol = data["symbol"].get<std::string>();
    std::string channel_id = generate_channel_id("trades", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void HollaexWS::handle_chart_update(const json& data) {
    std::string symbol = data["symbol"].get<std::string>();
    std::string interval = data["interval"].get<std::string>();
    std::string channel_id = generate_channel_id("chart", symbol, interval);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void HollaexWS::handle_market_update(const json& data) {
    std::string symbol = data["symbol"].get<std::string>();
    std::string channel_id = generate_channel_id("market", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void HollaexWS::handle_user_update(const json& data) {
    if (callbacks_.find("user") != callbacks_.end()) {
        callbacks_["user"](data);
    }
}

void HollaexWS::handle_orders_update(const json& data) {
    if (callbacks_.find("orders") != callbacks_.end()) {
        callbacks_["orders"](data);
    }
}

void HollaexWS::handle_trades_history_update(const json& data) {
    if (callbacks_.find("trades_history") != callbacks_.end()) {
        callbacks_["trades_history"](data);
    }
}

void HollaexWS::handle_balance_update(const json& data) {
    if (callbacks_.find("balance") != callbacks_.end()) {
        callbacks_["balance"](data);
    }
}

void HollaexWS::handle_wallet_update(const json& data) {
    if (callbacks_.find("wallet") != callbacks_.end()) {
        callbacks_["wallet"](data);
    }
}

} // namespace ccxt
