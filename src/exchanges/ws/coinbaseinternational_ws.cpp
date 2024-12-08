#include "ccxt/exchanges/ws/coinbaseinternational_ws.h"
#include "ccxt/error.h"
#include "ccxt/json.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <chrono>
#include <ctime>
#include <sstream>

namespace ccxt {

CoinbaseInternationalWS::CoinbaseInternationalWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, CoinbaseInternational& exchange)
    : WebSocketClient(ioc, ctx)
    , exchange_(exchange)
    , authenticated_(false) {
}

void CoinbaseInternationalWS::authenticate() {
    if (authenticated_) {
        return;
    }

    auto timestamp = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    
    std::string method = "GET";
    std::string path = "/ws";
    std::string body = "";

    std::string signature = exchange_.hmac(timestamp + method + path + body, exchange_.secret, "sha256");

    nlohmann::json auth = {
        {"type", "subscribe"},
        {"channel", "auth"},
        {"api_key", exchange_.apiKey},
        {"timestamp", timestamp},
        {"signature", signature}
    };

    send(auth.dump());
}

void CoinbaseInternationalWS::subscribe(const std::string& channel, const std::string& symbol, bool isPrivate) {
    if (isPrivate && !authenticated_) {
        authenticate();
    }

    nlohmann::json request = {
        {"type", "subscribe"},
        {"channel", channel}
    };

    if (!symbol.empty()) {
        request["product_ids"] = {symbol};
    }

    send(request.dump());
}

void CoinbaseInternationalWS::subscribeMultiple(const std::string& channel, const std::vector<std::string>& symbols, bool isPrivate) {
    if (isPrivate && !authenticated_) {
        authenticate();
    }

    nlohmann::json request = {
        {"type", "subscribe"},
        {"channel", channel},
        {"product_ids", symbols}
    };

    send(request.dump());
}

void CoinbaseInternationalWS::unsubscribe(const std::string& channel, const std::string& symbol) {
    nlohmann::json request = {
        {"type", "unsubscribe"},
        {"channel", channel}
    };

    if (!symbol.empty()) {
        request["product_ids"] = {symbol};
    }

    send(request.dump());
}

void CoinbaseInternationalWS::unsubscribeAll() {
    for (const auto& subscription : subscriptions_) {
        unsubscribe(subscription.first, subscription.second);
    }
    subscriptions_.clear();
}

void CoinbaseInternationalWS::watchTicker(const std::string& symbol, const std::map<std::string, std::string>& params) {
    subscribe("ticker", symbol);
}

void CoinbaseInternationalWS::watchTickers(const std::vector<std::string>& symbols, const std::map<std::string, std::string>& params) {
    subscribeMultiple("ticker", symbols);
}

void CoinbaseInternationalWS::watchOrderBook(const std::string& symbol, const std::map<std::string, std::string>& params) {
    subscribe("level2", symbol);
}

void CoinbaseInternationalWS::watchTrades(const std::string& symbol, const std::map<std::string, std::string>& params) {
    subscribe("matches", symbol);
}

void CoinbaseInternationalWS::watchOHLCV(const std::string& symbol, const std::string& timeframe, const std::map<std::string, std::string>& params) {
    subscribe("candles", symbol);
}

void CoinbaseInternationalWS::watchBalance(const std::map<std::string, std::string>& params) {
    subscribe("balances", "", true);
}

void CoinbaseInternationalWS::watchOrders(const std::string& symbol, const std::map<std::string, std::string>& params) {
    subscribe("orders", symbol, true);
}

void CoinbaseInternationalWS::watchMyTrades(const std::string& symbol, const std::map<std::string, std::string>& params) {
    subscribe("user_trades", symbol, true);
}

void CoinbaseInternationalWS::watchPositions(const std::vector<std::string>& symbols, const std::map<std::string, std::string>& params) {
    subscribe("positions", "", true);
}

