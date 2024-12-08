#include "ccxt/exchanges/ws/probit_ws.h"
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

ProbitWS::ProbitWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Probit& exchange)
    : WebSocketClient(ioc, ctx)
    , exchange_(exchange)
    , authenticated_(false)
    , sequenceNumber_(1) {
    this->onMessage = [this](const std::string& message) {
        this->handleMessage(message);
    };
}

std::string ProbitWS::getEndpoint(const std::string& type) {
    return "wss://api.probit.com/api/ws/" + type;
}

int64_t ProbitWS::getNextSequenceNumber() {
    return sequenceNumber_++;
}

std::string ProbitWS::generateSignature(const std::string& timestamp, const std::string& method,
                                      const std::string& path) {
    std::string message = timestamp + method + path;
    return exchange_.hmac(message, exchange_.secret, "sha256");
}

void ProbitWS::authenticate() {
    if (!authenticated_ && !exchange_.apiKey.empty()) {
        std::stringstream ss;
        ss << std::time(nullptr) * 1000;
        std::string timestamp = ss.str();
        std::string method = "GET";
        std::string path = "/api/ws/auth";
        
        std::string signature = generateSignature(timestamp, method, path);

        nlohmann::json auth_message = {
            {"type", "auth"},
            {"data", {
                {"api_key", exchange_.apiKey},
                {"timestamp", timestamp},
                {"signature", signature}
            }}
        };

        send(auth_message.dump());
    }
}

void ProbitWS::ping() {
    nlohmann::json ping_message = {
        {"type", "ping"},
        {"timestamp", std::time(nullptr) * 1000}
    };
    send(ping_message.dump());
}

void ProbitWS::subscribe(const std::string& channel, const std::string& symbol, bool isPrivate) {
    if (isPrivate) {
        authenticate();
    }

    nlohmann::json sub_data;
    if (!symbol.empty()) {
        sub_data["market_id"] = symbol;
    }
    sub_data["channel"] = channel;

    nlohmann::json sub_message = {
        {"type", "subscribe"},
        {"data", sub_data}
    };

    subscriptions_[channel + "_" + symbol] = symbol;
    send(sub_message.dump());
}

void ProbitWS::unsubscribe(const std::string& channel, const std::string& symbol) {
    nlohmann::json unsub_data;
    if (!symbol.empty()) {
        unsub_data["market_id"] = symbol;
    }
    unsub_data["channel"] = channel;

    nlohmann::json unsub_message = {
        {"type", "unsubscribe"},
        {"data", unsub_data}
    };

    subscriptions_.erase(channel + "_" + symbol);
    send(unsub_message.dump());
}

void ProbitWS::watchTicker(const std::string& symbol) {
    subscribe("ticker", symbol);
}

void ProbitWS::watchTickers(const std::vector<std::string>& symbols) {
    for (const auto& symbol : symbols) {
        watchTicker(symbol);
    }
}

void ProbitWS::watchOrderBook(const std::string& symbol, const int limit) {
    subscribe("order_book", symbol);
}

void ProbitWS::watchTrades(const std::string& symbol) {
    subscribe("trade", symbol);
}

void ProbitWS::watchOHLCV(const std::string& symbol, const std::string& timeframe) {
    subscribe("candle_" + timeframe, symbol);
}

void ProbitWS::watchBalance() {
    subscribe("balance", "", true);
}

void ProbitWS::watchOrders(const std::string& symbol) {
    subscribe("order", symbol, true);
}

void ProbitWS::watchMyTrades(const std::string& symbol) {
    subscribe("trade_history", symbol, true);
}

void ProbitWS::handleMessage(const std::string& message) {
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
    } else {
        if (!j.contains("data")) return;

        auto data = j["data"];
        
        if (type == "ticker") {
            handleTickerMessage(data);
        } else if (type == "order_book") {
            handleOrderBookMessage(data);
        } else if (type == "trade") {
            handleTradeMessage(data);
        } else if (type.find("candle_") == 0) {
            handleOHLCVMessage(data);
        } else if (type == "balance") {
            handleBalanceMessage(data);
        } else if (type == "order") {
            handleOrderMessage(data);
        } else if (type == "trade_history") {
            handleMyTradeMessage(data);
        }
    }
}

