#include "ccxt/exchanges/ws/coinone_ws.h"
#include <boost/format.hpp>
#include <chrono>

namespace ccxt {

CoinoneWS::CoinoneWS(const Config& config)
    : WsClient("wss://stream.coinone.co.kr", config)
    , coinone(config) {}

void CoinoneWS::on_connect() {
    if (!api_key.empty() && !secret.empty()) {
        authenticate();
    }
}

void CoinoneWS::authenticate() {
    long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    std::string payload = boost::str(boost::format("%s%lld") % api_key % timestamp);
    std::string signature = sign_message(payload, secret);

    json auth_message = {
        {"type", "auth"},
        {"key", api_key},
        {"timestamp", timestamp},
        {"signature", signature}
    };

    send(auth_message.dump());
}

void CoinoneWS::on_message(const json& message) {
    try {
        std::string type = message["type"].get<std::string>();

        if (type == "error") {
            on_error(message["message"].get<std::string>());
            return;
        }

        if (type == "ticker") {
            handle_ticker_update(message["data"]);
        } else if (type == "orderbook") {
            handle_orderbook_update(message["data"]);
        } else if (type == "trades") {
            handle_trades_update(message["data"]);
        } else if (type == "orders") {
            handle_orders_update(message["data"]);
        } else if (type == "balance") {
            handle_balance_update(message["data"]);
        } else if (type == "positions") {
            handle_positions_update(message["data"]);
        }
    } catch (const std::exception& e) {
        on_error(std::string("Error processing message: ") + e.what());
    }
}

void CoinoneWS::on_error(const std::string& error) {
    // Log error or notify error handler
}

void CoinoneWS::on_close() {
    // Clean up resources
    callbacks_.clear();
}

std::string CoinoneWS::generate_channel_id(const std::string& channel, const std::string& symbol) {
    return channel + ":" + symbol;
}

// Market Data Stream Methods
void CoinoneWS::subscribe_ticker(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("ticker", symbol);
    callbacks_[channel_id] = callback;

    json subscribe_msg = {
        {"type", "subscribe"},
        {"channel", "ticker"},
        {"symbol", symbol}
    };

    send(subscribe_msg.dump());
}

void CoinoneWS::subscribe_orderbook(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("orderbook", symbol);
    callbacks_[channel_id] = callback;

    json subscribe_msg = {
        {"type", "subscribe"},
        {"channel", "orderbook"},
        {"symbol", symbol}
    };

    send(subscribe_msg.dump());
}

void CoinoneWS::subscribe_trades(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("trades", symbol);
    callbacks_[channel_id] = callback;

    json subscribe_msg = {
        {"type", "subscribe"},
        {"channel", "trades"},
        {"symbol", symbol}
    };

    send(subscribe_msg.dump());
}

// Private Data Stream Methods
void CoinoneWS::subscribe_orders(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("orders", symbol);
    callbacks_[channel_id] = callback;

    json subscribe_msg = {
        {"type", "subscribe"},
        {"channel", "orders"},
        {"symbol", symbol}
    };

    send(subscribe_msg.dump());
}

void CoinoneWS::subscribe_balance(std::function<void(const json&)> callback) {
    callbacks_["balance"] = callback;

    json subscribe_msg = {
        {"type", "subscribe"},
        {"channel", "balance"}
    };

    send(subscribe_msg.dump());
}

void CoinoneWS::subscribe_positions(std::function<void(const json&)> callback) {
    callbacks_["positions"] = callback;

    json subscribe_msg = {
        {"type", "subscribe"},
        {"channel", "positions"}
    };

    send(subscribe_msg.dump());
}

// Unsubscribe Methods
void CoinoneWS::unsubscribe_ticker(const std::string& symbol) {
    std::string channel_id = generate_channel_id("ticker", symbol);
    callbacks_.erase(channel_id);

    json unsubscribe_msg = {
        {"type", "unsubscribe"},
        {"channel", "ticker"},
        {"symbol", symbol}
    };

    send(unsubscribe_msg.dump());
}

void CoinoneWS::unsubscribe_orderbook(const std::string& symbol) {
    std::string channel_id = generate_channel_id("orderbook", symbol);
    callbacks_.erase(channel_id);

    json unsubscribe_msg = {
        {"type", "unsubscribe"},
        {"channel", "orderbook"},
        {"symbol", symbol}
    };

    send(unsubscribe_msg.dump());
}

void CoinoneWS::unsubscribe_trades(const std::string& symbol) {
    std::string channel_id = generate_channel_id("trades", symbol);
    callbacks_.erase(channel_id);

    json unsubscribe_msg = {
        {"type", "unsubscribe"},
        {"channel", "trades"},
        {"symbol", symbol}
    };

    send(unsubscribe_msg.dump());
}

void CoinoneWS::unsubscribe_orders(const std::string& symbol) {
    std::string channel_id = generate_channel_id("orders", symbol);
    callbacks_.erase(channel_id);

    json unsubscribe_msg = {
        {"type", "unsubscribe"},
        {"channel", "orders"},
        {"symbol", symbol}
    };

    send(unsubscribe_msg.dump());
}

void CoinoneWS::unsubscribe_balance() {
    callbacks_.erase("balance");

    json unsubscribe_msg = {
        {"type", "unsubscribe"},
        {"channel", "balance"}
    };

    send(unsubscribe_msg.dump());
}

void CoinoneWS::unsubscribe_positions() {
    callbacks_.erase("positions");

    json unsubscribe_msg = {
        {"type", "unsubscribe"},
        {"channel", "positions"}
    };

    send(unsubscribe_msg.dump());
}

// Update Handlers
void CoinoneWS::handle_ticker_update(const json& data) {
    std::string symbol = data["symbol"].get<std::string>();
    std::string channel_id = generate_channel_id("ticker", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void CoinoneWS::handle_orderbook_update(const json& data) {
    std::string symbol = data["symbol"].get<std::string>();
    std::string channel_id = generate_channel_id("orderbook", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void CoinoneWS::handle_trades_update(const json& data) {
    std::string symbol = data["symbol"].get<std::string>();
    std::string channel_id = generate_channel_id("trades", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void CoinoneWS::handle_orders_update(const json& data) {
    std::string symbol = data["symbol"].get<std::string>();
    std::string channel_id = generate_channel_id("orders", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void CoinoneWS::handle_balance_update(const json& data) {
    if (callbacks_.find("balance") != callbacks_.end()) {
        callbacks_["balance"](data);
    }
}

void CoinoneWS::handle_positions_update(const json& data) {
    if (callbacks_.find("positions") != callbacks_.end()) {
        callbacks_["positions"](data);
    }
}

} // namespace ccxt
