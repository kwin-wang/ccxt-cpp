#include "../../../include/ccxt/exchanges/ws/binanceus_ws.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <openssl/hmac.h>
#include <iomanip>

namespace ccxt {

BinanceUSWS::BinanceUSWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, BinanceUS& exchange)
    : WebSocketClient(ioc, ctx), exchange_(exchange), authenticated_(false), lastPingTimestamp_(0), pingInterval_(3000) {
    apiKey_ = exchange_.apiKey;
    apiSecret_ = exchange_.secret;
}

std::string BinanceUSWS::getStreamUrl(bool isPrivate) const {
    return isPrivate ? "wss://stream.binance.us:9443/ws/" : "wss://stream.binance.us:9443/stream";
}

std::string BinanceUSWS::sign(const std::string& path, const std::string& method, const nlohmann::json& params) const {
    std::string query = exchange_.urlencode(params);
    std::string auth = query + "&timestamp=" + std::to_string(exchange_.milliseconds());
    
    unsigned char* digest = HMAC(EVP_sha256(),
                                apiSecret_.c_str(), apiSecret_.length(),
                                reinterpret_cast<const unsigned char*>(auth.c_str()), auth.length(),
                                nullptr, nullptr);
    
    std::stringstream ss;
    for(int i = 0; i < 32; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
    }
    return auth + "&signature=" + ss.str();
}

void BinanceUSWS::listenKey() {
    nlohmann::json params = {
        {"timestamp", exchange_.milliseconds()}
    };
    
    std::string signature = sign("/api/v3/userDataStream", "POST", params);
    // Make REST API call to get listenKey
    // This would typically be done through the exchange's REST API
    // For now, we'll assume we have the listenKey
    listenKey_ = "dummy_listen_key";
    startListenKeyTimer();
}

void BinanceUSWS::startListenKeyTimer() {
    std::thread([this]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::minutes(30));
            // Extend listenKey validity
            nlohmann::json params = {
                {"timestamp", exchange_.milliseconds()}
            };
            sign("/api/v3/userDataStream", "PUT", params);
        }
    }).detach();
}

void BinanceUSWS::authenticate() {
    if (!authenticated_) {
        listenKey();
        authenticated_ = true;
    }
}

void BinanceUSWS::subscribe(const std::string& channel, const std::string& symbol, const nlohmann::json& params) {
    nlohmann::json request = {
        {"method", "SUBSCRIBE"},
        {"params", nlohmann::json::array()},
        {"id", exchange_.milliseconds()}
    };

    std::string stream = symbol.empty() ? channel : symbol.lower() + "@" + channel;
    request["params"].push_back(stream);

    subscriptions_[stream] = request.dump();
    send(request.dump());
}

void BinanceUSWS::unsubscribe(const std::string& channel, const std::string& symbol) {
    nlohmann::json request = {
        {"method", "UNSUBSCRIBE"},
        {"params", nlohmann::json::array()},
        {"id", exchange_.milliseconds()}
    };

    std::string stream = symbol.empty() ? channel : symbol.lower() + "@" + channel;
    request["params"].push_back(stream);

    subscriptions_.erase(stream);
    send(request.dump());
}

void BinanceUSWS::ping() {
    nlohmann::json ping = {{"op", "ping"}};
    send(ping.dump());
}

void BinanceUSWS::startPingLoop() {
    std::thread([this]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(pingInterval_));
            ping();
        }
    }).detach();
}

std::string BinanceUSWS::getSymbol(const std::string& market) const {
    return market;  // Implement proper symbol conversion if needed
}

void BinanceUSWS::watchTicker(const std::string& symbol) {
    subscribe("ticker", symbol);
}

void BinanceUSWS::watchTickers(const std::vector<std::string>& symbols) {
    for (const auto& symbol : symbols) {
        watchTicker(symbol);
    }
}

void BinanceUSWS::watchOrderBook(const std::string& symbol, const int limit) {
    std::string channel = "depth" + std::to_string(limit);
    subscribe(channel, symbol);
}

void BinanceUSWS::watchTrades(const std::string& symbol) {
    subscribe("trade", symbol);
}

void BinanceUSWS::watchOHLCV(const std::string& symbol, const std::string& timeframe) {
    subscribe("kline_" + timeframe, symbol);
}

void BinanceUSWS::watchBidsAsks(const std::string& symbol) {
    subscribe("bookTicker", symbol);
}

void BinanceUSWS::watchBalance() {
    authenticate();
    subscribe(listenKey_);
}

void BinanceUSWS::watchOrders(const std::string& symbol) {
    authenticate();
    subscribe(listenKey_);
}

void BinanceUSWS::watchMyTrades(const std::string& symbol) {
    authenticate();
    subscribe(listenKey_);
}

void BinanceUSWS::handleMessage(const std::string& message) {
    try {
        auto j = nlohmann::json::parse(message);

        if (j.contains("e")) {  // Event type exists
            std::string eventType = j["e"];

            if (eventType == "24hrTicker") {
                handleTicker(j);
            } else if (eventType == "trade") {
                handleTrade(j);
            } else if (eventType == "depthUpdate") {
                handleOrderBook(j, false);
            } else if (eventType == "kline") {
                handleOHLCV(j);
            } else if (eventType == "outboundAccountInfo") {
                handleOutboundAccountInfo(j);
            } else if (eventType == "executionReport") {
                handleExecutionReport(j);
            }
        } else if (j.contains("result") && j["result"].is_null()) {
            // Subscription confirmation
            return;
        }
    } catch (const std::exception& e) {
        emit("error", {{"message", e.what()}});
    }
}