void CoinbaseInternationalWS::handleMessage(const std::string& message) {
    auto data = nlohmann::json::parse(message);
    auto type = data["type"].get<std::string>();

    if (type == "error") {
        handleErrorMessage(data);
    } else if (type == "subscribed") {
        handleSubscriptionMessage(data);
    } else if (type == "unsubscribed") {
        handleUnsubscriptionMessage(data);
    } else if (type == "authenticated") {
        handleAuthenticationMessage(data);
    } else if (type == "heartbeat") {
        handleHeartbeat(data);
    } else {
        auto channel = data["channel"].get<std::string>();
        
        if (channel == "ticker") {
            handleTickerMessage(data);
        } else if (channel == "level2") {
            handleOrderBookMessage(data);
        } else if (channel == "matches") {
            handleTradeMessage(data);
        } else if (channel == "candles") {
            handleOHLCVMessage(data);
        } else if (channel == "balances") {
            handleBalanceMessage(data);
        } else if (channel == "orders") {
            handleOrderMessage(data);
        } else if (channel == "user_trades") {
            handleMyTradeMessage(data);
        } else if (channel == "positions") {
            handlePositionMessage(data);
        }
    }
}

void CoinbaseInternationalWS::handleTickerMessage(const nlohmann::json& data) {
    // Implementation for ticker message handling
}

void CoinbaseInternationalWS::handleOrderBookMessage(const nlohmann::json& data) {
    // Implementation for order book message handling
}

void CoinbaseInternationalWS::handleTradeMessage(const nlohmann::json& data) {
    // Implementation for trade message handling
}

void CoinbaseInternationalWS::handleOHLCVMessage(const nlohmann::json& data) {
    // Implementation for OHLCV message handling
}

void CoinbaseInternationalWS::handleBalanceMessage(const nlohmann::json& data) {
    // Implementation for balance message handling
}

void CoinbaseInternationalWS::handleOrderMessage(const nlohmann::json& data) {
    // Implementation for order message handling
}

void CoinbaseInternationalWS::handleMyTradeMessage(const nlohmann::json& data) {
    // Implementation for user trade message handling
}

void CoinbaseInternationalWS::handlePositionMessage(const nlohmann::json& data) {
    // Implementation for position message handling
}

void CoinbaseInternationalWS::handleErrorMessage(const nlohmann::json& data) {
    throw ExchangeError(data["message"].get<std::string>());
}

void CoinbaseInternationalWS::handleSubscriptionMessage(const nlohmann::json& data) {
    auto channel = data["channel"].get<std::string>();
    if (data.contains("product_ids")) {
        for (const auto& symbol : data["product_ids"]) {
            subscriptions_[channel] = symbol;
        }
    } else {
        subscriptions_[channel] = "";
    }
}

void CoinbaseInternationalWS::handleUnsubscriptionMessage(const nlohmann::json& data) {
    auto channel = data["channel"].get<std::string>();
    subscriptions_.erase(channel);
}

void CoinbaseInternationalWS::handleAuthenticationMessage(const nlohmann::json& data) {
    authenticated_ = true;
}

void CoinbaseInternationalWS::handleHeartbeat(const nlohmann::json& data) {
    // Handle heartbeat message if needed
}

std::string CoinbaseInternationalWS::getEndpoint(const std::string& type) {
    return "wss://ws.coinbase.com/";
}

std::string CoinbaseInternationalWS::getMarketId(const std::string& symbol) {
    return exchange_.market_id(symbol);
}

std::string CoinbaseInternationalWS::getSymbol(const std::string& marketId) {
    return exchange_.market(marketId)["symbol"];
}

std::string CoinbaseInternationalWS::getChannel(const std::string& channel, const std::string& symbol) {
    return channel + (symbol.empty() ? "" : ":" + symbol);
}

std::map<std::string, std::string> CoinbaseInternationalWS::parseMarket(const std::string& marketId) {
    return exchange_.markets_by_id[marketId];
}

std::string CoinbaseInternationalWS::parseTimeframe(const std::string& timeframe) {
    static const std::map<std::string, std::string> timeframes = {
        {"1m", "1min"},
        {"5m", "5min"},
        {"15m", "15min"},
        {"30m", "30min"},
        {"1h", "1hour"},
        {"4h", "4hour"},
        {"1d", "1day"}
    };
    return timeframes.at(timeframe);
}

} // namespace ccxt
