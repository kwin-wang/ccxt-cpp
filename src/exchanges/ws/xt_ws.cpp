#include "ccxt/exchanges/ws/xt_ws.h"
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
#include <chrono>
#include <thread>

namespace ccxt {

XTWS::XTWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, XT& exchange)
    : WebSocketClient(ioc, ctx)
    , exchange_(exchange)
    , authenticated_(false)
    , sequenceNumber_(1) {
    this->onMessage = [this](const std::string& message) {
        this->handleMessage(message);
    };
}

std::string XTWS::getEndpoint(const std::string& type) {
    if (type == "public") {
        return "wss://stream.xt.com/public";
    } else {
        return "wss://stream.xt.com/private";
    }
}

int64_t XTWS::getNextSequenceNumber() {
    return sequenceNumber_++;
}

std::string XTWS::normalizeSymbol(const std::string& symbol) {
    // XT uses lowercase symbols
    std::string normalized = symbol;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    return normalized;
}

std::string XTWS::generateSignature(const std::string& timestamp, const std::string& method,
                                  const std::string& path, const std::string& body) {
    std::string message = timestamp + method + path + body;
    return exchange_.hmac(message, exchange_.secret, "sha256");
}

void XTWS::authenticate() {
    if (!authenticated_ && !exchange_.apiKey.empty()) {
        std::string timestamp = std::to_string(std::time(nullptr) * 1000);
        std::string signature = generateSignature(timestamp, "GET", "/ws/private", "");

        nlohmann::json auth_message = {
            {"method", "login"},
            {"params", {
                {"apiKey", exchange_.apiKey},
                {"timestamp", timestamp},
                {"signature", signature}
            }},
            {"id", getNextSequenceNumber()}
        };

        send(auth_message.dump());
    }
}

void XTWS::ping() {
    nlohmann::json ping_message = {
        {"method", "ping"},
        {"params", {}},
        {"id", getNextSequenceNumber()}
    };
    send(ping_message.dump());
}

void XTWS::startPingLoop() {
    std::thread([this]() {
        while (true) {
            ping();
            std::this_thread::sleep_for(std::chrono::seconds(20));
        }
    }).detach();
}

void XTWS::subscribe(const std::string& channel, const std::string& symbol, bool isPrivate) {
    if (isPrivate) {
        authenticate();
    }

    std::string topic = symbol.empty() ? channel : channel + "." + normalizeSymbol(symbol);
    
    nlohmann::json sub_message = {
        {"method", "subscribe"},
        {"params", {
            {"channel", topic}
        }},
        {"id", getNextSequenceNumber()}
    };

    subscriptions_[topic] = symbol;
    send(sub_message.dump());
}

void XTWS::unsubscribe(const std::string& channel, const std::string& symbol) {
    std::string topic = symbol.empty() ? channel : channel + "." + normalizeSymbol(symbol);
    
    nlohmann::json unsub_message = {
        {"method", "unsubscribe"},
        {"params", {
            {"channel", topic}
        }},
        {"id", getNextSequenceNumber()}
    };

    subscriptions_.erase(topic);
    send(unsub_message.dump());
}

void XTWS::watchTicker(const std::string& symbol) {
    subscribe("ticker", symbol);
}

void XTWS::watchTickers(const std::vector<std::string>& symbols) {
    for (const auto& symbol : symbols) {
        watchTicker(symbol);
    }
}

void XTWS::watchOrderBook(const std::string& symbol, const int limit) {
    std::string channel = limit <= 20 ? "depth20" : "depth";
    subscribe(channel, symbol);
}

void XTWS::watchTrades(const std::string& symbol) {
    subscribe("trade", symbol);
}

void XTWS::watchOHLCV(const std::string& symbol, const std::string& timeframe) {
    subscribe("kline_" + timeframe, symbol);
}

void XTWS::watchBalance() {
    subscribe("balance", "", true);
}

void XTWS::watchOrders(const std::string& symbol) {
    subscribe("order", symbol, true);
}

void XTWS::watchMyTrades(const std::string& symbol) {
    subscribe("trade", symbol, true);
}

void XTWS::handleMessage(const std::string& message) {
    auto j = nlohmann::json::parse(message);

    if (j.contains("method")) {
        std::string method = j["method"];
        if (method == "login") {
            handleAuthMessage(j);
        } else if (method == "pong") {
            handlePongMessage(j);
        }
        return;
    }

    if (!j.contains("channel")) {
        if (j.contains("error")) {
            handleErrorMessage(j);
        }
        return;
    }

    std::string channel = j["channel"];
    
    if (channel.find("ticker.") == 0) {
        handleTickerMessage(j);
    } else if (channel.find("depth") == 0) {
        handleOrderBookMessage(j);
    } else if (channel.find("trade.") == 0) {
        if (j.contains("private") && j["private"].get<bool>()) {
            handleMyTradeMessage(j);
        } else {
            handleTradeMessage(j);
        }
    } else if (channel.find("kline_") == 0) {
        handleOHLCVMessage(j);
    } else if (channel == "balance") {
        handleBalanceMessage(j);
    } else if (channel.find("order.") == 0) {
        handleOrderMessage(j);
    }
}

