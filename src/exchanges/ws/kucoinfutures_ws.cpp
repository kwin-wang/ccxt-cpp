#include "../../../include/ccxt/exchanges/ws/kucoinfutures_ws.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>

namespace ccxt {

KuCoinFuturesWS::KuCoinFuturesWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, KuCoinFutures& exchange)
    : WebSocketClient(ioc, ctx), exchange_(exchange), tradesLimit_(1000),
      snapshotDelay_(5), snapshotMaxRetries_(3) {
    // Initialize connection settings specific to KuCoin Futures
}

void KuCoinFuturesWS::negotiate(bool privateChannel) {
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

void KuCoinFuturesWS::ping() {
    nlohmann::json pingMessage = {
        {"id", std::to_string(std::chrono::system_clock::now().time_since_epoch().count())},
        {"type", "ping"}
    };
    send(pingMessage.dump());
}

void KuCoinFuturesWS::authenticate() {
    negotiate(true);
    nlohmann::json request = {
        {"id", std::to_string(std::chrono::system_clock::now().time_since_epoch().count())},
        {"type", "subscribe"},
        {"topic", "/contractMarket/tradeOrders"},  // Example private channel
        {"privateChannel", true},
        {"response", true}
    };
    send(request.dump());
}

void KuCoinFuturesWS::subscribe(const std::string& topic, const nlohmann::json& params) {
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

void KuCoinFuturesWS::unsubscribe(const std::string& topic, const nlohmann::json& params) {
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

void KuCoinFuturesWS::watchTicker(const std::string& symbol) {
    std::string topic = "/contractMarket/ticker:" + symbol;
    subscribe(topic);
}

void KuCoinFuturesWS::watchTickers(const std::vector<std::string>& symbols) {
    for (const auto& symbol : symbols) {
        watchTicker(symbol);
    }
}

void KuCoinFuturesWS::watchOrderBook(const std::string& symbol) {
    std::string topic = "/contractMarket/level2:" + symbol;
    subscribe(topic);
}

void KuCoinFuturesWS::watchTrades(const std::string& symbol) {
    std::string topic = "/contractMarket/execution:" + symbol;
    subscribe(topic);
}

void KuCoinFuturesWS::watchOHLCV(const std::string& symbol, const std::string& timeframe) {
    std::string topic = "/contractMarket/candles:" + symbol + "_" + timeframe;
    subscribe(topic);
}

void KuCoinFuturesWS::watchMarkPrice(const std::string& symbol) {
    std::string topic = "/contractMarket/mark:" + symbol;
    subscribe(topic);
}

void KuCoinFuturesWS::watchFundingRate(const std::string& symbol) {
    std::string topic = "/contractMarket/funding:" + symbol;
    subscribe(topic);
}

void KuCoinFuturesWS::watchIndex(const std::string& symbol) {
    std::string topic = "/contractMarket/index:" + symbol;
    subscribe(topic);
}

void KuCoinFuturesWS::watchPremiumIndex(const std::string& symbol) {
    std::string topic = "/contractMarket/premium:" + symbol;
    subscribe(topic);
}

void KuCoinFuturesWS::watchBalance() {
    authenticate();
    subscribe("/contractAccount/wallet", {{"privateChannel", true}});
}

void KuCoinFuturesWS::watchOrders() {
    authenticate();
    subscribe("/contractMarket/tradeOrders", {{"privateChannel", true}});
}

void KuCoinFuturesWS::watchMyTrades() {
    authenticate();
    subscribe("/contractMarket/execution", {{"privateChannel", true}});
}

void KuCoinFuturesWS::watchPositions() {
    authenticate();
    subscribe("/contract/position", {{"privateChannel", true}});
}

void KuCoinFuturesWS::handleMessage(const std::string& message) {
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
            
            if (topic.find("/contractMarket/ticker") == 0) {
                handleTicker(data);
            } else if (topic.find("/contractMarket/level2") == 0) {
                handleOrderBook(data);
            } else if (topic.find("/contractMarket/execution") == 0) {
                if (j.contains("privateChannel") && j["privateChannel"]) {
                    handleMyTrade(data);
                } else {
                    handleTrade(data);
                }
            } else if (topic.find("/contractMarket/candles") == 0) {
                handleOHLCV(data);
            } else if (topic.find("/contractMarket/mark") == 0) {
                handleMarkPrice(data);
            } else if (topic.find("/contractMarket/funding") == 0) {
                handleFundingRate(data);
            } else if (topic.find("/contractMarket/index") == 0) {
                handleIndex(data);
            } else if (topic.find("/contractMarket/premium") == 0) {
                handlePremiumIndex(data);
            } else if (topic == "/contractAccount/wallet") {
                handleBalance(data);
            } else if (topic == "/contractMarket/tradeOrders") {
                handleOrder(data);
            } else if (topic == "/contract/position") {
                handlePosition(data);
            }
        }
    } catch (const std::exception& e) {
        emit("error", {{"message", e.what()}});
    }
}

void KuCoinFuturesWS::handleTicker(const nlohmann::json& data) {
    nlohmann::json parsedTicker = {
        {"symbol", data["symbol"]},
        {"timestamp", std::stoll(data["ts"])},
        {"datetime", exchange_.iso8601(std::stoll(data["ts"]))},
        {"high", std::stod(data["high24h"])},
        {"low", std::stod(data["low24h"])},
        {"bid", std::stod(data["bestBidPrice"])},
        {"bidVolume", std::stod(data["bestBidSize"])},
        {"ask", std::stod(data["bestAskPrice"])},
        {"askVolume", std::stod(data["bestAskSize"])},
        {"vwap", nullptr},  // Not provided by KuCoin Futures
        {"open", std::stod(data["openPrice"])},
        {"close", std::stod(data["lastTradePrice"])},
        {"last", std::stod(data["lastTradePrice"])},
        {"previousClose", nullptr},  // Not provided by KuCoin Futures
        {"change", std::stod(data["priceChangePercent"])},
        {"percentage", std::stod(data["priceChangePercent"]) * 100},
        {"average", nullptr},  // Not provided by KuCoin Futures
        {"baseVolume", std::stod(data["volume"])},
        {"quoteVolume", std::stod(data["turnover"])},
        {"info", data}
    };
    emit("ticker", parsedTicker);
}

void KuCoinFuturesWS::handleOrderBook(const nlohmann::json& data) {
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
        {"timestamp", std::stoll(data["ts"])},
        {"datetime", exchange_.iso8601(std::stoll(data["ts"]))},
        {"nonce", data.value("sequence", 0)},
        {"info", data}
    };
    
    std::string event = isSnapshot ? "orderBook" : "orderBookUpdate";
    emit(event, parsedBook);
}

