#include "ccxt/exchanges/ws/woo_ws.h"
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

WooWS::WooWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Woo& exchange)
    : WebSocketClient(ioc, ctx)
    , exchange_(exchange)
    , authenticated_(false)
    , sequenceNumber_(1) {
    this->onMessage = [this](const std::string& message) {
        this->handleMessage(message);
    };
}

std::string WooWS::getEndpoint(const std::string& type) {
    if (type == "public") {
        return "wss://wss.woo.org/ws/stream";
    } else {
        return "wss://wss.woo.org/v2/ws/private/stream";
    }
}

int64_t WooWS::getNextSequenceNumber() {
    return sequenceNumber_++;
}

std::string WooWS::normalizeSymbol(const std::string& symbol) {
    // WOO uses SPOT_BTC_USDT format
    return "SPOT_" + symbol;
}

std::string WooWS::generateSignature(const std::string& timestamp) {
    std::string message = timestamp + "|" + exchange_.apiKey;
    return exchange_.hmac(message, exchange_.secret, "sha256");
}

void WooWS::authenticate() {
    if (!authenticated_ && !exchange_.apiKey.empty()) {
        std::string timestamp = std::to_string(std::time(nullptr) * 1000);
        std::string signature = generateSignature(timestamp);

        nlohmann::json auth_message = {
            {"event", "auth"},
            {"params", {
                {"apikey", exchange_.apiKey},
                {"sign", signature},
                {"timestamp", timestamp}
            }}
        };

        send(auth_message.dump());
    }
}

void WooWS::ping() {
    nlohmann::json ping_message = {
        {"event", "ping"},
        {"ts", std::time(nullptr) * 1000}
    };
    send(ping_message.dump());
}

void WooWS::startPingLoop() {
    std::thread([this]() {
        while (true) {
            ping();
            std::this_thread::sleep_for(std::chrono::seconds(30));
        }
    }).detach();
}

void WooWS::subscribe(const std::string& channel, const std::string& symbol, bool isPrivate) {
    if (isPrivate) {
        authenticate();
    }

    std::string topic = symbol.empty() ? channel : channel + "@" + normalizeSymbol(symbol);
    
    nlohmann::json sub_message = {
        {"event", "subscribe"},
        {"id", getNextSequenceNumber()},
        {"topic", topic}
    };

    subscriptions_[topic] = symbol;
    send(sub_message.dump());
}

void WooWS::unsubscribe(const std::string& channel, const std::string& symbol) {
    std::string topic = symbol.empty() ? channel : channel + "@" + normalizeSymbol(symbol);
    
    nlohmann::json unsub_message = {
        {"event", "unsubscribe"},
        {"id", getNextSequenceNumber()},
        {"topic", topic}
    };

    subscriptions_.erase(topic);
    send(unsub_message.dump());
}

void WooWS::watchTicker(const std::string& symbol) {
    subscribe("ticker", symbol);
}

void WooWS::watchTickers(const std::vector<std::string>& symbols) {
    for (const auto& symbol : symbols) {
        watchTicker(symbol);
    }
}

void WooWS::watchOrderBook(const std::string& symbol, const int limit) {
    subscribe("orderbook", symbol);
}

void WooWS::watchTrades(const std::string& symbol) {
    subscribe("trade", symbol);
}

void WooWS::watchOHLCV(const std::string& symbol, const std::string& timeframe) {
    subscribe("kline_" + timeframe, symbol);
}

void WooWS::watchBalance() {
    subscribe("balance", "", true);
}

void WooWS::watchOrders(const std::string& symbol) {
    subscribe("order", symbol, true);
}

void WooWS::watchMyTrades(const std::string& symbol) {
    subscribe("execution", symbol, true);
}

void WooWS::handleMessage(const std::string& message) {
    auto j = nlohmann::json::parse(message);

    if (!j.contains("topic")) {
        if (j.contains("event")) {
            std::string event = j["event"];
            if (event == "auth") {
                handleAuthMessage(j);
            } else if (event == "pong") {
                handlePongMessage(j);
            } else if (event == "error") {
                handleErrorMessage(j);
            }
        }
        return;
    }

    std::string topic = j["topic"];
    
    if (topic.find("ticker@") == 0) {
        handleTickerMessage(j);
    } else if (topic.find("orderbook@") == 0) {
        handleOrderBookMessage(j);
    } else if (topic.find("trade@") == 0) {
        handleTradeMessage(j);
    } else if (topic.find("kline_") == 0) {
        handleOHLCVMessage(j);
    } else if (topic == "balance") {
        handleBalanceMessage(j);
    } else if (topic.find("order@") == 0) {
        handleOrderMessage(j);
    } else if (topic.find("execution@") == 0) {
        handleMyTradeMessage(j);
    }
}

