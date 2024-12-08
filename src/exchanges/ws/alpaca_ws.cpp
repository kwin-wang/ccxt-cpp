#include "../../../include/ccxt/exchanges/ws/alpaca_ws.h"
#include <iostream>
#include <sstream>
#include <chrono>

namespace ccxt {

AlpacaWS::AlpacaWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Alpaca& exchange)
    : WebSocketClient(ioc, ctx), exchange_(exchange), tradesLimit_(1000) {
    apiKey_ = exchange_.apiKey;
    apiSecret_ = exchange_.secret;
}

std::string AlpacaWS::getStreamUrl(bool isPrivate) const {
    if (exchange_.testnet) {
        return isPrivate ? "wss://paper-api.alpaca.markets/stream" : "wss://stream.data.alpaca.markets/v2/iex";
    }
    return isPrivate ? "wss://api.alpaca.markets/stream" : "wss://stream.data.alpaca.markets/v2/iex";
}

void AlpacaWS::authenticate() {
    nlohmann::json auth = {
        {"action", "auth"},
        {"key", apiKey_},
        {"secret", apiSecret_}
    };
    send(auth.dump());
}

void AlpacaWS::subscribe(const std::string& channel, const std::vector<std::string>& symbols) {
    nlohmann::json request = {
        {"action", "subscribe"}
    };

    if (symbols.empty()) {
        request["trades"] = {"*"};
        request["quotes"] = {"*"};
        request["bars"] = {"*"};
    } else {
        request["trades"] = symbols;
        request["quotes"] = symbols;
        request["bars"] = symbols;
    }

    send(request.dump());
}

void AlpacaWS::unsubscribe(const std::string& channel, const std::vector<std::string>& symbols) {
    nlohmann::json request = {
        {"action", "unsubscribe"}
    };

    if (symbols.empty()) {
        request["trades"] = {"*"};
        request["quotes"] = {"*"};
        request["bars"] = {"*"};
    } else {
        request["trades"] = symbols;
        request["quotes"] = symbols;
        request["bars"] = symbols;
    }

    send(request.dump());
}

void AlpacaWS::watchTicker(const std::string& symbol) {
    subscribe("quotes", {symbol});
}

void AlpacaWS::watchTickers(const std::vector<std::string>& symbols) {
    subscribe("quotes", symbols);
}

void AlpacaWS::watchOrderBook(const std::string& symbol) {
    subscribe("quotes", {symbol});
}

void AlpacaWS::watchTrades(const std::string& symbol) {
    subscribe("trades", {symbol});
}

void AlpacaWS::watchBidsAsks(const std::string& symbol) {
    subscribe("quotes", {symbol});
}

void AlpacaWS::watchOHLCV(const std::string& symbol, const std::string& timeframe) {
    subscribe("bars", {symbol});
}

void AlpacaWS::watchBalance() {
    authenticate();
    subscribe("account_updates");
}

void AlpacaWS::watchOrders() {
    authenticate();
    subscribe("trade_updates");
}

void AlpacaWS::watchMyTrades() {
    authenticate();
    subscribe("trade_updates");
}

void AlpacaWS::watchPositions() {
    authenticate();
    subscribe("position_updates");
}

void AlpacaWS::handleMessage(const std::string& message) {
    try {
        auto j = nlohmann::json::parse(message);

        if (j.contains("stream")) {
            std::string stream = j["stream"];
            auto data = j["data"];

            if (stream == "authorization") {
                if (data["status"] == "authorized") {
                    emit("authenticated", data);
                } else {
                    emit("error", data);
                }
            } else if (stream == "trade_updates") {
                handleOrder(data);
            } else if (stream == "position_updates") {
                handlePosition(data);
            } else if (stream == "account_updates") {
                handleBalance(data);
            } else if (stream == "trades") {
                handleTrade(data, false);
            } else if (stream == "quotes") {
                handleQuote(data);
            } else if (stream == "bars") {
                handleBar(data);
            }
        }
    } catch (const std::exception& e) {
        emit("error", {{"message", e.what()}});
    }
}

