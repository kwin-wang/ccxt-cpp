#include "ccxt/exchanges/ws/hitbtc_ws.h"
#include <boost/format.hpp>
#include <chrono>

namespace ccxt {

HitbtcWS::HitbtcWS(const Config& config)
    : WsClient("wss://api.hitbtc.com/api/3/ws", config)
    , hitbtc(config) {}

void HitbtcWS::on_connect() {
    if (!api_key.empty() && !secret.empty()) {
        authenticate();
    }
}

void HitbtcWS::authenticate() {
    long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    std::string payload = boost::str(boost::format("%lld") % timestamp);
    std::string signature = sign_message(payload, secret);

    json auth_message = {
        {"method", "login"},
        {"params", {
            {"algo", "HS256"},
            {"api_key", api_key},
            {"signature", signature},
            {"nonce", timestamp}
        }}
    };

    send(auth_message.dump());
}

void HitbtcWS::on_message(const json& message) {
    try {
        if (message.contains("error")) {
            on_error(message["error"]["message"].get<std::string>());
            return;
        }

        if (message.contains("method")) {
            std::string method = message["method"].get<std::string>();
            
            if (method == "ticker") {
                handle_ticker_update(message["params"]);
            } else if (method == "snapshotOrderbook" || method == "updateOrderbook") {
                handle_orderbook_update(message["params"]);
            } else if (method == "updateTrades") {
                handle_trades_update(message["params"]);
            } else if (method == "snapshotCandles" || method == "updateCandles") {
                handle_candles_update(message["params"]);
            } else if (method == "miniTicker") {
                handle_mini_ticker_update(message["params"]);
            } else if (method == "activeOrders" || method == "report") {
                handle_reports_update(message["params"]);
            } else if (method == "trading") {
                handle_trading_update(message["params"]);
            } else if (method == "spot_balance" || method == "margin_balance") {
                handle_account_update(message["params"]);
            } else if (method == "transactions") {
                handle_transactions_update(message["params"]);
            }
        }
    } catch (const std::exception& e) {
        on_error(std::string("Error processing message: ") + e.what());
    }
}

void HitbtcWS::on_error(const std::string& error) {
    // Log error or notify error handler
}

void HitbtcWS::on_close() {
    // Clean up resources
    callbacks_.clear();
}

std::string HitbtcWS::generate_channel_id(const std::string& channel, const std::string& symbol, const std::string& timeframe) {
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
void HitbtcWS::subscribe_ticker(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("ticker", symbol);
    callbacks_[channel_id] = callback;
    
    json params = {
        {"symbol", symbol}
    };
    
    send_subscribe_message("subscribeTicker", params);
}

void HitbtcWS::subscribe_orderbook(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("orderbook", symbol);
    callbacks_[channel_id] = callback;
    
    json params = {
        {"symbol", symbol}
    };
    
    send_subscribe_message("subscribeOrderbook", params);
}

void HitbtcWS::subscribe_trades(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("trades", symbol);
    callbacks_[channel_id] = callback;
    
    json params = {
        {"symbol", symbol}
    };
    
    send_subscribe_message("subscribeTrades", params);
}

void HitbtcWS::subscribe_candles(const std::string& symbol, const std::string& timeframe, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("candles", symbol, timeframe);
    callbacks_[channel_id] = callback;
    
    json params = {
        {"symbol", symbol},
        {"period", timeframe}
    };
    
    send_subscribe_message("subscribeCandles", params);
}

void HitbtcWS::subscribe_mini_ticker(const std::string& symbol, std::function<void(const json&)> callback) {
    std::string channel_id = generate_channel_id("miniTicker", symbol);
    callbacks_[channel_id] = callback;
    
    json params = {
        {"symbol", symbol}
    };
    
    send_subscribe_message("subscribeMiniTicker", params);
}

// Private Data Stream Methods
void HitbtcWS::subscribe_reports(std::function<void(const json&)> callback) {
    callbacks_["reports"] = callback;
    send_authenticated_request("subscribeReports", {});
}

void HitbtcWS::subscribe_trading(std::function<void(const json&)> callback) {
    callbacks_["trading"] = callback;
    send_authenticated_request("subscribeTrading", {});
}

void HitbtcWS::subscribe_account(std::function<void(const json&)> callback) {
    callbacks_["account"] = callback;
    send_authenticated_request("subscribeBalance", {});
}

void HitbtcWS::subscribe_transactions(std::function<void(const json&)> callback) {
    callbacks_["transactions"] = callback;
    send_authenticated_request("subscribeTransactions", {});
}

// Trading Operations
void HitbtcWS::place_order(const std::string& symbol, const std::string& side, const std::string& type,
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

    send_authenticated_request("newOrder", order_params);
}

void HitbtcWS::cancel_order(const std::string& order_id) {
    json params = {
        {"clientOrderId", order_id}
    };

    send_authenticated_request("cancelOrder", params);
}

void HitbtcWS::cancel_all_orders(const std::string& symbol) {
    json params = {};
    if (!symbol.empty()) {
        params["symbol"] = symbol;
    }

    send_authenticated_request("cancelAllOrders", params);
}

void HitbtcWS::replace_order(const std::string& order_id, const std::string& symbol,
                            const std::string& side, const std::string& type,
                            double quantity, double price) {
    json params = {
        {"clientOrderId", order_id},
        {"symbol", symbol},
        {"side", side},
        {"type", type},
        {"quantity", quantity}
    };

    if (price > 0) {
        params["price"] = price;
    }

    send_authenticated_request("replaceOrder", params);
}

// Unsubscribe Methods
void HitbtcWS::unsubscribe_ticker(const std::string& symbol) {
    std::string channel_id = generate_channel_id("ticker", symbol);
    callbacks_.erase(channel_id);
    
    json params = {
        {"symbol", symbol}
    };
    
    send_unsubscribe_message("unsubscribeTicker", params);
}

void HitbtcWS::unsubscribe_orderbook(const std::string& symbol) {
    std::string channel_id = generate_channel_id("orderbook", symbol);
    callbacks_.erase(channel_id);
    
    json params = {
        {"symbol", symbol}
    };
    
    send_unsubscribe_message("unsubscribeOrderbook", params);
}

void HitbtcWS::unsubscribe_trades(const std::string& symbol) {
    std::string channel_id = generate_channel_id("trades", symbol);
    callbacks_.erase(channel_id);
    
    json params = {
        {"symbol", symbol}
    };
    
    send_unsubscribe_message("unsubscribeTrades", params);
}

void HitbtcWS::unsubscribe_candles(const std::string& symbol, const std::string& timeframe) {
    std::string channel_id = generate_channel_id("candles", symbol, timeframe);
    callbacks_.erase(channel_id);
    
    json params = {
        {"symbol", symbol},
        {"period", timeframe}
    };
    
    send_unsubscribe_message("unsubscribeCandles", params);
}

void HitbtcWS::unsubscribe_mini_ticker(const std::string& symbol) {
    std::string channel_id = generate_channel_id("miniTicker", symbol);
    callbacks_.erase(channel_id);
    
    json params = {
        {"symbol", symbol}
    };
    
    send_unsubscribe_message("unsubscribeMiniTicker", params);
}

void HitbtcWS::unsubscribe_reports() {
    callbacks_.erase("reports");
    send_authenticated_request("unsubscribeReports", {});
}

void HitbtcWS::unsubscribe_trading() {
    callbacks_.erase("trading");
    send_authenticated_request("unsubscribeTrading", {});
}

void HitbtcWS::unsubscribe_account() {
    callbacks_.erase("account");
    send_authenticated_request("unsubscribeBalance", {});
}

void HitbtcWS::unsubscribe_transactions() {
    callbacks_.erase("transactions");
    send_authenticated_request("unsubscribeTransactions", {});
}

// Helper Methods
void HitbtcWS::send_subscribe_message(const std::string& method, const json& params) {
    json subscribe_msg = {
        {"method", method},
        {"params", params},
        {"id", std::time(nullptr)}
    };

    send(subscribe_msg.dump());
}

void HitbtcWS::send_unsubscribe_message(const std::string& method, const json& params) {
    json unsubscribe_msg = {
        {"method", method},
        {"params", params},
        {"id", std::time(nullptr)}
    };

    send(unsubscribe_msg.dump());
}

void HitbtcWS::send_authenticated_request(const std::string& method, const json& params) {
    if (api_key.empty() || secret.empty()) {
        throw std::runtime_error("API key and secret are required for authenticated requests");
    }

    json request = {
        {"method", method},
        {"params", params},
        {"id", std::time(nullptr)}
    };

    send(request.dump());
}

// Update Handlers
void HitbtcWS::handle_ticker_update(const json& data) {
    std::string symbol = data["symbol"].get<std::string>();
    std::string channel_id = generate_channel_id("ticker", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void HitbtcWS::handle_orderbook_update(const json& data) {
    std::string symbol = data["symbol"].get<std::string>();
    std::string channel_id = generate_channel_id("orderbook", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void HitbtcWS::handle_trades_update(const json& data) {
    std::string symbol = data["symbol"].get<std::string>();
    std::string channel_id = generate_channel_id("trades", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void HitbtcWS::handle_candles_update(const json& data) {
    std::string symbol = data["symbol"].get<std::string>();
    std::string period = data["period"].get<std::string>();
    std::string channel_id = generate_channel_id("candles", symbol, period);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void HitbtcWS::handle_mini_ticker_update(const json& data) {
    std::string symbol = data["symbol"].get<std::string>();
    std::string channel_id = generate_channel_id("miniTicker", symbol);
    
    if (callbacks_.find(channel_id) != callbacks_.end()) {
        callbacks_[channel_id](data);
    }
}

void HitbtcWS::handle_reports_update(const json& data) {
    if (callbacks_.find("reports") != callbacks_.end()) {
        callbacks_["reports"](data);
    }
}

void HitbtcWS::handle_trading_update(const json& data) {
    if (callbacks_.find("trading") != callbacks_.end()) {
        callbacks_["trading"](data);
    }
}

void HitbtcWS::handle_account_update(const json& data) {
    if (callbacks_.find("account") != callbacks_.end()) {
        callbacks_["account"](data);
    }
}

void HitbtcWS::handle_transactions_update(const json& data) {
    if (callbacks_.find("transactions") != callbacks_.end()) {
        callbacks_["transactions"](data);
    }
}

} // namespace ccxt
