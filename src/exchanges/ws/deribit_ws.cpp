#include "ccxt/exchanges/ws/deribit_ws.h"
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

DeribitWS::DeribitWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Deribit& exchange)
    : WebSocketClient(ioc, ctx)
    , exchange_(exchange)
    , authenticated_(false)
    , requestId_(1) {
}

void DeribitWS::authenticate() {
    if (authenticated_) return;

    auto timestamp = std::to_string(std::time(nullptr) * 1000);
    auto nonce = timestamp;
    auto data = "";
    auto signature = exchange_.hmac(timestamp + "\n" + nonce + "\n" + data, exchange_.secret, "sha256");

    nlohmann::json request = {
        {"jsonrpc", "2.0"},
        {"id", getNextRequestId()},
        {"method", "public/auth"},
        {"params", {
            {"grant_type", "client_signature"},
            {"client_id", exchange_.apiKey},
            {"timestamp", timestamp},
            {"signature", signature},
            {"nonce", nonce},
            {"data", data}
        }}
    };

    send(request.dump());
}

void DeribitWS::watchTicker(const std::string& symbol, const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    subscribe("ticker", marketId);
}

void DeribitWS::watchTickers(const std::vector<std::string>& symbols, const std::map<std::string, std::string>& params) {
    subscribeMultiple("ticker", symbols);
}

void DeribitWS::watchOrderBook(const std::string& symbol, int limit, const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    auto interval = params.count("interval") ? params.at("interval") : "100ms";
    auto useDepthEndpoint = params.count("useDepthEndpoint") ? params.at("useDepthEndpoint") == "true" : false;
    auto depth = params.count("depth") ? params.at("depth") : "20";
    auto group = params.count("group") ? params.at("group") : "none";

    std::string channel;
    if (useDepthEndpoint) {
        channel = "book.group." + group + "." + depth + "." + interval;
    } else {
        channel = "book." + interval;
    }
    subscribe(channel, marketId);
}

void DeribitWS::watchOrderBookForSymbols(const std::vector<std::string>& symbols, int limit, const std::map<std::string, std::string>& params) {
    auto interval = params.count("interval") ? params.at("interval") : "100ms";
    auto useDepthEndpoint = params.count("useDepthEndpoint") ? params.at("useDepthEndpoint") == "true" : false;
    auto depth = params.count("depth") ? params.at("depth") : "20";
    auto group = params.count("group") ? params.at("group") : "none";

    std::string channel;
    if (useDepthEndpoint) {
        channel = "book.group." + group + "." + depth + "." + interval;
    } else {
        channel = "book." + interval;
    }
    subscribeMultiple(channel, symbols);
}

void DeribitWS::watchTrades(const std::string& symbol, const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    auto interval = params.count("interval") ? params.at("interval") : "100ms";
    subscribe("trades." + interval, marketId);
}

void DeribitWS::watchTradesForSymbols(const std::vector<std::string>& symbols, const std::map<std::string, std::string>& params) {
    auto interval = params.count("interval") ? params.at("interval") : "100ms";
    subscribeMultiple("trades." + interval, symbols);
}

void DeribitWS::watchOHLCV(const std::string& symbol, const std::string& timeframe, const std::map<std::string, std::string>& params) {
    auto marketId = getMarketId(symbol);
    auto interval = exchange_.timeframes.count(timeframe) ? exchange_.timeframes[timeframe] : "1";
    subscribe("chart." + interval, marketId);
}

void DeribitWS::watchOHLCVForSymbols(const std::vector<std::string>& symbols, const std::string& timeframe, const std::map<std::string, std::string>& params) {
    auto interval = exchange_.timeframes.count(timeframe) ? exchange_.timeframes[timeframe] : "1";
    subscribeMultiple("chart." + interval, symbols);
}

void DeribitWS::watchBidsAsks(const std::vector<std::string>& symbols, const std::map<std::string, std::string>& params) {
    subscribeMultiple("quote", symbols);
}

void DeribitWS::watchBalance(const std::map<std::string, std::string>& params) {
    authenticate();
    std::vector<std::string> currencies = {"BTC", "ETH", "SOL", "USDC"};
    for (const auto& currency : currencies) {
        subscribe("user.portfolio." + currency, "", true);
    }
}

void DeribitWS::watchOrders(const std::string& symbol, const std::map<std::string, std::string>& params) {
    authenticate();
    if (!symbol.empty()) {
        auto marketId = getMarketId(symbol);
        subscribe("user.orders." + marketId, "", true);
    } else {
        subscribe("user.orders", "", true);
    }
}

void DeribitWS::watchMyTrades(const std::string& symbol, const std::map<std::string, std::string>& params) {
    authenticate();
    if (!symbol.empty()) {
        auto marketId = getMarketId(symbol);
        subscribe("user.trades." + marketId, "", true);
    } else {
        subscribe("user.trades", "", true);
    }
}

void DeribitWS::subscribe(const std::string& channel, const std::string& symbol, bool isPrivate) {
    auto id = getNextRequestId();
    std::string channelId = channel;
    
    if (!symbol.empty()) {
        channelId += "." + symbol + ".raw";
    }

    nlohmann::json request = {
        {"jsonrpc", "2.0"},
        {"id", id},
        {"method", "public/subscribe"},
        {"params", {
            {"channels", {channelId}}
        }}
    };

    send(request.dump());
    subscriptions_[channelId] = symbol;
}

