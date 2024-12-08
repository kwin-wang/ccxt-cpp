#include "../../../include/ccxt/exchanges/ws/binancecoinm_ws.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <openssl/hmac.h>
#include <iomanip>

namespace ccxt {

BinanceCoinMWS::BinanceCoinMWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, BinanceCoinM& exchange)
    : WebSocketClient(ioc, ctx), exchange_(exchange), authenticated_(false), lastPingTimestamp_(0), pingInterval_(3000) {
    apiKey_ = exchange_.apiKey;
    apiSecret_ = exchange_.secret;
}

std::string BinanceCoinMWS::getStreamUrl(bool isPrivate) const {
    if (exchange_.testnet) {
        return isPrivate ? "wss://dstream.binancefuture.com/ws/" : "wss://dstream.binancefuture.com/stream";
    }
    return isPrivate ? "wss://dstream.binance.com/ws/" : "wss://dstream.binance.com/stream";
}

std::string BinanceCoinMWS::sign(const std::string& path, const std::string& method, const nlohmann::json& params) const {
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

void BinanceCoinMWS::listenKey() {
    nlohmann::json params = {
        {"timestamp", exchange_.milliseconds()}
    };
    
    std::string signature = sign("/dapi/v1/listenKey", "POST", params);
    // Make REST API call to get listenKey
    // This would typically be done through the exchange's REST API
    // For now, we'll assume we have the listenKey
    listenKey_ = "dummy_listen_key";
    startListenKeyTimer();
}

void BinanceCoinMWS::startListenKeyTimer() {
    std::thread([this]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::minutes(30));
            // Extend listenKey validity
            nlohmann::json params = {
                {"timestamp", exchange_.milliseconds()}
            };
            sign("/dapi/v1/listenKey", "PUT", params);
        }
    }).detach();
}

void BinanceCoinMWS::authenticate() {
    if (!authenticated_) {
        listenKey();
        authenticated_ = true;
    }
}

void BinanceCoinMWS::subscribe(const std::string& channel, const std::string& symbol, const nlohmann::json& params) {
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

void BinanceCoinMWS::unsubscribe(const std::string& channel, const std::string& symbol) {
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

void BinanceCoinMWS::ping() {
    nlohmann::json ping = {{"op", "ping"}};
    send(ping.dump());
}

void BinanceCoinMWS::startPingLoop() {
    std::thread([this]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(pingInterval_));
            ping();
        }
    }).detach();
}

std::string BinanceCoinMWS::getSymbol(const std::string& market) const {
    return market;  // Implement proper symbol conversion if needed
}

void BinanceCoinMWS::watchTicker(const std::string& symbol) {
    subscribe("ticker", symbol);
}

void BinanceCoinMWS::watchTickers(const std::vector<std::string>& symbols) {
    for (const auto& symbol : symbols) {
        watchTicker(symbol);
    }
}

void BinanceCoinMWS::watchOrderBook(const std::string& symbol, const int limit) {
    std::string channel = "depth" + std::to_string(limit);
    subscribe(channel, symbol);
}

void BinanceCoinMWS::watchTrades(const std::string& symbol) {
    subscribe("trade", symbol);
}

void BinanceCoinMWS::watchOHLCV(const std::string& symbol, const std::string& timeframe) {
    subscribe("kline_" + timeframe, symbol);
}

void BinanceCoinMWS::watchMarkPrice(const std::string& symbol) {
    subscribe("markPrice", symbol);
}

void BinanceCoinMWS::watchLiquidations(const std::string& symbol) {
    subscribe("forceOrder", symbol);
}

void BinanceCoinMWS::watchBalance() {
    authenticate();
    subscribe(listenKey_);
}

void BinanceCoinMWS::watchOrders(const std::string& symbol) {
    authenticate();
    subscribe(listenKey_);
}

void BinanceCoinMWS::watchMyTrades(const std::string& symbol) {
    authenticate();
    subscribe(listenKey_);
}

void BinanceCoinMWS::watchPositions(const std::string& symbol) {
    authenticate();
    subscribe(listenKey_);
}

void BinanceCoinMWS::handleMessage(const std::string& message) {
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
            } else if (eventType == "markPriceUpdate") {
                handleMarkPrice(j);
            } else if (eventType == "forceOrder") {
                handleLiquidation(j);
            } else if (eventType == "ACCOUNT_UPDATE") {
                handleAccountUpdate(j);
            } else if (eventType == "ORDER_TRADE_UPDATE") {
                handleOrder(j);
            }
        } else if (j.contains("result") && j["result"].is_null()) {
            // Subscription confirmation
            return;
        }
    } catch (const std::exception& e) {
        emit("error", {{"message", e.what()}});
    }
}

