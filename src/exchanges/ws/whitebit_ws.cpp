#include "ccxt/exchanges/ws/whitebit_ws.h"
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

WhiteBitWS::WhiteBitWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, WhiteBit& exchange)
    : WebSocketClient(ioc, ctx)
    , exchange_(exchange)
    , authenticated_(false) {
}

void WhiteBitWS::authenticate() {
    if (authenticated_) return;

    auto timestamp = std::to_string(std::time(nullptr));
    auto nonce = timestamp + "000";
    auto message = nonce + exchange_.apiKey;
    auto signature = exchange_.hmac(message, exchange_.secret, "sha512", "hex");

    nlohmann::json request = {
        {"id", getNextRequestId()},
        {"method", "authorize"},
        {"params", {
            {"request", "/api/v4/trade-account/ws/balance"},
            {"nonce", nonce},
            {"api_key", exchange_.apiKey},
            {"signature", signature}
        }}
    };

    send(request.dump());
}

void WhiteBitWS::watchTicker(const std::string& symbol, const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    subscribe("marketData", marketId);
}

void WhiteBitWS::watchTickers(const std::vector<std::string>& symbols, const std::map<std::string, std::string>& params) {
    subscribeMultiple("marketData", symbols);
}

void WhiteBitWS::watchOrderBook(const std::string& symbol, int limit, const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    auto channel = limit > 0 ? "depth" + std::to_string(limit) : "depth";
    subscribe(channel, marketId);
}

void WhiteBitWS::watchTrades(const std::string& symbol, const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    subscribe("trades", marketId);
}

void WhiteBitWS::watchOHLCV(const std::string& symbol, const std::string& timeframe,
                           const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    subscribe("kline_" + timeframe, marketId);
}

void WhiteBitWS::watchBalance(const std::map<std::string, std::string>& params) {
    authenticate();
    subscribe("balanceSpot", "", true);
}

void WhiteBitWS::watchOrders(const std::string& symbol, const std::map<std::string, std::string>& params) {
    authenticate();
    if (!symbol.empty()) {
        auto marketId = getMarketId(symbol);
        subscribe("activeOrders", marketId, true);
    } else {
        subscribe("activeOrders", "", true);
    }
}

void WhiteBitWS::watchMyTrades(const std::string& symbol, const std::map<std::string, std::string>& params) {
    authenticate();
    if (!symbol.empty()) {
        auto marketId = getMarketId(symbol);
        subscribe("executedOrders", marketId, true);
    } else {
        subscribe("executedOrders", "", true);
    }
}

void WhiteBitWS::subscribe(const std::string& channel, const std::string& symbol, bool isPrivate) {
    nlohmann::json request = {
        {"id", getNextRequestId()},
        {"method", "subscribe"}
    };

    if (!symbol.empty()) {
        request["params"] = {channel, symbol};
    } else {
        request["params"] = {channel};
    }

    send(request.dump());
    subscriptions_[channel + (symbol.empty() ? "" : ":" + symbol)] = symbol;
}

void WhiteBitWS::subscribeMultiple(const std::string& channel, const std::vector<std::string>& symbols, bool isPrivate) {
    for (const auto& symbol : symbols) {
        auto marketId = getMarketId(symbol);
        subscribe(channel, marketId, isPrivate);
    }
}

void WhiteBitWS::unsubscribe(const std::string& channel, const std::string& symbol) {
    nlohmann::json request = {
        {"id", getNextRequestId()},
        {"method", "unsubscribe"}
    };

    if (!symbol.empty()) {
        request["params"] = {channel, symbol};
    } else {
        request["params"] = {channel};
    }

    send(request.dump());
    subscriptions_.erase(channel + (symbol.empty() ? "" : ":" + symbol));
}

void WhiteBitWS::unsubscribeMultiple(const std::string& channel, const std::vector<std::string>& symbols) {
    for (const auto& symbol : symbols) {
        auto marketId = getMarketId(symbol);
        unsubscribe(channel, marketId);
    }
}

std::string WhiteBitWS::getEndpoint(const std::string& type) {
    return exchange_.urls["api"]["ws"];
}

std::string WhiteBitWS::getMarketId(const std::string& symbol) {
    auto market = exchange_.market(symbol);
    return market["id"];
}

