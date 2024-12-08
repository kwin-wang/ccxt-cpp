#include "../../../include/ccxt/exchanges/ws/ascendex_ws.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <openssl/hmac.h>
#include <iomanip>

namespace ccxt {

AscendexWS::AscendexWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Ascendex& exchange)
    : WebSocketClient(ioc, ctx), exchange_(exchange), lastPingTimestamp_(0), pingInterval_(15000), authenticated_(false) {
    apiKey_ = exchange_.apiKey;
    apiSecret_ = exchange_.secret;
}

std::string AscendexWS::getStreamUrl(bool isPrivate) const {
    if (exchange_.testnet) {
        return isPrivate ? "wss://api-test.ascendex-sandbox.io:443/api/pro/v1/stream" : "wss://api-test.ascendex-sandbox.io:443/api/pro/v1/stream";
    }
    return isPrivate ? "wss://ascendex.com:443/api/pro/v1/stream" : "wss://ascendex.com:443/api/pro/v1/stream";
}

std::string AscendexWS::sign(const std::string& message) const {
    unsigned char* digest = HMAC(EVP_sha256(),
                                apiSecret_.c_str(), apiSecret_.length(),
                                reinterpret_cast<const unsigned char*>(message.c_str()), message.length(),
                                nullptr, nullptr);
    
    std::stringstream ss;
    for(int i = 0; i < 32; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
    }
    return ss.str();
}

void AscendexWS::authenticate() {
    int64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    std::string message = std::to_string(timestamp) + "+stream";
    std::string signature = sign(message);

    nlohmann::json auth = {
        {"op", "auth"},
        {"id", std::to_string(timestamp)},
        {"t", timestamp},
        {"key", apiKey_},
        {"sig", signature}
    };

    send(auth.dump());
}

void AscendexWS::subscribe(const std::string& channel, const std::vector<std::string>& symbols, bool isPrivate) {
    if (isPrivate && !authenticated_) {
        authenticate();
    }

    nlohmann::json request = {
        {"op", "sub"},
        {"id", std::to_string(std::chrono::system_clock::now().time_since_epoch().count())}
    };

    if (symbols.empty()) {
        request["ch"] = channel;
    } else {
        std::vector<std::string> channels;
        for (const auto& symbol : symbols) {
            channels.push_back(channel + ":" + symbol);
        }
        request["ch"] = channels;
    }

    send(request.dump());
}

void AscendexWS::unsubscribe(const std::string& channel, const std::vector<std::string>& symbols, bool isPrivate) {
    nlohmann::json request = {
        {"op", "unsub"},
        {"id", std::to_string(std::chrono::system_clock::now().time_since_epoch().count())}
    };

    if (symbols.empty()) {
        request["ch"] = channel;
    } else {
        std::vector<std::string> channels;
        for (const auto& symbol : symbols) {
            channels.push_back(channel + ":" + symbol);
        }
        request["ch"] = channels;
    }

    send(request.dump());
}

void AscendexWS::ping() {
    nlohmann::json ping = {
        {"op", "ping"},
        {"id", std::to_string(std::chrono::system_clock::now().time_since_epoch().count())}
    };
    send(ping.dump());
}

void AscendexWS::startPingLoop() {
    std::thread([this]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(pingInterval_));
            ping();
        }
    }).detach();
}

void AscendexWS::watchTicker(const std::string& symbol) {
    subscribe("bbo", {symbol});
}

void AscendexWS::watchTickers(const std::vector<std::string>& symbols) {
    subscribe("bbo", symbols);
}

void AscendexWS::watchOrderBook(const std::string& symbol) {
    subscribe("depth", {symbol});
}

void AscendexWS::watchTrades(const std::string& symbol) {
    subscribe("trades", {symbol});
}

void AscendexWS::watchOHLCV(const std::string& symbol, const std::string& timeframe) {
    subscribe("bar:" + timeframe, {symbol});
}

