#include "ccxt/exchanges/ws/ndax_ws.h"
#include "ccxt/base/json.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <ctime>

namespace ccxt {

NdaxWS::NdaxWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Ndax& exchange)
    : WebSocketClient(ioc, ctx)
    , exchange_(exchange)
    , authenticated_(false)
    , sequenceNumber_(1) {
    this->onMessage = [this](const std::string& message) {
        this->handleMessage(message);
    };
}

std::string NdaxWS::getEndpoint(const std::string& type) {
    return "wss://api.ndax.io/WSGateway/";
}

int NdaxWS::getNextSequenceNumber() {
    return sequenceNumber_++;
}

int NdaxWS::getSymbolId(const std::string& symbol) {
    auto market = exchange_.market(symbol);
    return std::stoi(market.id);
}

void NdaxWS::authenticate() {
    if (!authenticated_ && !exchange_.apiKey.empty()) {
        long timestamp = std::time(nullptr) * 1000;
        std::string signData = exchange_.apiKey + std::to_string(timestamp);
        std::string signature = exchange_.hmac(signData, exchange_.secret, "sha256");

        nlohmann::json auth_message = {
            {"m", 0}, // message type: authenticate
            {"i", getNextSequenceNumber()},
            {"n", "AuthenticateUser"},
            {"o", {
                {"ApiKey", exchange_.apiKey},
                {"Timestamp", timestamp},
                {"Signature", signature}
            }}
        };

        send(auth_message.dump());
    }
}

void NdaxWS::ping() {
    nlohmann::json ping_message = {
        {"m", 0},
        {"i", getNextSequenceNumber()},
        {"n", "Ping"}
    };
    send(ping_message.dump());
}

void NdaxWS::subscribe(const std::string& channel, const std::string& symbol, bool isPrivate) {
    if (isPrivate) {
        authenticate();
    }

    nlohmann::json sub_message = {
        {"m", 0},
        {"i", getNextSequenceNumber()},
        {"n", "Subscribe"},
        {"o", {
            {"OMSId", 1},
            {"InstrumentId", symbol.empty() ? 0 : getSymbolId(symbol)},
            {"ChannelId", channel}
        }}
    };

    subscriptions_[channel + "_" + symbol] = symbol;
    send(sub_message.dump());
}

void NdaxWS::unsubscribe(const std::string& channel, const std::string& symbol) {
    nlohmann::json unsub_message = {
        {"m", 0},
        {"i", getNextSequenceNumber()},
        {"n", "Unsubscribe"},
        {"o", {
            {"OMSId", 1},
            {"InstrumentId", symbol.empty() ? 0 : getSymbolId(symbol)},
            {"ChannelId", channel}
        }}
    };

    subscriptions_.erase(channel + "_" + symbol);
    send(unsub_message.dump());
}

void NdaxWS::watchTicker(const std::string& symbol) {
    subscribe("1", symbol); // Channel 1: Level1UpdateEvent
}

void NdaxWS::watchTickers(const std::vector<std::string>& symbols) {
    for (const auto& symbol : symbols) {
        watchTicker(symbol);
    }
}

void NdaxWS::watchOrderBook(const std::string& symbol, const int limit) {
    subscribe("2", symbol); // Channel 2: Level2UpdateEvent
}

void NdaxWS::watchTrades(const std::string& symbol) {
    subscribe("3", symbol); // Channel 3: TradeDataUpdateEvent
}

void NdaxWS::watchOHLCV(const std::string& symbol, const std::string& timeframe) {
    subscribe("4", symbol); // Channel 4: CandleDataUpdateEvent
}

void NdaxWS::watchBalance() {
    subscribe("5", "", true); // Channel 5: AccountEvents
}

void NdaxWS::watchOrders(const std::string& symbol) {
    subscribe("6", symbol, true); // Channel 6: OrderStateEvents
}

void NdaxWS::watchMyTrades(const std::string& symbol) {
    subscribe("7", symbol, true); // Channel 7: TradeEvents
}

void NdaxWS::handleMessage(const std::string& message) {
    auto j = nlohmann::json::parse(message);

    if (!j.contains("n") || !j.contains("o")) return;

    std::string event = j["n"];
    auto data = j["o"];

    if (event == "AuthenticateUser") {
        handleAuthMessage(data);
    } else if (event == "SubscribeComplete") {
        handleSubscriptionMessage(data);
    } else if (event == "UnsubscribeComplete") {
        handleUnsubscriptionMessage(data);
    } else if (event == "Level1UpdateEvent") {
        handleTickerMessage(data);
    } else if (event == "Level2UpdateEvent") {
        handleOrderBookMessage(data);
    } else if (event == "TradeDataUpdateEvent") {
        handleTradeMessage(data);
    } else if (event == "CandleDataUpdateEvent") {
        handleOHLCVMessage(data);
    } else if (event == "AccountPositionEvent") {
        handleBalanceMessage(data);
    } else if (event == "OrderStateEvent") {
        handleOrderMessage(data);
    } else if (event == "TradeEvent") {
        handleMyTradeMessage(data);
    } else if (event == "Error") {
        handleErrorMessage(data);
    }
}

