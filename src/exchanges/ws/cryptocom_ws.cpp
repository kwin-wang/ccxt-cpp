#include "ccxt/exchanges/ws/cryptocom_ws.h"
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

CryptocomWS::CryptocomWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Cryptocom& exchange)
    : WebSocketClient(ioc, ctx)
    , exchange_(exchange)
    , authenticated_(false)
    , requestId_(1) {
}

void CryptocomWS::authenticate() {
    if (authenticated_) return;

    auto timestamp = std::to_string(std::time(nullptr) * 1000);
    auto method = "user.auth";
    auto id = getNextRequestId();

    nlohmann::json params = {
        {"api_key", exchange_.apiKey},
        {"timestamp", timestamp},
        {"sign", exchange_.sign(timestamp)}
    };

    nlohmann::json request = {
        {"id", id},
        {"method", method},
        {"params", params}
    };

    send(request.dump());
}

void CryptocomWS::watchTicker(const std::string& symbol, const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    auto channel = "ticker";
    subscribe(channel, marketId);
}

void CryptocomWS::watchTickers(const std::vector<std::string>& symbols, const std::map<std::string, std::string>& params) {
    auto channel = "ticker";
    subscribeMultiple(channel, symbols);
}

void CryptocomWS::watchOrderBook(const std::string& symbol, int limit, const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    auto channel = "book";
    if (limit > 0) {
        channel += "." + std::to_string(limit);
    }
    subscribe(channel, marketId);
}

void CryptocomWS::watchOrderBookForSymbols(const std::vector<std::string>& symbols, int limit, const std::map<std::string, std::string>& params) {
    auto channel = "book";
    if (limit > 0) {
        channel += "." + std::to_string(limit);
    }
    subscribeMultiple(channel, symbols);
}

void CryptocomWS::watchTrades(const std::string& symbol, const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    auto channel = "trade";
    subscribe(channel, marketId);
}

void CryptocomWS::watchTradesForSymbols(const std::vector<std::string>& symbols, const std::map<std::string, std::string>& params) {
    auto channel = "trade";
    subscribeMultiple(channel, symbols);
}

void CryptocomWS::watchOHLCV(const std::string& symbol, const std::string& timeframe, const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    auto channel = "candlestick." + timeframe;
    subscribe(channel, marketId);
}

void CryptocomWS::watchBalance(const std::map<std::string, std::string>& params) {
    authenticate();
    subscribe("user.balance", "", true);
}

void CryptocomWS::watchOrders(const std::string& symbol, const std::map<std::string, std::string>& params) {
    authenticate();
    if (!symbol.empty()) {
        auto marketId = getMarketId(symbol);
        subscribe("user.order", marketId, true);
    } else {
        subscribe("user.order", "", true);
    }
}

void CryptocomWS::watchMyTrades(const std::string& symbol, const std::map<std::string, std::string>& params) {
    authenticate();
    if (!symbol.empty()) {
        auto marketId = getMarketId(symbol);
        subscribe("user.trade", marketId, true);
    } else {
        subscribe("user.trade", "", true);
    }
}

void CryptocomWS::watchPositions(const std::map<std::string, std::string>& params) {
    authenticate();
    subscribe("user.position", "", true);
}

void CryptocomWS::createOrderWs(const std::string& symbol, const std::string& type, const std::string& side,
                               double amount, double price, const std::map<std::string, std::string>& params) {
    authenticate();
    auto marketId = getMarketId(symbol);
    auto id = getNextRequestId();

    nlohmann::json orderParams = {
        {"instrument_name", marketId},
        {"side", side},
        {"type", type},
        {"quantity", amount}
    };

    if (price > 0) {
        orderParams["price"] = price;
    }

    for (const auto& param : params) {
        orderParams[param.first] = param.second;
    }

    nlohmann::json request = {
        {"id", id},
        {"method", "order.create"},
        {"params", orderParams}
    };

    send(request.dump());
}

void CryptocomWS::cancelOrderWs(const std::string& id, const std::string& symbol, const std::map<std::string, std::string>& params) {
    authenticate();
    auto reqId = getNextRequestId();

    nlohmann::json cancelParams = {
        {"order_id", id}
    };

    if (!symbol.empty()) {
        cancelParams["instrument_name"] = getMarketId(symbol);
    }

    nlohmann::json request = {
        {"id", reqId},
        {"method", "order.cancel"},
        {"params", cancelParams}
    };

    send(request.dump());
}

void CryptocomWS::cancelAllOrders(const std::string& symbol, const std::map<std::string, std::string>& params) {
    authenticate();
    auto id = getNextRequestId();

    nlohmann::json cancelParams = {};
    if (!symbol.empty()) {
        cancelParams["instrument_name"] = getMarketId(symbol);
    }

    nlohmann::json request = {
        {"id", id},
        {"method", "order.cancel_all"},
        {"params", cancelParams}
    };

    send(request.dump());
}

void CryptocomWS::subscribe(const std::string& channel, const std::string& symbol, bool isPrivate) {
    auto id = getNextRequestId();
    std::string method = "subscribe";
    std::string channelId = channel;
    
    if (!symbol.empty()) {
        channelId += "." + symbol;
    }

    nlohmann::json request = {
        {"id", id},
        {"method", method},
        {"params", {
            {"channels", {channelId}}
        }}
    };

    send(request.dump());
    subscriptions_[channelId] = symbol;
}

