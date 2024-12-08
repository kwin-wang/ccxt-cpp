#include "ccxt/exchanges/ws/hashkey_ws.h"
#include "ccxt/base/json.hpp"
#include "ccxt/base/errors.h"
#include "ccxt/base/functions.h"
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <ctime>
#include <sstream>

namespace ccxt {

HashkeyWS::HashkeyWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Hashkey& exchange)
    : WebSocketClient(ioc, ctx)
    , exchange_(exchange)
    , authenticated_(false) {
}

void HashkeyWS::authenticate() {
    if (authenticated_) return;

    auto timestamp = std::to_string(std::time(nullptr) * 1000);
    auto message = timestamp + "GET" + "/ws/v1/auth";
    auto signature = exchange_.hmac(message, exchange_.secret, "sha256", "hex");

    nlohmann::json request = {
        {"type", "auth"},
        {"key", exchange_.apiKey},
        {"timestamp", timestamp},
        {"signature", signature}
    };

    send(request.dump());
}

void HashkeyWS::watchTicker(const std::string& symbol, const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    subscribe("ticker", marketId);
}

void HashkeyWS::watchTickers(const std::vector<std::string>& symbols, const std::map<std::string, std::string>& params) {
    subscribeMultiple("ticker", symbols);
}

void HashkeyWS::watchOrderBook(const std::string& symbol, int limit, const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    auto channel = limit > 0 ? "depth" + std::to_string(limit) : "depth";
    subscribe(channel, marketId);
}

void HashkeyWS::watchTrades(const std::string& symbol, const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    subscribe("trade", marketId);
}

void HashkeyWS::watchOHLCV(const std::string& symbol, const std::string& timeframe, 
                          const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    auto channel = "kline." + timeframe;
    subscribe(channel, marketId);
}

void HashkeyWS::watchBalance(const std::map<std::string, std::string>& params) {
    authenticate();
    subscribe("account", "", true);
}

void HashkeyWS::watchOrders(const std::string& symbol, const std::map<std::string, std::string>& params) {
    authenticate();
    if (!symbol.empty()) {
        auto marketId = getMarketId(symbol);
        subscribe("order", marketId, true);
    } else {
        subscribe("order", "", true);
    }
}

void HashkeyWS::watchMyTrades(const std::string& symbol, const std::map<std::string, std::string>& params) {
    authenticate();
    if (!symbol.empty()) {
        auto marketId = getMarketId(symbol);
        subscribe("trade", marketId, true);
    } else {
        subscribe("trade", "", true);
    }
}

void HashkeyWS::subscribe(const std::string& channel, const std::string& symbol, bool isPrivate) {
    nlohmann::json request;
    std::string topic = channel;
    
    if (!symbol.empty()) {
        topic += "." + symbol;
    }

    request = {
        {"type", "subscribe"},
        {"topic", topic},
        {"id", getNextRequestId()}
    };

    send(request.dump());
    subscriptions_[topic] = symbol;
}

void HashkeyWS::subscribeMultiple(const std::string& channel, const std::vector<std::string>& symbols, bool isPrivate) {
    std::vector<std::string> topics;
    
    for (const auto& symbol : symbols) {
        auto marketId = getMarketId(symbol);
        auto topic = channel + "." + marketId;
        topics.push_back(topic);
        subscriptions_[topic] = symbol;
    }

    nlohmann::json request = {
        {"type", "subscribe"},
        {"topics", topics},
        {"id", getNextRequestId()}
    };

    send(request.dump());
}

void HashkeyWS::unsubscribe(const std::string& channel, const std::string& symbol) {
    std::string topic = channel;
    if (!symbol.empty()) {
        topic += "." + symbol;
    }

    nlohmann::json request = {
        {"type", "unsubscribe"},
        {"topic", topic},
        {"id", getNextRequestId()}
    };

    send(request.dump());
    subscriptions_.erase(topic);
}

void HashkeyWS::unsubscribeMultiple(const std::string& channel, const std::vector<std::string>& symbols) {
    std::vector<std::string> topics;
    
    for (const auto& symbol : symbols) {
        auto marketId = getMarketId(symbol);
        auto topic = channel + "." + marketId;
        topics.push_back(topic);
        subscriptions_.erase(topic);
    }

    nlohmann::json request = {
        {"type", "unsubscribe"},
        {"topics", topics},
        {"id", getNextRequestId()}
    };

    send(request.dump());
}

std::string HashkeyWS::getEndpoint(const std::string& type) {
    return exchange_.urls["api"]["ws"];
}

std::string HashkeyWS::getMarketId(const std::string& symbol) {
    auto market = exchange_.market(symbol);
    return market["id"];
}

std::string HashkeyWS::getSymbol(const std::string& marketId) {
    for (const auto& market : exchange_.markets) {
        if (market.second["id"] == marketId) {
            return market.first;
        }
    }
    return marketId;
}

std::string HashkeyWS::getChannel(const std::string& channel, const std::string& symbol) {
    return channel + "." + symbol;
}

int HashkeyWS::getNextRequestId() {
    static int requestId = 1;
    return requestId++;
}

