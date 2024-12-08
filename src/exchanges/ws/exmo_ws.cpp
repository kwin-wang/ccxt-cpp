#include "ccxt/exchanges/ws/exmo_ws.h"
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

ExmoWS::ExmoWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Exmo& exchange)
    : WebSocketClient(ioc, ctx)
    , exchange_(exchange)
    , authenticated_(false) {
}

void ExmoWS::authenticate() {
    if (authenticated_) return;

    auto timestamp = std::to_string(std::time(nullptr) * 1000);
    auto signData = exchange_.apiKey + timestamp;
    auto signature = exchange_.hmac(signData, exchange_.secret, "sha512", "base64");

    nlohmann::json request = {
        {"method", "login"},
        {"api_key", exchange_.apiKey},
        {"sign", signature},
        {"nonce", timestamp}
    };

    send(request.dump());
}

void ExmoWS::watchTicker(const std::string& symbol, const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    subscribe("spot/ticker", marketId);
}

void ExmoWS::watchTickers(const std::vector<std::string>& symbols, const std::map<std::string, std::string>& params) {
    subscribeMultiple("spot/ticker", symbols);
}

void ExmoWS::watchOrderBook(const std::string& symbol, int limit, const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    subscribe("spot/order_book_updates", marketId);
}

void ExmoWS::watchTrades(const std::string& symbol, const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    subscribe("spot/trades", marketId);
}

void ExmoWS::watchBalance(const std::map<std::string, std::string>& params) {
    authenticate();
    subscribe("spot/wallet", "", true);
}

void ExmoWS::watchOrders(const std::string& symbol, const std::map<std::string, std::string>& params) {
    authenticate();
    if (!symbol.empty()) {
        auto marketId = getMarketId(symbol);
        subscribe("spot/orders", marketId, true);
    } else {
        subscribe("spot/orders", "", true);
    }
}

void ExmoWS::watchMyTrades(const std::string& symbol, const std::map<std::string, std::string>& params) {
    authenticate();
    if (!symbol.empty()) {
        auto marketId = getMarketId(symbol);
        subscribe("spot/user_trades", marketId, true);
    } else {
        subscribe("spot/user_trades", "", true);
    }
}

void ExmoWS::subscribe(const std::string& channel, const std::string& symbol, bool isPrivate) {
    std::string topic = channel;
    if (!symbol.empty()) {
        topic += ":" + symbol;
    }

    nlohmann::json request = {
        {"method", "subscribe"},
        {"id", getNextRequestId()},
        {"topics", {topic}}
    };

    send(request.dump());
    subscriptions_[topic] = symbol;
}

void ExmoWS::subscribeMultiple(const std::string& channel, const std::vector<std::string>& symbols, bool isPrivate) {
    std::vector<std::string> topics;

    for (const auto& symbol : symbols) {
        auto marketId = getMarketId(symbol);
        auto topic = channel + ":" + marketId;
        topics.push_back(topic);
        subscriptions_[topic] = symbol;
    }

    nlohmann::json request = {
        {"method", "subscribe"},
        {"id", getNextRequestId()},
        {"topics", topics}
    };

    send(request.dump());
}

void ExmoWS::unsubscribe(const std::string& channel, const std::string& symbol) {
    std::string topic = channel;
    if (!symbol.empty()) {
        topic += ":" + symbol;
    }

    nlohmann::json request = {
        {"method", "unsubscribe"},
        {"id", getNextRequestId()},
        {"topics", {topic}}
    };

    send(request.dump());
    subscriptions_.erase(topic);
}

void ExmoWS::unsubscribeMultiple(const std::string& channel, const std::vector<std::string>& symbols) {
    std::vector<std::string> topics;

    for (const auto& symbol : symbols) {
        auto marketId = getMarketId(symbol);
        auto topic = channel + ":" + marketId;
        topics.push_back(topic);
        subscriptions_.erase(topic);
    }

    nlohmann::json request = {
        {"method", "unsubscribe"},
        {"id", getNextRequestId()},
        {"topics", topics}
    };

    send(request.dump());
}

std::string ExmoWS::getEndpoint(const std::string& type) {
    if (type == "private") {
        return exchange_.urls["api"]["ws"]["spot"];
    }
    return exchange_.urls["api"]["ws"]["public"];
}

std::string ExmoWS::getMarketId(const std::string& symbol) {
    auto market = exchange_.market(symbol);
    return market["id"];
}

std::string ExmoWS::getSymbol(const std::string& marketId) {
    for (const auto& market : exchange_.markets) {
        if (market.second["id"] == marketId) {
            return market.first;
        }
    }
    return marketId;
}

std::string ExmoWS::getChannel(const std::string& channel, const std::string& symbol) {
    return channel + ":" + symbol;
}

int ExmoWS::getNextRequestId() {
    static int requestId = 1;
    return requestId++;
}

