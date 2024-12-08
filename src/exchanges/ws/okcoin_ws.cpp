#include "ccxt/exchanges/ws/okcoin_ws.h"
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

OKCoinWS::OKCoinWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, OKCoin& exchange)
    : WebSocketClient(ioc, ctx)
    , exchange_(exchange)
    , authenticated_(false) {
}

void OKCoinWS::authenticate() {
    if (authenticated_) return;

    auto timestamp = std::to_string(std::time(nullptr));
    auto method = "GET";
    auto requestPath = "/users/self/verify";
    auto message = timestamp + method + requestPath;
    auto signature = exchange_.hmac(message, exchange_.secret, "sha256", "base64");

    nlohmann::json request = {
        {"op", "login"},
        {"args", {{
            {"apiKey", exchange_.apiKey},
            {"passphrase", exchange_.password},
            {"timestamp", timestamp},
            {"sign", signature}
        }}}
    };

    send(request.dump());
}

void OKCoinWS::watchTicker(const std::string& symbol, const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    auto instType = getInstrumentType(symbol);
    subscribe("tickers", marketId);
}

void OKCoinWS::watchTickers(const std::vector<std::string>& symbols, const std::map<std::string, std::string>& params) {
    subscribeMultiple("tickers", symbols);
}

void OKCoinWS::watchOrderBook(const std::string& symbol, int limit, const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    auto channel = limit > 0 ? "books" + std::to_string(limit) : "books";
    subscribe(channel, marketId);
}

void OKCoinWS::watchTrades(const std::string& symbol, const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    subscribe("trades", marketId);
}

void OKCoinWS::watchOHLCV(const std::string& symbol, const std::string& timeframe, 
                         const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    auto channel = "candle" + timeframe;
    subscribe(channel, marketId);
}

void OKCoinWS::watchBalance(const std::map<std::string, std::string>& params) {
    authenticate();
    subscribe("account", "", true);
}

void OKCoinWS::watchOrders(const std::string& symbol, const std::map<std::string, std::string>& params) {
    authenticate();
    if (!symbol.empty()) {
        auto marketId = getMarketId(symbol);
        subscribe("orders", marketId, true);
    } else {
        subscribe("orders", "", true);
    }
}

void OKCoinWS::watchMyTrades(const std::string& symbol, const std::map<std::string, std::string>& params) {
    authenticate();
    if (!symbol.empty()) {
        auto marketId = getMarketId(symbol);
        subscribe("trades", marketId, true);
    } else {
        subscribe("trades", "", true);
    }
}

void OKCoinWS::watchPositions(const std::string& symbol, const std::map<std::string, std::string>& params) {
    authenticate();
    if (!symbol.empty()) {
        auto marketId = getMarketId(symbol);
        subscribe("positions", marketId, true);
    } else {
        subscribe("positions", "", true);
    }
}

void OKCoinWS::subscribe(const std::string& channel, const std::string& symbol, bool isPrivate) {
    std::string instType = symbol.empty() ? "SPOT" : getInstrumentType(symbol);
    nlohmann::json args;
    
    if (symbol.empty()) {
        args = {
            {"channel", channel},
            {"instType", instType}
        };
    } else {
        args = {
            {"channel", channel},
            {"instId", symbol}
        };
    }

    nlohmann::json request = {
        {"op", "subscribe"},
        {"args", {args}}
    };

    send(request.dump());
    subscriptions_[channel + ":" + symbol] = symbol;
}

void OKCoinWS::subscribeMultiple(const std::string& channel, const std::vector<std::string>& symbols, bool isPrivate) {
    std::vector<nlohmann::json> args;
    
    for (const auto& symbol : symbols) {
        auto marketId = getMarketId(symbol);
        args.push_back({
            {"channel", channel},
            {"instId", marketId}
        });
        subscriptions_[channel + ":" + marketId] = symbol;
    }

    nlohmann::json request = {
        {"op", "subscribe"},
        {"args", args}
    };

    send(request.dump());
}