void BinanceCoinMWS::handleTicker(const nlohmann::json& data) {
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

void BinanceCoinMWS::handleOrderBook(const nlohmann::json& data, bool isSnapshot) {
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

void BinanceCoinMWS::handleTrade(const nlohmann::json& data) {
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

void BinanceCoinMWS::handleOHLCV(const nlohmann::json& data) {
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

void BinanceCoinMWS::handleMarkPrice(const nlohmann::json& data) {
    nlohmann::json parsedMarkPrice = {
        {"symbol", data["s"]},
        {"timestamp", data["E"]},
        {"datetime", exchange_.iso8601(data["E"])},
        {"markPrice", std::stod(data["p"])},
        {"indexPrice", std::stod(data["i"])},
        {"fundingRate", std::stod(data["r"])},
        {"fundingTime", data["T"]},
        {"info", data}
    };
    emit("markPrice", parsedMarkPrice);
}

void BinanceCoinMWS::handleLiquidation(const nlohmann::json& data) {
    auto o = data["o"];
    nlohmann::json parsedLiquidation = {
        {"symbol", o["s"]},
        {"timestamp", data["E"]},
        {"datetime", exchange_.iso8601(data["E"])},
        {"price", std::stod(o["p"])},
        {"amount", std::stod(o["q"])},
        {"side", o["S"].get<std::string>() == "BUY" ? "buy" : "sell"},
        {"type", "liquidation"},
        {"info", data}
    };
    emit("liquidation", parsedLiquidation);
}

void BinanceCoinMWS::handleAccountUpdate(const nlohmann::json& data) {
    auto a = data["a"];
    
    // Handle balance updates
    if (a.contains("B")) {
        for (const auto& balance : a["B"]) {
            nlohmann::json parsedBalance = {
                {"currency", balance["a"]},
                {"free", std::stod(balance["wb"])},
                {"used", std::stod(balance["cw"])},
                {"total", std::stod(balance["wb"]) + std::stod(balance["cw"])},
                {"info", balance}
            };
            emit("balance", parsedBalance);
        }
    }

    // Handle position updates
    if (a.contains("P")) {
        for (const auto& position : a["P"]) {
            nlohmann::json parsedPosition = {
                {"info", position},
                {"symbol", position["s"]},
                {"timestamp", data["E"]},
                {"datetime", exchange_.iso8601(data["E"])},
                {"long", position["ps"] == "LONG"},
                {"short", position["ps"] == "SHORT"},
                {"side", position["ps"]},
                {"quantity", std::stod(position["pa"])},
                {"unrealizedPnl", std::stod(position["up"])},
                {"leverage", std::stod(position["l"])},
                {"entryPrice", std::stod(position["ep"])}
            };
            emit("position", parsedPosition);
        }
    }
}

void BinanceCoinMWS::handleOrder(const nlohmann::json& data) {
    auto o = data["o"];
    
    std::string status;
    std::string orderStatus = o["X"];
    if (orderStatus == "NEW") {
        status = "open";
    } else if (orderStatus == "PARTIALLY_FILLED") {
        status = "open";
    } else if (orderStatus == "FILLED") {
        status = "closed";
    } else if (orderStatus == "CANCELED") {
        status = "canceled";
    } else if (orderStatus == "EXPIRED") {
        status = "expired";
    } else if (orderStatus == "REJECTED") {
        status = "rejected";
    } else {
        status = "unknown";
    }

    nlohmann::json parsedOrder = {
        {"id", o["i"]},
        {"clientOrderId", o["c"]},
        {"info", o},
        {"timestamp", o["T"]},
        {"datetime", exchange_.iso8601(o["T"])},
        {"lastTradeTimestamp", data["E"]},
        {"status", status},
        {"symbol", o["s"]},
        {"type", o["o"]},
        {"side", o["S"].get<std::string>() == "BUY" ? "buy" : "sell"},
        {"price", std::stod(o["p"])},
        {"amount", std::stod(o["q"])},
        {"filled", std::stod(o["z"])},
        {"remaining", std::stod(o["q"]) - std::stod(o["z"])},
        {"cost", std::stod(o["z"]) * std::stod(o["ap"])},
        {"average", std::stod(o["ap"])},
        {"fee", {
            {"cost", std::stod(o["n"])},
            {"currency", o["N"]}
        }}
    };
    emit("order", parsedOrder);
}

void BinanceCoinMWS::handleMyTrade(const nlohmann::json& data) {
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
        {"amount", std::stod(data["q"])},
        {"cost", std::stod(data["L"]) * std::stod(data["q"])},
        {"fee", {
            {"cost", std::stod(data["n"])},
            {"currency", data["N"]}
        }}
    };
    emit("trade", parsedTrade);
}

} // namespace ccxt
