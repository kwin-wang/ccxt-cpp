#include "ccxt/exchanges/ws/poloniexfutures_ws.h"
#include "ccxt/base/json.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace ccxt {

PoloniexFuturesWS::PoloniexFuturesWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, PoloniexFutures& exchange)
    : WebSocketClient(ioc, ctx)
    , exchange_(exchange)
    , authenticated_(false)
    , sequenceNumber_(1) {
    this->onMessage = [this](const std::string& message) {
        this->handleMessage(message);
    };
}

std::string PoloniexFuturesWS::getEndpoint(const std::string& type) {
    return "wss://futures-ws.poloniex.com/" + type;
}

int64_t PoloniexFuturesWS::getNextSequenceNumber() {
    return sequenceNumber_++;
}

std::string PoloniexFuturesWS::generateSignature(const std::string& timestamp, const std::string& method,
                                                const std::string& path, const std::string& body) {
    std::string message = timestamp + method + path + body;
    return exchange_.hmac(message, exchange_.secret, "sha256");
}

void PoloniexFuturesWS::authenticate() {
    if (!authenticated_ && !exchange_.apiKey.empty()) {
        std::stringstream ss;
        ss << std::time(nullptr) * 1000;
        std::string timestamp = ss.str();
        std::string method = "GET";
        std::string path = "/ws/auth";
        std::string body = "";
        
        std::string signature = generateSignature(timestamp, method, path, body);

        nlohmann::json auth_message = {
            {"event", "subscribe"},
            {"channel", ["auth"]},
            {"params", {
                {"apiKey", exchange_.apiKey},
                {"timestamp", timestamp},
                {"signature", signature}
            }}
        };

        send(auth_message.dump());
    }
}

void PoloniexFuturesWS::ping() {
    nlohmann::json ping_message = {
        {"event", "ping"},
        {"ts", std::time(nullptr) * 1000}
    };
    send(ping_message.dump());
}

void PoloniexFuturesWS::subscribe(const std::string& channel, const std::string& symbol, bool isPrivate) {
    if (isPrivate) {
        authenticate();
    }

    nlohmann::json params;
    if (!symbol.empty()) {
        params["symbol"] = symbol;
    }

    nlohmann::json sub_message = {
        {"event", "subscribe"},
        {"channel", {channel}},
        {"params", params}
    };

    subscriptions_[channel + "_" + symbol] = symbol;
    send(sub_message.dump());
}

void PoloniexFuturesWS::unsubscribe(const std::string& channel, const std::string& symbol) {
    nlohmann::json params;
    if (!symbol.empty()) {
        params["symbol"] = symbol;
    }

    nlohmann::json unsub_message = {
        {"event", "unsubscribe"},
        {"channel", {channel}},
        {"params", params}
    };

    subscriptions_.erase(channel + "_" + symbol);
    send(unsub_message.dump());
}

void PoloniexFuturesWS::watchTicker(const std::string& symbol) {
    subscribe("ticker", symbol);
}

void PoloniexFuturesWS::watchTickers(const std::vector<std::string>& symbols) {
    for (const auto& symbol : symbols) {
        watchTicker(symbol);
    }
}

void PoloniexFuturesWS::watchOrderBook(const std::string& symbol, const int limit) {
    subscribe("book", symbol);
}

void PoloniexFuturesWS::watchTrades(const std::string& symbol) {
    subscribe("trades", symbol);
}

void PoloniexFuturesWS::watchOHLCV(const std::string& symbol, const std::string& timeframe) {
    subscribe("candles_" + timeframe, symbol);
}

void PoloniexFuturesWS::watchMarkPrice(const std::string& symbol) {
    subscribe("mark_price", symbol);
}

void PoloniexFuturesWS::watchFundingRate(const std::string& symbol) {
    subscribe("funding_rate", symbol);
}

void PoloniexFuturesWS::watchBalance() {
    subscribe("account", "", true);
}

void PoloniexFuturesWS::watchOrders(const std::string& symbol) {
    subscribe("orders", symbol, true);
}

void PoloniexFuturesWS::watchMyTrades(const std::string& symbol) {
    subscribe("myTrades", symbol, true);
}

void PoloniexFuturesWS::watchPositions(const std::string& symbol) {
    subscribe("positions", symbol, true);
}

void PoloniexFuturesWS::watchLiquidations(const std::string& symbol) {
    subscribe("liquidations", symbol);
}

