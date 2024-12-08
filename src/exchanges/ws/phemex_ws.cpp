#include "ccxt/exchanges/ws/phemex_ws.h"
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
#include <cmath>

namespace ccxt {

PhemexWS::PhemexWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Phemex& exchange)
    : WebSocketClient(ioc, ctx)
    , exchange_(exchange)
    , authenticated_(false) {
}

void PhemexWS::authenticate() {
    if (authenticated_) return;

    auto expiry = std::time(nullptr) + 60;
    auto message = "GET/auth" + std::to_string(expiry);
    auto signature = exchange_.hmac(message, exchange_.secret, "sha256", "hex");

    nlohmann::json request = {
        {"method", "user.auth"},
        {"params", {
            {"apiKey", exchange_.apiKey},
            {"expiry", expiry},
            {"signature", signature}
        }},
        {"id", getNextRequestId()}
    };

    send(request.dump());
}

void PhemexWS::watchTicker(const std::string& symbol, const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    subscribe("market." + marketId + ".tick", marketId);
}

void PhemexWS::watchTickers(const std::vector<std::string>& symbols, const std::map<std::string, std::string>& params) {
    subscribeMultiple("market.*.tick", symbols);
}

void PhemexWS::watchOrderBook(const std::string& symbol, int limit, const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    auto channel = limit > 0 ? "orderbook." + std::to_string(limit) : "orderbook";
    subscribe("market." + marketId + "." + channel, marketId);
}

void PhemexWS::watchTrades(const std::string& symbol, const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    subscribe("market." + marketId + ".trade", marketId);
}

void PhemexWS::watchOHLCV(const std::string& symbol, const std::string& timeframe,
                         const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    subscribe("market." + marketId + ".kline." + timeframe, marketId);
}

void PhemexWS::watchBalance(const std::map<std::string, std::string>& params) {
    authenticate();
    subscribe("account.balance", "", true);
}

void PhemexWS::watchOrders(const std::string& symbol, const std::map<std::string, std::string>& params) {
    authenticate();
    if (!symbol.empty()) {
        auto marketId = getMarketId(symbol);
        subscribe("order." + marketId, marketId, true);
    } else {
        subscribe("order.*", "", true);
    }
}

void PhemexWS::watchMyTrades(const std::string& symbol, const std::map<std::string, std::string>& params) {
    authenticate();
    if (!symbol.empty()) {
        auto marketId = getMarketId(symbol);
        subscribe("trade." + marketId, marketId, true);
    } else {
        subscribe("trade.*", "", true);
    }
}

void PhemexWS::watchPositions(const std::string& symbol, const std::map<std::string, std::string>& params) {
    authenticate();
    if (!symbol.empty()) {
        auto marketId = getMarketId(symbol);
        subscribe("position." + marketId, marketId, true);
    } else {
        subscribe("position.*", "", true);
    }
}

void PhemexWS::subscribe(const std::string& channel, const std::string& symbol, bool isPrivate) {
    nlohmann::json request = {
        {"method", "subscribe"},
        {"params", {channel}},
        {"id", getNextRequestId()}
    };

    send(request.dump());
    subscriptions_[channel] = symbol;
}

void PhemexWS::subscribeMultiple(const std::string& channel, const std::vector<std::string>& symbols, bool isPrivate) {
    std::vector<std::string> channels;
    
    for (const auto& symbol : symbols) {
        auto marketId = getMarketId(symbol);
        auto topic = channel;
        std::string::size_type pos = topic.find("*");
        if (pos != std::string::npos) {
            topic.replace(pos, 1, marketId);
        }
        channels.push_back(topic);
        subscriptions_[topic] = symbol;
    }

    nlohmann::json request = {
        {"method", "subscribe"},
        {"params", channels},
        {"id", getNextRequestId()}
    };

    send(request.dump());
}

void PhemexWS::unsubscribe(const std::string& channel, const std::string& symbol) {
    nlohmann::json request = {
        {"method", "unsubscribe"},
        {"params", {channel}},
        {"id", getNextRequestId()}
    };

    send(request.dump());
    subscriptions_.erase(channel);
}

