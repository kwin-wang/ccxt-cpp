#include "../../../include/ccxt/exchanges/ws/kucoin_ws.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>

namespace ccxt {

KucoinWS::KucoinWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Kucoin& exchange)
    : WebSocketClient(ioc, ctx), exchange_(exchange), tradesLimit_(1000),
      snapshotDelay_(5), snapshotMaxRetries_(3) {
    // Initialize connection settings specific to KuCoin
}

void KucoinWS::negotiate(bool privateChannel) {
    try {
        connectId_ = privateChannel ? "private" : "public";
        nlohmann::json response;

        if (privateChannel) {
            response = exchange_.privatePost("bullet-private");
        } else {
            response = exchange_.publicPost("bullet-public");
        }

        if (response.contains("data")) {
            auto data = response["data"];
            if (data.contains("token")) {
                token_ = data["token"];
            }
            if (data.contains("instanceServers") && data["instanceServers"].is_array()) {
                auto server = data["instanceServers"][0];
                if (server.contains("pingInterval")) {
                    pingInterval_ = server["pingInterval"];
                }
                if (server.contains("pingTimeout")) {
                    pingTimeout_ = server["pingTimeout"];
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in negotiate: " << e.what() << std::endl;
    }
}

void KucoinWS::ping() {
    nlohmann::json pingMessage = {
        {"id", std::to_string(std::chrono::system_clock::now().time_since_epoch().count())},
        {"type", "ping"}
    };
    send(pingMessage.dump());
}

void KucoinWS::authenticate() {
    negotiate(true);
    nlohmann::json request = {
        {"id", std::to_string(std::chrono::system_clock::now().time_since_epoch().count())},
        {"type", "subscribe"},
        {"topic", "/spotMarket/tradeOrders"},  // Example private channel
        {"privateChannel", true},
        {"response", true}
    };
    send(request.dump());
}

void KucoinWS::subscribe(const std::string& topic, const nlohmann::json& params) {
    nlohmann::json request = {
        {"id", std::to_string(std::chrono::system_clock::now().time_since_epoch().count())},
        {"type", "subscribe"},
        {"topic", topic},
        {"response", true}
    };

    if (!params.empty()) {
        request["privateChannel"] = params.value("privateChannel", false);
    }

    subscriptions_[topic] = request.dump();
    send(request.dump());
}

void KucoinWS::unsubscribe(const std::string& topic, const nlohmann::json& params) {
    nlohmann::json request = {
        {"id", std::to_string(std::chrono::system_clock::now().time_since_epoch().count())},
        {"type", "unsubscribe"},
        {"topic", topic},
        {"response", true}
    };

    if (!params.empty()) {
        request["privateChannel"] = params.value("privateChannel", false);
    }

    subscriptions_.erase(topic);
    send(request.dump());
}

void KucoinWS::watchTicker(const std::string& symbol) {
    std::string topic = "/market/ticker:" + symbol;
    subscribe(topic);
}

void KucoinWS::watchTickers(const std::vector<std::string>& symbols) {
    for (const auto& symbol : symbols) {
        watchTicker(symbol);
    }
}

void KucoinWS::watchOrderBook(const std::string& symbol) {
    std::string topic = "/market/level2:" + symbol;
    subscribe(topic);
}

void KucoinWS::watchTrades(const std::string& symbol) {
    std::string topic = "/market/match:" + symbol;
    subscribe(topic);
}

void KucoinWS::watchOHLCV(const std::string& symbol, const std::string& timeframe) {
    std::string topic = "/market/candles:" + symbol + "_" + timeframe;
    subscribe(topic);
}

void KucoinWS::watchBidsAsks(const std::string& symbol) {
    watchOrderBook(symbol);
}

void KucoinWS::watchBalance() {
    authenticate();
    subscribe("/account/balance", {{"privateChannel", true}});
}

void KucoinWS::watchOrders() {
    authenticate();
    subscribe("/spotMarket/tradeOrders", {{"privateChannel", true}});
}

void KucoinWS::watchMyTrades() {
    authenticate();
    subscribe("/spot/matches", {{"privateChannel", true}});
}

void KucoinWS::handleMessage(const std::string& message) {
    try {
        auto j = nlohmann::json::parse(message);
        
        // Handle ping/pong messages
        if (j["type"] == "ping") {
            nlohmann::json pongMessage = {
                {"id", j["id"]},
                {"type", "pong"}
            };
            send(pongMessage.dump());
            return;
        }
        
        // Handle welcome message
        if (j["type"] == "welcome") {
            // Start ping interval
            std::thread([this]() {
                while (true) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(pingInterval_));
                    ping();
                }
            }).detach();
            return;
        }
        
        // Handle subscription acknowledgments
        if (j["type"] == "ack") {
            emit("subscribed", j);
            return;
        }
        
        // Handle error messages
        if (j["type"] == "error") {
            emit("error", j);
            return;
        }
        
        // Handle data messages
        if (j.contains("data") && j.contains("topic")) {
            std::string topic = j["topic"];
            const auto& data = j["data"];
            
            if (topic.find("/market/ticker") == 0) {
                handleTicker(data);
            } else if (topic.find("/market/level2") == 0) {
                handleOrderBook(data);
            } else if (topic.find("/market/match") == 0) {
                handleTrade(data);
            } else if (topic.find("/market/candles") == 0) {
                handleOHLCV(data);
            } else if (topic == "/account/balance") {
                handleBalance(data);
            } else if (topic == "/spotMarket/tradeOrders") {
                handleOrder(data);
            } else if (topic == "/spot/matches") {
                handleMyTrade(data);
            }
        }
    } catch (const std::exception& e) {
        emit("error", {{"message", e.what()}});
    }
}

void KucoinWS::handleTicker(const nlohmann::json& data) {
    nlohmann::json parsedTicker = {
        {"symbol", data["symbol"]},
        {"timestamp", std::stoll(data["time"])},
        {"datetime", exchange_.iso8601(std::stoll(data["time"]))},
        {"high", std::stod(data["high24h"])},
        {"low", std::stod(data["low24h"])},
        {"bid", std::stod(data["bestBid"])},
        {"bidVolume", std::stod(data["bestBidSize"])},
        {"ask", std::stod(data["bestAsk"])},
        {"askVolume", std::stod(data["bestAskSize"])},
        {"vwap", nullptr},  // Not provided by KuCoin
        {"open", std::stod(data["open24h"])},
        {"close", std::stod(data["price"])},
        {"last", std::stod(data["price"])},
        {"previousClose", nullptr},  // Not provided by KuCoin
        {"change", std::stod(data["price"]) - std::stod(data["open24h"])},
        {"percentage", std::stod(data["changeRate"]) * 100},
        {"average", nullptr},  // Not provided by KuCoin
        {"baseVolume", std::stod(data["size"])},
        {"quoteVolume", std::stod(data["volValue"])},
        {"info", data}
    };
    emit("ticker", parsedTicker);
}

void KucoinWS::handleOrderBook(const nlohmann::json& data) {
    bool isSnapshot = data.contains("type") && data["type"] == "snapshot";
    
    nlohmann::json bids = nlohmann::json::array();
    nlohmann::json asks = nlohmann::json::array();
    
    if (data.contains("bids")) {
        for (const auto& bid : data["bids"]) {
            bids.push_back({
                std::stod(bid[0]),  // price
                std::stod(bid[1])   // amount
            });
        }
    }
    
    if (data.contains("asks")) {
        for (const auto& ask : data["asks"]) {
            asks.push_back({
                std::stod(ask[0]),  // price
                std::stod(ask[1])   // amount
            });
        }
    }
    
    nlohmann::json parsedBook = {
        {"symbol", data["symbol"]},
        {"bids", bids},
        {"asks", asks},
        {"timestamp", std::stoll(data["time"])},
        {"datetime", exchange_.iso8601(std::stoll(data["time"]))},
        {"nonce", data.value("sequence", 0)},
        {"info", data}
    };
    
    std::string event = isSnapshot ? "orderBook" : "orderBookUpdate";
    emit(event, parsedBook);
}

void KucoinWS::handleTrade(const nlohmann::json& data) {
    nlohmann::json parsedTrade = {
        {"id", data["tradeId"]},
        {"info", data},
        {"timestamp", std::stoll(data["time"])},
        {"datetime", exchange_.iso8601(std::stoll(data["time"]))},
        {"symbol", data["symbol"]},
        {"type", nullptr},
        {"side", data["side"]},
        {"price", std::stod(data["price"])},
        {"amount", std::stod(data["size"])},
        {"cost", std::stod(data["price"]) * std::stod(data["size"])}
    };
    emit("trade", parsedTrade);
}

void KucoinWS::handleOHLCV(const nlohmann::json& data) {
    nlohmann::json parsedCandle = {
        std::stoll(data[0]),      // timestamp
        std::stod(data[1]),       // open
        std::stod(data[2]),       // high
        std::stod(data[3]),       // low
        std::stod(data[4]),       // close
        std::stod(data[5]),       // volume
    };
    emit("ohlcv", parsedCandle);
}

void KucoinWS::handleBalance(const nlohmann::json& data) {
    nlohmann::json parsedBalance = {
        {"info", data},
        {"type", data["type"]},
        {"currency", data["currency"]},
        {"total", std::stod(data["total"])},
        {"used", std::stod(data["hold"])},
        {"free", std::stod(data["available"])}
    };
    emit("balance", parsedBalance);
}

void KucoinWS::handleOrder(const nlohmann::json& data) {
    std::string status;
    if (data["status"] == "open") {
        status = "open";
    } else if (data["status"] == "done") {
        status = "closed";
    } else if (data["status"] == "match") {
        status = "open";
    } else if (data["status"] == "cancel") {
        status = "canceled";
    } else {
        status = "unknown";
    }

    nlohmann::json parsedOrder = {
        {"id", data["orderId"]},
        {"clientOrderId", data.value("clientOid", "")},
        {"info", data},
        {"timestamp", std::stoll(data["ts"])},
        {"datetime", exchange_.iso8601(std::stoll(data["ts"]))},
        {"lastTradeTimestamp", std::stoll(data["ts"])},
        {"status", status},
        {"symbol", data["symbol"]},
        {"type", data["orderType"]},
        {"side", data["side"]},
        {"price", std::stod(data["price"])},
        {"amount", std::stod(data["size"])},
        {"filled", std::stod(data["filledSize"])},
        {"remaining", std::stod(data["remainSize"])},
        {"cost", std::stod(data["price"]) * std::stod(data["filledSize"])},
        {"average", data.value("averagePrice", "0") == "0" ? 0.0 : std::stod(data["averagePrice"])},
        {"fee", {
            {"cost", std::stod(data["fee"])},
            {"currency", data["feeCurrency"]}
        }}
    };
    emit("order", parsedOrder);
}

void KucoinWS::handleMyTrade(const nlohmann::json& data) {
    nlohmann::json parsedTrade = {
        {"id", data["tradeId"]},
        {"order", data["orderId"]},
        {"info", data},
        {"timestamp", std::stoll(data["ts"])},
        {"datetime", exchange_.iso8601(std::stoll(data["ts"]))},
        {"symbol", data["symbol"]},
        {"type", data["orderType"]},
        {"side", data["side"]},
        {"price", std::stod(data["price"])},
        {"amount", std::stod(data["size"])},
        {"cost", std::stod(data["price"]) * std::stod(data["size"])},
        {"fee", {
            {"cost", std::stod(data["fee"])},
            {"currency", data["feeCurrency"]}
        }}
    };
    emit("trade", parsedTrade);
}

} // namespace ccxt