void KuCoinFuturesWS::handleTrade(const nlohmann::json& data) {
    nlohmann::json parsedTrade = {
        {"id", data["tradeId"]},
        {"info", data},
        {"timestamp", std::stoll(data["ts"])},
        {"datetime", exchange_.iso8601(std::stoll(data["ts"]))},
        {"symbol", data["symbol"]},
        {"type", "futures"},
        {"side", data["side"]},
        {"price", std::stod(data["price"])},
        {"amount", std::stod(data["size"])},
        {"cost", std::stod(data["price"]) * std::stod(data["size"])}
    };
    emit("trade", parsedTrade);
}

void KuCoinFuturesWS::handleOHLCV(const nlohmann::json& data) {
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

void KuCoinFuturesWS::handleMarkPrice(const nlohmann::json& data) {
    nlohmann::json parsedMarkPrice = {
        {"symbol", data["symbol"]},
        {"markPrice", std::stod(data["value"])},
        {"timestamp", std::stoll(data["ts"])},
        {"datetime", exchange_.iso8601(std::stoll(data["ts"]))},
        {"info", data}
    };
    emit("markPrice", parsedMarkPrice);
}

void KuCoinFuturesWS::handleFundingRate(const nlohmann::json& data) {
    nlohmann::json parsedFundingRate = {
        {"symbol", data["symbol"]},
        {"fundingRate", std::stod(data["fundingRate"])},
        {"timestamp", std::stoll(data["ts"])},
        {"datetime", exchange_.iso8601(std::stoll(data["ts"]))},
        {"predictedRate", std::stod(data["predictedRate"])},
        {"nextFundingTime", std::stoll(data["nextFundingTime"])},
        {"info", data}
    };
    emit("fundingRate", parsedFundingRate);
}

void KuCoinFuturesWS::handleIndex(const nlohmann::json& data) {
    nlohmann::json parsedIndex = {
        {"symbol", data["symbol"]},
        {"indexPrice", std::stod(data["value"])},
        {"timestamp", std::stoll(data["ts"])},
        {"datetime", exchange_.iso8601(std::stoll(data["ts"]))},
        {"info", data}
    };
    emit("index", parsedIndex);
}

void KuCoinFuturesWS::handlePremiumIndex(const nlohmann::json& data) {
    nlohmann::json parsedPremiumIndex = {
        {"symbol", data["symbol"]},
        {"premiumIndex", std::stod(data["value"])},
        {"timestamp", std::stoll(data["ts"])},
        {"datetime", exchange_.iso8601(std::stoll(data["ts"]))},
        {"info", data}
    };
    emit("premiumIndex", parsedPremiumIndex);
}

void KuCoinFuturesWS::handleBalance(const nlohmann::json& data) {
    nlohmann::json parsedBalance = {
        {"info", data},
        {"currency", data["currency"]},
        {"total", std::stod(data["accountEquity"])},
        {"used", std::stod(data["positionMargin"]) + std::stod(data["orderMargin"])},
        {"free", std::stod(data["availableBalance"])},
        {"unrealizedPnl", std::stod(data["unrealisedPNL"])},
        {"marginRatio", std::stod(data["marginRatio"])},
        {"maintenanceMarginRequirement", std::stod(data["maintenanceMargin"])}
    };
    emit("balance", parsedBalance);
}

void KuCoinFuturesWS::handleOrder(const nlohmann::json& data) {
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
        {"type", data["type"]},
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
        }},
        {"leverage", std::stod(data["leverage"])}
    };
    emit("order", parsedOrder);
}

