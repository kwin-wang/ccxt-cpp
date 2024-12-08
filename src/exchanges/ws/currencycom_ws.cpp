#include "ccxt/exchanges/ws/currencycom_ws.h"
#include <boost/format.hpp>
#include <chrono>
#include <openssl/hmac.h>
#include <iomanip>
#include <sstream>

namespace ccxt {

CurrencycomWS::CurrencycomWS(const Config& config)
    : WsClient("wss://api-adapter.backend.currency.com/connect", config)
    , currencycom(config) {}

void CurrencycomWS::on_connect() {
    if (!api_key.empty() && !secret.empty()) {
        authenticate();
    }
}

void CurrencycomWS::authenticate() {
    auto timestamp = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count());

    std::string signature = sign_request(timestamp, "GET", "/api/v2/auth");

    json auth_message = {
        {"method", "AUTH"},
        {"params", {
            {"apiKey", api_key},
            {"timestamp", timestamp},
            {"signature", signature}
        }}
    };

    send(auth_message.dump());
}

std::string CurrencycomWS::sign_request(const std::string& timestamp, const std::string& method, 
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

void CurrencycomWS::on_message(const json& message) {
    try {
        if (message.contains("error")) {
            on_error(message["error"]["message"].get<std::string>());
            return;
        }

        if (message.contains("method")) {
            std::string method = message["method"].get<std::string>();
            
            if (method == "ticker") {
                handle_ticker_update(message["params"]);
            } else if (method == "miniTicker") {
                handle_miniTicker_update(message["params"]);
            } else if (method == "depth") {
                handle_orderbook_update(message["params"]);
            } else if (method == "trade") {
                handle_trades_update(message["params"]);
            } else if (method == "kline") {
                handle_kline_update(message["params"]);
            } else if (method == "aggTrade") {
                handle_aggTrades_update(message["params"]);
            } else if (method == "account") {
                handle_account_update(message["params"]);
            } else if (method == "order") {
                handle_orders_update(message["params"]);
            } else if (method == "balance") {
                handle_balance_update(message["params"]);
            } else if (method == "position") {
                handle_positions_update(message["params"]);
            }
        }
    } catch (const std::exception& e) {
        on_error(std::string("Error processing message: ") + e.what());
    }
}

void CurrencycomWS::on_error(const std::string& error) {
    // Log error or notify error handler
}

void CurrencycomWS::on_close() {
    // Clean up resources
    callbacks_.clear();
}

std::string CurrencycomWS::generate_channel_id(const std::string& channel, const std::string& symbol, const std::string& interval) {
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
void CurrencycomWS::subscribe_ticker(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("ticker", symbol);
    callbacks_[channel_id] = callback;
    
    json params = {
        {"method", "SUBSCRIBE"},
        {"params", {symbol + "@ticker"}},
        {"id", std::time(nullptr)}
    };
    
    send_subscribe_message("SUBSCRIBE", params);
}

void CurrencycomWS::subscribe_miniTicker(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("miniTicker", symbol);
    callbacks_[channel_id] = callback;
    
    json params = {
        {"method", "SUBSCRIBE"},
        {"params", {symbol + "@miniTicker"}},
        {"id", std::time(nullptr)}
    };
    
    send_subscribe_message("SUBSCRIBE", params);
}

void CurrencycomWS::subscribe_orderbook(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("orderbook", symbol);
    callbacks_[channel_id] = callback;
    
    json params = {
        {"method", "SUBSCRIBE"},
        {"params", {symbol + "@depth"}},
        {"id", std::time(nullptr)}
    };
    
    send_subscribe_message("SUBSCRIBE", params);
}

void CurrencycomWS::subscribe_trades(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("trades", symbol);
    callbacks_[channel_id] = callback;
    
    json params = {
        {"method", "SUBSCRIBE"},
        {"params", {symbol + "@trade"}},
        {"id", std::time(nullptr)}
    };
    
    send_subscribe_message("SUBSCRIBE", params);
}

void CurrencycomWS::subscribe_kline(const std::string& symbol, const std::string& interval, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("kline", symbol, interval);
    callbacks_[channel_id] = callback;
    
    json params = {
        {"method", "SUBSCRIBE"},
        {"params", {symbol + "@kline_" + interval}},
        {"id", std::time(nullptr)}
    };
    
    send_subscribe_message("SUBSCRIBE", params);
}

void CurrencycomWS::subscribe_aggTrades(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("aggTrades", symbol);
    callbacks_[channel_id] = callback;
    
    json params = {
        {"method", "SUBSCRIBE"},
        {"params", {symbol + "@aggTrade"}},
        {"id", std::time(nullptr)}
    };
    
    send_subscribe_message("SUBSCRIBE", params);
}

// Private Data Stream Methods
void CurrencycomWS::subscribe_account(std::function<void(const json&)> callback) {
    callbacks_["account"] = callback;
    
    json params = {
        {"method", "SUBSCRIBE"},
        {"params", {"account"}},
        {"id", std::time(nullptr)}
    };
    
    send_authenticated_request("SUBSCRIBE", params);
}

void CurrencycomWS::subscribe_orders(std::function<void(const json&)> callback) {
    callbacks_["orders"] = callback;
    
    json params = {
        {"method", "SUBSCRIBE"},
        {"params", {"orders"}},
        {"id", std::time(nullptr)}
    };
    
    send_authenticated_request("SUBSCRIBE", params);
}

void CurrencycomWS::subscribe_balance(std::function<void(const json&)> callback) {
    callbacks_["balance"] = callback;
    
    json params = {
        {"method", "SUBSCRIBE"},
        {"params", {"balance"}},
        {"id", std::time(nullptr)}
    };
    
    send_authenticated_request("SUBSCRIBE", params);
}

void CurrencycomWS::subscribe_positions(std::function<void(const json&)> callback) {
    callbacks_["positions"] = callback;
    
    json params = {
        {"method", "SUBSCRIBE"},
        {"params", {"positions"}},
        {"id", std::time(nullptr)}
    };
    
    send_authenticated_request("SUBSCRIBE", params);
}

// Trading Operations
void CurrencycomWS::place_order(const std::string& symbol, const std::string& side, const std::string& type,
                               double quantity, double price, const std::map<std::string, std::string>& params) {
    json order_params = {
        {"symbol", symbol},
        {"side", side},
        {"type", type},
        {"quantity", quantity}
    };

    if (price > 0) {
        order_params["price"] = price;
    }

    for (const auto& [key, value] : params) {
        order_params[key] = value;
    }

    send_authenticated_request("ORDER", order_params);
}

void CurrencycomWS::cancel_order(const std::string& order_id, const std::string& symbol) {
    json params = {
        {"orderId", order_id},
        {"symbol", symbol}
    };

    send_authenticated_request("ORDER_CANCEL", params);
}

void CurrencycomWS::cancel_all_orders(const std::string& symbol) {
    json params = {};
    if (!symbol.empty()) {
        params["symbol"] = symbol;
    }

    send_authenticated_request("ORDER_CANCEL_ALL", params);
}

void CurrencycomWS::modify_order(const std::string& order_id, const std::string& symbol,
                                double quantity, double price) {
    json params = {
        {"orderId", order_id},
        {"symbol", symbol},
        {"quantity", quantity},
        {"price", price}
    };

    send_authenticated_request("ORDER_MODIFY", params);
}

// Unsubscribe Methods
void CurrencycomWS::unsubscribe_ticker(const std::string& symbol) {
    std::string channel_id = generate_channel_id("ticker", symbol);
    callbacks_.erase(channel_id);
    
    json params = {
        {"method", "UNSUBSCRIBE"},
        {"params", {symbol + "@ticker"}},
        {"id", std::time(nullptr)}
    };
    
    send_unsubscribe_message("UNSUBSCRIBE", params);
}

void CurrencycomWS::unsubscribe_miniTicker(const std::string& symbol) {
    std::string channel_id = generate_channel_id("miniTicker", symbol);
    callbacks_.erase(channel_id);
    
    json params = {
        {"method", "UNSUBSCRIBE"},
        {"params", {symbol + "@miniTicker"}},
        {"id", std::time(nullptr)}
    };
    
    send_unsubscribe_message("UNSUBSCRIBE", params);
}

void CurrencycomWS::unsubscribe_orderbook(const std::string& symbol) {
    std::string channel_id = generate_channel_id("orderbook", symbol);
    callbacks_.erase(channel_id);
    
    json params = {
        {"method", "UNSUBSCRIBE"},
        {"params", {symbol + "@depth"}},
        {"id", std::time(nullptr)}
    };
    
    send_unsubscribe_message("UNSUBSCRIBE", params);
}

void CurrencycomWS::unsubscribe_trades(const std::string& symbol) {
    std::string channel_id = generate_channel_id("trades", symbol);
    callbacks_.erase(channel_id);
    
    json params = {
        {"method", "UNSUBSCRIBE"},
        {"params", {symbol + "@trade"}},
        {"id", std::time(nullptr)}
    };
    
    send_unsubscribe_message("UNSUBSCRIBE", params);
}

void CurrencycomWS::unsubscribe_kline(const std::string& symbol, const std::string& interval) {
    std::string channel_id = generate_channel_id("kline", symbol, interval);
    callbacks_.erase(channel_id);
    
    json params = {
        {"method", "UNSUBSCRIBE"},
        {"params", {symbol + "@kline_" + interval}},
        {"id", std::time(nullptr)}
    };
    
    send_unsubscribe_message("UNSUBSCRIBE", params);
}

void CurrencycomWS::unsubscribe_aggTrades(const std::string& symbol) {
    std::string channel_id = generate_channel_id("aggTrades", symbol);
    callbacks_.erase(channel_id);
    
    json params = {
        {"method", "UNSUBSCRIBE"},
        {"params", {symbol + "@aggTrade"}},
        {"id", std::time(nullptr)}
    };
    
    send_unsubscribe_message("UNSUBSCRIBE", params);
}

void CurrencycomWS::unsubscribe_account() {
    callbacks_.erase("account");
    
    json params = {
        {"method", "UNSUBSCRIBE"},
        {"params", {"account"}},
        {"id", std::time(nullptr)}
    };
    
    send_authenticated_request("UNSUBSCRIBE", params);
}

void CurrencycomWS::unsubscribe_orders() {
    callbacks_.erase("orders");
    
    json params = {
        {"method", "UNSUBSCRIBE"},
        {"params", {"orders"}},
        {"id", std::time(nullptr)}
    };
    
    send_authenticated_request("UNSUBSCRIBE", params);
}

void CurrencycomWS::unsubscribe_balance() {
    callbacks_.erase("balance");
    
    json params = {
        {"method", "UNSUBSCRIBE"},
        {"params", {"balance"}},
        {"id", std::time(nullptr)}
    };
    
    send_authenticated_request("UNSUBSCRIBE", params);
}

void CurrencycomWS::unsubscribe_positions() {
    callbacks_.erase("positions");
    
    json params = {
        {"method", "UNSUBSCRIBE"},
        {"params", {"positions"}},
        {"id", std::time(nullptr)}
    };
    
    send_authenticated_request("UNSUBSCRIBE", params);
}

// Helper Methods
void CurrencycomWS::send_subscribe_message(const std::string& method, const json& params) {
    send(params.dump());
}

void CurrencycomWS::send_unsubscribe_message(const std::string& method, const json& params) {
    send(params.dump());
}

void CurrencycomWS::send_authenticated_request(const std::string& method, const json& params) {
    if (api_key.empty() || secret.empty()) {
        throw std::runtime_error("API key and secret are required for authenticated requests");
    }

    auto timestamp = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count());

    json request = params;
    request["apiKey"] = api_key;
    request["timestamp"] = timestamp;
    
    std::string body = request.dump();
    request["signature"] = sign_request(timestamp, "GET", "/api/v2/" + method, body);

    send(request.dump());
}

// Update Handlers
void CurrencycomWS::handle_ticker_update(const json& data) {
    std::string symbol = data["s"].get<std::string>();
    std::string channel_id = generate_channel_id("ticker", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void CurrencycomWS::handle_miniTicker_update(const json& data) {
    std::string symbol = data["s"].get<std::string>();
    std::string channel_id = generate_channel_id("miniTicker", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void CurrencycomWS::handle_orderbook_update(const json& data) {
    std::string symbol = data["s"].get<std::string>();
    std::string channel_id = generate_channel_id("orderbook", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void CurrencycomWS::handle_trades_update(const json& data) {
    std::string symbol = data["s"].get<std::string>();
    std::string channel_id = generate_channel_id("trades", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void CurrencycomWS::handle_kline_update(const json& data) {
    std::string symbol = data["s"].get<std::string>();
    std::string interval = data["i"].get<std::string>();
    std::string channel_id = generate_channel_id("kline", symbol, interval);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void CurrencycomWS::handle_aggTrades_update(const json& data) {
    std::string symbol = data["s"].get<std::string>();
    std::string channel_id = generate_channel_id("aggTrades", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void CurrencycomWS::handle_account_update(const json& data) {
    if (callbacks_.find("account") != callbacks_.end()) {
        callbacks_["account"](data);
    }
}

void CurrencycomWS::handle_orders_update(const json& data) {
    if (callbacks_.find("orders") != callbacks_.end()) {
        callbacks_["orders"](data);
    }
}

void CurrencycomWS::handle_balance_update(const json& data) {
    if (callbacks_.find("balance") != callbacks_.end()) {
        callbacks_["balance"](data);
    }
}

void CurrencycomWS::handle_positions_update(const json& data) {
    if (callbacks_.find("positions") != callbacks_.end()) {
        callbacks_["positions"](data);
    }
}

} // namespace ccxt