std::string WhiteBitWS::getSymbol(const std::string& marketId) {
    for (const auto& market : exchange_.markets) {
        if (market.second["id"] == marketId) {
            return market.first;
        }
    }
    return marketId;
}

std::string WhiteBitWS::getChannel(const std::string& channel, const std::string& symbol) {
    return channel + ":" + symbol;
}

int WhiteBitWS::getNextRequestId() {
    static int requestId = 1;
    return requestId++;
}

void WhiteBitWS::handleMessage(const std::string& message) {
    auto j = nlohmann::json::parse(message);

    if (j.contains("method")) {
        auto method = j["method"].get<std::string>();
        
        if (method == "authorize") {
            handleAuthenticationMessage(j);
        } else if (method == "subscribe") {
            handleSubscriptionMessage(j);
        } else if (method == "unsubscribe") {
            handleUnsubscriptionMessage(j);
        }
    } else if (j.contains("error")) {
        handleErrorMessage(j);
    } else if (j.contains("params")) {
        auto channel = j["params"][0].get<std::string>();
        
        if (channel == "marketData") {
            handleTickerMessage(j);
        } else if (channel.find("depth") != std::string::npos) {
            handleOrderBookMessage(j);
        } else if (channel == "trades") {
            handleTradeMessage(j);
        } else if (channel.find("kline_") != std::string::npos) {
            handleOHLCVMessage(j);
        } else if (channel == "balanceSpot") {
            handleBalanceMessage(j);
        } else if (channel == "activeOrders") {
            handleOrderMessage(j);
        } else if (channel == "executedOrders") {
            handleMyTradeMessage(j);
        }
    }
}

void WhiteBitWS::handleTickerMessage(const nlohmann::json& data) {
    auto params = data["params"];
    auto marketId = params[1].get<std::string>();
    auto symbol = getSymbol(marketId);
    auto tickerData = params[2];
    
    nlohmann::json ticker = {
        {"symbol", symbol},
        {"timestamp", std::time(nullptr) * 1000},
        {"datetime", exchange_.iso8601(std::time(nullptr) * 1000)},
        {"high", std::stod(tickerData["high"].get<std::string>())},
        {"low", std::stod(tickerData["low"].get<std::string>())},
        {"bid", std::stod(tickerData["bid"].get<std::string>())},
        {"bidVolume", std::stod(tickerData["bidVolume"].get<std::string>())},
        {"ask", std::stod(tickerData["ask"].get<std::string>())},
        {"askVolume", std::stod(tickerData["askVolume"].get<std::string>())},
        {"vwap", std::stod(tickerData["vwap"].get<std::string>())},
        {"open", std::stod(tickerData["open"].get<std::string>())},
        {"close", std::stod(tickerData["last"].get<std::string>())},
        {"last", std::stod(tickerData["last"].get<std::string>())},
        {"previousClose", nullptr},
        {"change", std::stod(tickerData["change"].get<std::string>())},
        {"percentage", std::stod(tickerData["priceChange"].get<std::string>())},
        {"average", std::stod(tickerData["average"].get<std::string>())},
        {"baseVolume", std::stod(tickerData["volume"].get<std::string>())},
        {"quoteVolume", std::stod(tickerData["quoteVolume"].get<std::string>())},
        {"info", tickerData}
    };
    
    emit(symbol, "ticker", ticker);
}

void WhiteBitWS::handleOrderBookMessage(const nlohmann::json& data) {
    auto params = data["params"];
    auto marketId = params[1].get<std::string>();
    auto symbol = getSymbol(marketId);
    auto bookData = params[2];
    
    std::vector<std::vector<double>> bids;
    std::vector<std::vector<double>> asks;
    
    for (const auto& bid : bookData["bids"]) {
        bids.push_back({
            std::stod(bid[0].get<std::string>()),  // price
            std::stod(bid[1].get<std::string>())   // amount
        });
    }
    
    for (const auto& ask : bookData["asks"]) {
        asks.push_back({
            std::stod(ask[0].get<std::string>()),  // price
            std::stod(ask[1].get<std::string>())   // amount
        });
    }
    
    nlohmann::json orderbook = {
        {"symbol", symbol},
        {"bids", bids},
        {"asks", asks},
        {"timestamp", std::time(nullptr) * 1000},
        {"datetime", exchange_.iso8601(std::time(nullptr) * 1000)},
        {"nonce", bookData["timestamp"].get<int64_t>()}
    };
    
    emit(symbol, "orderbook", orderbook);
}