void AscendexWS::watchBalance() {
    subscribe("order:cash", {}, true);
}

void AscendexWS::watchOrders() {
    subscribe("order:futures", {}, true);
}

void AscendexWS::watchMyTrades() {
    subscribe("trades:futures", {}, true);
}

void AscendexWS::watchPositions() {
    subscribe("position:futures", {}, true);
}

void AscendexWS::handleMessage(const std::string& message) {
    try {
        auto j = nlohmann::json::parse(message);

        if (j.contains("m")) {
            std::string method = j["m"];

            if (method == "ping") {
                nlohmann::json pong = {
                    {"op", "pong"},
                    {"id", j.value("id", "0")}
                };
                send(pong.dump());
                return;
            }

            if (method == "auth") {
                authenticated_ = (j.value("code", 0) == 0);
                if (authenticated_) {
                    emit("authenticated", j);
                    startPingLoop();
                } else {
                    emit("error", j);
                }
                return;
            }

            if (j.contains("data")) {
                auto data = j["data"];

                if (method == "bbo") {
                    handleTicker(data);
                } else if (method == "depth") {
                    handleOrderBook(data);
                } else if (method == "trades") {
                    handleTrade(data);
                } else if (method.find("bar") == 0) {
                    handleOHLCV(data);
                } else if (method == "order") {
                    handleOrder(data);
                } else if (method == "balance") {
                    handleBalance(data);
                } else if (method == "trades:futures") {
                    handleMyTrade(data);
                } else if (method == "position") {
                    handlePosition(data);
                }
            }
        }
    } catch (const std::exception& e) {
        emit("error", {{"message", e.what()}});
    }
}

void AscendexWS::handleTicker(const nlohmann::json& data) {
    nlohmann::json parsedTicker = {
        {"symbol", data["s"]},
        {"timestamp", data["t"]},
        {"datetime", exchange_.iso8601(data["t"])},
        {"high", nullptr},
        {"low", nullptr},
        {"bid", std::stod(data["b"])},
        {"bidVolume", std::stod(data["bq"])},
        {"ask", std::stod(data["a"])},
        {"askVolume", std::stod(data["aq"])},
        {"vwap", nullptr},
        {"open", nullptr},
        {"close", std::stod(data["a"])},  // Using ask as the close price
        {"last", std::stod(data["a"])},   // Using ask as the last price
        {"previousClose", nullptr},
        {"change", nullptr},
        {"percentage", nullptr},
        {"average", nullptr},
        {"baseVolume", nullptr},
        {"quoteVolume", nullptr},
        {"info", data}
    };
    emit("ticker", parsedTicker);
}

void AscendexWS::handleOrderBook(const nlohmann::json& data) {
    nlohmann::json bids = nlohmann::json::array();
    nlohmann::json asks = nlohmann::json::array();

    for (const auto& bid : data["bids"]) {
        bids.push_back({
            std::stod(bid[0]),  // price
            std::stod(bid[1])   // amount
        });
    }

    for (const auto& ask : data["asks"]) {
        asks.push_back({
            std::stod(ask[0]),  // price
            std::stod(ask[1])   // amount
        });
    }

    nlohmann::json parsedBook = {
        {"symbol", data["s"]},
        {"bids", bids},
        {"asks", asks},
        {"timestamp", data["t"]},
        {"datetime", exchange_.iso8601(data["t"])},
        {"nonce", data["seqnum"]},
        {"info", data}
    };

    emit("orderBook", parsedBook);
}

void AscendexWS::handleTrade(const nlohmann::json& data) {
    nlohmann::json parsedTrade = {
        {"id", data["tid"]},
        {"info", data},
        {"timestamp", data["t"]},
        {"datetime", exchange_.iso8601(data["t"])},
        {"symbol", data["s"]},
        {"type", nullptr},
        {"side", data["bm"] ? "buy" : "sell"},
        {"price", std::stod(data["p"])},
        {"amount", std::stod(data["q"])},
        {"cost", std::stod(data["p"]) * std::stod(data["q"])}
    };
    emit("trade", parsedTrade);
}

