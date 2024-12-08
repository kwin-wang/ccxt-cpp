#include "ccxt/exchanges/ws/upbit_ws.h"
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
#include <jwt-cpp/jwt.h>

namespace ccxt {

UpbitWS::UpbitWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Upbit& exchange)
    : WebSocketClient(ioc, ctx)
    , exchange_(exchange)
    , authenticated_(false)
    , sequenceNumber_(1) {
    this->onMessage = [this](const std::string& message) {
        this->handleMessage(message);
    };
}

std::string UpbitWS::getEndpoint(const std::string& type) {
    return "wss://api.upbit.com/websocket/v1";
}

int64_t UpbitWS::getNextSequenceNumber() {
    return sequenceNumber_++;
}

std::string UpbitWS::normalizeSymbol(const std::string& symbol) {
    // Upbit uses KRW-BTC format
    return symbol;
}

std::string UpbitWS::generateSignature(const std::string& message) {
    auto token = jwt::create()
        .set_issuer("upbit")
        .set_type("JWT")
        .set_issued_at(std::chrono::system_clock::now())
        .set_payload_claim("access_key", jwt::claim(exchange_.apiKey))
        .set_payload_claim("nonce", jwt::claim(std::to_string(getNextSequenceNumber())))
        .sign(jwt::algorithm::hs256{exchange_.secret});
    
    return token;
}

void UpbitWS::authenticate() {
    if (!authenticated_ && !exchange_.apiKey.empty()) {
        std::string token = generateSignature("");
        
        nlohmann::json auth_message = {
            {"ticket", token}
        };

        send(auth_message.dump());
    }
}

void UpbitWS::ping() {
    nlohmann::json ping_message = {
        {"type", "ping"},
        {"timestamp", std::time(nullptr) * 1000}
    };
    send(ping_message.dump());
}

void UpbitWS::subscribe(const std::string& channel, const std::string& symbol, bool isPrivate) {
    if (isPrivate) {
        authenticate();
    }

    nlohmann::json sub_message;
    
    if (channel == "ticker") {
        sub_message = {
            {"type", "ticker"},
            {"codes", {symbol}},
            {"isOnlyRealtime", true}
        };
    } else if (channel == "orderbook") {
        sub_message = {
            {"type", "orderbook"},
            {"codes", {symbol}}
        };
    } else if (channel == "trade") {
        sub_message = {
            {"type", "trade"},
            {"codes", {symbol}}
        };
    }

    subscriptions_[channel + "_" + symbol] = symbol;
    send(sub_message.dump());
}

void UpbitWS::unsubscribe(const std::string& channel, const std::string& symbol) {
    subscriptions_.erase(channel + "_" + symbol);
}

void UpbitWS::watchTicker(const std::string& symbol) {
    subscribe("ticker", normalizeSymbol(symbol));
}

void UpbitWS::watchTickers(const std::vector<std::string>& symbols) {
    for (const auto& symbol : symbols) {
        watchTicker(symbol);
    }
}

void UpbitWS::watchOrderBook(const std::string& symbol, const int limit) {
    subscribe("orderbook", normalizeSymbol(symbol));
}

void UpbitWS::watchTrades(const std::string& symbol) {
    subscribe("trade", normalizeSymbol(symbol));
}

void UpbitWS::watchBalance() {
    subscribe("balance", "", true);
}

void UpbitWS::watchOrders(const std::string& symbol) {
    subscribe("order", normalizeSymbol(symbol), true);
}

void UpbitWS::watchMyTrades(const std::string& symbol) {
    subscribe("trade_history", normalizeSymbol(symbol), true);
}

void UpbitWS::handleMessage(const std::string& message) {
    auto j = nlohmann::json::parse(message);

    if (!j.contains("type")) return;

    std::string type = j["type"];
    
    if (type == "ticker") {
        handleTickerMessage(j);
    } else if (type == "orderbook") {
        handleOrderBookMessage(j);
    } else if (type == "trade") {
        handleTradeMessage(j);
    } else if (type == "balance") {
        handleBalanceMessage(j);
    } else if (type == "order") {
        handleOrderMessage(j);
    } else if (type == "trade_history") {
        handleMyTradeMessage(j);
    } else if (type == "error") {
        handleErrorMessage(j);
    } else if (type == "auth") {
        handleAuthMessage(j);
    }
}