void PoloniexFuturesWS::handleMessage(const std::string& message) {
    auto j = nlohmann::json::parse(message);

    if (j.contains("event")) {
        std::string event = j["event"];
        
        if (event == "pong") {
            return;
        } else if (event == "auth") {
            handleAuthMessage(j["data"]);
        } else if (event == "error") {
            handleErrorMessage(j["data"]);
        } else if (event == "subscribe") {
            handleSubscriptionMessage(j["data"]);
        } else if (event == "unsubscribe") {
            handleUnsubscriptionMessage(j["data"]);
        }
    } else if (j.contains("channel")) {
        std::string channel = j["channel"][0];
        auto data = j["data"];
        
        if (channel == "ticker") {
            handleTickerMessage(data);
        } else if (channel == "book") {
            handleOrderBookMessage(data);
        } else if (channel == "trades") {
            handleTradeMessage(data);
        } else if (channel.find("candles_") == 0) {
            handleOHLCVMessage(data);
        } else if (channel == "mark_price") {
            handleMarkPriceMessage(data);
        } else if (channel == "funding_rate") {
            handleFundingRateMessage(data);
        } else if (channel == "account") {
            handleBalanceMessage(data);
        } else if (channel == "orders") {
            handleOrderMessage(data);
        } else if (channel == "myTrades") {
            handleMyTradeMessage(data);
        } else if (channel == "positions") {
            handlePositionMessage(data);
        } else if (channel == "liquidations") {
            handleLiquidationMessage(data);
        }
    }
}

void PoloniexFuturesWS::handleTickerMessage(const nlohmann::json& data) {
    if (!data.contains("symbol")) return;

    std::string symbol = data["symbol"];
    
    emit(symbol, "ticker", {
        {"symbol", symbol},
        {"high", std::stod(data["high24hr"].get<std::string>())},
        {"low", std::stod(data["low24hr"].get<std::string>())},
        {"last", std::stod(data["last"].get<std::string>())},
        {"bid", std::stod(data["highestBid"].get<std::string>())},
        {"ask", std::stod(data["lowestAsk"].get<std::string>())},
        {"baseVolume", std::stod(data["baseVolume24hr"].get<std::string>())},
        {"quoteVolume", std::stod(data["quoteVolume24hr"].get<std::string>())},
        {"percentage", std::stod(data["percentChange"].get<std::string>())},
        {"timestamp", data["timestamp"]}
    });
}

void PoloniexFuturesWS::handleOrderBookMessage(const nlohmann::json& data) {
    if (!data.contains("symbol")) return;

    std::string symbol = data["symbol"];
    
    nlohmann::json orderbook;
    orderbook["symbol"] = symbol;
    orderbook["timestamp"] = data["timestamp"];
    orderbook["nonce"] = data["seq"];
    
    std::vector<std::vector<double>> bids;
    std::vector<std::vector<double>> asks;
    
    for (const auto& bid : data["bids"]) {
        bids.push_back({std::stod(bid[0].get<std::string>()), std::stod(bid[1].get<std::string>())});
    }
    
    for (const auto& ask : data["asks"]) {
        asks.push_back({std::stod(ask[0].get<std::string>()), std::stod(ask[1].get<std::string>())});
    }
    
    orderbook["bids"] = bids;
    orderbook["asks"] = asks;
    
    emit(symbol, "orderbook", orderbook);
}

void PoloniexFuturesWS::handleTradeMessage(const nlohmann::json& data) {
    if (!data.contains("symbol")) return;

    std::string symbol = data["symbol"];
    
    emit(symbol, "trade", {
        {"id", data["id"]},
        {"symbol", symbol},
        {"price", std::stod(data["price"].get<std::string>())},
        {"amount", std::stod(data["amount"].get<std::string>())},
        {"side", data["side"]},
        {"timestamp", data["timestamp"]}
    });
}

void PoloniexFuturesWS::handleOHLCVMessage(const nlohmann::json& data) {
    if (!data.contains("symbol")) return;

    std::string symbol = data["symbol"];
    
    emit(symbol, "ohlcv", {
        {"timestamp", data["timestamp"]},
        {"open", std::stod(data["open"].get<std::string>())},
        {"high", std::stod(data["high"].get<std::string>())},
        {"low", std::stod(data["low"].get<std::string>())},
        {"close", std::stod(data["close"].get<std::string>())},
        {"volume", std::stod(data["volume"].get<std::string>())}
    });
}

void PoloniexFuturesWS::handleMarkPriceMessage(const nlohmann::json& data) {
    if (!data.contains("symbol")) return;

    std::string symbol = data["symbol"];
    
    emit(symbol, "markPrice", {
        {"symbol", symbol},
        {"markPrice", std::stod(data["markPrice"].get<std::string>())},
        {"indexPrice", std::stod(data["indexPrice"].get<std::string>())},
        {"timestamp", data["timestamp"]}
    });
}