void PhemexWS::unsubscribeMultiple(const std::string& channel, const std::vector<std::string>& symbols) {
    std::vector<std::string> channels;
    
    for (const auto& symbol : symbols) {
        auto marketId = getMarketId(symbol);
        auto topic = channel;
        std::string::size_type pos = topic.find("*");
        if (pos != std::string::npos) {
            topic.replace(pos, 1, marketId);
        }
        channels.push_back(topic);
        subscriptions_.erase(topic);
    }

    nlohmann::json request = {
        {"method", "unsubscribe"},
        {"params", channels},
        {"id", getNextRequestId()}
    };

    send(request.dump());
}

std::string PhemexWS::getEndpoint(const std::string& type) {
    return exchange_.urls["api"]["ws"];
}

std::string PhemexWS::getMarketId(const std::string& symbol) {
    auto market = exchange_.market(symbol);
    return market["id"];
}

std::string PhemexWS::getSymbol(const std::string& marketId) {
    for (const auto& market : exchange_.markets) {
        if (market.second["id"] == marketId) {
            return market.first;
        }
    }
    return marketId;
}

std::string PhemexWS::getChannel(const std::string& channel, const std::string& symbol) {
    return channel + "." + symbol;
}

std::string PhemexWS::getInstrumentType(const std::string& symbol) {
    auto market = exchange_.market(symbol);
    return market["type"];
}

int PhemexWS::getNextRequestId() {
    static int requestId = 1;
    return requestId++;
}

double PhemexWS::parseNumber(const std::string& value, double scale) {
    return std::stod(value) / std::pow(10, scale);
}

std::string PhemexWS::formatNumber(double value, double scale) {
    return std::to_string(static_cast<int64_t>(value * std::pow(10, scale)));
}

void PhemexWS::handleMessage(const std::string& message) {
    auto j = nlohmann::json::parse(message);

    if (j.contains("result")) {
        if (j.contains("method")) {
            auto method = j["method"].get<std::string>();
            if (method == "user.auth") {
                handleAuthenticationMessage(j);
            }
        }
    } else if (j.contains("error")) {
        handleErrorMessage(j);
    } else if (j.contains("type")) {
        auto type = j["type"].get<std::string>();
        
        if (type == "snapshot" || type == "incremental") {
            if (j.contains("market")) {
                auto market = j["market"].get<std::string>();
                if (market.find("tick") != std::string::npos) {
                    handleTickerMessage(j);
                } else if (market.find("orderbook") != std::string::npos) {
                    handleOrderBookMessage(j);
                } else if (market.find("trade") != std::string::npos) {
                    handleTradeMessage(j);
                } else if (market.find("kline") != std::string::npos) {
                    handleOHLCVMessage(j);
                }
            } else if (j.contains("account")) {
                handleBalanceMessage(j);
            } else if (j.contains("order")) {
                handleOrderMessage(j);
            } else if (j.contains("trade")) {
                handleMyTradeMessage(j);
            } else if (j.contains("position")) {
                handlePositionMessage(j);
            }
        }
    }
}

void PhemexWS::handleTickerMessage(const nlohmann::json& data) {
    // Implementation details
}

void PhemexWS::handleOrderBookMessage(const nlohmann::json& data) {
    // Implementation details
}

void PhemexWS::handleTradeMessage(const nlohmann::json& data) {
    // Implementation details
}

void PhemexWS::handleOHLCVMessage(const nlohmann::json& data) {
    // Implementation details
}

void PhemexWS::handleBalanceMessage(const nlohmann::json& data) {
    // Implementation details
}

void PhemexWS::handleOrderMessage(const nlohmann::json& data) {
    // Implementation details
}

void PhemexWS::handleMyTradeMessage(const nlohmann::json& data) {
    // Implementation details
}

void PhemexWS::handlePositionMessage(const nlohmann::json& data) {
    // Implementation details
}

void PhemexWS::handleErrorMessage(const nlohmann::json& data) {
    throw ExchangeError(exchange_.id + " " + data["error"]["message"].get<std::string>());
}

void PhemexWS::handleSubscriptionMessage(const nlohmann::json& data) {
    // Implementation details
}

void PhemexWS::handleUnsubscriptionMessage(const nlohmann::json& data) {
    // Implementation details
}

void PhemexWS::handleAuthenticationMessage(const nlohmann::json& data) {
    if (data["result"].get<bool>()) {
        authenticated_ = true;
    } else {
        throw AuthenticationError(exchange_.id + " authentication failed");
    }
}