void BinanceUSWS::handleTicker(const nlohmann::json& data) {
    nlohmann::json parsedTicker = {
        {"symbol", data["s"]},
        {"timestamp", data["E"]},
        {"datetime", exchange_.iso8601(data["E"])},
        {"high", std::stod(data["h"])},
        {"low", std::stod(data["l"])},
        {"bid", std::stod(data["b"])},
        {"bidVolume", std::stod(data["B"])},
        {"ask", std::stod(data["a"])},
        {"askVolume", std::stod(data["A"])},
        {"vwap", std::stod(data["w"])},
        {"open", std::stod(data["o"])},
        {"close", std::stod(data["c"])},
        {"last", std::stod(data["c"])},
        {"previousClose", std::stod(data["x"])},
        {"change", std::stod(data["p"])},
        {"percentage", std::stod(data["P"])},
        {"average", nullptr},
        {"baseVolume", std::stod(data["v"])},
        {"quoteVolume", std::stod(data["q"])},
        {"info", data}
    };
    emit("ticker", parsedTicker);
}

void BinanceUSWS::handleOrderBook(const nlohmann::json& data, bool isSnapshot) {
    nlohmann::json bids = nlohmann::json::array();
    nlohmann::json asks = nlohmann::json::array();

    for (const auto& bid : data["b"]) {
        bids.push_back({
            std::stod(bid[0]),  // price
            std::stod(bid[1])   // amount
        });
    }

    for (const auto& ask : data["a"]) {
        asks.push_back({
            std::stod(ask[0]),  // price
            std::stod(ask[1])   // amount
        });
    }

    nlohmann::json parsedBook = {
        {"symbol", data["s"]},
        {"bids", bids},
        {"asks", asks},
        {"timestamp", data["E"]},
        {"datetime", exchange_.iso8601(data["E"])},
        {"nonce", data["u"]},  // Update ID
        {"info", data}
    };

    std::string event = isSnapshot ? "orderBook" : "orderBookUpdate";
    emit(event, parsedBook);
}

void BinanceUSWS::handleTrade(const nlohmann::json& data) {
    nlohmann::json parsedTrade = {
        {"id", data["t"]},
        {"info", data},
        {"timestamp", data["E"]},
        {"datetime", exchange_.iso8601(data["E"])},
        {"symbol", data["s"]},
        {"type", nullptr},
        {"side", data["m"] ? "sell" : "buy"},
        {"price", std::stod(data["p"])},
        {"amount", std::stod(data["q"])},
        {"cost", std::stod(data["p"]) * std::stod(data["q"])}
    };
    emit("trade", parsedTrade);
}

void BinanceUSWS::handleOHLCV(const nlohmann::json& data) {
    auto k = data["k"];
    nlohmann::json parsedCandle = {
        k["t"],                     // timestamp
        std::stod(k["o"]),         // open
        std::stod(k["h"]),         // high
        std::stod(k["l"]),         // low
        std::stod(k["c"]),         // close
        std::stod(k["v"])          // volume
    };
    emit("ohlcv", parsedCandle);
}

void BinanceUSWS::handleOutboundAccountInfo(const nlohmann::json& data) {
    for (const auto& balance : data["B"]) {
        nlohmann::json parsedBalance = {
            {"currency", balance["a"]},
            {"free", std::stod(balance["f"])},
            {"used", std::stod(balance["l"])},
            {"total", std::stod(balance["f"]) + std::stod(balance["l"])},
            {"info", balance}
        };
        emit("balance", parsedBalance);
    }
}

void BinanceUSWS::handleExecutionReport(const nlohmann::json& data) {
    // Handle both order updates and trade updates
    std::string executionType = data["x"];
    
    // Handle order update
    std::string status;
    if (executionType == "NEW") {
        status = "open";
    } else if (executionType == "PARTIALLY_FILLED") {
        status = "open";
    } else if (executionType == "FILLED") {
        status = "closed";
    } else if (executionType == "CANCELED") {
        status = "canceled";
    } else if (executionType == "REJECTED") {
        status = "rejected";
    } else if (executionType == "EXPIRED") {
        status = "expired";
    } else {
        status = "unknown";
    }

    nlohmann::json parsedOrder = {
        {"id", data["i"]},
        {"clientOrderId", data["c"]},
        {"info", data},
        {"timestamp", data["T"]},
        {"datetime", exchange_.iso8601(data["T"])},
        {"lastTradeTimestamp", data["E"]},
        {"status", status},
        {"symbol", data["s"]},
        {"type", data["o"]},
        {"side", data["S"].get<std::string>() == "BUY" ? "buy" : "sell"},
        {"price", std::stod(data["p"])},
        {"amount", std::stod(data["q"])},
        {"filled", std::stod(data["z"])},
        {"remaining", std::stod(data["q"]) - std::stod(data["z"])},
        {"cost", std::stod(data["z"]) * std::stod(data["L"])},
        {"average", std::stod(data["L"])},
        {"fee", {
            {"cost", std::stod(data["n"])},
            {"currency", data["N"]}
        }}
    };
    emit("order", parsedOrder);

    // If it's a trade, emit a trade event
    if (executionType == "TRADE") {
        nlohmann::json parsedTrade = {
            {"id", data["t"]},
            {"order", data["i"]},
            {"info", data},
            {"timestamp", data["T"]},
            {"datetime", exchange_.iso8601(data["T"])},
            {"symbol", data["s"]},
            {"type", data["o"]},
            {"side", data["S"].get<std::string>() == "BUY" ? "buy" : "sell"},
            {"price", std::stod(data["L"])},
            {"amount", std::stod(data["l"])},
            {"cost", std::stod(data["L"]) * std::stod(data["l"])},
            {"fee", {
                {"cost", std::stod(data["n"])},
                {"currency", data["N"]}
            }}
        };
        emit("trade", parsedTrade);
    }
}

} // namespace ccxt