void WooWS::handleTickerMessage(const nlohmann::json& data) {
    auto symbol = data["symbol"];
    auto ticker = data["data"];
    
    emit(symbol, "ticker", {
        {"symbol", symbol},
        {"timestamp", ticker["timestamp"]},
        {"datetime", exchange_.iso8601(ticker["timestamp"].get<int64_t>())},
        {"high", ticker["high"].get<double>()},
        {"low", ticker["low"].get<double>()},
        {"bid", ticker["bid"].get<double>()},
        {"ask", ticker["ask"].get<double>()},
        {"last", ticker["last"].get<double>()},
        {"close", ticker["last"].get<double>()},
        {"baseVolume", ticker["volume"].get<double>()},
        {"quoteVolume", ticker["quoteVolume"].get<double>()},
        {"change", ticker["change"].get<double>()},
        {"percentage", ticker["changePercentage"].get<double>()},
        {"info", ticker}
    });
}

void WooWS::handleOrderBookMessage(const nlohmann::json& data) {
    auto symbol = data["symbol"];
    auto book = data["data"];
    
    std::vector<std::vector<double>> bids;
    std::vector<std::vector<double>> asks;
    
    for (const auto& bid : book["bids"]) {
        bids.push_back({
            bid[0].get<double>(),  // price
            bid[1].get<double>()   // amount
        });
    }
    
    for (const auto& ask : book["asks"]) {
        asks.push_back({
            ask[0].get<double>(),  // price
            ask[1].get<double>()   // amount
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

void WooWS::handleTradeMessage(const nlohmann::json& data) {
    auto symbol = data["symbol"];
    auto trades = data["data"];
    
    for (const auto& t : trades) {
        emit(symbol, "trade", {
            {"id", t["id"]},
            {"symbol", symbol},
            {"timestamp", t["timestamp"]},
            {"datetime", exchange_.iso8601(t["timestamp"].get<int64_t>())},
            {"side", t["side"]},
            {"price", t["price"].get<double>()},
            {"amount", t["size"].get<double>()},
            {"cost", t["price"].get<double>() * t["size"].get<double>()},
            {"info", t}
        });
    }
}

void WooWS::handleOHLCVMessage(const nlohmann::json& data) {
    auto symbol = data["symbol"];
    auto kline = data["data"];
    
    emit(symbol, "ohlcv", {
        {"timestamp", kline["timestamp"]},
        {"datetime", exchange_.iso8601(kline["timestamp"].get<int64_t>())},
        {"open", kline["open"].get<double>()},
        {"high", kline["high"].get<double>()},
        {"low", kline["low"].get<double>()},
        {"close", kline["close"].get<double>()},
        {"volume", kline["volume"].get<double>()}
    });
}

void WooWS::handleBalanceMessage(const nlohmann::json& data) {
    auto balances = data["data"];
    nlohmann::json result;
    
    for (const auto& balance : balances.items()) {
        result[balance.key()] = {
            {"free", balance.value()["available"].get<double>()},
            {"used", balance.value()["frozen"].get<double>()},
            {"total", balance.value()["total"].get<double>()}
        };
    }
    
    emit("", "balance", result);
}

void WooWS::handleOrderMessage(const nlohmann::json& data) {
    auto symbol = data["symbol"];
    auto order = data["data"];
    
    emit(symbol, "order", {
        {"id", order["orderId"]},
        {"clientOrderId", order["clientOrderId"]},
        {"symbol", symbol},
        {"type", order["type"]},
        {"side", order["side"]},
        {"price", order["price"].get<double>()},
        {"amount", order["quantity"].get<double>()},
        {"filled", order["executedQuantity"].get<double>()},
        {"remaining", order["quantity"].get<double>() - order["executedQuantity"].get<double>()},
        {"status", order["status"]},
        {"timestamp", order["timestamp"]},
        {"datetime", exchange_.iso8601(order["timestamp"].get<int64_t>())},
        {"info", order}
    });
}

void WooWS::handleMyTradeMessage(const nlohmann::json& data) {
    auto symbol = data["symbol"];
    auto trade = data["data"];
    
    emit(symbol, "trade", {
        {"id", trade["tradeId"]},
        {"order", trade["orderId"]},
        {"symbol", symbol},
        {"side", trade["side"]},
        {"price", trade["price"].get<double>()},
        {"amount", trade["quantity"].get<double>()},
        {"cost", trade["price"].get<double>() * trade["quantity"].get<double>()},
        {"fee", {
            {"cost", trade["fee"].get<double>()},
            {"currency", trade["feeCurrency"]}
        }},
        {"timestamp", trade["timestamp"]},
        {"datetime", exchange_.iso8601(trade["timestamp"].get<int64_t>())},
        {"info", trade}
    });
}

void WooWS::handleErrorMessage(const nlohmann::json& data) {
    if (data.contains("message")) {
        std::string error_message = data["message"];
        // Handle error appropriately
    }
}

void WooWS::handleAuthMessage(const nlohmann::json& data) {
    if (data.contains("success") && data["success"].get<bool>()) {
        authenticated_ = true;
        startPingLoop();
    }
}

void WooWS::handlePongMessage(const nlohmann::json& data) {
    // Handle pong response if needed
}

} // namespace ccxt