void ProbitWS::handleTickerMessage(const nlohmann::json& data) {
    if (!data.contains("market_id")) return;

    std::string symbol = data["market_id"];
    
    emit(symbol, "ticker", {
        {"symbol", symbol},
        {"high", std::stod(data["high"].get<std::string>())},
        {"low", std::stod(data["low"].get<std::string>())},
        {"last", std::stod(data["last"].get<std::string>())},
        {"bid", std::stod(data["bid"].get<std::string>())},
        {"ask", std::stod(data["ask"].get<std::string>())},
        {"baseVolume", std::stod(data["base_volume"].get<std::string>())},
        {"quoteVolume", std::stod(data["quote_volume"].get<std::string>())},
        {"percentage", std::stod(data["change"].get<std::string>())},
        {"timestamp", data["timestamp"]}
    });
}

void ProbitWS::handleOrderBookMessage(const nlohmann::json& data) {
    if (!data.contains("market_id")) return;

    std::string symbol = data["market_id"];
    
    nlohmann::json orderbook;
    orderbook["symbol"] = symbol;
    orderbook["timestamp"] = data["timestamp"];
    
    std::vector<std::vector<double>> bids;
    std::vector<std::vector<double>> asks;
    
    for (const auto& bid : data["bids"]) {
        bids.push_back({std::stod(bid["price"].get<std::string>()), std::stod(bid["quantity"].get<std::string>())});
    }
    
    for (const auto& ask : data["asks"]) {
        asks.push_back({std::stod(ask["price"].get<std::string>()), std::stod(ask["quantity"].get<std::string>())});
    }
    
    orderbook["bids"] = bids;
    orderbook["asks"] = asks;
    
    emit(symbol, "orderbook", orderbook);
}

void ProbitWS::handleTradeMessage(const nlohmann::json& data) {
    if (!data.contains("market_id")) return;

    std::string symbol = data["market_id"];
    
    emit(symbol, "trade", {
        {"id", data["id"]},
        {"symbol", symbol},
        {"price", std::stod(data["price"].get<std::string>())},
        {"amount", std::stod(data["quantity"].get<std::string>())},
        {"side", data["side"]},
        {"timestamp", data["timestamp"]}
    });
}

void ProbitWS::handleOHLCVMessage(const nlohmann::json& data) {
    if (!data.contains("market_id")) return;

    std::string symbol = data["market_id"];
    
    emit(symbol, "ohlcv", {
        {"timestamp", data["timestamp"]},
        {"open", std::stod(data["open"].get<std::string>())},
        {"high", std::stod(data["high"].get<std::string>())},
        {"low", std::stod(data["low"].get<std::string>())},
        {"close", std::stod(data["close"].get<std::string>())},
        {"volume", std::stod(data["volume"].get<std::string>())}
    });
}

void ProbitWS::handleBalanceMessage(const nlohmann::json& data) {
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

void ProbitWS::handleOrderMessage(const nlohmann::json& data) {
    if (!data.contains("market_id")) return;

    std::string symbol = data["market_id"];
    
    emit(symbol, "order", {
        {"id", data["id"]},
        {"clientOrderId", data["client_order_id"]},
        {"symbol", symbol},
        {"type", data["type"]},
        {"side", data["side"]},
        {"price", std::stod(data["price"].get<std::string>())},
        {"amount", std::stod(data["quantity"].get<std::string>())},
        {"filled", std::stod(data["filled_quantity"].get<std::string>())},
        {"remaining", std::stod(data["remaining_quantity"].get<std::string>())},
        {"status", data["status"]},
        {"timestamp", data["timestamp"]}
    });
}

void ProbitWS::handleMyTradeMessage(const nlohmann::json& data) {
    if (!data.contains("market_id")) return;

    std::string symbol = data["market_id"];
    
    emit(symbol, "mytrade", {
        {"id", data["id"]},
        {"orderId", data["order_id"]},
        {"symbol", symbol},
        {"type", data["type"]},
        {"side", data["side"]},
        {"price", std::stod(data["price"].get<std::string>())},
        {"amount", std::stod(data["quantity"].get<std::string>())},
        {"fee", std::stod(data["fee"].get<std::string>())},
        {"feeCurrency", data["fee_currency_id"]},
        {"timestamp", data["timestamp"]}
    });
}

void ProbitWS::handleErrorMessage(const nlohmann::json& data) {
    if (data.contains("message")) {
        std::string error_message = data["message"];
        // Handle error appropriately
    }
}

void ProbitWS::handleAuthMessage(const nlohmann::json& data) {
    if (data.contains("authenticated")) {
        authenticated_ = data["authenticated"].get<bool>();
    }
}

void ProbitWS::handleSubscriptionMessage(const nlohmann::json& data) {
    // Handle subscription confirmation
}

void ProbitWS::handleUnsubscriptionMessage(const nlohmann::json& data) {
    // Handle unsubscription confirmation
}

} // namespace ccxt