void CryptocomWS::subscribeMultiple(const std::string& channel, const std::vector<std::string>& symbols, bool isPrivate) {
    auto id = getNextRequestId();
    std::string method = "subscribe";
    std::vector<std::string> channels;

    for (const auto& symbol : symbols) {
        auto marketId = getMarketId(symbol);
        auto channelId = channel + "." + marketId;
        channels.push_back(channelId);
        subscriptions_[channelId] = symbol;
    }

    nlohmann::json request = {
        {"id", id},
        {"method", method},
        {"params", {
            {"channels", channels}
        }}
    };

    send(request.dump());
}

void CryptocomWS::unsubscribe(const std::string& channel, const std::string& symbol) {
    auto id = getNextRequestId();
    std::string method = "unsubscribe";
    std::string channelId = channel;
    
    if (!symbol.empty()) {
        channelId += "." + symbol;
    }

    nlohmann::json request = {
        {"id", id},
        {"method", method},
        {"params", {
            {"channels", {channelId}}
        }}
    };

    send(request.dump());
    subscriptions_.erase(channelId);
}

void CryptocomWS::unsubscribeMultiple(const std::string& channel, const std::vector<std::string>& symbols) {
    auto id = getNextRequestId();
    std::string method = "unsubscribe";
    std::vector<std::string> channels;

    for (const auto& symbol : symbols) {
        auto marketId = getMarketId(symbol);
        auto channelId = channel + "." + marketId;
        channels.push_back(channelId);
        subscriptions_.erase(channelId);
    }

    nlohmann::json request = {
        {"id", id},
        {"method", method},
        {"params", {
            {"channels", channels}
        }}
    };

    send(request.dump());
}

void CryptocomWS::handleMessage(const std::string& message) {
    auto j = nlohmann::json::parse(message);

    if (j.contains("method")) {
        auto method = j["method"].get<std::string>();
        
        if (method == "public/heartbeat") {
            handleHeartbeat(j);
        } else if (method == "subscribe") {
            handleSubscriptionMessage(j);
        } else if (method == "unsubscribe") {
            handleUnsubscriptionMessage(j);
        } else if (method == "auth") {
            handleAuthenticationMessage(j);
        }
    } else if (j.contains("result")) {
        // Handle response to request
        if (j.contains("id")) {
            // Match with sent request
        }
    } else if (j.contains("code")) {
        handleErrorMessage(j);
    }

    if (j.contains("channel")) {
        auto channel = j["channel"].get<std::string>();
        
        if (channel.find("ticker") == 0) {
            handleTickerMessage(j);
        } else if (channel.find("book") == 0) {
            handleOrderBookMessage(j);
        } else if (channel.find("trade") == 0) {
            handleTradeMessage(j);
        } else if (channel.find("candlestick") == 0) {
            handleOHLCVMessage(j);
        } else if (channel.find("user.balance") == 0) {
            handleBalanceMessage(j);
        } else if (channel.find("user.order") == 0) {
            handleOrderMessage(j);
        } else if (channel.find("user.trade") == 0) {
            handleMyTradeMessage(j);
        } else if (channel.find("user.position") == 0) {
            handlePositionMessage(j);
        }
    }
}

void CryptocomWS::handleTickerMessage(const nlohmann::json& data) {
    // Implementation details
}

void CryptocomWS::handleOrderBookMessage(const nlohmann::json& data) {
    // Implementation details
}

void CryptocomWS::handleTradeMessage(const nlohmann::json& data) {
    // Implementation details
}

void CryptocomWS::handleOHLCVMessage(const nlohmann::json& data) {
    // Implementation details
}

void CryptocomWS::handleBalanceMessage(const nlohmann::json& data) {
    // Implementation details
}

void CryptocomWS::handleOrderMessage(const nlohmann::json& data) {
    // Implementation details
}

void CryptocomWS::handleMyTradeMessage(const nlohmann::json& data) {
    // Implementation details
}

void CryptocomWS::handlePositionMessage(const nlohmann::json& data) {
    // Implementation details
}

void CryptocomWS::handleErrorMessage(const nlohmann::json& data) {
    // Implementation details
}

void CryptocomWS::handleSubscriptionMessage(const nlohmann::json& data) {
    // Implementation details
}

void CryptocomWS::handleUnsubscriptionMessage(const nlohmann::json& data) {
    // Implementation details
}

void CryptocomWS::handleAuthenticationMessage(const nlohmann::json& data) {
    if (data.contains("code") && data["code"].get<int>() == 0) {
        authenticated_ = true;
    }
}

void CryptocomWS::handleHeartbeat(const nlohmann::json& data) {
    pong(data);
}

void CryptocomWS::pong(const nlohmann::json& message) {
    auto id = getNextRequestId();
    nlohmann::json pong = {
        {"id", id},
        {"method", "public/respond-heartbeat"}
    };
    send(pong.dump());
}

std::string CryptocomWS::getMarketId(const std::string& symbol) {
    auto market = exchange_.market(symbol);
    return market["id"];
}

std::string CryptocomWS::getSymbol(const std::string& marketId) {
    for (const auto& market : exchange_.markets) {
        if (market.second["id"] == marketId) {
            return market.first;
        }
    }
    return marketId;
}

int CryptocomWS::getNextRequestId() {
    return requestId_++;
}

} // namespace ccxt