void AscendexWS::handleOHLCV(const nlohmann::json& data) {
    nlohmann::json parsedCandle = {
        data["t"],                    // timestamp
        std::stod(data["o"]),        // open
        std::stod(data["h"]),        // high
        std::stod(data["l"]),        // low
        std::stod(data["c"]),        // close
        std::stod(data["v"])         // volume
    };
    emit("ohlcv", parsedCandle);
}

void AscendexWS::handleBalance(const nlohmann::json& data) {
    nlohmann::json parsedBalance = {
        {"info", data},
        {"currency", data["a"]},
        {"free", std::stod(data["f"])},
        {"used", std::stod(data["l"])},
        {"total", std::stod(data["f"]) + std::stod(data["l"])}
    };
    emit("balance", parsedBalance);
}

void AscendexWS::handleOrder(const nlohmann::json& data) {
    std::string status;
    if (data["s"] == "New") {
        status = "open";
    } else if (data["s"] == "PartiallyFilled") {
        status = "open";
    } else if (data["s"] == "Filled") {
        status = "closed";
    } else if (data["s"] == "Canceled") {
        status = "canceled";
    } else if (data["s"] == "Rejected") {
        status = "rejected";
    } else {
        status = "unknown";
    }

    nlohmann::json parsedOrder = {
        {"id", data["orderId"]},
        {"clientOrderId", data["clientOrderId"]},
        {"info", data},
        {"timestamp", data["t"]},
        {"datetime", exchange_.iso8601(data["t"])},
        {"lastTradeTimestamp", nullptr},
        {"status", status},
        {"symbol", data["s"]},
        {"type", data["ot"]},
        {"side", data["sd"]},
        {"price", std::stod(data["p"])},
        {"amount", std::stod(data["q"])},
        {"filled", std::stod(data["cf"])},
        {"remaining", std::stod(data["q"]) - std::stod(data["cf"])},
        {"cost", std::stod(data["cf"]) * std::stod(data["ap"])},
        {"average", std::stod(data["ap"])},
        {"fee", {
            {"cost", std::stod(data["fee"])},
            {"currency", data["feeAsset"]}
        }}
    };
    emit("order", parsedOrder);
}

void AscendexWS::handleMyTrade(const nlohmann::json& data) {
    nlohmann::json parsedTrade = {
        {"id", data["tid"]},
        {"order", data["orderId"]},
        {"info", data},
        {"timestamp", data["t"]},
        {"datetime", exchange_.iso8601(data["t"])},
        {"symbol", data["s"]},
        {"type", data["ot"]},
        {"side", data["sd"]},
        {"price", std::stod(data["p"])},
        {"amount", std::stod(data["q"])},
        {"cost", std::stod(data["p"]) * std::stod(data["q"])},
        {"fee", {
            {"cost", std::stod(data["fee"])},
            {"currency", data["feeAsset"]}
        }}
    };
    emit("trade", parsedTrade);
}

void AscendexWS::handlePosition(const nlohmann::json& data) {
    nlohmann::json parsedPosition = {
        {"info", data},
        {"symbol", data["s"]},
        {"timestamp", data["t"]},
        {"datetime", exchange_.iso8601(data["t"])},
        {"long", data["sd"] == "Buy"},
        {"short", data["sd"] == "Sell"},
        {"side", data["sd"]},
        {"quantity", std::stod(data["pos"])},
        {"unrealizedPnl", std::stod(data["upl"])},
        {"leverage", std::stod(data["lever"])},
        {"collateral", std::stod(data["col"])},
        {"notional", std::stod(data["notional"])},
        {"markPrice", std::stod(data["mp"])},
        {"liquidationPrice", std::stod(data["liq"])}
    };
    emit("position", parsedPosition);
}

} // namespace ccxt
