#include "ccxt/exchanges/ws/wazirx_ws.h"
#include "ccxt/base/json.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace ccxt {

WazirXWS::WazirXWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, WazirX& exchange)
    : WebSocketClient(ioc, ctx)
    , exchange_(exchange)
    , authenticated_(false)
    , sequenceNumber_(1) {
    this->onMessage = [this](const std::string& message) {
        this->handleMessage(message);
    };
}

std::string WazirXWS::getEndpoint(const std::string& type) {
    return "wss://stream.wazirx.com/stream";
}

int64_t WazirXWS::getNextSequenceNumber() {
    return sequenceNumber_++;
}

std::string WazirXWS::normalizeSymbol(const std::string& symbol) {
    // WazirX uses lowercase symbols without separator
    std::string normalized = symbol;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    return normalized;
}

std::string WazirXWS::generateSignature(const std::string& timestamp, const std::string& method,
                                      const std::string& path) {
    std::string message = timestamp + method + path;
    return exchange_.hmac(message, exchange_.secret, "sha256");
}

void WazirXWS::authenticate() {
    if (!authenticated_ && !exchange_.apiKey.empty()) {
        std::stringstream ss;
        ss << std::time(nullptr) * 1000;
        std::string timestamp = ss.str();
        
        nlohmann::json auth_message = {
            {"event", "auth"},
            {"auth", {
                {"key", exchange_.apiKey},
                {"timestamp", timestamp},
                {"signature", generateSignature(timestamp, "GET", "/stream")}
            }}
        };

        send(auth_message.dump());
    }
}

void WazirXWS::ping() {
    nlohmann::json ping_message = {
        {"event", "ping"},
        {"ts", std::time(nullptr) * 1000}
    };
    send(ping_message.dump());
}

void WazirXWS::subscribe(const std::string& channel, const std::string& symbol, bool isPrivate) {
    if (isPrivate) {
        authenticate();
    }

    std::string stream = symbol.empty() ? channel : channel + "@" + normalizeSymbol(symbol);
    
    nlohmann::json sub_message = {
        {"event", "subscribe"},
        {"streams", {stream}}
    };

    subscriptions_[stream] = symbol;
    send(sub_message.dump());
}

void WazirXWS::unsubscribe(const std::string& channel, const std::string& symbol) {
    std::string stream = symbol.empty() ? channel : channel + "@" + normalizeSymbol(symbol);
    
    nlohmann::json unsub_message = {
        {"event", "unsubscribe"},
        {"streams", {stream}}
    };

    subscriptions_.erase(stream);
    send(unsub_message.dump());
}

void WazirXWS::watchTicker(const std::string& symbol) {
    subscribe("ticker", symbol);
}

void WazirXWS::watchTickers(const std::vector<std::string>& symbols) {
    for (const auto& symbol : symbols) {
        watchTicker(symbol);
    }
}

void WazirXWS::watchOrderBook(const std::string& symbol, const int limit) {
    subscribe("depth", symbol);
}

void WazirXWS::watchTrades(const std::string& symbol) {
    subscribe("trades", symbol);
}

void WazirXWS::watchOHLCV(const std::string& symbol, const std::string& timeframe) {
    subscribe("kline_" + timeframe, symbol);
}

void WazirXWS::watchBalance() {
    subscribe("balances", "", true);
}

void WazirXWS::watchOrders(const std::string& symbol) {
    subscribe("orders", symbol, true);
}

void WazirXWS::watchMyTrades(const std::string& symbol) {
    subscribe("mytrades", symbol, true);
}

void WazirXWS::handleMessage(const std::string& message) {
    auto j = nlohmann::json::parse(message);

    if (!j.contains("stream")) return;

    std::string stream = j["stream"];
    auto data = j["data"];
    
    if (stream.find("ticker@") == 0) {
        handleTickerMessage(data);
    } else if (stream.find("depth@") == 0) {
        handleOrderBookMessage(data);
    } else if (stream.find("trades@") == 0) {
        handleTradeMessage(data);
    } else if (stream.find("kline_") == 0) {
        handleOHLCVMessage(data);
    } else if (stream == "balances") {
        handleBalanceMessage(data);
    } else if (stream.find("orders@") == 0) {
        handleOrderMessage(data);
    } else if (stream.find("mytrades@") == 0) {
        handleMyTradeMessage(data);
    } else if (j.contains("error")) {
        handleErrorMessage(j);
    } else if (stream == "auth") {
        handleAuthMessage(data);
    }
}

