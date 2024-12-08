#include "ccxt/exchanges/ws/bitpanda_ws.h"
#include <boost/format.hpp>
#include <chrono>

namespace ccxt {

BitpandaWS::BitpandaWS(const Config& config)
    : WsClient("wss://ws.exchange.bitpanda.com/v1", config)
    , bitpanda(config) {}

void BitpandaWS::on_connect() {
    if (!api_key.empty() && !secret.empty()) {
        authenticate();
    }
}

void BitpandaWS::authenticate() {
    long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    std::string payload = boost::str(boost::format("%lld") % timestamp);
    std::string signature = sign_message(payload, secret);

    json auth_message = {
        {"type", "AUTHENTICATE"},
        {"client_id", api_key},
        {"timestamp", timestamp},
        {"signature", signature}
    };

    send(auth_message.dump());
}

void BitpandaWS::on_message(const json& message) {
    try {
        std::string type = message["type"].get<std::string>();

        if (type == "ERROR") {
            on_error(message["error_message"].get<std::string>());
            return;
        }

        if (type == "PRICE_TICKER") {
            handle_ticker_update(message);
        } else if (type == "ORDER_BOOK_UPDATE") {
            handle_orderbook_update(message);
        } else if (type == "TRADES") {
            handle_trades_update(message);
        } else if (type == "CANDLESTICKS") {
            handle_candlesticks_update(message);
        } else if (type == "MARKET_STATE") {
            handle_market_state_update(message);
        } else if (type == "MARKET_TICKER") {
            handle_market_ticker_update(message);
        } else if (type == "ACCOUNT_UPDATE") {
            handle_account_update(message);
        } else if (type == "ORDER_UPDATE") {
            handle_orders_update(message);
        } else if (type == "TRADE_UPDATE") {
            handle_trades_history_update(message);
        } else if (type == "BALANCE_UPDATE") {
            handle_balances_update(message);
        }
    } catch (const std::exception& e) {
        on_error(std::string("Error processing message: ") + e.what());
    }
}

void BitpandaWS::on_error(const std::string& error) {
    // Log error or notify error handler
}

void BitpandaWS::on_close() {
    // Clean up resources
    callbacks_.clear();
}

std::string BitpandaWS::generate_channel_id(const std::string& channel, const std::string& symbol, const std::string& timeframe) {
    std::string id = channel;
    if (!symbol.empty()) {
        id += ":" + symbol;
    }
    if (!timeframe.empty()) {
        id += ":" + timeframe;
    }
    return id;
}

// Market Data Stream Methods
void BitpandaWS::subscribe_ticker(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("PRICE_TICKER", symbol);
    callbacks_[channel_id] = callback;
    send_subscribe_message("PRICE_TICKER", symbol);
}

void BitpandaWS::subscribe_orderbook(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("ORDER_BOOK", symbol);
    callbacks_[channel_id] = callback;
    send_subscribe_message("ORDER_BOOK", symbol);
}

void BitpandaWS::subscribe_trades(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("TRADES", symbol);
    callbacks_[channel_id] = callback;
    send_subscribe_message("TRADES", symbol);
}

void BitpandaWS::subscribe_candlesticks(const std::string& symbol, const std::string& timeframe, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("CANDLESTICKS", symbol, timeframe);
    callbacks_[channel_id] = callback;
    send_subscribe_message("CANDLESTICKS", symbol, timeframe);
}

void BitpandaWS::subscribe_market_state(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("MARKET_STATE", symbol);
    callbacks_[channel_id] = callback;
    send_subscribe_message("MARKET_STATE", symbol);
}

void BitpandaWS::subscribe_market_ticker(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("MARKET_TICKER", symbol);
    callbacks_[channel_id] = callback;
    send_subscribe_message("MARKET_TICKER", symbol);
}

// Private Data Stream Methods
void BitpandaWS::subscribe_account(std::function<void(const json&)> callback) {
    callbacks_["ACCOUNT"] = callback;
    send_subscribe_message("ACCOUNT", "", "", true);
}

void BitpandaWS::subscribe_orders(std::function<void(const json&)> callback) {
    callbacks_["ORDERS"] = callback;
    send_subscribe_message("ORDERS", "", "", true);
}

void BitpandaWS::subscribe_trades_history(std::function<void(const json&)> callback) {
    callbacks_["TRADES_HISTORY"] = callback;
    send_subscribe_message("TRADES_HISTORY", "", "", true);
}

void BitpandaWS::subscribe_balances(std::function<void(const json&)> callback) {
    callbacks_["BALANCES"] = callback;
    send_subscribe_message("BALANCES", "", "", true);
}

// Unsubscribe Methods
void BitpandaWS::unsubscribe_ticker(const std::string& symbol) {
    std::string channel_id = generate_channel_id("PRICE_TICKER", symbol);
    callbacks_.erase(channel_id);
    send_unsubscribe_message("PRICE_TICKER", symbol);
}

void BitpandaWS::unsubscribe_orderbook(const std::string& symbol) {
    std::string channel_id = generate_channel_id("ORDER_BOOK", symbol);
    callbacks_.erase(channel_id);
    send_unsubscribe_message("ORDER_BOOK", symbol);
}

void BitpandaWS::unsubscribe_trades(const std::string& symbol) {
    std::string channel_id = generate_channel_id("TRADES", symbol);
    callbacks_.erase(channel_id);
    send_unsubscribe_message("TRADES", symbol);
}

void BitpandaWS::unsubscribe_candlesticks(const std::string& symbol, const std::string& timeframe) {
    std::string channel_id = generate_channel_id("CANDLESTICKS", symbol, timeframe);
    callbacks_.erase(channel_id);
    send_unsubscribe_message("CANDLESTICKS", symbol, timeframe);
}

void BitpandaWS::unsubscribe_market_state(const std::string& symbol) {
    std::string channel_id = generate_channel_id("MARKET_STATE", symbol);
    callbacks_.erase(channel_id);
    send_unsubscribe_message("MARKET_STATE", symbol);
}

void BitpandaWS::unsubscribe_market_ticker(const std::string& symbol) {
    std::string channel_id = generate_channel_id("MARKET_TICKER", symbol);
    callbacks_.erase(channel_id);
    send_unsubscribe_message("MARKET_TICKER", symbol);
}

void BitpandaWS::unsubscribe_account() {
    callbacks_.erase("ACCOUNT");
    send_unsubscribe_message("ACCOUNT", "", "", true);
}

void BitpandaWS::unsubscribe_orders() {
    callbacks_.erase("ORDERS");
    send_unsubscribe_message("ORDERS", "", "", true);
}

void BitpandaWS::unsubscribe_trades_history() {
    callbacks_.erase("TRADES_HISTORY");
    send_unsubscribe_message("TRADES_HISTORY", "", "", true);
}

void BitpandaWS::unsubscribe_balances() {
    callbacks_.erase("BALANCES");
    send_unsubscribe_message("BALANCES", "", "", true);
}

void BitpandaWS::send_subscribe_message(const std::string& channel, const std::string& symbol, const std::string& timeframe, bool is_private) {
    json subscribe_msg = {
        {"type", "SUBSCRIBE"},
        {"channels", {{
            {"name", channel}
        }}}
    };

    if (!symbol.empty()) {
        subscribe_msg["channels"][0]["instrument_code"] = symbol;
    }

    if (!timeframe.empty()) {
        subscribe_msg["channels"][0]["granularity"] = timeframe;
    }

    if (is_private) {
        authenticate();
    }

    send(subscribe_msg.dump());
}

void BitpandaWS::send_unsubscribe_message(const std::string& channel, const std::string& symbol, const std::string& timeframe, bool is_private) {
    json unsubscribe_msg = {
        {"type", "UNSUBSCRIBE"},
        {"channels", {{
            {"name", channel}
        }}}
    };

    if (!symbol.empty()) {
        unsubscribe_msg["channels"][0]["instrument_code"] = symbol;
    }

    if (!timeframe.empty()) {
        unsubscribe_msg["channels"][0]["granularity"] = timeframe;
    }

    send(unsubscribe_msg.dump());
}

// Update Handlers
void BitpandaWS::handle_ticker_update(const json& data) {
    std::string symbol = data["instrument_code"].get<std::string>();
    std::string channel_id = generate_channel_id("PRICE_TICKER", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void BitpandaWS::handle_orderbook_update(const json& data) {
    std::string symbol = data["instrument_code"].get<std::string>();
    std::string channel_id = generate_channel_id("ORDER_BOOK", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void BitpandaWS::handle_trades_update(const json& data) {
    std::string symbol = data["instrument_code"].get<std::string>();
    std::string channel_id = generate_channel_id("TRADES", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void BitpandaWS::handle_candlesticks_update(const json& data) {
    std::string symbol = data["instrument_code"].get<std::string>();
    std::string timeframe = data["granularity"].get<std::string>();
    std::string channel_id = generate_channel_id("CANDLESTICKS", symbol, timeframe);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void BitpandaWS::handle_market_state_update(const json& data) {
    std::string symbol = data["instrument_code"].get<std::string>();
    std::string channel_id = generate_channel_id("MARKET_STATE", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void BitpandaWS::handle_market_ticker_update(const json& data) {
    std::string symbol = data["instrument_code"].get<std::string>();
    std::string channel_id = generate_channel_id("MARKET_TICKER", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void BitpandaWS::handle_account_update(const json& data) {
    if (callbacks_.find("ACCOUNT") != callbacks_.end()) {
        callbacks_["ACCOUNT"](data);
    }
}

void BitpandaWS::handle_orders_update(const json& data) {
    if (callbacks_.find("ORDERS") != callbacks_.end()) {
        callbacks_["ORDERS"](data);
    }
}

void BitpandaWS::handle_trades_history_update(const json& data) {
    if (callbacks_.find("TRADES_HISTORY") != callbacks_.end()) {
        callbacks_["TRADES_HISTORY"](data);
    }
}

void BitpandaWS::handle_balances_update(const json& data) {
    if (callbacks_.find("BALANCES") != callbacks_.end()) {
        callbacks_["BALANCES"](data);
    }
}

} // namespace ccxt