void PoloniexFuturesWS::handleFundingRateMessage(const nlohmann::json& data) {
    if (!data.contains("symbol")) return;

    std::string symbol = data["symbol"];
    
    emit(symbol, "fundingRate", {
        {"symbol", symbol},
        {"fundingRate", std::stod(data["fundingRate"].get<std::string>())},
        {"estimatedRate", std::stod(data["estimatedRate"].get<std::string>())},
        {"nextFundingTime", data["nextFundingTime"]},
        {"timestamp", data["timestamp"]}
    });
}

void PoloniexFuturesWS::handleBalanceMessage(const nlohmann::json& data) {
    if (!data.contains("balances")) return;

    nlohmann::json balance;
    
    for (const auto& asset : data["balances"].items()) {
        balance[asset.key()] = {
            {"free", std::stod(asset.value()["available"].get<std::string>())},
            {"used", std::stod(asset.value()["frozen"].get<std::string>())},
            {"total", std::stod(asset.value()["total"].get<std::string>())}
        };
    }
    
    emit("", "balance", balance);
}

void PoloniexFuturesWS::handleOrderMessage(const nlohmann::json& data) {
    if (!data.contains("symbol")) return;

    std::string symbol = data["symbol"];
    
    emit(symbol, "order", {
        {"id", data["orderId"]},
        {"clientOrderId", data["clientOrderId"]},
        {"symbol", symbol},
        {"type", data["type"]},
        {"side", data["side"]},
        {"price", std::stod(data["price"].get<std::string>())},
        {"amount", std::stod(data["amount"].get<std::string>())},
        {"filled", std::stod(data["filled"].get<std::string>())},
        {"remaining", std::stod(data["remaining"].get<std::string>())},
        {"status", data["status"]},
        {"timestamp", data["timestamp"]}
    });
}

void PoloniexFuturesWS::handleMyTradeMessage(const nlohmann::json& data) {
    if (!data.contains("symbol")) return;

    std::string symbol = data["symbol"];
    
    emit(symbol, "mytrade", {
        {"id", data["tradeId"]},
        {"orderId", data["orderId"]},
        {"symbol", symbol},
        {"type", data["type"]},
        {"side", data["side"]},
        {"price", std::stod(data["price"].get<std::string>())},
        {"amount", std::stod(data["amount"].get<std::string>())},
        {"fee", std::stod(data["fee"].get<std::string>())},
        {"feeCurrency", data["feeCurrency"]},
        {"timestamp", data["timestamp"]}
    });
}

void PoloniexFuturesWS::handlePositionMessage(const nlohmann::json& data) {
    if (!data.contains("symbol")) return;

    std::string symbol = data["symbol"];
    
    emit(symbol, "position", {
        {"symbol", symbol},
        {"size", std::stod(data["size"].get<std::string>())},
        {"entryPrice", std::stod(data["entryPrice"].get<std::string>())},
        {"markPrice", std::stod(data["markPrice"].get<std::string>())},
        {"liquidationPrice", std::stod(data["liquidationPrice"].get<std::string>())},
        {"margin", std::stod(data["margin"].get<std::string>())},
        {"leverage", std::stod(data["leverage"].get<std::string>())},
        {"unrealizedPnl", std::stod(data["unrealizedPnl"].get<std::string>())},
        {"marginRatio", std::stod(data["marginRatio"].get<std::string>())},
        {"timestamp", data["timestamp"]}
    });
}

void PoloniexFuturesWS::handleLiquidationMessage(const nlohmann::json& data) {
    if (!data.contains("symbol")) return;

    std::string symbol = data["symbol"];
    
    emit(symbol, "liquidation", {
        {"symbol", symbol},
        {"side", data["side"]},
        {"price", std::stod(data["price"].get<std::string>())},
        {"amount", std::stod(data["amount"].get<std::string>())},
        {"timestamp", data["timestamp"]}
    });
}

void PoloniexFuturesWS::handleErrorMessage(const nlohmann::json& data) {
    if (data.contains("message")) {
        std::string error_message = data["message"];
        // Handle error appropriately
    }
}

void PoloniexFuturesWS::handleAuthMessage(const nlohmann::json& data) {
    if (data.contains("authenticated")) {
        authenticated_ = data["authenticated"].get<bool>();
    }
}

void PoloniexFuturesWS::handleSubscriptionMessage(const nlohmann::json& data) {
    // Handle subscription confirmation
}

void PoloniexFuturesWS::handleUnsubscriptionMessage(const nlohmann::json& data) {
    // Handle unsubscription confirmation
}

} // namespace ccxt