void WazirXWS::handleTickerMessage(const nlohmann::json& data) {
    std::string symbol = data["symbol"];
    
    emit(symbol, "ticker", {
        {"symbol", symbol},
        {"high", std::stod(data["high"].get<std::string>())},
        {"low", std::stod(data["low"].get<std::string>())},
        {"last", std::stod(data["last_price"].get<std::string>())},
        {"bid", std::stod(data["best_bid"].get<std::string>())},
        {"ask", std::stod(data["best_ask"].get<std::string>())},
        {"baseVolume", std::stod(data["volume"].get<std::string>())},
        {"quoteVolume", std::stod(data["quote_volume"].get<std::string>())},
        {"percentage", std::stod(data["price_change_percent"].get<std::string>())},
        {"timestamp", data["timestamp"]}
    });
}

void WazirXWS::handleOrderBookMessage(const nlohmann::json& data) {
    std::string symbol = data["symbol"];
    
    nlohmann::json orderbook;
    orderbook["symbol"] = symbol;
    orderbook["timestamp"] = data["timestamp"];
    
    std::vector<std::vector<double>> bids;
    std::vector<std::vector<double>> asks;
    
    for (const auto& bid : data["bids"]) {
        bids.push_back({
            std::stod(bid[0].get<std::string>()),  // price
            std::stod(bid[1].get<std::string>())   // amount
        });
    }
    
    for (const auto& ask : data["asks"]) {
        asks.push_back({
            std::stod(ask[0].get<std::string>()),  // price
            std::stod(ask[1].get<std::string>())   // amount
        });
    }
    
    orderbook["bids"] = bids;
    orderbook["asks"] = asks;
    orderbook["nonce"] = data["lastUpdateId"];
    
    emit(symbol, "orderbook", orderbook);
}

void WazirXWS::handleTradeMessage(const nlohmann::json& data) {
    std::string symbol = data["symbol"];
    
    emit(symbol, "trade", {
        {"id", data["id"]},
        {"symbol", symbol},
        {"price", std::stod(data["price"].get<std::string>())},
        {"amount", std::stod(data["quantity"].get<std::string>())},
        {"side", data["side"]},
        {"timestamp", data["timestamp"]}
    });
}

void WazirXWS::handleOHLCVMessage(const nlohmann::json& data) {
    std::string symbol = data["symbol"];
    
    emit(symbol, "ohlcv", {
        {"timestamp", data["timestamp"]},
        {"open", std::stod(data["open"].get<std::string>())},
        {"high", std::stod(data["high"].get<std::string>())},
        {"low", std::stod(data["low"].get<std::string>())},
        {"close", std::stod(data["close"].get<std::string>())},
        {"volume", std::stod(data["volume"].get<std::string>())}
    });
}

void WazirXWS::handleBalanceMessage(const nlohmann::json& data) {
    nlohmann::json balance;
    
    for (const auto& asset : data.items()) {
        balance[asset.key()] = {
            {"free", std::stod(asset.value()["available"].get<std::string>())},
            {"used", std::stod(asset.value()["locked"].get<std::string>())},
            {"total", std::stod(asset.value()["total"].get<std::string>())}
        };
    }
    
    emit("", "balance", balance);
}

void WazirXWS::handleOrderMessage(const nlohmann::json& data) {
    std::string symbol = data["symbol"];
    
    emit(symbol, "order", {
        {"id", data["id"]},
        {"clientOrderId", data["client_order_id"]},
        {"symbol", symbol},
        {"type", data["type"]},
        {"side", data["side"]},
        {"price", std::stod(data["price"].get<std::string>())},
        {"amount", std::stod(data["quantity"].get<std::string>())},
        {"filled", std::stod(data["executed_quantity"].get<std::string>())},
        {"remaining", std::stod(data["remaining_quantity"].get<std::string>())},
        {"status", data["status"]},
        {"timestamp", data["timestamp"]}
    });
}

void WazirXWS::handleMyTradeMessage(const nlohmann::json& data) {
    std::string symbol = data["symbol"];
    
    emit(symbol, "mytrade", {
        {"id", data["id"]},
        {"orderId", data["order_id"]},
        {"symbol", symbol},
        {"type", data["type"]},
        {"side", data["side"]},
        {"price", std::stod(data["price"].get<std::string>())},
        {"amount", std::stod(data["quantity"].get<std::string>())},
        {"fee", std::stod(data["fee"].get<std::string>())},
        {"feeCurrency", data["fee_currency"]},
        {"timestamp", data["timestamp"]}
    });
}

void WazirXWS::handleErrorMessage(const nlohmann::json& data) {
    if (data.contains("message")) {
        std::string error_message = data["message"];
        // Handle error appropriately
    }
}

void WazirXWS::handleAuthMessage(const nlohmann::json& data) {
    if (data.contains("success")) {
        authenticated_ = data["success"].get<bool>();
    }
}

} // namespace ccxt