void NdaxWS::handleTickerMessage(const nlohmann::json& data) {
    std::string symbol = exchange_.findSymbolById(std::to_string(data["InstrumentId"].get<int>()));
    
    emit(symbol, "ticker", {
        {"symbol", symbol},
        {"high", data["HighPrice"]},
        {"low", data["LowPrice"]},
        {"bid", data["BestBid"]},
        {"ask", data["BestOffer"]},
        {"last", data["LastTradedPx"]},
        {"volume", data["Rolling24HrVolume"]},
        {"timestamp", data["TimeStamp"]}
    });
}

void NdaxWS::handleOrderBookMessage(const nlohmann::json& data) {
    std::string symbol = exchange_.findSymbolById(std::to_string(data["InstrumentId"].get<int>()));
    
    nlohmann::json orderbook;
    orderbook["symbol"] = symbol;
    orderbook["timestamp"] = data["TimeStamp"];
    
    std::vector<std::vector<double>> bids;
    std::vector<std::vector<double>> asks;
    
    for (const auto& entry : data["Bids"]) {
        bids.push_back({entry["Price"], entry["Quantity"]});
    }
    
    for (const auto& entry : data["Asks"]) {
        asks.push_back({entry["Price"], entry["Quantity"]});
    }
    
    orderbook["bids"] = bids;
    orderbook["asks"] = asks;
    
    emit(symbol, "orderbook", orderbook);
}

void NdaxWS::handleTradeMessage(const nlohmann::json& data) {
    std::string symbol = exchange_.findSymbolById(std::to_string(data["InstrumentId"].get<int>()));
    
    emit(symbol, "trade", {
        {"id", data["TradeId"]},
        {"symbol", symbol},
        {"price", data["Price"]},
        {"amount", data["Quantity"]},
        {"side", data["Direction"] == 0 ? "buy" : "sell"},
        {"timestamp", data["TimeStamp"]}
    });
}

void NdaxWS::handleOHLCVMessage(const nlohmann::json& data) {
    std::string symbol = exchange_.findSymbolById(std::to_string(data["InstrumentId"].get<int>()));
    
    emit(symbol, "ohlcv", {
        {"timestamp", data["TimeStamp"]},
        {"open", data["OpenPrice"]},
        {"high", data["HighPrice"]},
        {"low", data["LowPrice"]},
        {"close", data["ClosePrice"]},
        {"volume", data["Volume"]}
    });
}

void NdaxWS::handleBalanceMessage(const nlohmann::json& data) {
    nlohmann::json balance;
    
    for (const auto& position : data["Positions"]) {
        std::string currency = position["ProductSymbol"];
        balance[currency] = {
            {"free", position["Amount"]},
            {"used", position["Hold"]},
            {"total", position["Amount"].get<double>() + position["Hold"].get<double>()}
        };
    }
    
    emit("", "balance", balance);
}

void NdaxWS::handleOrderMessage(const nlohmann::json& data) {
    std::string symbol = exchange_.findSymbolById(std::to_string(data["InstrumentId"].get<int>()));
    
    emit(symbol, "order", {
        {"id", data["OrderId"]},
        {"symbol", symbol},
        {"type", data["OrderType"]},
        {"side", data["Side"] == 0 ? "buy" : "sell"},
        {"price", data["Price"]},
        {"amount", data["Quantity"]},
        {"filled", data["QuantityExecuted"]},
        {"remaining", data["Quantity"].get<double>() - data["QuantityExecuted"].get<double>()},
        {"status", data["OrderState"]},
        {"timestamp", data["TimeStamp"]}
    });
}

void NdaxWS::handleMyTradeMessage(const nlohmann::json& data) {
    std::string symbol = exchange_.findSymbolById(std::to_string(data["InstrumentId"].get<int>()));
    
    emit(symbol, "mytrade", {
        {"id", data["TradeId"]},
        {"order", data["OrderId"]},
        {"symbol", symbol},
        {"type", data["OrderType"]},
        {"side", data["Side"] == 0 ? "buy" : "sell"},
        {"price", data["Price"]},
        {"amount", data["Quantity"]},
        {"fee", data["Fee"]},
        {"feeCurrency", data["FeeCurrency"]},
        {"timestamp", data["TimeStamp"]}
    });
}

void NdaxWS::handleErrorMessage(const nlohmann::json& data) {
    if (data.contains("ErrorMessage")) {
        std::string error_message = data["ErrorMessage"];
        // Handle error appropriately
    }
}

void NdaxWS::handleAuthMessage(const nlohmann::json& data) {
    authenticated_ = data.contains("Authenticated") && data["Authenticated"].get<bool>();
}

void NdaxWS::handleSubscriptionMessage(const nlohmann::json& data) {
    // Handle subscription confirmation
}

void NdaxWS::handleUnsubscriptionMessage(const nlohmann::json& data) {
    // Handle unsubscription confirmation
}

} // namespace ccxt