Order PhemexWS::parseWsOrder(const nlohmann::json& order, const Market* market) {
    auto id = order["orderID"].get<std::string>();
    auto timestamp = std::stoll(order["transactTime"].get<std::string>());
    auto type = order["ordType"].get<std::string>();
    auto side = order["side"].get<std::string>();
    auto marketId = order["symbol"].get<std::string>();
    auto symbol = market ? market->symbol : getSymbol(marketId);
    auto priceScale = market ? market->price_scale : scales_[marketId];
    auto amountScale = market ? market->amount_scale : scales_[marketId];

    auto price = formatNumber(parseNumber(order["price"].get<std::string>(), priceScale), priceScale);
    auto amount = formatNumber(parseNumber(order["orderQty"].get<std::string>(), amountScale), amountScale);
    auto filled = formatNumber(parseNumber(order["cumQty"].get<std::string>(), amountScale), amountScale);
    auto remaining = std::to_string(std::stod(amount) - std::stod(filled));
    auto status = exchange_.parseOrderStatus(order["ordStatus"].get<std::string>());
    auto clientOrderId = order.contains("clOrdID") ? order["clOrdID"].get<std::string>() : "";

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

Trade PhemexWS::parseWsTrade(const nlohmann::json& trade, const Market* market) {
    auto id = trade["tradeID"].get<std::string>();
    auto timestamp = std::stoll(trade["transactTime"].get<std::string>());
    auto side = trade["side"].get<std::string>();
    auto marketId = trade["symbol"].get<std::string>();
    auto symbol = market ? market->symbol : getSymbol(marketId);
    auto priceScale = market ? market->price_scale : scales_[marketId];
    auto amountScale = market ? market->amount_scale : scales_[marketId];

    auto price = formatNumber(parseNumber(trade["price"].get<std::string>(), priceScale), priceScale);
    auto amount = formatNumber(parseNumber(trade["qty"].get<std::string>(), amountScale), amountScale);
    auto cost = std::to_string(std::stod(price) * std::stod(amount));
    auto orderId = trade.contains("orderID") ? trade["orderID"].get<std::string>() : "";

    nlohmann::json fee;
    if (trade.contains("fee") && trade.contains("feeCurrency")) {
        fee = {
            {"cost", formatNumber(parseNumber(trade["fee"].get<std::string>(), 8), 8)},
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
        {"takerOrMaker", trade["execType"].get<std::string>()},
        {"price", price},
        {"amount", amount},
        {"cost", cost},
        {"fee", fee}
    };
}

Position PhemexWS::parseWsPosition(const nlohmann::json& position, const Market* market) {
    auto marketId = position["symbol"].get<std::string>();
    auto symbol = market ? market->symbol : getSymbol(marketId);
    auto timestamp = std::stoll(position["timestamp"].get<std::string>());
    auto side = position["side"].get<std::string>();
    auto priceScale = market ? market->price_scale : scales_[marketId];
    auto amountScale = market ? market->amount_scale : scales_[marketId];

    auto contracts = formatNumber(parseNumber(position["size"].get<std::string>(), amountScale), amountScale);
    auto entryPrice = formatNumber(parseNumber(position["avgEntry"].get<std::string>(), priceScale), priceScale);
    auto notional = std::to_string(std::stod(contracts) * std::stod(entryPrice));

    return {
        {"info", position},
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", exchange_.iso8601(timestamp)},
        {"side", side},
        {"contracts", contracts},
        {"contractSize", position["contractSize"].get<std::string>()},
        {"entryPrice", entryPrice},
        {"markPrice", formatNumber(parseNumber(position["markPrice"].get<std::string>(), priceScale), priceScale)},
        {"notional", notional},
        {"leverage", position["leverage"].get<std::string>()},
        {"collateral", formatNumber(parseNumber(position["positionMargin"].get<std::string>(), 8), 8)},
        {"initialMargin", nullptr},
        {"maintenanceMargin", nullptr},
        {"initialMarginPercentage", nullptr},
        {"maintenanceMarginPercentage", nullptr},
        {"unrealizedPnl", formatNumber(parseNumber(position["unrealisedPnl"].get<std::string>(), 8), 8)},
        {"liquidationPrice", formatNumber(parseNumber(position["liquidationPrice"].get<std::string>(), priceScale), priceScale)},
        {"marginMode", position["crossMargin"].get<bool>() ? "cross" : "isolated"},
        {"percentage", nullptr}
    };
}

} // namespace ccxt