void HashkeyWS::handleMessage(const std::string& message) {
    auto j = nlohmann::json::parse(message);

    if (j.contains("type")) {
        auto type = j["type"].get<std::string>();
        
        if (type == "auth") {
            handleAuthenticationMessage(j);
        } else if (type == "subscribed") {
            handleSubscriptionMessage(j);
        } else if (type == "unsubscribed") {
            handleUnsubscriptionMessage(j);
        } else if (type == "error") {
            handleErrorMessage(j);
        } else if (type == "update") {
            if (j.contains("topic")) {
                auto topic = j["topic"].get<std::string>();
                
                if (topic.find("ticker") != std::string::npos) {
                    handleTickerMessage(j);
                } else if (topic.find("depth") != std::string::npos) {
                    handleOrderBookMessage(j);
                } else if (topic.find("trade") != std::string::npos) {
                    if (j.contains("private") && j["private"].get<bool>()) {
                        handleMyTradeMessage(j);
                    } else {
                        handleTradeMessage(j);
                    }
                } else if (topic.find("kline") != std::string::npos) {
                    handleOHLCVMessage(j);
                } else if (topic == "account") {
                    handleBalanceMessage(j);
                } else if (topic.find("order") != std::string::npos) {
                    handleOrderMessage(j);
                }
            }
        }
    }
}

void HashkeyWS::handleTickerMessage(const nlohmann::json& data) {
    // Implementation details
}

void HashkeyWS::handleOrderBookMessage(const nlohmann::json& data) {
    // Implementation details
}

void HashkeyWS::handleTradeMessage(const nlohmann::json& data) {
    // Implementation details
}

void HashkeyWS::handleOHLCVMessage(const nlohmann::json& data) {
    // Implementation details
}

void HashkeyWS::handleBalanceMessage(const nlohmann::json& data) {
    // Implementation details
}

void HashkeyWS::handleOrderMessage(const nlohmann::json& data) {
    // Implementation details
}

void HashkeyWS::handleMyTradeMessage(const nlohmann::json& data) {
    // Implementation details
}

void HashkeyWS::handleErrorMessage(const nlohmann::json& data) {
    throw ExchangeError(exchange_.id + " " + data["message"].get<std::string>());
}

void HashkeyWS::handleSubscriptionMessage(const nlohmann::json& data) {
    // Implementation details
}

void HashkeyWS::handleUnsubscriptionMessage(const nlohmann::json& data) {
    // Implementation details
}

void HashkeyWS::handleAuthenticationMessage(const nlohmann::json& data) {
    if (data["code"].get<int>() == 0) {
        authenticated_ = true;
    } else {
        throw AuthenticationError(exchange_.id + " authentication failed: " + data["message"].get<std::string>());
    }
}

Order HashkeyWS::parseWsOrder(const nlohmann::json& order, const Market* market) {
    auto id = order["orderId"].get<std::string>();
    auto timestamp = std::stoll(order["timestamp"].get<std::string>());
    auto type = order["type"].get<std::string>();
    auto side = order["side"].get<std::string>();
    auto marketId = order["symbol"].get<std::string>();
    auto symbol = market ? market->symbol : getSymbol(marketId);
    auto price = order["price"].get<std::string>();
    auto amount = order["quantity"].get<std::string>();
    auto filled = order["executedQty"].get<std::string>();
    auto remaining = std::to_string(std::stod(amount) - std::stod(filled));
    auto status = exchange_.parseOrderStatus(order["status"].get<std::string>());
    auto clientOrderId = order.contains("clientOrderId") ? order["clientOrderId"].get<std::string>() : "";

    return {
        {"id", id},
        {"clientOrderId", clientOrderId},
        {"datetime", exchange_.iso8601(timestamp)},
        {"timestamp", timestamp},
        {"lastTradeTimestamp", nullptr},
        {"status", status},
        {"symbol", symbol},
        {"type", type},
        {"timeInForce", order["timeInForce"].get<std::string>()},
        {"postOnly", nullptr},
        {"side", side},
        {"price", price},
        {"stopPrice", nullptr},
        {"amount", amount},
        {"cost", nullptr},
        {"filled", filled},
        {"remaining", remaining},
        {"trades", nullptr},
        {"fee", nullptr},
        {"info", order}
    };
}

Trade HashkeyWS::parseWsTrade(const nlohmann::json& trade, const Market* market) {
    auto id = trade["tradeId"].get<std::string>();
    auto timestamp = std::stoll(trade["timestamp"].get<std::string>());
    auto side = trade["side"].get<std::string>();
    auto marketId = trade["symbol"].get<std::string>();
    auto symbol = market ? market->symbol : getSymbol(marketId);
    auto price = trade["price"].get<std::string>();
    auto amount = trade["quantity"].get<std::string>();
    auto cost = std::to_string(std::stod(price) * std::stod(amount));
    auto orderId = trade.contains("orderId") ? trade["orderId"].get<std::string>() : "";

    nlohmann::json fee;
    if (trade.contains("fee") && trade.contains("feeCurrency")) {
        fee = {
            {"cost", trade["fee"].get<std::string>()},
            {"currency", trade["feeCurrency"].get<std::string>()}
        };
    }

    return {
        {"id", id},
        {"info", trade},
        {"timestamp", timestamp},
        {"datetime", exchange_.iso8601(timestamp)},
        {"symbol", symbol},
        {"type", nullptr},
        {"side", side},
        {"order", orderId},
        {"takerOrMaker", trade["liquidity"].get<std::string>()},
        {"price", price},
        {"amount", amount},
        {"cost", cost},
        {"fee", fee}
    };
}

} // namespace ccxt