void OKCoinWS::unsubscribe(const std::string& channel, const std::string& symbol) {
    nlohmann::json args;
    
    if (symbol.empty()) {
        args = {
            {"channel", channel},
            {"instType", "SPOT"}
        };
    } else {
        args = {
            {"channel", channel},
            {"instId", symbol}
        };
    }

    nlohmann::json request = {
        {"op", "unsubscribe"},
        {"args", {args}}
    };

    send(request.dump());
    subscriptions_.erase(channel + ":" + symbol);
}

void OKCoinWS::unsubscribeMultiple(const std::string& channel, const std::vector<std::string>& symbols) {
    std::vector<nlohmann::json> args;
    
    for (const auto& symbol : symbols) {
        auto marketId = getMarketId(symbol);
        args.push_back({
            {"channel", channel},
            {"instId", marketId}
        });
        subscriptions_.erase(channel + ":" + marketId);
    }

    nlohmann::json request = {
        {"op", "unsubscribe"},
        {"args", args}
    };

    send(request.dump());
}

std::string OKCoinWS::getEndpoint(const std::string& type) {
    return exchange_.urls["api"]["ws"];
}

std::string OKCoinWS::getMarketId(const std::string& symbol) {
    auto market = exchange_.market(symbol);
    return market["id"];
}

std::string OKCoinWS::getSymbol(const std::string& marketId) {
    for (const auto& market : exchange_.markets) {
        if (market.second["id"] == marketId) {
            return market.first;
        }
    }
    return marketId;
}

std::string OKCoinWS::getChannel(const std::string& channel, const std::string& symbol) {
    return channel + ":" + symbol;
}

std::string OKCoinWS::getInstrumentType(const std::string& symbol) {
    auto market = exchange_.market(symbol);
    return market["type"];
}

int OKCoinWS::getNextRequestId() {
    static int requestId = 1;
    return requestId++;
}

void OKCoinWS::handleMessage(const std::string& message) {
    auto j = nlohmann::json::parse(message);

    if (j.contains("event")) {
        auto event = j["event"].get<std::string>();
        
        if (event == "login") {
            handleLoginMessage(j);
        } else if (event == "subscribe") {
            handleSubscriptionMessage(j);
        } else if (event == "unsubscribe") {
            handleUnsubscriptionMessage(j);
        } else if (event == "error") {
            handleErrorMessage(j);
        }
    } else if (j.contains("arg") && j.contains("data")) {
        auto channel = j["arg"]["channel"].get<std::string>();
        
        if (channel == "tickers") {
            handleTickerMessage(j);
        } else if (channel.find("books") != std::string::npos) {
            handleOrderBookMessage(j);
        } else if (channel == "trades") {
            handleTradeMessage(j);
        } else if (channel.find("candle") != std::string::npos) {
            handleOHLCVMessage(j);
        } else if (channel == "account") {
            handleBalanceMessage(j);
        } else if (channel == "orders") {
            handleOrderMessage(j);
        } else if (channel == "positions") {
            handlePositionMessage(j);
        }
    }
}

void OKCoinWS::handleTickerMessage(const nlohmann::json& data) {
    // Implementation details
}

void OKCoinWS::handleOrderBookMessage(const nlohmann::json& data) {
    // Implementation details
}

void OKCoinWS::handleTradeMessage(const nlohmann::json& data) {
    // Implementation details
}

void OKCoinWS::handleOHLCVMessage(const nlohmann::json& data) {
    // Implementation details
}

void OKCoinWS::handleBalanceMessage(const nlohmann::json& data) {
    // Implementation details
}

void OKCoinWS::handleOrderMessage(const nlohmann::json& data) {
    // Implementation details
}

void OKCoinWS::handleMyTradeMessage(const nlohmann::json& data) {
    // Implementation details
}

void OKCoinWS::handlePositionMessage(const nlohmann::json& data) {
    // Implementation details
}

void OKCoinWS::handleErrorMessage(const nlohmann::json& data) {
    throw ExchangeError(exchange_.id + " " + data["msg"].get<std::string>());
}

