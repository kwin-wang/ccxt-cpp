#include "../../../include/ccxt/exchanges/ws/bequant_ws.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <openssl/hmac.h>
#include <iomanip>

namespace ccxt {

BequantWS::BequantWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Bequant& exchange)
    : WebSocketClient(ioc, ctx), exchange_(exchange), authenticated_(false), lastNonce_(0) {
    apiKey_ = exchange_.apiKey;
    apiSecret_ = exchange_.secret;
}

std::string BequantWS::getStreamUrl(bool isPrivate) const {
    return "wss://api.bequant.io/api/3/ws/public";  // Bequant uses the same URL for both public and private
}

int64_t BequantWS::getNonce() {
    int64_t nonce = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    if (nonce <= lastNonce_) {
        nonce = lastNonce_ + 1;
    }
    lastNonce_ = nonce;
    return nonce;
}

std::string BequantWS::sign(const std::string& path, const std::string& nonce, const std::string& data) const {
    std::string message = path + nonce + data;
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

void BequantWS::authenticate() {
    std::string nonce = std::to_string(getNonce());
    std::string signature = sign("/api/3/ws/public", nonce);

    nlohmann::json auth = {
        {"method", "login"},
        {"params", {
            {"algo", "HS256"},
            {"pKey", apiKey_},
            {"nonce", nonce},
            {"signature", signature}
        }},
        {"id", nonce}
    };

    send(auth.dump());
}

void BequantWS::subscribe(const std::string& channel, const std::string& symbol, const nlohmann::json& params) {
    nlohmann::json request = {
        {"method", "subscribe"},
        {"ch", channel},
        {"params", params},
        {"id", std::to_string(getNonce())}
    };

    if (!symbol.empty()) {
        request["params"]["symbol"] = symbol;
    }

    std::string key = channel + (symbol.empty() ? "" : ":" + symbol);
    subscriptions_[key] = request.dump();
    send(request.dump());
}

void BequantWS::unsubscribe(const std::string& channel, const std::string& symbol) {
    nlohmann::json request = {
        {"method", "unsubscribe"},
        {"ch", channel},
        {"id", std::to_string(getNonce())}
    };

    if (!symbol.empty()) {
        request["params"]["symbol"] = symbol;
    }

    std::string key = channel + (symbol.empty() ? "" : ":" + symbol);
    subscriptions_.erase(key);
    send(request.dump());
}

void BequantWS::watchTicker(const std::string& symbol) {
    subscribe("ticker", symbol);
}

void BequantWS::watchTickers(const std::vector<std::string>& symbols) {
    for (const auto& symbol : symbols) {
        watchTicker(symbol);
    }
}

void BequantWS::watchOrderBook(const std::string& symbol, const int limit) {
    nlohmann::json params = {{"limit", limit}};
    subscribe("orderbook", symbol, params);
}

void BequantWS::watchTrades(const std::string& symbol, const int limit) {
    nlohmann::json params = {{"limit", limit}};
    subscribe("trades", symbol, params);
}

void BequantWS::watchOHLCV(const std::string& symbol, const std::string& timeframe) {
    nlohmann::json params = {{"period", timeframe}};
    subscribe("candles", symbol, params);
}

void BequantWS::watchBalance() {
    if (!authenticated_) {
        authenticate();
    }
    subscribe("spot/balance", "");
}

void BequantWS::watchOrders(const std::string& symbol) {
    if (!authenticated_) {
        authenticate();
    }
    subscribe("spot/order", symbol);
}

void BequantWS::watchMyTrades(const std::string& symbol) {
    if (!authenticated_) {
        authenticate();
    }
    subscribe("spot/trade", symbol);
}

void BequantWS::handleMessage(const std::string& message) {
    try {
        auto j = nlohmann::json::parse(message);

        if (j.contains("method")) {
            std::string method = j["method"];
            
            if (method == "ticker") {
                handleTicker(j["data"]);
            } else if (method == "orderbook") {
                bool isSnapshot = j.contains("snapshot") && j["snapshot"].get<bool>();
                handleOrderBook(j["data"], isSnapshot);
            } else if (method == "trades") {
                handleTrade(j["data"]);
            } else if (method == "candles") {
                handleOHLCV(j["data"]);
            } else if (method == "spot/balance") {
                handleBalance(j["data"]);
            } else if (method == "spot/order") {
                handleOrder(j["data"]);
            } else if (method == "spot/trade") {
                handleMyTrade(j["data"]);
            }
        } else if (j.contains("result")) {
            if (j.contains("id") && j["id"].is_string() && j["id"].get<std::string>() == "login") {
                authenticated_ = true;
                emit("authenticated", j);
            }
        } else if (j.contains("error")) {
            emit("error", j);
        }
    } catch (const std::exception& e) {
        emit("error", {{"message", e.what()}});
    }
}

void BequantWS::handleTicker(const nlohmann::json& data) {
    nlohmann::json parsedTicker = {
        {"symbol", data["symbol"]},
        {"timestamp", data["timestamp"]},
        {"datetime", exchange_.iso8601(data["timestamp"])},
        {"high", std::stod(data["high"])},
        {"low", std::stod(data["low"])},
        {"bid", std::stod(data["bid"])},
        {"bidVolume", std::stod(data["bidSize"])},
        {"ask", std::stod(data["ask"])},
        {"askVolume", std::stod(data["askSize"])},
        {"vwap", std::stod(data["volumeQuote"]) / std::stod(data["volume"])},
        {"open", std::stod(data["open"])},
        {"close", std::stod(data["last"])},
        {"last", std::stod(data["last"])},
        {"previousClose", nullptr},
        {"change", std::stod(data["last"]) - std::stod(data["open"])},
        {"percentage", ((std::stod(data["last"]) - std::stod(data["open"])) / std::stod(data["open"])) * 100},
        {"average", (std::stod(data["high"]) + std::stod(data["low"])) / 2},
        {"baseVolume", std::stod(data["volume"])},
        {"quoteVolume", std::stod(data["volumeQuote"])},
        {"info", data}
    };
    emit("ticker", parsedTicker);
}

void BequantWS::handleOrderBook(const nlohmann::json& data, bool isSnapshot) {
    nlohmann::json bids = nlohmann::json::array();
    nlohmann::json asks = nlohmann::json::array();

    for (const auto& bid : data["bid"]) {
        bids.push_back({
            std::stod(bid["price"]),
            std::stod(bid["size"])
        });
    }

    for (const auto& ask : data["ask"]) {
        asks.push_back({
            std::stod(ask["price"]),
            std::stod(ask["size"])
        });
    }

    nlohmann::json parsedBook = {
        {"symbol", data["symbol"]},
        {"bids", bids},
        {"asks", asks},
        {"timestamp", data["timestamp"]},
        {"datetime", exchange_.iso8601(data["timestamp"])},
        {"nonce", data["sequence"]},
        {"info", data}
    };

    std::string event = isSnapshot ? "orderBook" : "orderBookUpdate";
    emit(event, parsedBook);
}

void BequantWS::handleTrade(const nlohmann::json& data) {
    nlohmann::json parsedTrade = {
        {"id", data["id"]},
        {"info", data},
        {"timestamp", data["timestamp"]},
        {"datetime", exchange_.iso8601(data["timestamp"])},
        {"symbol", data["symbol"]},
        {"type", nullptr},
        {"side", data["side"]},
        {"price", std::stod(data["price"])},
        {"amount", std::stod(data["quantity"])},
        {"cost", std::stod(data["price"]) * std::stod(data["quantity"])}
    };
    emit("trade", parsedTrade);
}

void BequantWS::handleOHLCV(const nlohmann::json& data) {
    nlohmann::json parsedCandle = {
        data["timestamp"],
        std::stod(data["open"]),
        std::stod(data["high"]),
        std::stod(data["low"]),
        std::stod(data["close"]),
        std::stod(data["volume"])
    };
    emit("ohlcv", parsedCandle);
}

void BequantWS::handleBalance(const nlohmann::json& data) {
    for (const auto& [currency, balance] : data.items()) {
        nlohmann::json parsedBalance = {
            {"info", balance},
            {"free", std::stod(balance["available"])},
            {"used", std::stod(balance["reserved"])},
            {"total", std::stod(balance["available"]) + std::stod(balance["reserved"])},
            {"currency", currency}
        };
        emit("balance", parsedBalance);
    }
}

void BequantWS::handleOrder(const nlohmann::json& data) {
    std::string status;
    if (data["status"] == "new") {
        status = "open";
    } else if (data["status"] == "suspended") {
        status = "open";
    } else if (data["status"] == "partiallyFilled") {
        status = "open";
    } else if (data["status"] == "filled") {
        status = "closed";
    } else if (data["status"] == "canceled") {
        status = "canceled";
    } else if (data["status"] == "expired") {
        status = "expired";
    } else {
        status = "unknown";
    }

    nlohmann::json parsedOrder = {
        {"id", data["id"]},
        {"clientOrderId", data["clientOrderId"]},
        {"info", data},
        {"timestamp", data["createdAt"]},
        {"datetime", exchange_.iso8601(data["createdAt"])},
        {"lastTradeTimestamp", data["updatedAt"]},
        {"status", status},
        {"symbol", data["symbol"]},
        {"type", data["type"]},
        {"side", data["side"]},
        {"price", std::stod(data["price"])},
        {"amount", std::stod(data["quantity"])},
        {"filled", std::stod(data["cumQuantity"])},
        {"remaining", std::stod(data["quantity"]) - std::stod(data["cumQuantity"])},
        {"cost", std::stod(data["cumQuantity"]) * std::stod(data["price"])},
        {"average", data.contains("avgPrice") ? std::stod(data["avgPrice"]) : 0.0},
        {"fee", nullptr}  // Fee information not provided in real-time
    };
    emit("order", parsedOrder);
}

void BequantWS::handleMyTrade(const nlohmann::json& data) {
    nlohmann::json parsedTrade = {
        {"id", data["id"]},
        {"order", data["orderId"]},
        {"info", data},
        {"timestamp", data["timestamp"]},
        {"datetime", exchange_.iso8601(data["timestamp"])},
        {"symbol", data["symbol"]},
        {"type", data["type"]},
        {"side", data["side"]},
        {"price", std::stod(data["price"])},
        {"amount", std::stod(data["quantity"])},
        {"cost", std::stod(data["price"]) * std::stod(data["quantity"])},
        {"fee", {
            {"cost", std::stod(data["fee"])},
            {"currency", data["feeCurrency"]}
        }}
    };
    emit("trade", parsedTrade);
}

} // namespace ccxt
