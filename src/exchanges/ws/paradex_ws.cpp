#include "ccxt/exchanges/ws/paradex_ws.h"
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

ParadexWS::ParadexWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Paradex& exchange)
    : WebSocketClient(ioc, ctx)
    , exchange_(exchange)
    , authenticated_(false)
    , sequenceNumber_(1) {
    this->onMessage = [this](const std::string& message) {
        this->handleMessage(message);
    };
}

std::string ParadexWS::getEndpoint(const std::string& type) {
    return "wss://ws.paradex.io/" + type;
}

int64_t ParadexWS::getNextSequenceNumber() {
    return sequenceNumber_++;
}

std::string ParadexWS::generateSignature(const std::string& path, const std::string& method,
                                       const std::string& body, const std::string& timestamp) {
    std::string message = timestamp + method + path + body;
    return exchange_.hmac(message, exchange_.secret, "sha256");
}

void ParadexWS::authenticate() {
    if (!authenticated_ && !exchange_.apiKey.empty()) {
        std::stringstream ss;
        ss << std::time(nullptr) * 1000;
        std::string timestamp = ss.str();
        std::string path = "/ws/auth";
        std::string method = "GET";
        std::string body = "";
        
        std::string signature = generateSignature(path, method, body, timestamp);

        nlohmann::json auth_message = {
            {"type", "auth"},
            {"data", {
                {"apiKey", exchange_.apiKey},
                {"timestamp", timestamp},
                {"signature", signature}
            }}
        };

        send(auth_message.dump());
    }
}

void ParadexWS::ping() {
    nlohmann::json ping_message = {
        {"type", "ping"},
        {"data", {
            {"ts", std::time(nullptr) * 1000}
        }}
    };
    send(ping_message.dump());
}

void ParadexWS::subscribe(const std::string& channel, const std::string& symbol, bool isPrivate) {
    if (isPrivate) {
        authenticate();
    }

    nlohmann::json sub_data;
    if (!symbol.empty()) {
        sub_data["symbol"] = symbol;
    }
    sub_data["channel"] = channel;

    nlohmann::json sub_message = {
        {"type", "subscribe"},
        {"data", sub_data}
    };

    subscriptions_[channel + "_" + symbol] = symbol;
    send(sub_message.dump());
}

void ParadexWS::unsubscribe(const std::string& channel, const std::string& symbol) {
    nlohmann::json unsub_data;
    if (!symbol.empty()) {
        unsub_data["symbol"] = symbol;
    }
    unsub_data["channel"] = channel;

    nlohmann::json unsub_message = {
        {"type", "unsubscribe"},
        {"data", unsub_data}
    };

    subscriptions_.erase(channel + "_" + symbol);
    send(unsub_message.dump());
}

void ParadexWS::watchTicker(const std::string& symbol) {
    subscribe("ticker", symbol);
}

void ParadexWS::watchTickers(const std::vector<std::string>& symbols) {
    for (const auto& symbol : symbols) {
        watchTicker(symbol);
    }
}

void ParadexWS::watchOrderBook(const std::string& symbol, const int limit) {
    subscribe("orderbook", symbol);
}

void ParadexWS::watchTrades(const std::string& symbol) {
    subscribe("trades", symbol);
}

void ParadexWS::watchOHLCV(const std::string& symbol, const std::string& timeframe) {
    subscribe("kline_" + timeframe, symbol);
}

void ParadexWS::watchBalance() {
    subscribe("balance", "", true);
}

void ParadexWS::watchOrders(const std::string& symbol) {
    subscribe("orders", symbol, true);
}

void ParadexWS::watchMyTrades(const std::string& symbol) {
    subscribe("mytrades", symbol, true);
}

void ParadexWS::watchPositions(const std::string& symbol) {
    subscribe("positions", symbol, true);
}

void ParadexWS::handleMessage(const std::string& message) {
    auto j = nlohmann::json::parse(message);

    if (!j.contains("type")) return;

    std::string type = j["type"];
    
    if (type == "pong") {
        return;
    } else if (type == "auth") {
        handleAuthMessage(j["data"]);
    } else if (type == "error") {
        handleErrorMessage(j["data"]);
    } else if (type == "subscribed") {
        handleSubscriptionMessage(j["data"]);
    } else if (type == "unsubscribed") {
        handleUnsubscriptionMessage(j["data"]);
    } else if (type == "update") {
        if (!j.contains("data") || !j["data"].contains("channel")) return;

        std::string channel = j["data"]["channel"];
        auto data = j["data"];
        
        if (channel == "ticker") {
            handleTickerMessage(data);
        } else if (channel == "orderbook") {
            handleOrderBookMessage(data);
        } else if (channel == "trades") {
            handleTradeMessage(data);
        } else if (channel.find("kline_") == 0) {
            handleOHLCVMessage(data);
        } else if (channel == "balance") {
            handleBalanceMessage(data);
        } else if (channel == "orders") {
            handleOrderMessage(data);
        } else if (channel == "mytrades") {
            handleMyTradeMessage(data);
        } else if (channel == "positions") {
            handlePositionMessage(data);
        }
    }
}