void ExmoWS::handleMessage(const std::string& message) {
    auto j = nlohmann::json::parse(message);

    if (j.contains("event")) {
        auto event = j["event"].get<std::string>();
        
        if (event == "info") {
            handleInfoMessage(j);
        } else if (event == "logged_in") {
            handleAuthenticationMessage(j);
        } else if (event == "subscribed") {
            handleSubscriptionMessage(j);
        } else if (event == "unsubscribed") {
            handleUnsubscriptionMessage(j);
        } else if (event == "error") {
            handleErrorMessage(j);
        } else if (event == "update" || event == "snapshot") {
            auto topic = j["topic"].get<std::string>();
            auto parts = topic.split(":");
            auto channel = parts[0];
            
            if (channel == "spot/ticker") {
                handleTickerMessage(j);
            } else if (channel == "spot/order_book_updates") {
                handleOrderBookMessage(j);
            } else if (channel == "spot/trades") {
                handleTradeMessage(j);
            } else if (channel == "spot/wallet" || channel == "margin/wallet") {
                handleBalanceMessage(j);
            } else if (channel == "spot/orders" || channel == "margin/orders") {
                handleOrderMessage(j);
            } else if (channel == "spot/user_trades" || channel == "margin/user_trades") {
                handleMyTradeMessage(j);
            }
        }
    }
}

void ExmoWS::handleTickerMessage(const nlohmann::json& data) {
    // Implementation details
}

void ExmoWS::handleOrderBookMessage(const nlohmann::json& data) {
    // Implementation details
}

void ExmoWS::handleTradeMessage(const nlohmann::json& data) {
    // Implementation details
}

void ExmoWS::handleBalanceMessage(const nlohmann::json& data) {
    // Implementation details
}

void ExmoWS::handleOrderMessage(const nlohmann::json& data) {
    // Implementation details
}

void ExmoWS::handleMyTradeMessage(const nlohmann::json& data) {
    // Implementation details
}

void ExmoWS::handleErrorMessage(const nlohmann::json& data) {
    throw ExchangeError(exchange_.id + " " + data.dump());
}

void ExmoWS::handleSubscriptionMessage(const nlohmann::json& data) {
    // Implementation details
}

void ExmoWS::handleUnsubscriptionMessage(const nlohmann::json& data) {
    // Implementation details
}

void ExmoWS::handleAuthenticationMessage(const nlohmann::json& data) {
    authenticated_ = true;
}

void ExmoWS::handleInfoMessage(const nlohmann::json& data) {
    sessionId_ = data["session_id"].get<std::string>();
}

Order ExmoWS::parseWsOrder(const nlohmann::json& order, const Market* market) {
    auto id = order["order_id"].get<std::string>();
    auto timestamp = std::stoll(order["created"]) * 1000;
    auto orderType = order["type"].get<std::string>();
    auto side = orderType.find("buy") != std::string::npos ? "buy" : "sell";
    auto type = orderType.find("market") != std::string::npos ? "market" : "limit";
    auto marketId = order["pair"].get<std::string>();
    auto symbol = market ? market->symbol : getSymbol(marketId);
    auto amount = order["original_quantity"].get<std::string>();
    auto remaining = order["quantity"].get<std::string>();
    auto price = order["price"].get<std::string>();
    auto clientOrderId = order["client_id"].get<std::string>();
    if (clientOrderId == "0") {
        clientOrderId = "";
    }
    auto stopPrice = order["stop_price"].get<std::string>();
    if (stopPrice == "0") {
        stopPrice = "";
    }

    return {
        {"id", id},
        {"clientOrderId", clientOrderId},
        {"datetime", exchange_.iso8601(timestamp)},
        {"timestamp", timestamp},
        {"lastTradeTimestamp", nullptr},
        {"status", exchange_.parseOrderStatus(order["status"].get<std::string>())},
        {"symbol", symbol},
        {"type", type},
        {"timeInForce", nullptr},
        {"postOnly", nullptr},
        {"side", side},
        {"price", price},
        {"stopPrice", stopPrice},
        {"amount", amount},
        {"cost", nullptr},
        {"filled", nullptr},
        {"remaining", remaining},
        {"trades", nullptr},
        {"fee", nullptr},
        {"info", order}
    };
}

Trade ExmoWS::parseWsTrade(const nlohmann::json& trade, const Market* market) {
    auto id = trade["trade_id"].get<std::string>();
    auto timestamp = std::stoll(trade["date"].get<std::string>()) * 1000;
    auto side = trade["type"].get<std::string>();
    auto marketId = trade["pair"].get<std::string>();
    auto symbol = market ? market->symbol : getSymbol(marketId);
    auto price = trade["price"].get<std::string>();
    auto amount = trade["quantity"].get<std::string>();
    auto cost = trade["amount"].get<std::string>();
    auto orderId = trade["order_id"].get<std::string>();
    auto takerOrMaker = trade["exec_type"].get<std::string>();

    nlohmann::json fee;
    if (trade.contains("commission_amount") && trade.contains("commission_currency")) {
        fee = {
            {"cost", trade["commission_amount"].get<std::string>()},
            {"currency", trade["commission_currency"].get<std::string>()}
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
        {"takerOrMaker", takerOrMaker},
        {"price", price},
        {"amount", amount},
        {"cost", cost},
        {"fee", fee}
    };
}

} // namespace ccxt