void AlpacaWS::handleTrade(const nlohmann::json& data, bool isPrivate) {
    nlohmann::json parsedTrade = {
        {"id", data["i"]},
        {"info", data},
        {"timestamp", std::stoll(data["t"])},
        {"datetime", exchange_.iso8601(std::stoll(data["t"]))},
        {"symbol", data["S"]},
        {"type", nullptr},
        {"side", nullptr},
        {"price", std::stod(data["p"])},
        {"amount", std::stod(data["s"])},
        {"cost", std::stod(data["p"]) * std::stod(data["s"])}
    };

    if (isPrivate) {
        parsedTrade["order"] = data["o"];
        parsedTrade["fee"] = {
            {"cost", data.value("f", 0.0)},
            {"currency", "USD"}
        };
    }

    emit("trade", parsedTrade);
}

void AlpacaWS::handleQuote(const nlohmann::json& data) {
    nlohmann::json parsedQuote = {
        {"symbol", data["S"]},
        {"timestamp", std::stoll(data["t"])},
        {"datetime", exchange_.iso8601(std::stoll(data["t"]))},
        {"bid", std::stod(data["bp"])},
        {"bidVolume", std::stod(data["bs"])},
        {"ask", std::stod(data["ap"])},
        {"askVolume", std::stod(data["as"])},
        {"vwap", nullptr},
        {"open", nullptr},
        {"close", nullptr},
        {"last", nullptr},
        {"previousClose", nullptr},
        {"change", nullptr},
        {"percentage", nullptr},
        {"average", nullptr},
        {"baseVolume", nullptr},
        {"quoteVolume", nullptr},
        {"info", data}
    };
    emit("ticker", parsedQuote);
}

void AlpacaWS::handleBar(const nlohmann::json& data) {
    nlohmann::json parsedBar = {
        std::stoll(data["t"]),      // timestamp
        std::stod(data["o"]),       // open
        std::stod(data["h"]),       // high
        std::stod(data["l"]),       // low
        std::stod(data["c"]),       // close
        std::stod(data["v"]),       // volume
    };
    emit("ohlcv", parsedBar);
}

void AlpacaWS::handleOrder(const nlohmann::json& data) {
    std::string status;
    auto orderStatus = data["event"];
    
    if (orderStatus == "new" || orderStatus == "replaced") {
        status = "open";
    } else if (orderStatus == "filled") {
        status = "closed";
    } else if (orderStatus == "canceled" || orderStatus == "expired") {
        status = "canceled";
    } else if (orderStatus == "rejected") {
        status = "rejected";
    } else {
        status = "unknown";
    }

    auto order = data["order"];
    nlohmann::json parsedOrder = {
        {"id", order["id"]},
        {"clientOrderId", order["client_order_id"]},
        {"info", data},
        {"timestamp", std::stoll(order["created_at"])},
        {"datetime", exchange_.iso8601(std::stoll(order["created_at"]))},
        {"lastTradeTimestamp", std::stoll(order["updated_at"])},
        {"status", status},
        {"symbol", order["symbol"]},
        {"type", order["type"]},
        {"side", order["side"]},
        {"price", std::stod(order["limit_price"].get<std::string>())},
        {"amount", std::stod(order["qty"])},
        {"filled", std::stod(order["filled_qty"])},
        {"remaining", std::stod(order["qty"]) - std::stod(order["filled_qty"])},
        {"cost", std::stod(order["filled_avg_price"].get<std::string>()) * std::stod(order["filled_qty"])},
        {"average", std::stod(order["filled_avg_price"].get<std::string>())},
        {"fee", nullptr}  // Alpaca doesn't provide fee information in real-time
    };
    emit("order", parsedOrder);
}

void AlpacaWS::handlePosition(const nlohmann::json& data) {
    nlohmann::json parsedPosition = {
        {"symbol", data["symbol"]},
        {"info", data},
        {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()},
        {"datetime", exchange_.iso8601(std::chrono::system_clock::now().time_since_epoch().count())},
        {"long", data["side"] == "long"},
        {"short", data["side"] == "short"},
        {"side", data["side"]},
        {"quantity", std::stod(data["qty"])},
        {"price", std::stod(data["avg_entry_price"])},
        {"cost", std::stod(data["market_value"])},
        {"unrealizedPnl", std::stod(data["unrealized_pl"])},
        {"realizedPnl", std::stod(data["realized_pl"])}
    };
    emit("position", parsedPosition);
}

void AlpacaWS::handleBalance(const nlohmann::json& data) {
    nlohmann::json parsedBalance = {
        {"info", data},
        {"free", std::stod(data["cash"])},
        {"used", std::stod(data["long_market_value"]) + std::stod(data["short_market_value"])},
        {"total", std::stod(data["equity"])},
        {"currency", "USD"}
    };
    emit("balance", parsedBalance);
}

} // namespace ccxt
