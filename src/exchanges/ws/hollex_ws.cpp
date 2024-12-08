#include "ccxt/exchanges/ws/hollex_ws.h"
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

HollexWS::HollexWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Hollex& exchange)
    : WebSocketClient(ioc, ctx)
    , exchange_(exchange)
    , authenticated_(false) {
}

void HollexWS::authenticate() {
    if (authenticated_) return;

    auto timestamp = std::to_string(std::time(nullptr) * 1000);
    auto message = timestamp + "GET/ws/v1/auth";
    auto signature = exchange_.hmac(message, exchange_.secret, "sha256", "hex");

    nlohmann::json request = {
        {"event", "auth"},
        {"data", {
            {"apiKey", exchange_.apiKey},
            {"timestamp", timestamp},
            {"signature", signature}
        }}
    };

    send(request.dump());
}

void HollexWS::watchTicker(const std::string& symbol, const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    subscribe("ticker", marketId);
}

void HollexWS::watchTickers(const std::vector<std::string>& symbols, const std::map<std::string, std::string>& params) {
    subscribeMultiple("ticker", symbols);
}

void HollexWS::watchOrderBook(const std::string& symbol, int limit, const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    auto channel = limit > 0 ? "orderbook" + std::to_string(limit) : "orderbook";
    subscribe(channel, marketId);
}

void HollexWS::watchTrades(const std::string& symbol, const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    subscribe("trades", marketId);
}

void HollexWS::watchOHLCV(const std::string& symbol, const std::string& timeframe,
                         const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    subscribe("kline_" + timeframe, marketId);
}

void HollexWS::watchBalance(const std::map<std::string, std::string>& params) {
    authenticate();
    subscribe("balance", "", true);
}

void HollexWS::watchOrders(const std::string& symbol, const std::map<std::string, std::string>& params) {
    authenticate();
    if (!symbol.empty()) {
        auto marketId = getMarketId(symbol);
        subscribe("orders", marketId, true);
    } else {
        subscribe("orders", "", true);
    }
}

void HollexWS::watchMyTrades(const std::string& symbol, const std::map<std::string, std::string>& params) {
    authenticate();
    if (!symbol.empty()) {
        auto marketId = getMarketId(symbol);
        subscribe("user_trades", marketId, true);
    } else {
        subscribe("user_trades", "", true);
    }
}

void HollexWS::subscribe(const std::string& channel, const std::string& symbol, bool isPrivate) {
    nlohmann::json request = {
        {"event", "subscribe"},
        {"channel", channel}
    };

    if (!symbol.empty()) {
        request["symbol"] = symbol;
    }

    send(request.dump());
    subscriptions_[channel + (symbol.empty() ? "" : ":" + symbol)] = symbol;
}

void HollexWS::subscribeMultiple(const std::string& channel, const std::vector<std::string>& symbols, bool isPrivate) {
    for (const auto& symbol : symbols) {
        auto marketId = getMarketId(symbol);
        subscribe(channel, marketId, isPrivate);
    }
}

void HollexWS::unsubscribe(const std::string& channel, const std::string& symbol) {
    nlohmann::json request = {
        {"event", "unsubscribe"},
        {"channel", channel}
    };

    if (!symbol.empty()) {
        request["symbol"] = symbol;
    }

    send(request.dump());
    subscriptions_.erase(channel + (symbol.empty() ? "" : ":" + symbol));
}

void HollexWS::unsubscribeMultiple(const std::string& channel, const std::vector<std::string>& symbols) {
    for (const auto& symbol : symbols) {
        auto marketId = getMarketId(symbol);
        unsubscribe(channel, marketId);
    }
}

std::string HollexWS::getEndpoint(const std::string& type) {
    return exchange_.urls["api"]["ws"];
}

std::string HollexWS::getMarketId(const std::string& symbol) {
    auto market = exchange_.market(symbol);
    return market["id"];
}

std::string HollexWS::getSymbol(const std::string& marketId) {
    for (const auto& market : exchange_.markets) {
        if (market.second["id"] == marketId) {
            return market.first;
        }
    }
    return marketId;
}

