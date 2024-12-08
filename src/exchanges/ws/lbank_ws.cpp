#include "ccxt/exchanges/ws/lbank_ws.h"
#include "ccxt/base/json.hpp"
#include "ccxt/base/exchange.h"
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <ctime>
#include <string>

namespace ccxt {

LBankWS::LBankWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, LBank& exchange)
    : WebSocketClient(ioc, ctx, "wss://www.lbkex.net/ws/V2/")
    , exchange_(exchange)
    , authenticated_(false)
    , tradesLimit_(1000) {
    this->onMessage = [this](const std::string& message) {
        this->handleMessage(message);
    };
}

void LBankWS::authenticate() {
    if (!authenticated_ && !exchange_.apiKey.empty()) {
        long timestamp = std::time(nullptr) * 1000;
        std::string signature = exchange_.hmac(
            std::to_string(timestamp),
            exchange_.secret,
            "sha256"
        );

        nlohmann::json auth_message = {
            {"action", "auth"},
            {"key", exchange_.apiKey},
            {"timestamp", timestamp},
            {"sign", signature}
        };

        send(auth_message.dump());
    }
}

void LBankWS::ping() {
    nlohmann::json ping_message = {
        {"action", "ping"}
    };
    send(ping_message.dump());
}

void LBankWS::subscribe(const std::string& channel, const std::string& symbol) {
    nlohmann::json sub_message = {
        {"action", "subscribe"},
        {"channel", channel},
        {"symbol", symbol}
    };
    subscriptions_[channel + "_" + symbol] = symbol;
    send(sub_message.dump());
}

void LBankWS::unsubscribe(const std::string& channel, const std::string& symbol) {
    nlohmann::json unsub_message = {
        {"action", "unsubscribe"},
        {"channel", channel},
        {"symbol", symbol}
    };
    subscriptions_.erase(channel + "_" + symbol);
    send(unsub_message.dump());
}

void LBankWS::watchTicker(const std::string& symbol) {
    subscribe("ticker", symbol);
}

void LBankWS::watchOrderBook(const std::string& symbol, const std::string& limit) {
    subscribe("depth", symbol);
}

void LBankWS::watchTrades(const std::string& symbol) {
    subscribe("trade", symbol);
}

void LBankWS::watchOHLCV(const std::string& symbol, const std::string& timeframe) {
    subscribe("kline_" + timeframe, symbol);
}

void LBankWS::watchBalance() {
    authenticate();
    subscribe("user_account", "");
}

void LBankWS::watchOrders() {
    authenticate();
    subscribe("user_order", "");
}

void LBankWS::watchMyTrades() {
    authenticate();
    subscribe("user_trade", "");
}

void LBankWS::handleMessage(const std::string& message) {
    auto j = nlohmann::json::parse(message);

    if (j.contains("action")) {
        std::string action = j["action"];
        
        if (action == "pong") {
            return;
        }
        
        if (action == "auth") {
            authenticated_ = j["result"].get<bool>();
            return;
        }
    }

    if (j.contains("type")) {
        std::string type = j["type"];
        
        if (type == "ticker") {
            handleTicker(j);
        } else if (type == "depth") {
            handleOrderBook(j);
        } else if (type == "trade") {
            handleTrade(j);
        } else if (type.find("kline_") == 0) {
            handleOHLCV(j);
        } else if (type == "user_account") {
            handleBalance(j);
        } else if (type == "user_order") {
            handleOrder(j);
        } else if (type == "user_trade") {
            handleMyTrade(j);
        }
    }
}

void LBankWS::handleTicker(const nlohmann::json& data) {
    if (!data.contains("data")) return;

    auto ticker_data = data["data"];
    std::string symbol = ticker_data["symbol"];
    
    emit(symbol, "ticker", {
        {"symbol", symbol},
        {"high", std::stod(ticker_data["high"].get<std::string>())},
        {"low", std::stod(ticker_data["low"].get<std::string>())},
        {"last", std::stod(ticker_data["close"].get<std::string>())},
        {"vol", std::stod(ticker_data["vol"].get<std::string>())},
        {"change", std::stod(ticker_data["change"].get<std::string>())},
        {"timestamp", ticker_data["timestamp"]}
    });
}

