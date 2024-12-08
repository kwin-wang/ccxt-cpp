#include "ccxt/exchanges/ws/binanceusdm_ws.h"
#include "ccxt/base/json.hpp"

namespace ccxt {

binanceusdm_ws::binanceusdm_ws() : binance_ws() {
    this->define_rest_api({
        {"fapiPublic", WS_BASE},
        {"fapiPrivate", REST_BASE},
    });
}

std::string binanceusdm_ws::get_url() const {
    return WS_BASE;
}

std::string binanceusdm_ws::get_ws_base() const {
    return WS_BASE;
}

std::string binanceusdm_ws::get_rest_base() const {
    return REST_BASE;
}

std::string binanceusdm_ws::get_listen_key() const {
    return "/fapi/v1/listenKey";
}

void binanceusdm_ws::watch_ticker_impl(const std::string& symbol, const json& params) {
    std::string market_id = this->market_id(symbol);
    std::string url = this->get_url() + "/ws/" + market_id + "@ticker";
    this->subscribe(url, "ticker", symbol);
}

void binanceusdm_ws::watch_trades_impl(const std::string& symbol, const json& params) {
    std::string market_id = this->market_id(symbol);
    std::string url = this->get_url() + "/ws/" + market_id + "@trade";
    this->subscribe(url, "trades", symbol);
}

void binanceusdm_ws::watch_ohlcv_impl(const std::string& symbol, const std::string& timeframe, const json& params) {
    std::string market_id = this->market_id(symbol);
    std::string interval = this->timeframes[timeframe];
    std::string url = this->get_url() + "/ws/" + market_id + "@kline_" + interval;
    this->subscribe(url, "ohlcv", symbol);
}

void binanceusdm_ws::watch_order_book_impl(const std::string& symbol, const json& params) {
    std::string market_id = this->market_id(symbol);
    std::string url = this->get_url() + "/ws/" + market_id + "@depth20";
    this->subscribe(url, "orderbook", symbol);
}

void binanceusdm_ws::watch_bids_asks_impl(const std::string& symbol, const json& params) {
    std::string market_id = this->market_id(symbol);
    std::string url = this->get_url() + "/ws/" + market_id + "@bookTicker";
    this->subscribe(url, "bidsasks", symbol);
}

void binanceusdm_ws::watch_balance_impl(const json& params) {
    if (!this->authenticated) {
        throw AuthenticationError("Authentication required for private endpoints");
    }
    std::string listen_key = this->get_listen_key();
    std::string url = this->get_url() + "/ws/" + listen_key;
    this->subscribe(url, "balance");
}

void binanceusdm_ws::watch_orders_impl(const std::string& symbol, const json& params) {
    if (!this->authenticated) {
        throw AuthenticationError("Authentication required for private endpoints");
    }
    std::string listen_key = this->get_listen_key();
    std::string url = this->get_url() + "/ws/" + listen_key;
    this->subscribe(url, "orders", symbol);
}

void binanceusdm_ws::watch_my_trades_impl(const std::string& symbol, const json& params) {
    if (!this->authenticated) {
        throw AuthenticationError("Authentication required for private endpoints");
    }
    std::string listen_key = this->get_listen_key();
    std::string url = this->get_url() + "/ws/" + listen_key;
    this->subscribe(url, "mytrades", symbol);
}

void binanceusdm_ws::watch_positions_impl(const json& params) {
    if (!this->authenticated) {
        throw AuthenticationError("Authentication required for private endpoints");
    }
    std::string listen_key = this->get_listen_key();
    std::string url = this->get_url() + "/ws/" + listen_key;
    this->subscribe(url, "positions");
}

void binanceusdm_ws::handle_message(const json& message) {
    if (message.contains("e")) {
        std::string event_type = message["e"].get<std::string>();
        
        if (event_type == "24hrTicker") {
            this->handle_ticker(message);
        } else if (event_type == "trade") {
            this->handle_trade(message);
        } else if (event_type == "kline") {
            this->handle_ohlcv(message);
        } else if (event_type == "depthUpdate") {
            this->handle_order_book(message);
        } else if (event_type == "bookTicker") {
            this->handle_bids_asks(message);
        } else if (event_type == "ACCOUNT_UPDATE") {
            this->handle_balance_update(message);
            this->handle_position_update(message);
        } else if (event_type == "ORDER_TRADE_UPDATE") {
            this->handle_order_update(message);
        }
    }
}

void binanceusdm_ws::handle_error(const json& message) {
    if (message.contains("code") && message.contains("msg")) {
        int code = message["code"].get<int>();
        std::string msg = message["msg"].get<std::string>();
        throw ExchangeError("Binance USDM WebSocket error: " + std::to_string(code) + " " + msg);
    }
}

void binanceusdm_ws::handle_subscription(const json& message) {
    // Handle subscription confirmation messages
    if (message.contains("result") && message["result"].is_null()) {
        // Successful subscription
        return;
    }
}

void binanceusdm_ws::handle_user_update(const json& message) {
    // Process user data update messages
    if (message.contains("e")) {
        std::string event_type = message["e"].get<std::string>();
        if (event_type == "ACCOUNT_UPDATE") {
            this->handle_balance_update(message);
            this->handle_position_update(message);
        } else if (event_type == "ORDER_TRADE_UPDATE") {
            this->handle_order_update(message);
        }
    }
}

void binanceusdm_ws::handle_order_update(const json& message) {
    if (message.contains("o")) {
        json order_data = message["o"];
        std::string symbol = this->safe_string(order_data, "s");
        std::string client_order_id = this->safe_string(order_data, "c");
        std::string order_id = this->safe_string(order_data, "i");
        std::string side = this->safe_string_lower(order_data, "S");
        std::string type = this->safe_string_lower(order_data, "o");
        std::string status = this->safe_string(order_data, "X");
        
        json order = {
            {"symbol", symbol},
            {"id", order_id},
            {"clientOrderId", client_order_id},
            {"side", side},
            {"type", type},
            {"status", status}
        };
        
        this->emit("order", order);
    }
}

void binanceusdm_ws::handle_balance_update(const json& message) {
    if (message.contains("a")) {
        json account_data = message["a"];
        if (account_data.contains("B")) {
            json balances = account_data["B"];
            json result;
            
            for (const auto& balance : balances) {
                std::string asset = this->safe_string(balance, "a");
                result[asset] = {
                    {"free", this->safe_string(balance, "wb")},
                    {"used", this->safe_string(balance, "cw")},
                    {"total", this->safe_string(balance, "wb")}
                };
            }
            
            this->emit("balance", result);
        }
    }
}

void binanceusdm_ws::handle_position_update(const json& message) {
    if (message.contains("a")) {
        json account_data = message["a"];
        if (account_data.contains("P")) {
            json positions = account_data["P"];
            json result;
            
            for (const auto& position : positions) {
                std::string symbol = this->safe_string(position, "s");
                result[symbol] = {
                    {"size", this->safe_string(position, "pa")},
                    {"side", this->safe_string(position, "ps")},
                    {"unrealizedPnl", this->safe_string(position, "up")},
                    {"leverage", this->safe_string(position, "l")}
                };
            }
            
            this->emit("position", result);
        }
    }
}

} // namespace ccxt
