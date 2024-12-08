#include "ccxt/exchanges/ws/mixcoin_ws.h"
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

MixcoinWS::MixcoinWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Mixcoin& exchange)
    : WebSocketClient(ioc, ctx)
    , exchange_(exchange)
    , authenticated_(false) {
    this->onMessage = [this](const std::string& message) {
        this->handleMessage(message);
    };
}

std::string MixcoinWS::getEndpoint(const std::string& type) {
    return "wss://www.mixcoin.com/ws";
}

void MixcoinWS::authenticate() {
    if (!authenticated_ && !exchange_.apiKey.empty()) {
        long timestamp = std::time(nullptr) * 1000;
        std::string signData = std::to_string(timestamp) + exchange_.apiKey;
        std::string signature = exchange_.hmac(signData, exchange_.secret, "sha256");

        nlohmann::json auth_message = {
            {"op", "auth"},
            {"args", {
                {"apiKey", exchange_.apiKey},
                {"timestamp", timestamp},
                {"signature", signature}
            }}
        };

        send(auth_message.dump());
    }
}

void MixcoinWS::ping() {
    nlohmann::json ping_message = {
        {"op", "ping"}
    };
    send(ping_message.dump());
}

void MixcoinWS::subscribe(const std::string& channel, const std::string& symbol, bool isPrivate) {
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

void MixcoinWS::unsubscribe(const std::string& channel, const std::string& symbol) {
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

void MixcoinWS::watchTicker(const std::string& symbol) {
    subscribe("ticker", symbol);
}

void MixcoinWS::watchTickers(const std::vector<std::string>& symbols) {
    for (const auto& symbol : symbols) {
        watchTicker(symbol);
    }
}

void MixcoinWS::watchOrderBook(const std::string& symbol, const int limit) {
    subscribe("orderbook", symbol);
}

void MixcoinWS::watchTrades(const std::string& symbol) {
    subscribe("trades", symbol);
}

void MixcoinWS::watchOHLCV(const std::string& symbol, const std::string& timeframe) {
    subscribe("kline_" + timeframe, symbol);
}

void MixcoinWS::watchBalance() {
    subscribe("balance", "", true);
}

void MixcoinWS::watchOrders(const std::string& symbol) {
    subscribe("orders", symbol, true);
}

void MixcoinWS::watchMyTrades(const std::string& symbol) {
    subscribe("mytrades", symbol, true);
}

void MixcoinWS::handleMessage(const std::string& message) {
    auto j = nlohmann::json::parse(message);

    if (j.contains("event")) {
        std::string event = j["event"];
        
        if (event == "pong") {
            return;
        } else if (event == "auth") {
            handleAuthMessage(j);
        } else if (event == "error") {
            handleErrorMessage(j);
        }
    }

    if (j.contains("channel")) {
        std::string channel = j["channel"];
        
        if (channel == "ticker") {
            handleTickerMessage(j);
        } else if (channel == "orderbook") {
            handleOrderBookMessage(j);
        } else if (channel == "trades") {
            handleTradeMessage(j);
        } else if (channel.find("kline_") == 0) {
            handleOHLCVMessage(j);
        } else if (channel == "balance") {
            handleBalanceMessage(j);
        } else if (channel == "orders") {
            handleOrderMessage(j);
        } else if (channel == "mytrades") {
            handleMyTradeMessage(j);
        }
    }
}

void MixcoinWS::handleTickerMessage(const nlohmann::json& data) {
    if (!data.contains("data")) return;

    auto ticker = data["data"];
    std::string symbol = ticker["symbol"];
    
    emit(symbol, "ticker", {
        {"symbol", symbol},
        {"high", std::stod(ticker["high"].get<std::string>())},
        {"low", std::stod(ticker["low"].get<std::string>())},
        {"bid", std::stod(ticker["bid"].get<std::string>())},
        {"ask", std::stod(ticker["ask"].get<std::string>())},
        {"last", std::stod(ticker["last"].get<std::string>())},
        {"volume", std::stod(ticker["volume"].get<std::string>())},
        {"timestamp", ticker["timestamp"]}
    });
}

void MixcoinWS::handleOrderBookMessage(const nlohmann::json& data) {
    if (!data.contains("data")) return;

    auto ob = data["data"];
    std::string symbol = ob["symbol"];
    
    nlohmann::json orderbook;
    orderbook["symbol"] = symbol;
    orderbook["timestamp"] = ob["timestamp"];
    
    std::vector<std::vector<double>> bids;
    std::vector<std::vector<double>> asks;
    
    for (const auto& bid : ob["bids"]) {
        bids.push_back({std::stod(bid[0].get<std::string>()), std::stod(bid[1].get<std::string>())});
    }
    
    for (const auto& ask : ob["asks"]) {
        asks.push_back({std::stod(ask[0].get<std::string>()), std::stod(ask[1].get<std::string>())});
    }
    
    orderbook["bids"] = bids;
    orderbook["asks"] = asks;
    
    emit(symbol, "orderbook", orderbook);
}

void MixcoinWS::handleTradeMessage(const nlohmann::json& data) {
    if (!data.contains("data")) return;

    auto trade = data["data"];
    std::string symbol = trade["symbol"];
    
    emit(symbol, "trade", {
        {"id", trade["id"]},
        {"symbol", symbol},
        {"price", std::stod(trade["price"].get<std::string>())},
        {"amount", std::stod(trade["amount"].get<std::string>())},
        {"side", trade["side"]},
        {"timestamp", trade["timestamp"]}
    });
}

void MixcoinWS::handleOHLCVMessage(const nlohmann::json& data) {
    if (!data.contains("data")) return;

    auto kline = data["data"];
    std::string symbol = kline["symbol"];
    
    emit(symbol, "ohlcv", {
        {"timestamp", kline["timestamp"]},
        {"open", std::stod(kline["open"].get<std::string>())},
        {"high", std::stod(kline["high"].get<std::string>())},
        {"low", std::stod(kline["low"].get<std::string>())},
        {"close", std::stod(kline["close"].get<std::string>())},
        {"volume", std::stod(kline["volume"].get<std::string>())}
    });
}

void MixcoinWS::handleBalanceMessage(const nlohmann::json& data) {
    if (!data.contains("data")) return;

    auto balances = data["data"];
    nlohmann::json balance;
    
    for (const auto& asset : balances.items()) {
        balance[asset.key()] = {
            {"free", std::stod(asset.value()["available"].get<std::string>())},
            {"used", std::stod(asset.value()["frozen"].get<std::string>())},
            {"total", std::stod(asset.value()["total"].get<std::string>())}
        };
    }
    
    emit("", "balance", balance);
}

void MixcoinWS::handleOrderMessage(const nlohmann::json& data) {
    if (!data.contains("data")) return;

    auto order = data["data"];
    std::string symbol = order["symbol"];
    
    emit(symbol, "order", {
        {"id", order["id"]},
        {"symbol", symbol},
        {"type", order["type"]},
        {"side", order["side"]},
        {"price", std::stod(order["price"].get<std::string>())},
        {"amount", std::stod(order["amount"].get<std::string>())},
        {"filled", std::stod(order["filled"].get<std::string>())},
        {"remaining", std::stod(order["remaining"].get<std::string>())},
        {"status", order["status"]},
        {"timestamp", order["timestamp"]}
    });
}

void MixcoinWS::handleMyTradeMessage(const nlohmann::json& data) {
    if (!data.contains("data")) return;

    auto trade = data["data"];
    std::string symbol = trade["symbol"];
    
    emit(symbol, "mytrade", {
        {"id", trade["id"]},
        {"order", trade["orderId"]},
        {"symbol", symbol},
        {"type", trade["type"]},
        {"side", trade["side"]},
        {"price", std::stod(trade["price"].get<std::string>())},
        {"amount", std::stod(trade["amount"].get<std::string>())},
        {"fee", std::stod(trade["fee"].get<std::string>())},
        {"feeCurrency", trade["feeCurrency"]},
        {"timestamp", trade["timestamp"]}
    });
}

void MixcoinWS::handleErrorMessage(const nlohmann::json& data) {
    if (data.contains("message")) {
        std::string error_message = data["message"];
        // Handle error appropriately
    }
}

void MixcoinWS::handleAuthMessage(const nlohmann::json& data) {
    if (data.contains("success")) {
        authenticated_ = data["success"].get<bool>();
    }
}

} // namespace ccxt
