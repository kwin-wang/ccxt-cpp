#include "ccxt/exchanges/ws/vertex_ws.h"
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

VertexWS::VertexWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Vertex& exchange)
    : WebSocketClient(ioc, ctx)
    , exchange_(exchange) {
    this->onMessage = [this](const std::string& message) {
        this->handleMessage(message);
    };
    
    // Set the WebSocket endpoint
    this->endpoint = "wss://api.vertex.fi/ws";
}

void VertexWS::subscribeTicker(const std::string& symbol) {
    nlohmann::json request = {
        {"type", "subscribe"},
        {"channel", "ticker"},
        {"market", symbol}
    };
    
    send(request.dump());
}

void VertexWS::subscribeOrderBook(const std::string& symbol) {
    nlohmann::json request = {
        {"type", "subscribe"},
        {"channel", "orderbook"},
        {"market", symbol}
    };
    
    send(request.dump());
}

void VertexWS::subscribeTrades(const std::string& symbol) {
    nlohmann::json request = {
        {"type", "subscribe"},
        {"channel", "trades"},
        {"market", symbol}
    };
    
    send(request.dump());
}

void VertexWS::handleMessage(const std::string& message) {
    try {
        auto j = nlohmann::json::parse(message);
        
        if (!j.contains("type")) {
            return;
        }
        
        std::string type = j["type"];
        
        if (type == "ticker") {
            handleTickerMessage(j);
        } else if (type == "orderbook") {
            handleOrderBookMessage(j);
        } else if (type == "trades") {
            handleTradesMessage(j);
        } else if (type == "error") {
            handleErrorMessage(j);
        }
    } catch (const std::exception& e) {
        // Handle parsing error
    }
}

void VertexWS::handleTickerMessage(const nlohmann::json& data) {
    if (!data.contains("market") || !data.contains("data")) {
        return;
    }
    
    std::string symbol = data["market"];
    auto tickerData = data["data"];
    
    nlohmann::json ticker = {
        {"symbol", symbol},
        {"timestamp", tickerData["timestamp"]},
        {"last", std::stod(tickerData["last"].get<std::string>())},
        {"high", std::stod(tickerData["high"].get<std::string>())},
        {"low", std::stod(tickerData["low"].get<std::string>())},
        {"bid", std::stod(tickerData["bid"].get<std::string>())},
        {"ask", std::stod(tickerData["ask"].get<std::string>())},
        {"baseVolume", std::stod(tickerData["baseVolume"].get<std::string>())},
        {"quoteVolume", std::stod(tickerData["quoteVolume"].get<std::string>())},
        {"percentage", std::stod(tickerData["change24h"].get<std::string>()) * 100}
    };
    
    emit(symbol, "ticker", ticker);
}

void VertexWS::handleOrderBookMessage(const nlohmann::json& data) {
    if (!data.contains("market") || !data.contains("data")) {
        return;
    }
    
    std::string symbol = data["market"];
    auto bookData = data["data"];
    
    std::vector<std::vector<double>> bids;
    std::vector<std::vector<double>> asks;
    
    // Process bids
    for (const auto& bid : bookData["bids"]) {
        if (bid.size() >= 2) {
            bids.push_back({
                std::stod(bid[0].get<std::string>()),  // price
                std::stod(bid[1].get<std::string>())   // amount
            });
        }
    }
    
    // Process asks
    for (const auto& ask : bookData["asks"]) {
        if (ask.size() >= 2) {
            asks.push_back({
                std::stod(ask[0].get<std::string>()),  // price
                std::stod(ask[1].get<std::string>())   // amount
            });
        }
    }
    
    nlohmann::json orderbook = {
        {"symbol", symbol},
        {"timestamp", bookData["timestamp"]},
        {"bids", bids},
        {"asks", asks},
        {"nonce", bookData["nonce"]}
    };
    
    emit(symbol, "orderbook", orderbook);
}

void VertexWS::handleTradesMessage(const nlohmann::json& data) {
    if (!data.contains("market") || !data.contains("data")) {
        return;
    }
    
    std::string symbol = data["market"];
    auto trades = data["data"];
    
    for (const auto& trade : trades) {
        nlohmann::json normalizedTrade = {
            {"id", trade["id"]},
            {"timestamp", trade["timestamp"]},
            {"symbol", symbol},
            {"side", trade["side"]},
            {"price", std::stod(trade["price"].get<std::string>())},
            {"amount", std::stod(trade["amount"].get<std::string>())},
            {"cost", std::stod(trade["cost"].get<std::string>())}
        };
        
        emit(symbol, "trade", normalizedTrade);
    }
}

void VertexWS::handleErrorMessage(const nlohmann::json& data) {
    if (data.contains("message")) {
        std::string errorMessage = data["message"];
        // Handle error appropriately
        // You might want to emit an error event or log the error
    }
}

} // namespace ccxt
