#include "ccxt/exchanges/ws/gate_ws.h"
#include "ccxt/base/json.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

namespace ccxt {

GateWS::GateWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Gate& exchange)
    : WebSocketClient(ioc, ctx)
    , exchange_(exchange) {
}

std::string GateWS::getEndpoint(const std::string& type, const std::string& settle) {
    std::string baseUrl;
    if (exchange_.getTestMode()) {
        if (type == "spot") {
            baseUrl = "wss://api.gateio.ws/ws/v4/";
        } else if (type == "swap") {
            baseUrl = "wss://fx-ws-testnet.gateio.ws/v4/ws/" + settle;
        } else if (type == "future") {
            baseUrl = "wss://fx-ws-testnet.gateio.ws/v4/ws/delivery/" + settle;
        } else if (type == "option") {
            baseUrl = "wss://op-ws-testnet.gateio.live/v4/ws/" + settle;
        }
    } else {
        if (type == "spot") {
            baseUrl = "wss://api.gateio.ws/ws/v4/";
        } else if (type == "swap") {
            baseUrl = "wss://fx-ws.gateio.ws/v4/ws/" + settle;
        } else if (type == "future") {
            baseUrl = "wss://fx-ws.gateio.ws/v4/ws/delivery/" + settle;
        } else if (type == "option") {
            baseUrl = "wss://op-ws.gateio.live/v4/ws/" + settle;
        }
    }
    return baseUrl;
}

void GateWS::watchTicker(const std::string& symbol) {
    auto market = exchange_.market(symbol);
    subscribe("spot.tickers", symbol);
}

void GateWS::watchTickers(const std::vector<std::string>& symbols) {
    for (const auto& symbol : symbols) {
        watchTicker(symbol);
    }
}

void GateWS::watchOrderBook(const std::string& symbol) {
    subscribe("spot.order_book", symbol);
}

void GateWS::watchTrades(const std::string& symbol) {
    subscribe("spot.trades", symbol);
}

void GateWS::watchTradesForSymbols(const std::vector<std::string>& symbols) {
    for (const auto& symbol : symbols) {
        watchTrades(symbol);
    }
}

void GateWS::watchOHLCV(const std::string& symbol, const std::string& timeframe) {
    subscribe("spot.candlesticks", symbol, timeframe);
}

void GateWS::watchBalance() {
    subscribePrivate("spot.balances");
}

void GateWS::watchOrders() {
    subscribePrivate("spot.orders");
}

void GateWS::watchMyTrades() {
    subscribePrivate("spot.usertrades");
}

void GateWS::watchPositions() {
    subscribePrivate("futures.positions");
}

void GateWS::watchMyLiquidations() {
    subscribePrivate("futures.liquidates");
}

void GateWS::createOrder(const std::string& symbol, const std::string& type, const std::string& side,
                        double amount, double price, const std::map<std::string, std::string>& params) {
    auto market = exchange_.market(symbol);
    
    nlohmann::json request = {
        {"time", std::time(nullptr)},
        {"channel", "spot.orders"},
        {"event", "create"},
        {"payload", {
            {"text", "t-" + std::to_string(std::time(nullptr))},
            {"currency_pair", market.id},
            {"type", type},
            {"side", side},
            {"amount", std::to_string(amount)},
            {"price", std::to_string(price)}
        }}
    };
    
    authenticate("spot.orders");
    send(request.dump());
}

void GateWS::cancelOrder(const std::string& id, const std::string& symbol) {
    auto market = exchange_.market(symbol);
    
    nlohmann::json request = {
        {"time", std::time(nullptr)},
        {"channel", "spot.orders"},
        {"event", "cancel"},
        {"payload", {
            {"id", id},
            {"currency_pair", market.id}
        }}
    };
    
    authenticate("spot.orders");
    send(request.dump());
}

void GateWS::cancelAllOrders(const std::string& symbol) {
    if (!symbol.empty()) {
        auto market = exchange_.market(symbol);
        
        nlohmann::json request = {
            {"time", std::time(nullptr)},
            {"channel", "spot.orders"},
            {"event", "cancel_all"},
            {"payload", {
                {"currency_pair", market.id}
            }}
        };
        
        authenticate("spot.orders");
        send(request.dump());
    }
}

void GateWS::editOrder(const std::string& id, const std::string& symbol, const std::string& type,
                      const std::string& side, double amount, double price) {
    auto market = exchange_.market(symbol);
    
    nlohmann::json request = {
        {"time", std::time(nullptr)},
        {"channel", "spot.orders"},
        {"event", "update"},
        {"payload", {
            {"id", id},
            {"currency_pair", market.id},
            {"amount", std::to_string(amount)},
            {"price", std::to_string(price)}
        }}
    };
    
    authenticate("spot.orders");
    send(request.dump());
}

void GateWS::subscribe(const std::string& channel, const std::string& symbol, const std::string& settle) {
    auto market = exchange_.market(symbol);
    std::string endpoint = getEndpoint(market.type, settle);
    
    nlohmann::json request = {
        {"time", std::time(nullptr)},
        {"channel", channel},
        {"event", "subscribe"},
        {"payload", {market.id}}
    };
    
    connect(endpoint);
    send(request.dump());
}