void WhiteBitWS::handleTradeMessage(const nlohmann::json& data) {
    auto params = data["params"];
    auto marketId = params[1].get<std::string>();
    auto symbol = getSymbol(marketId);
    auto trades = params[2];
    
    for (const auto& t : trades) {
        auto trade = parseWsTrade(t);
        emit(symbol, "trade", trade);
    }
}

void WhiteBitWS::handleOHLCVMessage(const nlohmann::json& data) {
    auto params = data["params"];
    auto marketId = params[1].get<std::string>();
    auto symbol = getSymbol(marketId);
    auto kline = params[2];
    
    nlohmann::json ohlcv = {
        {"timestamp", std::stoll(kline["timestamp"].get<std::string>())},
        {"open", std::stod(kline["open"].get<std::string>())},
        {"high", std::stod(kline["high"].get<std::string>())},
        {"low", std::stod(kline["low"].get<std::string>())},
        {"close", std::stod(kline["close"].get<std::string>())},
        {"volume", std::stod(kline["volume"].get<std::string>())}
    };
    
    emit(symbol, "ohlcv", ohlcv);
}

void WhiteBitWS::handleBalanceMessage(const nlohmann::json& data) {
    auto balances = data["params"][1];
    nlohmann::json result;
    
    for (const auto& balance : balances.items()) {
        auto currency = balance.key();
        auto value = balance.value();
        
        result[currency] = {
            {"free", std::stod(value["available"].get<std::string>())},
            {"used", std::stod(value["freeze"].get<std::string>())},
            {"total", std::stod(value["available"].get<std::string>()) + std::stod(value["freeze"].get<std::string>())}
        };
    }
    
    emit("", "balance", result);
}

void WhiteBitWS::handleOrderMessage(const nlohmann::json& data) {
    auto params = data["params"];
    auto marketId = params[1].get<std::string>();
    auto symbol = getSymbol(marketId);
    auto orderData = params[2];
    
    auto order = parseWsOrder(orderData);
    emit(symbol, "order", order);
}

void WhiteBitWS::handleMyTradeMessage(const nlohmann::json& data) {
    auto params = data["params"];
    auto marketId = params[1].get<std::string>();
    auto symbol = getSymbol(marketId);
    auto tradeData = params[2];
    
    auto trade = parseWsTrade(tradeData);
    emit(symbol, "trade", trade);
}

void WhiteBitWS::handleErrorMessage(const nlohmann::json& data) {
    throw ExchangeError(exchange_.id + " " + data["error"]["message"].get<std::string>());
}

void WhiteBitWS::handleSubscriptionMessage(const nlohmann::json& data) {
    // Implementation details
}

void WhiteBitWS::handleUnsubscriptionMessage(const nlohmann::json& data) {
    // Implementation details
}

void WhiteBitWS::handleAuthenticationMessage(const nlohmann::json& data) {
    if (data.contains("result") && data["result"].get<bool>()) {
        authenticated_ = true;
    } else {
        throw AuthenticationError(exchange_.id + " authentication failed: " + data["error"]["message"].get<std::string>());
    }
}

Order WhiteBitWS::parseWsOrder(const nlohmann::json& order, const Market* market) {
    auto id = order["orderId"].get<std::string>();
    auto timestamp = std::stoll(order["timestamp"].get<std::string>());
    auto type = order["type"].get<std::string>();
    auto side = order["side"].get<std::string>();
    auto marketId = order["market"].get<std::string>();
    auto symbol = market ? market->symbol : getSymbol(marketId);
    auto price = order["price"].get<std::string>();
    auto amount = order["amount"].get<std::string>();
    auto filled = order["dealAmount"].get<std::string>();
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

Trade WhiteBitWS::parseWsTrade(const nlohmann::json& trade, const Market* market) {
    auto id = trade["tradeId"].get<std::string>();
    auto timestamp = std::stoll(trade["timestamp"].get<std::string>());
    auto side = trade["side"].get<std::string>();
    auto marketId = trade["market"].get<std::string>();
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
        {"takerOrMaker", trade["role"].get<std::string>()},
        {"price", price},
        {"amount", amount},
        {"cost", cost},
        {"fee", fee}
    };
}

} // namespace ccxt
