#include "ccxt/exchanges/ws/bitfinex2_ws.h"
#include "ccxt/base/json.hpp"
#include <chrono>
#include <sstream>
#include <iomanip>

namespace ccxt {

bitfinex2_ws::bitfinex2_ws() : exchange_ws() {
    this->urls["ws"] = {
        {"public", WS_BASE},
        {"private", WS_PRIVATE}
    };
}

std::string bitfinex2_ws::get_url() const {
    return this->authenticated ? WS_PRIVATE : WS_BASE;
}

void bitfinex2_ws::watch_ticker_impl(const std::string& symbol, const json& params) {
    std::string market_id = this->market_id(symbol);
    subscribe_public("ticker", market_id, params);
}

void bitfinex2_ws::watch_trades_impl(const std::string& symbol, const json& params) {
    std::string market_id = this->market_id(symbol);
    subscribe_public("trades", market_id, params);
}

void bitfinex2_ws::watch_ohlcv_impl(const std::string& symbol, const std::string& timeframe, const json& params) {
    std::string market_id = this->market_id(symbol);
    std::string tf_code = get_timeframe_code(timeframe);
    json subscription_params = {
        {"key", "trade:"+tf_code+":"+market_id}
    };
    subscribe_public("candles", market_id, subscription_params);
}

void bitfinex2_ws::watch_order_book_impl(const std::string& symbol, const json& params) {
    std::string market_id = this->market_id(symbol);
    json subscription_params = {
        {"prec", "P0"},
        {"freq", "F0"},
        {"len", "25"}
    };
    if (params.contains("prec")) subscription_params["prec"] = params["prec"];
    if (params.contains("freq")) subscription_params["freq"] = params["freq"];
    if (params.contains("len")) subscription_params["len"] = params["len"];
    
    subscribe_public("book", market_id, subscription_params);
}

void bitfinex2_ws::watch_balance_impl(const json& params) {
    if (!this->authenticated) {
        throw AuthenticationError("Authentication required for private endpoints");
    }
    subscribe_private("wallet");
}

void bitfinex2_ws::watch_orders_impl(const std::string& symbol, const json& params) {
    if (!this->authenticated) {
        throw AuthenticationError("Authentication required for private endpoints");
    }
    std::string market_id = symbol.empty() ? "" : this->market_id(symbol);
    subscribe_private("orders", market_id);
}

void bitfinex2_ws::watch_my_trades_impl(const std::string& symbol, const json& params) {
    if (!this->authenticated) {
        throw AuthenticationError("Authentication required for private endpoints");
    }
    std::string market_id = symbol.empty() ? "" : this->market_id(symbol);
    subscribe_private("trades", market_id);
}

void bitfinex2_ws::watch_positions_impl(const json& params) {
    if (!this->authenticated) {
        throw AuthenticationError("Authentication required for private endpoints");
    }
    subscribe_private("positions");
}

void bitfinex2_ws::authenticate() {
    long long nonce = get_nonce();
    std::string payload = "AUTH" + std::to_string(nonce);
    std::string signature = this->hmac(payload, this->config_.secret, "sha384");
    
    json auth_message = {
        {"apiKey", this->config_.apiKey},
        {"authSig", signature},
        {"authNonce", nonce},
        {"authPayload", payload},
        {"event", "auth"},
        {"filter", json::array({"trading", "wallet", "balance"})}
    };
    
    this->send(auth_message);
}

void bitfinex2_ws::handle_message(const json& message) {
    if (message.is_array()) {
        int channel_id = message[0].get<int>();
        
        if (channelTypes.count(channel_id)) {
            std::string channel_type = channelTypes[channel_id];
            std::string symbol = channelSymbols[channel_id];
            
            if (message[1] == "hb") {
                // Heartbeat message, ignore
                return;
            }
            
            if (channel_type == "ticker") {
                handle_ticker_update(message[1], symbol);
            } else if (channel_type == "trades") {
                handle_trades_update(message[1], symbol);
            } else if (channel_type == "candles") {
                handle_ohlcv_update(message[1], symbol);
            } else if (channel_type == "book") {
                handle_order_book_update(message[1], symbol);
            } else if (channel_type == "wu") {
                handle_balance_update(message[1]);
            } else if (channel_type == "on" || channel_type == "ou" || channel_type == "oc") {
                handle_order_update(message[1]);
            } else if (channel_type == "ps") {
                handle_position_update(message[1]);
            }
        }
    } else if (message.is_object()) {
        if (message.contains("event")) {
            std::string event = message["event"];
            
            if (event == "subscribed") {
                parse_channel_id(message);
            } else if (event == "error") {
                handle_error(message);
            } else if (event == "auth") {
                if (message["status"] == "OK") {
                    this->authenticated = true;
                } else {
                    throw AuthenticationError("Authentication failed: " + message.dump());
                }
            }
        }
    }
}

void bitfinex2_ws::handle_error(const json& message) {
    if (message.contains("msg")) {
        std::string msg = message["msg"];
        throw ExchangeError("Bitfinex WebSocket error: " + msg);
    }
}

void bitfinex2_ws::handle_subscription(const json& message) {
    // Handled in parse_channel_id
}

void bitfinex2_ws::handle_ticker_update(const json& data, const std::string& symbol) {
    if (data.is_array() && data.size() >= 10) {
        json ticker = {
            {"symbol", symbol},
            {"bid", this->safe_string(data, 0)},
            {"bidSize", this->safe_string(data, 1)},
            {"ask", this->safe_string(data, 2)},
            {"askSize", this->safe_string(data, 3)},
            {"dailyChange", this->safe_string(data, 4)},
            {"dailyChangePerc", this->safe_string(data, 5)},
            {"lastPrice", this->safe_string(data, 6)},
            {"volume", this->safe_string(data, 7)},
            {"high", this->safe_string(data, 8)},
            {"low", this->safe_string(data, 9)}
        };
        
        this->emit("ticker", ticker);
    }
}

void bitfinex2_ws::handle_trades_update(const json& data, const std::string& symbol) {
    if (data.is_array()) {
        for (const auto& trade : data) {
            if (trade.is_array() && trade.size() >= 4) {
                json parsed_trade = {
                    {"id", this->safe_string(trade, 0)},
                    {"timestamp", this->safe_integer(trade, 1)},
                    {"amount", this->safe_string(trade, 2)},
                    {"price", this->safe_string(trade, 3)},
                    {"symbol", symbol}
                };
                
                this->emit("trade", parsed_trade);
            }
        }
    }
}

void bitfinex2_ws::handle_ohlcv_update(const json& data, const std::string& symbol) {
    if (data.is_array() && data.size() >= 6) {
        json ohlcv = {
            {"timestamp", this->safe_integer(data, 0)},
            {"open", this->safe_string(data, 1)},
            {"high", this->safe_string(data, 2)},
            {"low", this->safe_string(data, 3)},
            {"close", this->safe_string(data, 4)},
            {"volume", this->safe_string(data, 5)},
            {"symbol", symbol}
        };
        
        this->emit("ohlcv", ohlcv);
    }
}

void bitfinex2_ws::handle_order_book_update(const json& data, const std::string& symbol) {
    if (data.is_array()) {
        json orderbook = {
            {"symbol", symbol},
            {"bids", json::array()},
            {"asks", json::array()}
        };
        
        for (const auto& item : data) {
            if (item.is_array() && item.size() >= 3) {
                double price = this->safe_float(item, 0);
                int count = this->safe_integer(item, 1);
                double amount = this->safe_float(item, 2);
                
                json entry = {price, std::abs(amount)};
                if (amount > 0) {
                    orderbook["bids"].push_back(entry);
                } else if (amount < 0) {
                    orderbook["asks"].push_back(entry);
                }
            }
        }
        
        this->emit("orderbook", orderbook);
    }
}

void bitfinex2_ws::handle_balance_update(const json& data) {
    if (data.is_array() && data.size() >= 4) {
        std::string currency = this->safe_string(data, 1);
        std::string wallet_type = this->safe_string(data, 0);
        
        json balance = {
            {"currency", currency},
            {"type", wallet_type},
            {"total", this->safe_string(data, 2)},
            {"available", this->safe_string(data, 3)}
        };
        
        this->emit("balance", balance);
    }
}

void bitfinex2_ws::handle_order_update(const json& data) {
    if (data.is_array() && data.size() >= 12) {
        std::string symbol = this->safe_string(data, 3);
        symbol = this->safe_symbol(symbol);
        
        json order = {
            {"id", this->safe_string(data, 0)},
            {"clientOrderId", this->safe_string(data, 2)},
            {"symbol", symbol},
            {"type", this->safe_string(data, 6)},
            {"side", this->safe_string(data, 7)},
            {"price", this->safe_string(data, 16)},
            {"amount", this->safe_string(data, 10)},
            {"remaining", this->safe_string(data, 11)},
            {"status", this->safe_string(data, 13)},
            {"timestamp", this->safe_integer(data, 4)}
        };
        
        this->emit("order", order);
    }
}

void bitfinex2_ws::handle_position_update(const json& data) {
    if (data.is_array() && data.size() >= 10) {
        std::string symbol = this->safe_string(data, 0);
        symbol = this->safe_symbol(symbol);
        
        json position = {
            {"symbol", symbol},
            {"amount", this->safe_string(data, 2)},
            {"basePrice", this->safe_string(data, 3)},
            {"marginFunding", this->safe_string(data, 4)},
            {"marginFundingType", this->safe_integer(data, 5)},
            {"pl", this->safe_string(data, 6)},
            {"plPerc", this->safe_string(data, 7)},
            {"liquidationPrice", this->safe_string(data, 8)},
            {"leverage", this->safe_string(data, 9)}
        };
        
        this->emit("position", position);
    }
}

void bitfinex2_ws::subscribe_public(const std::string& channel, const std::string& symbol, const json& params) {
    json request = {
        {"event", "subscribe"},
        {"channel", channel},
        {"symbol", symbol}
    };
    
    for (auto& param : params.items()) {
        request[param.key()] = param.value();
    }
    
    this->send(request);
}

void bitfinex2_ws::subscribe_private(const std::string& channel, const std::string& symbol, const json& params) {
    if (!this->authenticated) {
        this->authenticate();
    }
    
    // Private channels are automatically subscribed after authentication
}

std::string bitfinex2_ws::get_channel_key(const std::string& channel, const std::string& symbol) {
    return channel + ":" + symbol;
}

void bitfinex2_ws::parse_channel_id(const json& message) {
    if (message.contains("chanId") && message.contains("channel")) {
        int channel_id = message["chanId"];
        std::string channel = message["channel"];
        std::string symbol = message.value("symbol", "");
        
        channelIds[channel_id] = get_channel_key(channel, symbol);
        channelTypes[channel_id] = channel;
        channelSymbols[channel_id] = symbol;
    }
}

std::string bitfinex2_ws::get_timeframe_code(const std::string& timeframe) {
    static std::map<std::string, std::string> timeframe_map = {
        {"1m", "1m"},
        {"5m", "5m"},
        {"15m", "15m"},
        {"30m", "30m"},
        {"1h", "1h"},
        {"3h", "3h"},
        {"6h", "6h"},
        {"12h", "12h"},
        {"1d", "1D"},
        {"1w", "1W"},
        {"2w", "2W"},
        {"1M", "1M"}
    };
    
    return timeframe_map.count(timeframe) ? timeframe_map[timeframe] : "1h";
}

long long bitfinex2_ws::get_nonce() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

} // namespace ccxt