void ParadexWS::handleTickerMessage(const nlohmann::json& data) {
    if (!data.contains("symbol")) return;

    std::string symbol = data["symbol"];
    
    emit(symbol, "ticker", {
        {"symbol", symbol},
        {"high", std::stod(data["high"].get<std::string>())},
        {"low", std::stod(data["low"].get<std::string>())},
        {"last", std::stod(data["last"].get<std::string>())},
        {"bid", std::stod(data["bid"].get<std::string>())},
        {"ask", std::stod(data["ask"].get<std::string>())},
        {"volume", std::stod(data["volume"].get<std::string>())},
        {"quoteVolume", std::stod(data["quoteVolume"].get<std::string>())},
        {"timestamp", data["timestamp"]}
    });
}

void ParadexWS::handleOrderBookMessage(const nlohmann::json& data) {
    if (!data.contains("symbol")) return;

    std::string symbol = data["symbol"];
    
    nlohmann::json orderbook;
    orderbook["symbol"] = symbol;
    orderbook["timestamp"] = data["timestamp"];
    orderbook["nonce"] = data["nonce"];
    
    std::vector<std::vector<double>> bids;
    std::vector<std::vector<double>> asks;
    
    for (const auto& bid : data["bids"]) {
        bids.push_back({std::stod(bid[0].get<std::string>()), std::stod(bid[1].get<std::string>())});
    }
    
    for (const auto& ask : data["asks"]) {
        asks.push_back({std::stod(ask[0].get<std::string>()), std::stod(ask[1].get<std::string>())});
    }
    
    orderbook["bids"] = bids;
    orderbook["asks"] = asks;
    
    emit(symbol, "orderbook", orderbook);
}

void ParadexWS::handleTradeMessage(const nlohmann::json& data) {
    if (!data.contains("symbol")) return;

    std::string symbol = data["symbol"];
    
    emit(symbol, "trade", {
        {"id", data["id"]},
        {"symbol", symbol},
        {"price", std::stod(data["price"].get<std::string>())},
        {"amount", std::stod(data["amount"].get<std::string>())},
        {"side", data["side"]},
        {"timestamp", data["timestamp"]}
    });
}

void ParadexWS::handleOHLCVMessage(const nlohmann::json& data) {
    if (!data.contains("symbol")) return;

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

void ParadexWS::handleBalanceMessage(const nlohmann::json& data) {
    if (!data.contains("balances")) return;

    nlohmann::json balance;
    
    for (const auto& asset : data["balances"].items()) {
        balance[asset.key()] = {
            {"free", std::stod(asset.value()["available"].get<std::string>())},
            {"used", std::stod(asset.value()["locked"].get<std::string>())},
            {"total", std::stod(asset.value()["total"].get<std::string>())}
        };
    }
    
    emit("", "balance", balance);
}

void ParadexWS::handleOrderMessage(const nlohmann::json& data) {
    if (!data.contains("symbol")) return;

    std::string symbol = data["symbol"];
    
    emit(symbol, "order", {
        {"id", data["id"]},
        {"clientOrderId", data["clientOrderId"]},
        {"symbol", symbol},
        {"type", data["type"]},
        {"side", data["side"]},
        {"price", std::stod(data["price"].get<std::string>())},
        {"amount", std::stod(data["amount"].get<std::string>())},
        {"filled", std::stod(data["filled"].get<std::string>())},
        {"remaining", std::stod(data["remaining"].get<std::string>())},
        {"status", data["status"]},
        {"timestamp", data["timestamp"]}
    });
}

void ParadexWS::handleMyTradeMessage(const nlohmann::json& data) {
    if (!data.contains("symbol")) return;

    std::string symbol = data["symbol"];
    
    emit(symbol, "mytrade", {
        {"id", data["id"]},
        {"orderId", data["orderId"]},
        {"symbol", symbol},
        {"type", data["type"]},
        {"side", data["side"]},
        {"price", std::stod(data["price"].get<std::string>())},
        {"amount", std::stod(data["amount"].get<std::string>())},
        {"fee", std::stod(data["fee"].get<std::string>())},
        {"feeCurrency", data["feeCurrency"]},
        {"timestamp", data["timestamp"]}
    });
}

void ParadexWS::handlePositionMessage(const nlohmann::json& data) {
    if (!data.contains("symbol")) return;

    std::string symbol = data["symbol"];
    
    emit(symbol, "position", {
        {"symbol", symbol},
        {"size", std::stod(data["size"].get<std::string>())},
        {"entryPrice", std::stod(data["entryPrice"].get<std::string>())},
        {"markPrice", std::stod(data["markPrice"].get<std::string>())},
        {"liquidationPrice", std::stod(data["liquidationPrice"].get<std::string>())},
        {"margin", std::stod(data["margin"].get<std::string>())},
        {"leverage", std::stod(data["leverage"].get<std::string>())},
        {"unrealizedPnl", std::stod(data["unrealizedPnl"].get<std::string>())},
        {"timestamp", data["timestamp"]}
    });
}

void ParadexWS::handleErrorMessage(const nlohmann::json& data) {
    if (data.contains("message")) {
        std::string error_message = data["message"];
        // Handle error appropriately
    }
}

void ParadexWS::handleAuthMessage(const nlohmann::json& data) {
    if (data.contains("authenticated")) {
        authenticated_ = data["authenticated"].get<bool>();
    }
}

void ParadexWS::handleSubscriptionMessage(const nlohmann::json& data) {
    // Handle subscription confirmation
}

void ParadexWS::handleUnsubscriptionMessage(const nlohmann::json& data) {
    // Handle unsubscription confirmation
}

} // namespace ccxt
