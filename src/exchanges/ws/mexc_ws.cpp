#include "ccxt/exchanges/ws/mexc_ws.h"
#include "ccxt/base/json.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <chrono>
#include <thread>

namespace ccxt {

MexcWS::MexcWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Mexc& exchange)
    : WebSocketClient(ioc, ctx)
    , exchange_(exchange)
    , listenKeyExpiry_(0) {
}

std::string MexcWS::getEndpoint(const std::string& type) {
    if (type == "spot") {
        return "wss://wbs.mexc.com/ws";
    } else if (type == "swap") {
        return "wss://contract.mexc.com/edge";
    }
    return "wss://wbs.mexc.com/ws"; // default to spot
}

void MexcWS::watchTicker(const std::string& symbol, bool miniTicker) {
    auto market = exchange_.market(symbol);
    std::string channel;
    if (miniTicker) {
        channel = "spot@public.miniTicker.v3.api@" + market.id + "@UTC+8";
    } else {
        channel = "spot@public.ticker.v3.api@" + market.id + "@UTC+8";
    }
    subscribePublic(channel, symbol);
}

void MexcWS::watchTickers(const std::vector<std::string>& symbols) {
    for (const auto& symbol : symbols) {
        watchTicker(symbol);
    }
}

void MexcWS::watchOrderBook(const std::string& symbol) {
    auto market = exchange_.market(symbol);
    std::string channel = "spot@public.bookTicker.v3.api@" + market.id + "@UTC+8";
    subscribePublic(channel, symbol);
}

void MexcWS::watchTrades(const std::string& symbol) {
    auto market = exchange_.market(symbol);
    std::string channel = "spot@public.deals.v3.api@" + market.id + "@UTC+8";
    subscribePublic(channel, symbol);
}

void MexcWS::watchOHLCV(const std::string& symbol, const std::string& timeframe) {
    auto market = exchange_.market(symbol);
    std::string interval;
    if (timeframe == "1m") interval = "Min1";
    else if (timeframe == "5m") interval = "Min5";
    else if (timeframe == "15m") interval = "Min15";
    else if (timeframe == "30m") interval = "Min30";
    else if (timeframe == "1h") interval = "Min60";
    else if (timeframe == "4h") interval = "Hour4";
    else if (timeframe == "8h") interval = "Hour8";
    else if (timeframe == "1d") interval = "Day1";
    else if (timeframe == "1w") interval = "Week1";
    else if (timeframe == "1M") interval = "Month1";
    
    std::string channel = "spot@public.kline.v3.api@" + market.id + "@" + interval + "@UTC+8";
    subscribePublic(channel, symbol);
}

void MexcWS::watchBidsAsks(const std::string& symbol) {
    watchOrderBook(symbol);
}

void MexcWS::watchBalance() {
    createListenKey();
    subscribePrivate("spot@private.account.v3.api");
}

void MexcWS::watchOrders() {
    createListenKey();
    subscribePrivate("spot@private.orders.v3.api");
}

void MexcWS::watchMyTrades() {
    createListenKey();
    subscribePrivate("spot@private.deals.v3.api");
}

void MexcWS::subscribe(const std::string& channel, const std::string& symbol, bool isPrivate) {
    std::string endpoint = getEndpoint("spot");
    connect(endpoint);
    
    nlohmann::json request = {
        {"method", "SUBSCRIPTION"},
        {"params", {channel}}
    };
    
    if (isPrivate) {
        authenticate();
    }
    
    send(request.dump());
}

void MexcWS::subscribePublic(const std::string& channel, const std::string& symbol) {
    subscribe(channel, symbol, false);
}

void MexcWS::subscribePrivate(const std::string& channel) {
    subscribe(channel, "", true);
}

void MexcWS::createListenKey() {
    long long now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    if (listenKey_.empty() || now >= listenKeyExpiry_) {
        // Create new listen key via REST API
        // This is a placeholder - actual implementation would use the exchange's REST API
        listenKey_ = "placeholder_listen_key";
        listenKeyExpiry_ = now + 1200000; // 20 minutes
    }
}

void MexcWS::extendListenKey() {
    // Extend listen key validity via REST API
    // This is a placeholder - actual implementation would use the exchange's REST API
    long long now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    listenKeyExpiry_ = now + 1200000; // 20 minutes
}

void MexcWS::authenticate() {
    createListenKey();
    nlohmann::json auth = {
        {"method", "LOGIN"},
        {"params", {listenKey_}}
    };
    send(auth.dump());
}

void MexcWS::ping() {
    nlohmann::json ping = {
        {"method", "PING"}
    };
    send(ping.dump());
}