void XTWS::handleTickerMessage(const nlohmann::json& data) {
    auto symbol = data["symbol"];
    auto ticker = data["data"];
    
    emit(symbol, "ticker", {
        {"symbol", symbol},
        {"timestamp", ticker["timestamp"]},
        {"datetime", exchange_.iso8601(ticker["timestamp"].get<int64_t>())},
        {"high", std::stod(ticker["high"])},
        {"low", std::stod(ticker["low"])},
        {"bid", std::stod(ticker["bid"])},
        {"ask", std::stod(ticker["ask"])},
        {"last", std::stod(ticker["last"])},
        {"close", std::stod(ticker["last"])},
        {"baseVolume", std::stod(ticker["volume"])},
        {"quoteVolume", std::stod(ticker["quoteVolume"])},
        {"change", std::stod(ticker["change"])},
        {"percentage", std::stod(ticker["percentage"])},
        {"info", ticker}
    });
}

void XTWS::handleOrderBookMessage(const nlohmann::json& data) {
    auto symbol = data["symbol"];
    auto book = data["data"];
    
    std::vector<std::vector<double>> bids;
    std::vector<std::vector<double>> asks;
    
    for (const auto& bid : book["bids"]) {
        bids.push_back({
            std::stod(bid[0]),  // price
            std::stod(bid[1])   // amount
        });
    }
    
    for (const auto& ask : book["asks"]) {
        asks.push_back({
            std::stod(ask[0]),  // price
            std::stod(ask[1])   // amount
        });
    }
    
    emit(symbol, "orderbook", {
        {"symbol", symbol},
        {"bids", bids},
        {"asks", asks},
        {"timestamp", book["timestamp"]},
        {"datetime", exchange_.iso8601(book["timestamp"].get<int64_t>())},
        {"nonce", book["version"]}
    });
}

void XTWS::handleTradeMessage(const nlohmann::json& data) {
    auto symbol = data["symbol"];
    auto trades = data["data"];
    
    for (const auto& t : trades) {
        emit(symbol, "trade", {
            {"id", t["id"]},
            {"symbol", symbol},
            {"timestamp", t["timestamp"]},
            {"datetime", exchange_.iso8601(t["timestamp"].get<int64_t>())},
            {"side", t["side"]},
            {"price", std::stod(t["price"])},
            {"amount", std::stod(t["amount"])},
            {"cost", std::stod(t["price"]) * std::stod(t["amount"])},
            {"info", t}
        });
    }
}

void XTWS::handleOHLCVMessage(const nlohmann::json& data) {
    auto symbol = data["symbol"];
    auto kline = data["data"];
    
    emit(symbol, "ohlcv", {
        {"timestamp", kline["timestamp"]},
        {"datetime", exchange_.iso8601(kline["timestamp"].get<int64_t>())},
        {"open", std::stod(kline["open"])},
        {"high", std::stod(kline["high"])},
        {"low", std::stod(kline["low"])},
        {"close", std::stod(kline["close"])},
        {"volume", std::stod(kline["volume"])}
    });
}

void XTWS::handleBalanceMessage(const nlohmann::json& data) {
    auto balances = data["data"];
    nlohmann::json result;
    
    for (const auto& balance : balances.items()) {
        result[balance.key()] = {
            {"free", std::stod(balance.value()["available"])},
            {"used", std::stod(balance.value()["frozen"])},
            {"total", std::stod(balance.value()["total"])}
        };
    }
    
    emit("", "balance", result);
}

void XTWS::handleOrderMessage(const nlohmann::json& data) {
    auto symbol = data["symbol"];
    auto order = data["data"];
    
    emit(symbol, "order", {
        {"id", order["orderId"]},
        {"clientOrderId", order["clientOrderId"]},
        {"symbol", symbol},
        {"type", order["type"]},
        {"side", order["side"]},
        {"price", std::stod(order["price"])},
        {"amount", std::stod(order["amount"])},
        {"filled", std::stod(order["filled"])},
        {"remaining", std::stod(order["amount"]) - std::stod(order["filled"])},
        {"status", order["status"]},
        {"timestamp", order["timestamp"]},
        {"datetime", exchange_.iso8601(order["timestamp"].get<int64_t>())},
        {"info", order}
    });
}

void XTWS::handleMyTradeMessage(const nlohmann::json& data) {
    auto symbol = data["symbol"];
    auto trade = data["data"];
    
    emit(symbol, "trade", {
        {"id", trade["tradeId"]},
        {"order", trade["orderId"]},
        {"symbol", symbol},
        {"side", trade["side"]},
        {"price", std::stod(trade["price"])},
        {"amount", std::stod(trade["amount"])},
        {"cost", std::stod(trade["price"]) * std::stod(trade["amount"])},
        {"fee", {
            {"cost", std::stod(trade["fee"])},
            {"currency", trade["feeCurrency"]}
        }},
        {"timestamp", trade["timestamp"]},
        {"datetime", exchange_.iso8601(trade["timestamp"].get<int64_t>())},
        {"info", trade}
    });
}

void XTWS::handleErrorMessage(const nlohmann::json& data) {
    if (data.contains("message")) {
        std::string error_message = data["message"];
        // Handle error appropriately
    }
}

void XTWS::handleAuthMessage(const nlohmann::json& data) {
    if (data.contains("result") && data["result"].get<bool>()) {
        authenticated_ = true;
        startPingLoop();
    }
}

void XTWS::handlePongMessage(const nlohmann::json& data) {
    // Handle pong response if needed
}

} // namespace ccxt
