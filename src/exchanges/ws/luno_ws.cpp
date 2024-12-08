#include "ccxt/exchanges/ws/luno_ws.h"
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

LunoWS::LunoWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Luno& exchange)
    : WebSocketClient(ioc, ctx)
    , exchange_(exchange)
    , authenticated_(false) {
}

void LunoWS::authenticate() {
    if (authenticated_) return;

    auto timestamp = std::to_string(std::time(nullptr) * 1000);
    auto signData = exchange_.apiKey + timestamp;
    auto signature = exchange_.hmac(signData, exchange_.secret, "sha512");

    nlohmann::json request = {
        {"method", "authenticate"},
        {"api_key_id", exchange_.apiKey},
        {"signature", signature},
        {"timestamp", timestamp}
    };

    send(request.dump());
}

void LunoWS::watchTicker(const std::string& symbol, const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    subscribe("ticker", marketId);
}

void LunoWS::watchTickers(const std::vector<std::string>& symbols, const std::map<std::string, std::string>& params) {
    subscribeMultiple("ticker", symbols);
}

void LunoWS::watchOrderBook(const std::string& symbol, int limit, const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    subscribe("orderbook", marketId);
}

void LunoWS::watchTrades(const std::string& symbol, const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    subscribe("trades", marketId);
}

void LunoWS::watchBalance(const std::map<std::string, std::string>& params) {
    authenticate();
    subscribe("accounts", "", true);
}

void LunoWS::watchOrders(const std::string& symbol, const std::map<std::string, std::string>& params) {
    authenticate();
    if (!symbol.empty()) {
        auto marketId = getMarketId(symbol);
        subscribe("orders", marketId, true);
    } else {
        subscribe("orders", "", true);
    }
}

void LunoWS::watchMyTrades(const std::string& symbol, const std::map<std::string, std::string>& params) {
    authenticate();
    if (!symbol.empty()) {
        auto marketId = getMarketId(symbol);
        subscribe("trades", marketId, true);
    } else {
        subscribe("trades", "", true);
    }
}

void LunoWS::subscribe(const std::string& channel, const std::string& symbol, bool isPrivate) {
    std::string topic = channel;
    if (!symbol.empty()) {
        topic += "-" + symbol;
    }

    nlohmann::json request = {
        {"type", "SUBSCRIBE"},
        {"subscriptions", {{
            {"event", topic}
        }}}
    };

    send(request.dump());
    subscriptions_[topic] = symbol;
}

void LunoWS::subscribeMultiple(const std::string& channel, const std::vector<std::string>& symbols, bool isPrivate) {
    std::vector<nlohmann::json> subscriptions;
    
    for (const auto& symbol : symbols) {
        auto marketId = getMarketId(symbol);
        auto topic = channel + "-" + marketId;
        subscriptions.push_back({{"event", topic}});
        subscriptions_[topic] = symbol;
    }

    nlohmann::json request = {
        {"type", "SUBSCRIBE"},
        {"subscriptions", subscriptions}
    };

    send(request.dump());
}

void LunoWS::unsubscribe(const std::string& channel, const std::string& symbol) {
    std::string topic = channel;
    if (!symbol.empty()) {
        topic += "-" + symbol;
    }

    nlohmann::json request = {
        {"type", "UNSUBSCRIBE"},
        {"subscriptions", {{
            {"event", topic}
        }}}
    };

    send(request.dump());
    subscriptions_.erase(topic);
}

void LunoWS::unsubscribeMultiple(const std::string& channel, const std::vector<std::string>& symbols) {
    std::vector<nlohmann::json> subscriptions;
    
    for (const auto& symbol : symbols) {
        auto marketId = getMarketId(symbol);
        auto topic = channel + "-" + marketId;
        subscriptions.push_back({{"event", topic}});
        subscriptions_.erase(topic);
    }

    nlohmann::json request = {
        {"type", "UNSUBSCRIBE"},
        {"subscriptions", subscriptions}
    };

    send(request.dump());
}

std::string LunoWS::getEndpoint(const std::string& type) {
    return exchange_.urls["api"]["ws"];
}

std::string LunoWS::getMarketId(const std::string& symbol) {
    auto market = exchange_.market(symbol);
    return market["id"];
}

std::string LunoWS::getSymbol(const std::string& marketId) {
    for (const auto& market : exchange_.markets) {
        if (market.second["id"] == marketId) {
            return market.first;
        }
    }
    return marketId;
}