void OKCoinWS::handleSubscriptionMessage(const nlohmann::json& data) {
    // Implementation details
}

void OKCoinWS::handleUnsubscriptionMessage(const nlohmann::json& data) {
    // Implementation details
}

void OKCoinWS::handleAuthenticationMessage(const nlohmann::json& data) {
    authenticated_ = true;
}

void OKCoinWS::handleLoginMessage(const nlohmann::json& data) {
    if (data["code"].get<std::string>() == "0") {
        authenticated_ = true;
    } else {
        throw AuthenticationError(exchange_.id + " authentication failed: " + data["msg"].get<std::string>());
    }
}

Order OKCoinWS::parseWsOrder(const nlohmann::json& order, const Market* market) {
    auto id = order["ordId"].get<std::string>();
    auto timestamp = std::stoll(order["cTime"].get<std::string>());
    auto type = order["ordType"].get<std::string>();
    auto side = order["side"].get<std::string>();
    auto marketId = order["instId"].get<std::string>();
    auto symbol = market ? market->symbol : getSymbol(marketId);
    auto price = order["px"].get<std::string>();
    auto amount = order["sz"].get<std::string>();
    auto filled = order["accFillSz"].get<std::string>();
    auto remaining = std::to_string(std::stod(amount) - std::stod(filled));
    auto status = exchange_.parseOrderStatus(order["state"].get<std::string>());
    auto clientOrderId = order.contains("clOrdId") ? order["clOrdId"].get<std::string>() : "";

    return {
        {"id", id},
        {"clientOrderId", clientOrderId},
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

Trade OKCoinWS::parseWsTrade(const nlohmann::json& trade, const Market* market) {
    auto id = trade["tradeId"].get<std::string>();
    auto timestamp = std::stoll(trade["ts"].get<std::string>());
    auto side = trade["side"].get<std::string>();
    auto marketId = trade["instId"].get<std::string>();
    auto symbol = market ? market->symbol : getSymbol(marketId);
    auto price = trade["px"].get<std::string>();
    auto amount = trade["sz"].get<std::string>();
    auto cost = std::to_string(std::stod(price) * std::stod(amount));
    auto orderId = trade.contains("ordId") ? trade["ordId"].get<std::string>() : "";

    nlohmann::json fee;
    if (trade.contains("fee") && trade.contains("feeCcy")) {
        fee = {
            {"cost", trade["fee"].get<std::string>()},
            {"currency", trade["feeCcy"].get<std::string>()}
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
        {"takerOrMaker", nullptr},
        {"price", price},
        {"amount", amount},
        {"cost", cost},
        {"fee", fee}
    };
}

Position OKCoinWS::parseWsPosition(const nlohmann::json& position, const Market* market) {
    auto marketId = position["instId"].get<std::string>();
    auto symbol = market ? market->symbol : getSymbol(marketId);
    auto timestamp = std::stoll(position["uTime"].get<std::string>());
    auto side = position["posSide"].get<std::string>();
    auto amount = position["pos"].get<std::string>();
    auto price = position["avgPx"].get<std::string>();
    auto cost = std::to_string(std::stod(price) * std::stod(amount));

    return {
        {"info", position},
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", exchange_.iso8601(timestamp)},
        {"side", side},
        {"contracts", amount},
        {"contractSize", nullptr},
        {"entryPrice", price},
        {"markPrice", nullptr},
        {"notional", cost},
        {"leverage", position["lever"].get<std::string>()},
        {"collateral", nullptr},
        {"initialMargin", nullptr},
        {"maintenanceMargin", nullptr},
        {"initialMarginPercentage", nullptr},
        {"maintenanceMarginPercentage", nullptr},
        {"unrealizedPnl", position["upl"].get<std::string>()},
        {"liquidationPrice", nullptr},
        {"marginMode", position["mgnMode"].get<std::string>()},
        {"percentage", nullptr}
    };
}

} // namespace ccxt
