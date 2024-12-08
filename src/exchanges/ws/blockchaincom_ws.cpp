#include "ccxt/exchanges/ws/blockchaincom_ws.h"
#include <boost/format.hpp>

namespace ccxt {

BlockchaincomWS::BlockchaincomWS(const Config& config)
    : WsClient("wss://ws.blockchain.info/mercury-gateway/v1/ws", config)
    , blockchaincom(config) {}

void BlockchaincomWS::on_connect() {
    if (!api_key.empty() && !secret.empty()) {
        authenticate();
    }
}

void BlockchaincomWS::authenticate() {
    long long timestamp = get_current_timestamp();
    std::string signature = sign_message(
        boost::str(boost::format("%lld") % timestamp),
        secret
    );

    json auth_message = {
        {"action", "subscribe"},
        {"channel", "auth"},
        {"key", api_key},
        {"timestamp", timestamp},
        {"signature", signature}
    };

    send(auth_message.dump());
}

void BlockchaincomWS::on_message(const json& message) {
    try {
        std::string channel = message["channel"].get<std::string>();
        std::string event = message["event"].get<std::string>();

        if (event == "subscribed") {
            // Handle subscription confirmation
            return;
        }

        if (event == "error") {
            on_error(message["message"].get<std::string>());
            return;
        }

        // Route message to appropriate handler
        if (channel == "ticker") {
            handle_ticker_update(message["data"]);
        } else if (channel == "l2") {
            handle_orderbook_update(message["data"]);
        } else if (channel == "trades") {
            handle_trades_update(message["data"]);
        } else if (channel == "candles") {
            handle_ohlcv_update(message["data"]);
        } else if (channel == "orders") {
            handle_orders_update(message["data"]);
        } else if (channel == "balances") {
            handle_balance_update(message["data"]);
        } else if (channel == "positions") {
            handle_positions_update(message["data"]);
        } else if (channel == "executions") {
            handle_executions_update(message["data"]);
        }
    } catch (const std::exception& e) {
        on_error(std::string("Error processing message: ") + e.what());
    }
}

void BlockchaincomWS::on_error(const std::string& error) {
    // Log error or notify error handler
}

void BlockchaincomWS::on_close() {
    // Clean up resources
    callbacks_.clear();
}

std::string BlockchaincomWS::generate_channel_id(const std::string& channel, const std::string& symbol) {
    return channel + ":" + symbol;
}

// Market Data Stream Methods
void BlockchaincomWS::subscribe_ticker(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("ticker", symbol);
    callbacks_[channel_id] = callback;

    json subscribe_msg = {
        {"action", "subscribe"},
        {"channel", "ticker"},
        {"symbol", symbol}
    };

    send(subscribe_msg.dump());
}

void BlockchaincomWS::subscribe_orderbook(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("l2", symbol);
    callbacks_[channel_id] = callback;

    json subscribe_msg = {
        {"action", "subscribe"},
        {"channel", "l2"},
        {"symbol", symbol}
    };

    send(subscribe_msg.dump());
}

void BlockchaincomWS::subscribe_trades(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("trades", symbol);
    callbacks_[channel_id] = callback;

    json subscribe_msg = {
        {"action", "subscribe"},
        {"channel", "trades"},
        {"symbol", symbol}
    };

    send(subscribe_msg.dump());
}

void BlockchaincomWS::subscribe_ohlcv(const std::string& symbol, const std::string& timeframe, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("candles", symbol + ":" + timeframe);
    callbacks_[channel_id] = callback;

    json subscribe_msg = {
        {"action", "subscribe"},
        {"channel", "candles"},
        {"symbol", symbol},
        {"interval", timeframe}
    };

    send(subscribe_msg.dump());
}

// Private Data Stream Methods
void BlockchaincomWS::subscribe_orders(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("orders", symbol);
    callbacks_[channel_id] = callback;

    json subscribe_msg = {
        {"action", "subscribe"},
        {"channel", "orders"},
        {"symbol", symbol}
    };

    send(subscribe_msg.dump());
}

void BlockchaincomWS::subscribe_balance(std::function<void(const json&)> callback) {
    callbacks_["balances"] = callback;

    json subscribe_msg = {
        {"action", "subscribe"},
        {"channel", "balances"}
    };

    send(subscribe_msg.dump());
}

void BlockchaincomWS::subscribe_positions(std::function<void(const json&)> callback) {
    callbacks_["positions"] = callback;

    json subscribe_msg = {
        {"action", "subscribe"},
        {"channel", "positions"}
    };

    send(subscribe_msg.dump());
}

void BlockchaincomWS::subscribe_executions(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("executions", symbol);
    callbacks_[channel_id] = callback;

    json subscribe_msg = {
        {"action", "subscribe"},
        {"channel", "executions"},
        {"symbol", symbol}
    };

    send(subscribe_msg.dump());
}

// Unsubscribe Methods
void BlockchaincomWS::unsubscribe_ticker(const std::string& symbol) {
    std::string channel_id = generate_channel_id("ticker", symbol);
    callbacks_.erase(channel_id);

    json unsubscribe_msg = {
        {"action", "unsubscribe"},
        {"channel", "ticker"},
        {"symbol", symbol}
    };

    send(unsubscribe_msg.dump());
}

void BlockchaincomWS::unsubscribe_orderbook(const std::string& symbol) {
    std::string channel_id = generate_channel_id("l2", symbol);
    callbacks_.erase(channel_id);

    json unsubscribe_msg = {
        {"action", "unsubscribe"},
        {"channel", "l2"},
        {"symbol", symbol}
    };

    send(unsubscribe_msg.dump());
}

void BlockchaincomWS::unsubscribe_trades(const std::string& symbol) {
    std::string channel_id = generate_channel_id("trades", symbol);
    callbacks_.erase(channel_id);

    json unsubscribe_msg = {
        {"action", "unsubscribe"},
        {"channel", "trades"},
        {"symbol", symbol}
    };

    send(unsubscribe_msg.dump());
}

void BlockchaincomWS::unsubscribe_ohlcv(const std::string& symbol, const std::string& timeframe) {
    std::string channel_id = generate_channel_id("candles", symbol + ":" + timeframe);
    callbacks_.erase(channel_id);

    json unsubscribe_msg = {
        {"action", "unsubscribe"},
        {"channel", "candles"},
        {"symbol", symbol},
        {"interval", timeframe}
    };

    send(unsubscribe_msg.dump());
}

void BlockchaincomWS::unsubscribe_orders(const std::string& symbol) {
    std::string channel_id = generate_channel_id("orders", symbol);
    callbacks_.erase(channel_id);

    json unsubscribe_msg = {
        {"action", "unsubscribe"},
        {"channel", "orders"},
        {"symbol", symbol}
    };

    send(unsubscribe_msg.dump());
}

void BlockchaincomWS::unsubscribe_balance() {
    callbacks_.erase("balances");

    json unsubscribe_msg = {
        {"action", "unsubscribe"},
        {"channel", "balances"}
    };

    send(unsubscribe_msg.dump());
}

void BlockchaincomWS::unsubscribe_positions() {
    callbacks_.erase("positions");

    json unsubscribe_msg = {
        {"action", "unsubscribe"},
        {"channel", "positions"}
    };

    send(unsubscribe_msg.dump());
}

void BlockchaincomWS::unsubscribe_executions(const std::string& symbol) {
    std::string channel_id = generate_channel_id("executions", symbol);
    callbacks_.erase(channel_id);

    json unsubscribe_msg = {
        {"action", "unsubscribe"},
        {"channel", "executions"},
        {"symbol", symbol}
    };

    send(unsubscribe_msg.dump());
}

// Update Handlers
void BlockchaincomWS::handle_ticker_update(const json& data) {
    std::string symbol = data["symbol"].get<std::string>();
    std::string channel_id = generate_channel_id("ticker", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void BlockchaincomWS::handle_orderbook_update(const json& data) {
    std::string symbol = data["symbol"].get<std::string>();
    std::string channel_id = generate_channel_id("l2", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void BlockchaincomWS::handle_trades_update(const json& data) {
    std::string symbol = data["symbol"].get<std::string>();
    std::string channel_id = generate_channel_id("trades", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void BlockchaincomWS::handle_ohlcv_update(const json& data) {
    std::string symbol = data["symbol"].get<std::string>();
    std::string timeframe = data["interval"].get<std::string>();
    std::string channel_id = generate_channel_id("candles", symbol + ":" + timeframe);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void BlockchaincomWS::handle_orders_update(const json& data) {
    std::string symbol = data["symbol"].get<std::string>();
    std::string channel_id = generate_channel_id("orders", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void BlockchaincomWS::handle_balance_update(const json& data) {
    if (callbacks_.find("balances") != callbacks_.end()) {
        callbacks_["balances"](data);
    }
}

void BlockchaincomWS::handle_positions_update(const json& data) {
    if (callbacks_.find("positions") != callbacks_.end()) {
        callbacks_["positions"](data);
    }
}

void BlockchaincomWS::handle_executions_update(const json& data) {
    std::string symbol = data["symbol"].get<std::string>();
    std::string channel_id = generate_channel_id("executions", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

} // namespace ccxt