void MexcWS::handleMessage(const std::string& message) {
    try {
        auto j = nlohmann::json::parse(message);
        
        if (j.contains("c")) {
            std::string channel = j["c"];
            
            if (channel.find("ticker") != std::string::npos || 
                channel.find("miniTicker") != std::string::npos) {
                handleTickerMessage(j);
            } else if (channel.find("bookTicker") != std::string::npos) {
                handleOrderBookMessage(j);
            } else if (channel.find("deals") != std::string::npos) {
                if (channel.find("private") != std::string::npos) {
                    handleMyTradeMessage(j);
                } else {
                    handleTradeMessage(j);
                }
            } else if (channel.find("kline") != std::string::npos) {
                handleOHLCVMessage(j);
            } else if (channel.find("account") != std::string::npos) {
                handleBalanceMessage(j);
            } else if (channel.find("orders") != std::string::npos) {
                handleOrderMessage(j);
            }
        }
        
        if (j.contains("method") && j["method"] == "PONG") {
            // Handle pong response
            return;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling message: " << e.what() << std::endl;
    }
}

void MexcWS::handleTickerMessage(const nlohmann::json& data) {
    if (data.contains("d")) {
        auto& tickerData = data["d"];
        nlohmann::json ticker = {
            {"symbol", tickerData["s"]},
            {"last", tickerData["c"]},
            {"high", tickerData["h"]},
            {"low", tickerData["l"]},
            {"bid", tickerData["b"]},
            {"ask", tickerData["a"]},
            {"baseVolume", tickerData["v"]},
            {"quoteVolume", tickerData["q"]},
            {"timestamp", tickerData["t"]},
            {"info", tickerData}
        };
        
        emit("ticker::" + std::string(tickerData["s"]), ticker);
    }
}

void MexcWS::handleOrderBookMessage(const nlohmann::json& data) {
    if (data.contains("d")) {
        auto& bookData = data["d"];
        nlohmann::json orderbook = {
            {"symbol", bookData["s"]},
            {"bids", bookData["b"]},
            {"asks", bookData["a"]},
            {"timestamp", bookData["t"]},
            {"info", bookData}
        };
        
        emit("orderbook::" + std::string(bookData["s"]), orderbook);
    }
}

void MexcWS::handleTradeMessage(const nlohmann::json& data) {
    if (data.contains("d")) {
        auto& trades = data["d"];
        for (const auto& trade : trades) {
            nlohmann::json parsedTrade = {
                {"id", trade["i"]},
                {"symbol", trade["s"]},
                {"side", trade["S"]},
                {"price", trade["p"]},
                {"amount", trade["v"]},
                {"timestamp", trade["t"]},
                {"info", trade}
            };
            
            emit("trades::" + std::string(trade["s"]), parsedTrade);
        }
    }
}

void MexcWS::handleOHLCVMessage(const nlohmann::json& data) {
    if (data.contains("d")) {
        auto& kline = data["d"];
        nlohmann::json ohlcv = {
            {"timestamp", kline["t"]},
            {"open", kline["o"]},
            {"high", kline["h"]},
            {"low", kline["l"]},
            {"close", kline["c"]},
            {"volume", kline["v"]},
            {"info", kline}
        };
        
        emit("ohlcv::" + std::string(kline["s"]), ohlcv);
    }
}

void MexcWS::handleBalanceMessage(const nlohmann::json& data) {
    if (data.contains("d")) {
        emit("balance", data["d"]);
    }
}

void MexcWS::handleOrderMessage(const nlohmann::json& data) {
    if (data.contains("d")) {
        auto& orderData = data["d"];
        nlohmann::json order = {
            {"id", orderData["i"]},
            {"symbol", orderData["s"]},
            {"type", orderData["T"]},
            {"side", orderData["S"]},
            {"price", orderData["p"]},
            {"amount", orderData["q"]},
            {"filled", orderData["z"]},
            {"remaining", orderData["l"]},
            {"status", orderData["X"]},
            {"timestamp", orderData["t"]},
            {"info", orderData}
        };
        
        emit("orders::" + std::string(orderData["s"]), order);
    }
}

void MexcWS::handleMyTradeMessage(const nlohmann::json& data) {
    if (data.contains("d")) {
        auto& tradeData = data["d"];
        nlohmann::json trade = {
            {"id", tradeData["i"]},
            {"order", tradeData["o"]},
            {"symbol", tradeData["s"]},
            {"side", tradeData["S"]},
            {"price", tradeData["p"]},
            {"amount", tradeData["q"]},
            {"fee", {
                {"cost", tradeData["n"]},
                {"currency", tradeData["N"]}
            }},
            {"timestamp", tradeData["t"]},
            {"info", tradeData}
        };
        
        emit("myTrades::" + std::string(tradeData["s"]), trade);
    }
}

} // namespace ccxt