void GateWS::subscribePrivate(const std::string& channel, const std::string& settle) {
    std::string endpoint = getEndpoint("spot", settle);
    
    nlohmann::json request = {
        {"time", std::time(nullptr)},
        {"channel", channel},
        {"event", "subscribe"},
        {"payload", {}}
    };
    
    authenticate(channel);
    connect(endpoint);
    send(request.dump());
}

void GateWS::authenticate(const std::string& channel) {
    long long timestamp = std::time(nullptr);
    std::string message = channel + std::to_string(timestamp);
    std::string signature = exchange_.hmac(message, exchange_.secret, "sha512");
    
    nlohmann::json auth = {
        {"method", "api_key"},
        {"KEY", exchange_.apiKey},
        {"SIGN", signature},
        {"Timestamp", timestamp}
    };
    
    send(auth.dump());
}

void GateWS::handleMessage(const std::string& message) {
    try {
        auto j = nlohmann::json::parse(message);
        
        if (j.contains("channel") && j.contains("event") && j.contains("result")) {
            std::string channel = j["channel"];
            std::string event = j["event"];
            
            if (channel == "spot.tickers") {
                handleTickerMessage(j["result"]);
            } else if (channel == "spot.order_book") {
                handleOrderBookMessage(j["result"]);
            } else if (channel == "spot.trades") {
                handleTradeMessage(j["result"]);
            } else if (channel == "spot.candlesticks") {
                handleOHLCVMessage(j["result"]);
            } else if (channel == "spot.balances") {
                handleBalanceMessage(j["result"]);
            } else if (channel == "spot.orders") {
                handleOrderMessage(j["result"]);
            } else if (channel == "futures.positions") {
                handlePositionMessage(j["result"]);
            } else if (channel == "futures.liquidates") {
                handleLiquidationMessage(j["result"]);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling message: " << e.what() << std::endl;
    }
}

void GateWS::handleTickerMessage(const nlohmann::json& data) {
    nlohmann::json ticker = {
        {"symbol", data["currency_pair"]},
        {"last", data["last"]},
        {"high", data["high_24h"]},
        {"low", data["low_24h"]},
        {"bid", data["highest_bid"]},
        {"ask", data["lowest_ask"]},
        {"baseVolume", data["base_volume"]},
        {"quoteVolume", data["quote_volume"]},
        {"timestamp", data["timestamp"]},
        {"info", data}
    };
    
    emit("ticker::" + std::string(data["currency_pair"]), ticker);
}

void GateWS::handleOrderBookMessage(const nlohmann::json& data) {
    nlohmann::json orderbook = {
        {"symbol", data["currency_pair"]},
        {"bids", data["bids"]},
        {"asks", data["asks"]},
        {"timestamp", data["timestamp"]},
        {"info", data}
    };
    
    emit("orderbook::" + std::string(data["currency_pair"]), orderbook);
}

void GateWS::handleTradeMessage(const nlohmann::json& data) {
    for (const auto& trade : data) {
        nlohmann::json parsedTrade = {
            {"id", trade["id"]},
            {"symbol", trade["currency_pair"]},
            {"timestamp", trade["create_time"]},
            {"side", trade["side"]},
            {"price", trade["price"]},
            {"amount", trade["amount"]},
            {"info", trade}
        };
        
        emit("trades::" + std::string(trade["currency_pair"]), parsedTrade);
    }
}

void GateWS::handleOHLCVMessage(const nlohmann::json& data) {
    nlohmann::json ohlcv = {
        {"timestamp", data[0]},
        {"open", data[1]},
        {"high", data[2]},
        {"low", data[3]},
        {"close", data[4]},
        {"volume", data[5]},
        {"info", data}
    };
    
    emit("ohlcv::" + std::string(data["currency_pair"]), ohlcv);
}

void GateWS::handleBalanceMessage(const nlohmann::json& data) {
    emit("balance", data);
}

void GateWS::handleOrderMessage(const nlohmann::json& data) {
    nlohmann::json order = {
        {"id", data["id"]},
        {"symbol", data["currency_pair"]},
        {"type", data["type"]},
        {"side", data["side"]},
        {"price", data["price"]},
        {"amount", data["amount"]},
        {"filled", data["filled_total"]},
        {"remaining", data["left"]},
        {"status", data["status"]},
        {"timestamp", data["create_time"]},
        {"info", data}
    };
    
    emit("orders::" + std::string(data["currency_pair"]), order);
}

void GateWS::handlePositionMessage(const nlohmann::json& data) {
    nlohmann::json position = {
        {"symbol", data["contract"]},
        {"size", data["size"]},
        {"side", data["side"]},
        {"notional", data["value"]},
        {"leverage", data["leverage"]},
        {"unrealizedPnl", data["unrealised_pnl"]},
        {"timestamp", data["timestamp"]},
        {"info", data}
    };
    
    emit("position::" + std::string(data["contract"]), position);
}

void GateWS::handleLiquidationMessage(const nlohmann::json& data) {
    nlohmann::json liquidation = {
        {"symbol", data["contract"]},
        {"type", "liquidation"},
        {"side", data["side"]},
        {"price", data["price"]},
        {"amount", data["amount"]},
        {"timestamp", data["timestamp"]},
        {"info", data}
    };
    
    emit("liquidation::" + std::string(data["contract"]), liquidation);
}

} // namespace ccxt
