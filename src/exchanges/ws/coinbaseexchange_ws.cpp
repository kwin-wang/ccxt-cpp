#include "ccxt/exchanges/ws/coinbaseexchange_ws.h"
#include <boost/format.hpp>
#include <chrono>
#include <openssl/hmac.h>
#include <openssl/sha.h>

namespace ccxt {

CoinbaseExchangeWS::CoinbaseExchangeWS(const Config& config)
    : WsClient("wss://ws-feed.exchange.coinbase.com", config)
    , coinbaseexchange(config) {}

void CoinbaseExchangeWS::on_connect() {
    if (!api_key.empty() && !secret.empty()) {
        authenticate();
    }
}

void CoinbaseExchangeWS::authenticate() {
    long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    std::string message = boost::str(boost::format("%lld") % timestamp);
    std::string signature = sign_message(message, secret);

    json auth_message = {
        {"type", "subscribe"},
        {"channels", {"user", "heartbeat"}},
        {"signature", signature},
        {"key", api_key},
        {"passphrase", api_passphrase},
        {"timestamp", timestamp}
    };

    send(auth_message.dump());
}

void CoinbaseExchangeWS::on_message(const json& message) {
    try {
        std::string type = message["type"].get<std::string>();

        if (type == "error") {
            on_error(message["message"].get<std::string>());
            return;
        }

        if (type == "ticker") {
            handle_ticker_update(message);
        } else if (type == "snapshot" || type == "l2update") {
            handle_level2_update(message);
        } else if (type == "match") {
            handle_trades_update(message);
        } else if (type == "status") {
            handle_status_update(message);
        } else if (type == "heartbeat") {
            handle_heartbeat_update(message);
        } else if (type == "user") {
            handle_user_update(message);
        } else if (type == "received" || type == "open" || type == "done" || type == "change") {
            handle_orders_update(message);
        } else if (type == "match") {
            handle_matches_update(message);
        } else if (type == "full") {
            handle_full_update(message);
        }
    } catch (const std::exception& e) {
        on_error(std::string("Error processing message: ") + e.what());
    }
}

void CoinbaseExchangeWS::on_error(const std::string& error) {
    // Log error or notify error handler
}

void CoinbaseExchangeWS::on_close() {
    // Clean up resources
    callbacks_.clear();
}

std::string CoinbaseExchangeWS::generate_channel_id(const std::string& channel, const std::string& symbol) {
    return symbol.empty() ? channel : channel + ":" + symbol;
}

// Market Data Stream Methods
void CoinbaseExchangeWS::subscribe_ticker(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("ticker", symbol);
    callbacks_[channel_id] = callback;
    send_subscribe_message("ticker", symbol);
}

void CoinbaseExchangeWS::subscribe_tickers(const std::vector<std::string>& symbols, std::function<void(const json&)> callback) {
    for (const auto& symbol : symbols) {
        subscribe_ticker(symbol, callback);
    }
}

void CoinbaseExchangeWS::subscribe_orderbook(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("level2", symbol);
    callbacks_[channel_id] = callback;
    send_subscribe_message("level2", symbol);
}

void CoinbaseExchangeWS::subscribe_trades(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("matches", symbol);
    callbacks_[channel_id] = callback;
    send_subscribe_message("matches", symbol);
}

void CoinbaseExchangeWS::subscribe_level2(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("level2", symbol);
    callbacks_[channel_id] = callback;
    send_subscribe_message("level2", symbol);
}

void CoinbaseExchangeWS::subscribe_status(std::function<void(const json&)> callback) {
    callbacks_["status"] = callback;
    send_subscribe_message("status");
}

void CoinbaseExchangeWS::subscribe_heartbeat(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("heartbeat", symbol);
    callbacks_[channel_id] = callback;
    send_subscribe_message("heartbeat", symbol);
}

// Private Data Stream Methods
void CoinbaseExchangeWS::subscribe_user(std::function<void(const json&)> callback) {
    callbacks_["user"] = callback;
    send_subscribe_message("user", "", true);
}

void CoinbaseExchangeWS::subscribe_orders(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("orders", symbol);
    callbacks_[channel_id] = callback;
    send_subscribe_message("orders", symbol, true);
}

void CoinbaseExchangeWS::subscribe_matches(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("matches", symbol);
    callbacks_[channel_id] = callback;
    send_subscribe_message("matches", symbol, true);
}

void CoinbaseExchangeWS::subscribe_full(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("full", symbol);
    callbacks_[channel_id] = callback;
    send_subscribe_message("full", symbol, true);
}

// Unsubscribe Methods
void CoinbaseExchangeWS::unsubscribe_ticker(const std::string& symbol) {
    std::string channel_id = generate_channel_id("ticker", symbol);
    callbacks_.erase(channel_id);
    send_unsubscribe_message("ticker", symbol);
}

void CoinbaseExchangeWS::unsubscribe_orderbook(const std::string& symbol) {
    std::string channel_id = generate_channel_id("level2", symbol);
    callbacks_.erase(channel_id);
    send_unsubscribe_message("level2", symbol);
}

void CoinbaseExchangeWS::unsubscribe_trades(const std::string& symbol) {
    std::string channel_id = generate_channel_id("matches", symbol);
    callbacks_.erase(channel_id);
    send_unsubscribe_message("matches", symbol);
}

void CoinbaseExchangeWS::unsubscribe_level2(const std::string& symbol) {
    std::string channel_id = generate_channel_id("level2", symbol);
    callbacks_.erase(channel_id);
    send_unsubscribe_message("level2", symbol);
}

void CoinbaseExchangeWS::unsubscribe_status() {
    callbacks_.erase("status");
    send_unsubscribe_message("status");
}

void CoinbaseExchangeWS::unsubscribe_heartbeat(const std::string& symbol) {
    std::string channel_id = generate_channel_id("heartbeat", symbol);
    callbacks_.erase(channel_id);
    send_unsubscribe_message("heartbeat", symbol);
}

void CoinbaseExchangeWS::unsubscribe_user() {
    callbacks_.erase("user");
    send_unsubscribe_message("user", "", true);
}

void CoinbaseExchangeWS::unsubscribe_orders(const std::string& symbol) {
    std::string channel_id = generate_channel_id("orders", symbol);
    callbacks_.erase(channel_id);
    send_unsubscribe_message("orders", symbol, true);
}

void CoinbaseExchangeWS::unsubscribe_matches(const std::string& symbol) {
    std::string channel_id = generate_channel_id("matches", symbol);
    callbacks_.erase(channel_id);
    send_unsubscribe_message("matches", symbol, true);
}

void CoinbaseExchangeWS::unsubscribe_full(const std::string& symbol) {
    std::string channel_id = generate_channel_id("full", symbol);
    callbacks_.erase(channel_id);
    send_unsubscribe_message("full", symbol, true);
}

void CoinbaseExchangeWS::send_subscribe_message(const std::string& channel, const std::string& symbol, bool is_private) {
    json subscribe_msg = {
        {"type", "subscribe"},
        {"channels", {{
            {"name", channel}
        }}}
    };

    if (!symbol.empty()) {
        subscribe_msg["channels"][0]["product_ids"] = {symbol};
    }

    if (is_private) {
        authenticate();
    }

    send(subscribe_msg.dump());
}

void CoinbaseExchangeWS::send_unsubscribe_message(const std::string& channel, const std::string& symbol, bool is_private) {
    json unsubscribe_msg = {
        {"type", "unsubscribe"},
        {"channels", {{
            {"name", channel}
        }}}
    };

    if (!symbol.empty()) {
        unsubscribe_msg["channels"][0]["product_ids"] = {symbol};
    }

    send(unsubscribe_msg.dump());
}

// Update Handlers
void CoinbaseExchangeWS::handle_ticker_update(const json& data) {
    std::string symbol = data["product_id"].get<std::string>();
    std::string channel_id = generate_channel_id("ticker", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void CoinbaseExchangeWS::handle_orderbook_update(const json& data) {
    std::string symbol = data["product_id"].get<std::string>();
    std::string channel_id = generate_channel_id("level2", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void CoinbaseExchangeWS::handle_trades_update(const json& data) {
    std::string symbol = data["product_id"].get<std::string>();
    std::string channel_id = generate_channel_id("matches", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void CoinbaseExchangeWS::handle_level2_update(const json& data) {
    std::string symbol = data["product_id"].get<std::string>();
    std::string channel_id = generate_channel_id("level2", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void CoinbaseExchangeWS::handle_status_update(const json& data) {
    if (callbacks_.find("status") != callbacks_.end()) {
        callbacks_["status"](data);
    }
}

void CoinbaseExchangeWS::handle_heartbeat_update(const json& data) {
    std::string symbol = data["product_id"].get<std::string>();
    std::string channel_id = generate_channel_id("heartbeat", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void CoinbaseExchangeWS::handle_user_update(const json& data) {
    if (callbacks_.find("user") != callbacks_.end()) {
        callbacks_["user"](data);
    }
}

void CoinbaseExchangeWS::handle_orders_update(const json& data) {
    std::string symbol = data["product_id"].get<std::string>();
    std::string channel_id = generate_channel_id("orders", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void CoinbaseExchangeWS::handle_matches_update(const json& data) {
    std::string symbol = data["product_id"].get<std::string>();
    std::string channel_id = generate_channel_id("matches", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void CoinbaseExchangeWS::handle_full_update(const json& data) {
    std::string symbol = data["product_id"].get<std::string>();
    std::string channel_id = generate_channel_id("full", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

} // namespace ccxt