std::string HollexWS::getChannel(const std::string& channel, const std::string& symbol) {
    return channel + ":" + symbol;
}

int HollexWS::getNextRequestId() {
    static int requestId = 1;
    return requestId++;
}

void HollexWS::handleMessage(const std::string& message) {
    auto j = nlohmann::json::parse(message);

    if (j.contains("event")) {
        auto event = j["event"].get<std::string>();
        
        if (event == "auth") {
            handleAuthenticationMessage(j);
        } else if (event == "subscribed") {
            handleSubscriptionMessage(j);
        } else if (event == "unsubscribed") {
            handleUnsubscriptionMessage(j);
        } else if (event == "error") {
            handleErrorMessage(j);
        }
    } else if (j.contains("channel")) {
        auto channel = j["channel"].get<std::string>();
        
        if (channel == "ticker") {
            handleTickerMessage(j);
        } else if (channel.find("orderbook") != std::string::npos) {
            handleOrderBookMessage(j);
        } else if (channel == "trades") {
            handleTradeMessage(j);
        } else if (channel.find("kline_") != std::string::npos) {
            handleOHLCVMessage(j);
        } else if (channel == "balance") {
            handleBalanceMessage(j);
        } else if (channel == "orders") {
            handleOrderMessage(j);
        } else if (channel == "user_trades") {
            handleMyTradeMessage(j);
        }
    }
}

void HollexWS::handleTickerMessage(const nlohmann::json& data) {
    // Implementation details
}

void HollexWS::handleOrderBookMessage(const nlohmann::json& data) {
    // Implementation details
}

void HollexWS::handleTradeMessage(const nlohmann::json& data) {
    // Implementation details
}

void HollexWS::handleOHLCVMessage(const nlohmann::json& data) {
    // Implementation details
}

void HollexWS::handleBalanceMessage(const nlohmann::json& data) {
    // Implementation details
}

void HollexWS::handleOrderMessage(const nlohmann::json& data) {
    // Implementation details
}

void HollexWS::handleMyTradeMessage(const nlohmann::json& data) {
    // Implementation details
}

void HollexWS::handleErrorMessage(const nlohmann::json& data) {
    throw ExchangeError(exchange_.id + " " + data["message"].get<std::string>());
}

void HollexWS::handleSubscriptionMessage(const nlohmann::json& data) {
    // Implementation details
}

void HollexWS::handleUnsubscriptionMessage(const nlohmann::json& data) {
    // Implementation details
}

void HollexWS::handleAuthenticationMessage(const nlohmann::json& data) {
    if (data["success"].get<bool>()) {
        authenticated_ = true;
    } else {
        throw AuthenticationError(exchange_.id + " authentication failed: " + data["message"].get<std::string>());
    }
}

Order HollexWS::parseWsOrder(const nlohmann::json& order, const Market* market) {
    auto id = order["orderId"].get<std::string>();
    auto timestamp = std::stoll(order["timestamp"].get<std::string>());
    auto type = order["type"].get<std::string>();
    auto side = order["side"].get<std::string>();
    auto marketId = order["symbol"].get<std::string>();
    auto symbol = market ? market->symbol : getSymbol(marketId);
    auto price = order["price"].get<std::string>();
    auto amount = order["amount"].get<std::string>();
    auto filled = order["filled"].get<std::string>();
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

Trade HollexWS::parseWsTrade(const nlohmann::json& trade, const Market* market) {
    auto id = trade["tradeId"].get<std::string>();
    auto timestamp = std::stoll(trade["timestamp"].get<std::string>());
    auto side = trade["side"].get<std::string>();
    auto marketId = trade["symbol"].get<std::string>();
    auto symbol = market ? market->symbol : getSymbol(marketId);
    auto price = trade["price"].get<std::string>();
    auto amount = trade["amount"].get<std::string>();
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
        {"takerOrMaker", trade["type"].get<std::string>()},
        {"price", price},
        {"amount", amount},
        {"cost", cost},
        {"fee", fee}
    };
}

} // namespace ccxt