std::string LunoWS::getChannel(const std::string& channel, const std::string& symbol) {
    return channel + "-" + symbol;
}

int LunoWS::getNextRequestId() {
    static int requestId = 1;
    return requestId++;
}

void LunoWS::handleMessage(const std::string& message) {
    auto j = nlohmann::json::parse(message);

    if (j.contains("type")) {
        auto type = j["type"].get<std::string>();
        
        if (type == "AUTHENTICATED") {
            handleAuthenticationMessage(j);
        } else if (type == "SUBSCRIBED") {
            handleSubscriptionMessage(j);
        } else if (type == "UNSUBSCRIBED") {
            handleUnsubscriptionMessage(j);
        } else if (type == "ERROR") {
            handleErrorMessage(j);
        } else if (type == "UPDATE") {
            if (j.contains("trade")) {
                handleTradeMessage(j);
            } else if (j.contains("orderbook")) {
                handleOrderBookMessage(j);
            } else if (j.contains("ticker")) {
                handleTickerMessage(j);
            } else if (j.contains("account")) {
                handleBalanceMessage(j);
            } else if (j.contains("order")) {
                handleOrderMessage(j);
            }
        }
    }
}

void LunoWS::handleTickerMessage(const nlohmann::json& data) {
    // Implementation details
}

void LunoWS::handleOrderBookMessage(const nlohmann::json& data) {
    // Implementation details
}

void LunoWS::handleTradeMessage(const nlohmann::json& data) {
    // Implementation details
}

void LunoWS::handleBalanceMessage(const nlohmann::json& data) {
    // Implementation details
}

void LunoWS::handleOrderMessage(const nlohmann::json& data) {
    // Implementation details
}

void LunoWS::handleMyTradeMessage(const nlohmann::json& data) {
    // Implementation details
}

void LunoWS::handleErrorMessage(const nlohmann::json& data) {
    throw ExchangeError(exchange_.id + " " + data["message"].get<std::string>());
}

void LunoWS::handleSubscriptionMessage(const nlohmann::json& data) {
    // Implementation details
}

void LunoWS::handleUnsubscriptionMessage(const nlohmann::json& data) {
    // Implementation details
}

void LunoWS::handleAuthenticationMessage(const nlohmann::json& data) {
    authenticated_ = true;
}

Order LunoWS::parseWsOrder(const nlohmann::json& order, const Market* market) {
    auto id = order["order_id"].get<std::string>();
    auto timestamp = std::stoll(order["creation_timestamp"].get<std::string>());
    auto type = order["type"].get<std::string>();
    auto side = order["type"].get<std::string>() == "BID" ? "buy" : "sell";
    auto marketId = order["pair"].get<std::string>();
    auto symbol = market ? market->symbol : getSymbol(marketId);
    auto price = order["limit_price"].get<std::string>();
    auto amount = order["volume"].get<std::string>();
    auto remaining = order["remaining_volume"].get<std::string>();
    auto filled = std::to_string(std::stod(amount) - std::stod(remaining));
    auto status = exchange_.parseOrderStatus(order["state"].get<std::string>());

    return {
        {"id", id},
        {"clientOrderId", nullptr},
        {"datetime", exchange_.iso8601(timestamp)},
        {"timestamp", timestamp},
        {"lastTradeTimestamp", nullptr},
        {"status", status},
        {"symbol", symbol},
        {"type", type},
        {"timeInForce", nullptr},
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

Trade LunoWS::parseWsTrade(const nlohmann::json& trade, const Market* market) {
    auto id = trade["sequence"].get<std::string>();
    auto timestamp = std::stoll(trade["timestamp"].get<std::string>());
    auto side = trade["is_buy"].get<bool>() ? "buy" : "sell";
    auto marketId = trade["pair"].get<std::string>();
    auto symbol = market ? market->symbol : getSymbol(marketId);
    auto price = trade["price"].get<std::string>();
    auto amount = trade["volume"].get<std::string>();
    auto cost = std::to_string(std::stod(price) * std::stod(amount));

    return {
        {"id", id},
        {"info", trade},
        {"timestamp", timestamp},
        {"datetime", exchange_.iso8601(timestamp)},
        {"symbol", symbol},
        {"type", nullptr},
        {"side", side},
        {"order", nullptr},
        {"takerOrMaker", nullptr},
        {"price", price},
        {"amount", amount},
        {"cost", cost},
        {"fee", nullptr}
    };
}

} // namespace ccxt