void KuCoinFuturesWS::handleMyTrade(const nlohmann::json& data) {
    nlohmann::json parsedTrade = {
        {"id", data["tradeId"]},
        {"order", data["orderId"]},
        {"info", data},
        {"timestamp", std::stoll(data["ts"])},
        {"datetime", exchange_.iso8601(std::stoll(data["ts"]))},
        {"symbol", data["symbol"]},
        {"type", "futures"},
        {"side", data["side"]},
        {"price", std::stod(data["price"])},
        {"amount", std::stod(data["size"])},
        {"cost", std::stod(data["price"]) * std::stod(data["size"])},
        {"fee", {
            {"cost", std::stod(data["fee"])},
            {"currency", data["feeCurrency"]}
        }},
        {"leverage", std::stod(data["leverage"])}
    };
    emit("trade", parsedTrade);
}

void KuCoinFuturesWS::handlePosition(const nlohmann::json& data) {
    nlohmann::json parsedPosition = {
        {"info", data},
        {"symbol", data["symbol"]},
        {"timestamp", std::stoll(data["ts"])},
        {"datetime", exchange_.iso8601(std::stoll(data["ts"]))},
        {"side", data["side"]},
        {"size", std::stod(data["currentQty"])},
        {"notional", std::stod(data["positionValue"])},
        {"leverage", std::stod(data["leverage"])},
        {"entryPrice", std::stod(data["avgEntryPrice"])},
        {"unrealizedPnl", std::stod(data["unrealisedPnl"])},
        {"realizedPnl", std::stod(data["realisedPnl"])},
        {"liquidationPrice", std::stod(data["liquidationPrice"])},
        {"marginType", data["marginType"]},
        {"maintenanceMargin", std::stod(data["maintMargin"])},
        {"marginRatio", std::stod(data["marginRatio"])},
        {"collateral", std::stod(data["positionMargin"])}
    };
    emit("position", parsedPosition);
}

} // namespace ccxt
