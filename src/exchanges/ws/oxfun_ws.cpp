#include "ccxt/exchanges/ws/oxfun_ws.h"
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

OxFunWS::OxFunWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, OxFun& exchange)
    : WebSocketClient(ioc, ctx)
    , exchange_(exchange)
    , authenticated_(false)
    , sequenceNumber_(1) {
    this->onMessage = [this](const std::string& message) {
        this->handleMessage(message);
    };
}

std::string OxFunWS::getEndpoint(const std::string& type) {
    return "wss://ws.oxfun.com/" + type;
}

int64_t OxFunWS::getNextSequenceNumber() {
    return sequenceNumber_++;
}

void OxFunWS::authenticate() {
    if (!authenticated_ && !exchange_.apiKey.empty()) {
        long timestamp = std::time(nullptr) * 1000;
        std::string signData = std::to_string(timestamp) + "GET/auth";
        std::string signature = exchange_.hmac(signData, exchange_.secret, "sha256");

        nlohmann::json auth_message = {
            {"op", "auth"},
            {"args", {
                {"apiKey", exchange_.apiKey},
                {"timestamp", timestamp},
                {"sign", signature}
            }}
        };

        send(auth_message.dump());
    }
}

void OxFunWS::ping() {
    nlohmann::json ping_message = {
        {"op", "ping"},
        {"ts", std::time(nullptr) * 1000}
    };
    send(ping_message.dump());
}

void OxFunWS::subscribe(const std::string& channel, const std::string& symbol, bool isPrivate) {
    if (isPrivate) {
        authenticate();
    }

    nlohmann::json sub_message = {
        {"op", "subscribe"},
        {"args", {
            {"channel", channel}
        }}
    };

    if (!symbol.empty()) {
        sub_message["args"]["symbol"] = symbol;
    }

    subscriptions_[channel + "_" + symbol] = symbol;
    send(sub_message.dump());
}

void OxFunWS::unsubscribe(const std::string& channel, const std::string& symbol) {
    nlohmann::json unsub_message = {
        {"op", "unsubscribe"},
        {"args", {
            {"channel", channel}
        }}
    };

    if (!symbol.empty()) {
        unsub_message["args"]["symbol"] = symbol;
    }

    subscriptions_.erase(channel + "_" + symbol);
    send(unsub_message.dump());
}

void OxFunWS::watchTicker(const std::string& symbol) {
    subscribe("ticker", symbol);
}

void OxFunWS::watchTickers(const std::vector<std::string>& symbols) {
    for (const auto& symbol : symbols) {
        watchTicker(symbol);
    }
}

void OxFunWS::watchOrderBook(const std::string& symbol, const int limit) {
    subscribe("orderbook", symbol);
}

void OxFunWS::watchTrades(const std::string& symbol) {
    subscribe("trades", symbol);
}

void OxFunWS::watchOHLCV(const std::string& symbol, const std::string& timeframe) {
    subscribe("kline_" + timeframe, symbol);
}

void OxFunWS::watchBalance() {
    subscribe("balance", "", true);
}

void OxFunWS::watchOrders(const std::string& symbol) {
    subscribe("orders", symbol, true);
}

void OxFunWS::watchMyTrades(const std::string& symbol) {
    subscribe("mytrades", symbol, true);
}

void OxFunWS::handleMessage(const std::string& message) {
    auto j = nlohmann::json::parse(message);

    if (!j.contains("event")) return;

    std::string event = j["event"];
    
    if (event == "pong") {
        return;
    } else if (event == "auth") {
        handleAuthMessage(j["data"]);
    } else if (event == "error") {
        handleErrorMessage(j["data"]);
    } else if (event == "subscribe") {
        handleSubscriptionMessage(j["data"]);
    } else if (event == "unsubscribe") {
        handleUnsubscriptionMessage(j["data"]);
    } else if (event == "update") {
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
        }
    }
}

void OxFunWS::handleTickerMessage(const nlohmann::json& data) {
    if (!data.contains("symbol")) return;

    std::string symbol = data["symbol"];
    
    emit(symbol, "ticker", {
        {"symbol", symbol},
        {"high", std::stod(data["high"].get<std::string>())},
        {"low", std::stod(data["low"].get<std::string>())},
        {"bid", std::stod(data["bid"].get<std::string>())},
        {"ask", std::stod(data["ask"].get<std::string>())},
        {"last", std::stod(data["last"].get<std::string>())},
        {"volume", std::stod(data["volume"].get<std::string>())},
        {"timestamp", data["timestamp"]}
    });
}

void OxFunWS::handleOrderBookMessage(const nlohmann::json& data) {
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

void OxFunWS::handleTradeMessage(const nlohmann::json& data) {
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

void OxFunWS::handleOHLCVMessage(const nlohmann::json& data) {
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

void OxFunWS::handleBalanceMessage(const nlohmann::json& data) {
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

void OxFunWS::handleOrderMessage(const nlohmann::json& data) {
    if (!data.contains("symbol")) return;

    std::string symbol = data["symbol"];
    
    emit(symbol, "order", {
        {"id", data["id"]},
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

void OxFunWS::handleMyTradeMessage(const nlohmann::json& data) {
    if (!data.contains("symbol")) return;

    std::string symbol = data["symbol"];
    
    emit(symbol, "mytrade", {
        {"id", data["id"]},
        {"order", data["orderId"]},
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

void OxFunWS::handleErrorMessage(const nlohmann::json& data) {
    if (data.contains("message")) {
        std::string error_message = data["message"];
        // Handle error appropriately
    }
}

void OxFunWS::handleAuthMessage(const nlohmann::json& data) {
    if (data.contains("authenticated")) {
        authenticated_ = data["authenticated"].get<bool>();
    }
}

void OxFunWS::handleSubscriptionMessage(const nlohmann::json& data) {
    // Handle subscription confirmation
}

void OxFunWS::handleUnsubscriptionMessage(const nlohmann::json& data) {
    // Handle unsubscription confirmation
}

} // namespace ccxt
