#include "ccxt/exchanges/ws/p2b_ws.h"
#include "ccxt/base/json.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <ctime>

namespace ccxt {

P2BWS::P2BWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, P2B& exchange)
    : WebSocketClient(ioc, ctx)
    , exchange_(exchange)
    , authenticated_(false)
    , sequenceNumber_(1) {
    this->onMessage = [this](const std::string& message) {
        this->handleMessage(message);
    };
}

std::string P2BWS::getEndpoint(const std::string& type) {
    return "wss://api.p2b.io/ws/" + type;
}

int64_t P2BWS::getNextSequenceNumber() {
    return sequenceNumber_++;
}

void P2BWS::authenticate() {
    if (!authenticated_ && !exchange_.apiKey.empty()) {
        long timestamp = std::time(nullptr) * 1000;
        std::string signData = exchange_.apiKey + std::to_string(timestamp);
        std::string signature = exchange_.hmac(signData, exchange_.secret, "sha256");

        nlohmann::json auth_message = {
            {"method", "auth"},
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

void P2BWS::ping() {
    nlohmann::json ping_message = {
        {"method", "ping"},
        {"params", {}},
        {"id", getNextSequenceNumber()}
    };
    send(ping_message.dump());
}

void P2BWS::subscribe(const std::string& channel, const std::string& symbol, bool isPrivate) {
    if (isPrivate) {
        authenticate();
    }

    nlohmann::json params;
    if (!symbol.empty()) {
        params["symbol"] = symbol;
    }
    params["channel"] = channel;

    nlohmann::json sub_message = {
        {"method", "subscribe"},
        {"params", params},
        {"id", getNextSequenceNumber()}
    };

    subscriptions_[channel + "_" + symbol] = symbol;
    send(sub_message.dump());
}

void P2BWS::unsubscribe(const std::string& channel, const std::string& symbol) {
    nlohmann::json params;
    if (!symbol.empty()) {
        params["symbol"] = symbol;
    }
    params["channel"] = channel;

    nlohmann::json unsub_message = {
        {"method", "unsubscribe"},
        {"params", params},
        {"id", getNextSequenceNumber()}
    };

    subscriptions_.erase(channel + "_" + symbol);
    send(unsub_message.dump());
}

void P2BWS::watchTicker(const std::string& symbol) {
    subscribe("ticker", symbol);
}

void P2BWS::watchTickers(const std::vector<std::string>& symbols) {
    for (const auto& symbol : symbols) {
        watchTicker(symbol);
    }
}

void P2BWS::watchOrderBook(const std::string& symbol, const int limit) {
    subscribe("orderbook", symbol);
}

void P2BWS::watchTrades(const std::string& symbol) {
    subscribe("trades", symbol);
}

void P2BWS::watchOHLCV(const std::string& symbol, const std::string& timeframe) {
    subscribe("kline_" + timeframe, symbol);
}

void P2BWS::watchBalance() {
    subscribe("balance", "", true);
}

void P2BWS::watchOrders(const std::string& symbol) {
    subscribe("orders", symbol, true);
}

void P2BWS::watchMyTrades(const std::string& symbol) {
    subscribe("mytrades", symbol, true);
}

void P2BWS::handleMessage(const std::string& message) {
    auto j = nlohmann::json::parse(message);

    if (j.contains("method")) {
        std::string method = j["method"];
        
        if (method == "pong") {
            return;
        } else if (method == "auth") {
            handleAuthMessage(j["result"]);
        } else if (method == "error") {
            handleErrorMessage(j["error"]);
        } else if (method == "subscribe") {
            handleSubscriptionMessage(j["result"]);
        } else if (method == "unsubscribe") {
            handleUnsubscriptionMessage(j["result"]);
        }
    } else if (j.contains("channel")) {
        std::string channel = j["channel"];
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
        }
    }
}

void P2BWS::handleTickerMessage(const nlohmann::json& data) {
    if (!data.contains("symbol")) return;

    std::string symbol = data["symbol"];
    
    emit(symbol, "ticker", {
        {"symbol", symbol},
        {"high", std::stod(data["high"].get<std::string>())},
        {"low", std::stod(data["low"].get<std::string>())},
        {"last", std::stod(data["last"].get<std::string>())},
        {"volume", std::stod(data["volume"].get<std::string>())},
        {"quoteVolume", std::stod(data["quoteVolume"].get<std::string>())},
        {"bid", std::stod(data["bid"].get<std::string>())},
        {"ask", std::stod(data["ask"].get<std::string>())},
        {"timestamp", data["timestamp"]}
    });
}

void P2BWS::handleOrderBookMessage(const nlohmann::json& data) {
    if (!data.contains("symbol")) return;

    std::string symbol = data["symbol"];
    
    nlohmann::json orderbook;
    orderbook["symbol"] = symbol;
    orderbook["timestamp"] = data["timestamp"];
    
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

void P2BWS::handleTradeMessage(const nlohmann::json& data) {
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

void P2BWS::handleOHLCVMessage(const nlohmann::json& data) {
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

void P2BWS::handleBalanceMessage(const nlohmann::json& data) {
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

void P2BWS::handleOrderMessage(const nlohmann::json& data) {
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

void P2BWS::handleMyTradeMessage(const nlohmann::json& data) {
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

void P2BWS::handleErrorMessage(const nlohmann::json& data) {
    if (data.contains("message")) {
        std::string error_message = data["message"];
        // Handle error appropriately
    }
}

void P2BWS::handleAuthMessage(const nlohmann::json& data) {
    if (data.contains("authenticated")) {
        authenticated_ = data["authenticated"].get<bool>();
    }
}

void P2BWS::handleSubscriptionMessage(const nlohmann::json& data) {
    // Handle subscription confirmation
}

void P2BWS::handleUnsubscriptionMessage(const nlohmann::json& data) {
    // Handle unsubscription confirmation
}

} // namespace ccxt