void LBankWS::handleOrderBook(const nlohmann::json& data) {
    if (!data.contains("data")) return;

    auto ob_data = data["data"];
    std::string symbol = ob_data["symbol"];
    
    nlohmann::json orderbook;
    orderbook["symbol"] = symbol;
    orderbook["timestamp"] = ob_data["timestamp"];
    
    std::vector<std::vector<double>> bids;
    std::vector<std::vector<double>> asks;
    
    for (const auto& bid : ob_data["bids"]) {
        bids.push_back({std::stod(bid[0].get<std::string>()), std::stod(bid[1].get<std::string>())});
    }
    
    for (const auto& ask : ob_data["asks"]) {
        asks.push_back({std::stod(ask[0].get<std::string>()), std::stod(ask[1].get<std::string>())});
    }
    
    orderbook["bids"] = bids;
    orderbook["asks"] = asks;
    
    emit(symbol, "orderbook", orderbook);
}

void LBankWS::handleTrade(const nlohmann::json& data) {
    if (!data.contains("data")) return;

    auto trade_data = data["data"];
    std::string symbol = trade_data["symbol"];
    
    nlohmann::json trade = {
        {"symbol", symbol},
        {"id", trade_data["id"]},
        {"price", std::stod(trade_data["price"].get<std::string>())},
        {"amount", std::stod(trade_data["amount"].get<std::string>())},
        {"side", trade_data["direction"]},
        {"timestamp", trade_data["timestamp"]}
    };
    
    emit(symbol, "trade", trade);
}

void LBankWS::handleOHLCV(const nlohmann::json& data) {
    if (!data.contains("data")) return;

    auto kline_data = data["data"];
    std::string symbol = kline_data["symbol"];
    
    nlohmann::json ohlcv = {
        {"timestamp", kline_data["timestamp"]},
        {"open", std::stod(kline_data["open"].get<std::string>())},
        {"high", std::stod(kline_data["high"].get<std::string>())},
        {"low", std::stod(kline_data["low"].get<std::string>())},
        {"close", std::stod(kline_data["close"].get<std::string>())},
        {"volume", std::stod(kline_data["volume"].get<std::string>())}
    };
    
    emit(symbol, "ohlcv", ohlcv);
}

void LBankWS::handleBalance(const nlohmann::json& data) {
    if (!data.contains("data")) return;

    auto balance_data = data["data"];
    nlohmann::json balance;
    
    for (const auto& asset : balance_data.items()) {
        balance[asset.key()] = {
            {"free", std::stod(asset.value()["available"].get<std::string>())},
            {"used", std::stod(asset.value()["frozen"].get<std::string>())},
            {"total", std::stod(asset.value()["total"].get<std::string>())}
        };
    }
    
    emit("", "balance", balance);
}

void LBankWS::handleOrder(const nlohmann::json& data) {
    if (!data.contains("data")) return;

    auto order_data = data["data"];
    std::string symbol = order_data["symbol"];
    
    nlohmann::json order = {
        {"id", order_data["orderId"]},
        {"symbol", symbol},
        {"type", order_data["type"]},
        {"side", order_data["side"]},
        {"price", std::stod(order_data["price"].get<std::string>())},
        {"amount", std::stod(order_data["amount"].get<std::string>())},
        {"filled", std::stod(order_data["filledAmount"].get<std::string>())},
        {"status", order_data["status"]},
        {"timestamp", order_data["timestamp"]}
    };
    
    emit(symbol, "order", order);
}

void LBankWS::handleMyTrade(const nlohmann::json& data) {
    if (!data.contains("data")) return;

    auto trade_data = data["data"];
    std::string symbol = trade_data["symbol"];
    
    nlohmann::json trade = {
        {"id", trade_data["tradeId"]},
        {"order", trade_data["orderId"]},
        {"symbol", symbol},
        {"type", trade_data["type"]},
        {"side", trade_data["side"]},
        {"price", std::stod(trade_data["price"].get<std::string>())},
        {"amount", std::stod(trade_data["amount"].get<std::string>())},
        {"fee", std::stod(trade_data["fee"].get<std::string>())},
        {"feeCurrency", trade_data["feeCurrency"]},
        {"timestamp", trade_data["timestamp"]}
    };
    
    emit(symbol, "mytrade", trade);
}

} // namespace ccxt