void UpbitWS::handleTickerMessage(const nlohmann::json& data) {
    std::string symbol = data["code"];
    
    emit(symbol, "ticker", {
        {"symbol", symbol},
        {"high", data["high_price"].get<double>()},
        {"low", data["low_price"].get<double>()},
        {"last", data["trade_price"].get<double>()},
        {"bid", data["highest_bid_price"].get<double>()},
        {"ask", data["lowest_ask_price"].get<double>()},
        {"baseVolume", data["acc_trade_volume_24h"].get<double>()},
        {"quoteVolume", data["acc_trade_price_24h"].get<double>()},
        {"percentage", data["signed_change_rate"].get<double>() * 100},
        {"timestamp", data["timestamp"]}
    });
}

void UpbitWS::handleOrderBookMessage(const nlohmann::json& data) {
    std::string symbol = data["code"];
    
    nlohmann::json orderbook;
    orderbook["symbol"] = symbol;
    orderbook["timestamp"] = data["timestamp"];
    
    std::vector<std::vector<double>> bids;
    std::vector<std::vector<double>> asks;
    
    for (const auto& bid : data["orderbook_units"]) {
        bids.push_back({bid["bid_price"].get<double>(), bid["bid_size"].get<double>()});
    }
    
    for (const auto& ask : data["orderbook_units"]) {
        asks.push_back({ask["ask_price"].get<double>(), ask["ask_size"].get<double>()});
    }
    
    orderbook["bids"] = bids;
    orderbook["asks"] = asks;
    
    emit(symbol, "orderbook", orderbook);
}

void UpbitWS::handleTradeMessage(const nlohmann::json& data) {
    std::string symbol = data["code"];
    
    emit(symbol, "trade", {
        {"id", data["sequential_id"]},
        {"symbol", symbol},
        {"price", data["trade_price"].get<double>()},
        {"amount", data["trade_volume"].get<double>()},
        {"side", data["ask_bid"] == "ASK" ? "sell" : "buy"},
        {"timestamp", data["timestamp"]}
    });
}

void UpbitWS::handleBalanceMessage(const nlohmann::json& data) {
    nlohmann::json balance;
    
    for (const auto& asset : data["data"]) {
        balance[asset["currency"]] = {
            {"free", asset["balance"].get<double>()},
            {"used", asset["locked"].get<double>()},
            {"total", asset["balance"].get<double>() + asset["locked"].get<double>()}
        };
    }
    
    emit("", "balance", balance);
}

void UpbitWS::handleOrderMessage(const nlohmann::json& data) {
    std::string symbol = data["market"];
    
    emit(symbol, "order", {
        {"id", data["uuid"]},
        {"clientOrderId", data["identifier"]},
        {"symbol", symbol},
        {"type", data["ord_type"]},
        {"side", data["side"]},
        {"price", data["price"].get<double>()},
        {"amount", data["volume"].get<double>()},
        {"filled", data["executed_volume"].get<double>()},
        {"remaining", data["remaining_volume"].get<double>()},
        {"status", data["state"]},
        {"timestamp", data["created_at"]}
    });
}

void UpbitWS::handleMyTradeMessage(const nlohmann::json& data) {
    std::string symbol = data["market"];
    
    emit(symbol, "mytrade", {
        {"id", data["uuid"]},
        {"orderId", data["order_uuid"]},
        {"symbol", symbol},
        {"type", data["ord_type"]},
        {"side", data["side"]},
        {"price", data["price"].get<double>()},
        {"amount", data["volume"].get<double>()},
        {"fee", data["paid_fee"].get<double>()},
        {"feeCurrency", data["fee_currency"]},
        {"timestamp", data["created_at"]}
    });
}

void UpbitWS::handleErrorMessage(const nlohmann::json& data) {
    if (data.contains("message")) {
        std::string error_message = data["message"];
        // Handle error appropriately
    }
}

void UpbitWS::handleAuthMessage(const nlohmann::json& data) {
    if (data.contains("result")) {
        authenticated_ = data["result"].get<bool>();
    }
}

} // namespace ccxt