void DeribitWS::subscribeMultiple(const std::string& channel, const std::vector<std::string>& symbols, bool isPrivate) {
    auto id = getNextRequestId();
    std::vector<std::string> channels;

    for (const auto& symbol : symbols) {
        auto marketId = getMarketId(symbol);
        auto channelId = channel + "." + marketId + ".raw";
        channels.push_back(channelId);
        subscriptions_[channelId] = symbol;
    }

    nlohmann::json request = {
        {"jsonrpc", "2.0"},
        {"id", id},
        {"method", "public/subscribe"},
        {"params", {
            {"channels", channels}
        }}
    };

    send(request.dump());
}

void DeribitWS::unsubscribe(const std::string& channel, const std::string& symbol) {
    auto id = getNextRequestId();
    std::string channelId = channel;
    
    if (!symbol.empty()) {
        channelId += "." + symbol + ".raw";
    }

    nlohmann::json request = {
        {"jsonrpc", "2.0"},
        {"id", id},
        {"method", "public/unsubscribe"},
        {"params", {
            {"channels", {channelId}}
        }}
    };

    send(request.dump());
    subscriptions_.erase(channelId);
}

void DeribitWS::unsubscribeMultiple(const std::string& channel, const std::vector<std::string>& symbols) {
    auto id = getNextRequestId();
    std::vector<std::string> channels;

    for (const auto& symbol : symbols) {
        auto marketId = getMarketId(symbol);
        auto channelId = channel + "." + marketId + ".raw";
        channels.push_back(channelId);
        subscriptions_.erase(channelId);
    }

    nlohmann::json request = {
        {"jsonrpc", "2.0"},
        {"id", id},
        {"method", "public/unsubscribe"},
        {"params", {
            {"channels", channels}
        }}
    };

    send(request.dump());
}

std::string DeribitWS::getEndpoint(const std::string& type) {
    if (type == "private") {
        return exchange_.urls["api"]["ws"];
    }
    return exchange_.urls["api"]["ws"];
}

std::string DeribitWS::getMarketId(const std::string& symbol) {
    auto market = exchange_.market(symbol);
    return market["id"];
}

std::string DeribitWS::getSymbol(const std::string& marketId) {
    for (const auto& market : exchange_.markets) {
        if (market.second["id"] == marketId) {
            return market.first;
        }
    }
    return marketId;
}

std::string DeribitWS::getChannel(const std::string& channel, const std::string& symbol) {
    return channel + "." + symbol + ".raw";
}

int DeribitWS::getNextRequestId() {
    return requestId_++;
}

void DeribitWS::handleMessage(const std::string& message) {
    auto j = nlohmann::json::parse(message);

    if (j.contains("error")) {
        handleErrorMessage(j);
        return;
    }

    if (j.contains("params")) {
        auto params = j["params"];
        if (params.contains("channel")) {
            auto channel = params["channel"].get<std::string>();
            
            if (channel.find("ticker") == 0) {
                handleTickerMessage(params);
            } else if (channel.find("book") == 0) {
                handleOrderBookMessage(params);
            } else if (channel.find("trades") == 0) {
                handleTradeMessage(params);
            } else if (channel.find("chart") == 0) {
                handleOHLCVMessage(params);
            } else if (channel.find("user.portfolio") == 0) {
                handleBalanceMessage(params);
            } else if (channel.find("user.orders") == 0) {
                handleOrderMessage(params);
            } else if (channel.find("user.trades") == 0) {
                handleMyTradeMessage(params);
            }
        }
    } else if (j.contains("result")) {
        if (j.contains("access_token")) {
            handleAuthenticationMessage(j);
        } else {
            handleSubscriptionMessage(j);
        }
    }
}

void DeribitWS::handleTickerMessage(const nlohmann::json& data) {
    // Implementation details
}

void DeribitWS::handleOrderBookMessage(const nlohmann::json& data) {
    // Implementation details
}

void DeribitWS::handleTradeMessage(const nlohmann::json& data) {
    // Implementation details
}

void DeribitWS::handleOHLCVMessage(const nlohmann::json& data) {
    // Implementation details
}

void DeribitWS::handleBalanceMessage(const nlohmann::json& data) {
    // Implementation details
}

void DeribitWS::handleOrderMessage(const nlohmann::json& data) {
    // Implementation details
}

void DeribitWS::handleMyTradeMessage(const nlohmann::json& data) {
    // Implementation details
}

void DeribitWS::handleErrorMessage(const nlohmann::json& data) {
    auto error = data["error"];
    throw ExchangeError(exchange_.id + " " + error.dump());
}

void DeribitWS::handleSubscriptionMessage(const nlohmann::json& data) {
    // Implementation details
}

void DeribitWS::handleUnsubscriptionMessage(const nlohmann::json& data) {
    // Implementation details
}

void DeribitWS::handleAuthenticationMessage(const nlohmann::json& data) {
    if (data.contains("result") && data["result"].contains("access_token")) {
        authenticated_ = true;
    }
}

} // namespace ccxt
