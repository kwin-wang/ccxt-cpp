#include "../../../include/ccxt/exchanges/ws/bitget_ws.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <chrono>
#include <boost/crc.hpp>

namespace ccxt {

BitgetWS::BitgetWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Bitget& exchange)
    : WebSocketClient(ioc, ctx), exchange_(exchange) {
    options_ = {
        {"watchOrderBookRate", 100},
        {"liquidationsLimit", 1000},
        {"tradesLimit", 1000},
        {"ordersLimit", 1000},
        {"OHLCVLimit", 1000},
        {"watchOrderBookLimit", 1000},
        {"watchOrderBook", {
            {"maxRetries", 3},
            {"checksum", true}
        }}
    };
}

std::string BitgetWS::getEndpoint() {
    return exchange_.getTestMode() ? "wss://ws-mix-test.bitget.com/spot/v1/stream" : "wss://ws-mix.bitget.com/spot/v1/stream";
}

void BitgetWS::authenticate() {
    auto timestamp = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count());

    std::string signature = sign(timestamp, "GET", "/user/verify", "");

    nlohmann::json request = {
        {"op", "login"},
        {"args", {{
            {"apiKey", exchange_.getApiKey()},
            {"passphrase", exchange_.getPassword()},
            {"timestamp", timestamp},
            {"sign", signature}
        }}}
    };

    send(request.dump());
}

void BitgetWS::watchTicker(const std::string& symbol) {
    subscribe("ticker", symbol);
}

void BitgetWS::watchTickers(const std::vector<std::string>& symbols) {
    for (const auto& symbol : symbols) {
        subscribe("ticker", symbol);
    }
}

void BitgetWS::watchOrderBook(const std::string& symbol, const std::string& limit) {
    std::string channel = limit.empty() ? "books" : "books" + limit;
    subscribe(channel, symbol);
}

void BitgetWS::watchTrades(const std::string& symbol) {
    subscribe("trade", symbol);
}

void BitgetWS::watchOHLCV(const std::string& symbol, const std::string& timeframe) {
    subscribe("candle" + timeframe, symbol);
}

void BitgetWS::watchBidsAsks(const std::string& symbol) {
    subscribe("bbo", symbol);
}

void BitgetWS::watchBalance() {
    authenticate();
    subscribe("account", "*");
}

void BitgetWS::watchOrders() {
    authenticate();
    subscribe("orders", "*");
}

void BitgetWS::watchMyTrades() {
    authenticate();
    subscribe("myTrades", "*");
}

void BitgetWS::watchPositions() {
    authenticate();
    subscribe("positions", "*");
}

std::string BitgetWS::sign(const std::string& timestamp, const std::string& method,
                          const std::string& path, const std::string& body) {
    std::string message = timestamp + method + path + body;
    return exchange_.hmac(message, exchange_.getSecret(), "sha256", "hex");
}

void BitgetWS::subscribe(const std::string& channel, const std::string& instId,
                        const nlohmann::json& args) {
    nlohmann::json request = {
        {"op", "subscribe"},
        {"args", {{
            {"channel", channel},
            {"instId", instId}
        }}}
    };

    // Add any additional arguments
    if (!args.empty()) {
        for (auto& [key, value] : args.items()) {
            request["args"][0][key] = value;
        }
    }

    // Store subscription for later use
    std::string key = channel + ":" + instId;
    subscriptions_[key] = request.dump();

    send(request.dump());
}

void BitgetWS::unsubscribe(const std::string& channel, const std::string& instId) {
    nlohmann::json request = {
        {"op", "unsubscribe"},
        {"args", {{
            {"channel", channel},
            {"instId", instId}
        }}}
    };

    // Remove subscription from storage
    std::string key = channel + ":" + instId;
    subscriptions_.erase(key);

    send(request.dump());
}

void BitgetWS::handleMessage(const std::string& message) {
    try {
        auto j = nlohmann::json::parse(message);
        
        if (j.contains("event")) {
            std::string event = j["event"];
            
            if (event == "login") {
                if (j["code"] == "00000") {
                    authenticated_ = true;
                    emit("authenticated", j);
                } else {
                    emit("error", j);
                }
                return;
            }
            
            if (event == "subscribe" || event == "unsubscribe") {
                emit(event, j);
                return;
            }
            
            if (event == "error") {
                handleError(j);
                return;
            }
        }
        
        if (j.contains("data") && j.contains("arg")) {
            const auto& data = j["data"];
            const auto& arg = j["arg"];
            std::string channel = arg["channel"];
            std::string instId = arg["instId"];
            
            if (channel == "ticker") {
                handleTicker(data);
            } else if (channel.find("books") == 0) {
                handleOrderBook(data);
            } else if (channel == "trade") {
                handleTrade(data);
            } else if (channel.find("candle") == 0) {
                handleOHLCV(data);
            } else if (channel == "bbo") {
                handleBidsAsks(data);
            } else if (channel == "account") {
                handleBalance(data);
            } else if (channel == "orders") {
                handleOrder(data);
            } else if (channel == "myTrades") {
                handleMyTrade(data);
            } else if (channel == "positions") {
                handlePosition(data);
            }
        }
    } catch (const std::exception& e) {
        emit("error", {{"message", e.what()}});
    }
}

void BitgetWS::handleError(const nlohmann::json& data) {
    emit("error", data);
}

void BitgetWS::handleTicker(const nlohmann::json& data) {
    for (const auto& ticker : data) {
        nlohmann::json parsedTicker = {
            {"symbol", ticker["instId"]},
            {"timestamp", std::stoll(ticker["ts"])},
            {"datetime", exchange_.iso8601(std::stoll(ticker["ts"]))},
            {"high", std::stod(ticker["high24h"])},
            {"low", std::stod(ticker["low24h"])},
            {"bid", std::stod(ticker["bestBid"])},
            {"bidVolume", std::stod(ticker["bidSz"])},
            {"ask", std::stod(ticker["bestAsk"])},
            {"askVolume", std::stod(ticker["askSz"])},
            {"vwap", std::stod(ticker["vwap"])},
            {"open", std::stod(ticker["open24h"])},
            {"close", std::stod(ticker["last"])},
            {"last", std::stod(ticker["last"])},
            {"previousClose", nullptr},
            {"change", std::stod(ticker["priceChange24h"])},
            {"percentage", std::stod(ticker["priceChangePercent24h"])},
            {"average", nullptr},
            {"baseVolume", std::stod(ticker["baseVolume"])},
            {"quoteVolume", std::stod(ticker["quoteVolume"])},
            {"info", ticker}
        };
        emit("ticker", parsedTicker);
    }
}

void BitgetWS::handleOrderBook(const nlohmann::json& data) {
    for (const auto& book : data) {
        bool isSnapshot = book.contains("action") && book["action"] == "snapshot";
        
        nlohmann::json bids = nlohmann::json::array();
        nlohmann::json asks = nlohmann::json::array();
        
        if (book.contains("bids")) {
            for (const auto& bid : book["bids"]) {
                bids.push_back({
                    std::stod(bid[0]),  // price
                    std::stod(bid[1])   // amount
                });
            }
        }
        
        if (book.contains("asks")) {
            for (const auto& ask : book["asks"]) {
                asks.push_back({
                    std::stod(ask[0]),  // price
                    std::stod(ask[1])   // amount
                });
            }
        }
        
        nlohmann::json parsedBook = {
            {"symbol", book["instId"]},
            {"bids", bids},
            {"asks", asks},
            {"timestamp", std::stoll(book["ts"])},
            {"datetime", exchange_.iso8601(std::stoll(book["ts"]))},
            {"nonce", book.value("seqId", 0)},
            {"info", book}
        };
        
        std::string event = isSnapshot ? "orderBook" : "orderBookUpdate";
        emit(event, parsedBook);
    }
}

void BitgetWS::handleTrade(const nlohmann::json& data) {
    for (const auto& trade : data) {
        nlohmann::json parsedTrade = {
            {"id", trade["tradeId"]},
            {"info", trade},
            {"timestamp", std::stoll(trade["ts"])},
            {"datetime", exchange_.iso8601(std::stoll(trade["ts"]))},
            {"symbol", trade["instId"]},
            {"type", nullptr},
            {"side", trade["side"]},
            {"price", std::stod(trade["px"])},
            {"amount", std::stod(trade["sz"])},
            {"cost", std::stod(trade["px"]) * std::stod(trade["sz"])}
        };
        emit("trade", parsedTrade);
    }
}

void BitgetWS::handleOHLCV(const nlohmann::json& data) {
    for (const auto& candle : data) {
        nlohmann::json parsedCandle = {
            std::stoll(candle[0]),      // timestamp
            std::stod(candle[1]),       // open
            std::stod(candle[2]),       // high
            std::stod(candle[3]),       // low
            std::stod(candle[4]),       // close
            std::stod(candle[5]),       // volume
        };
        emit("ohlcv", parsedCandle);
    }
}

void BitgetWS::handleBidsAsks(const nlohmann::json& data) {
    for (const auto& quote : data) {
        nlohmann::json parsedQuote = {
            {"symbol", quote["instId"]},
            {"timestamp", std::stoll(quote["ts"])},
            {"datetime", exchange_.iso8601(std::stoll(quote["ts"]))},
            {"bid", std::stod(quote["bid"])},
            {"bidVolume", std::stod(quote["bidSz"])},
            {"ask", std::stod(quote["ask"])},
            {"askVolume", std::stod(quote["askSz"])},
            {"info", quote}
        };
        emit("bidsAsks", parsedQuote);
    }
}

void BitgetWS::handleBalance(const nlohmann::json& data) {
    for (const auto& balance : data) {
        nlohmann::json parsedBalance = {
            {"info", balance},
            {"type", balance["marginCoin"]},
            {"currency", balance["marginCoin"]},
            {"total", std::stod(balance["equity"])},
            {"used", std::stod(balance["locked"])},
            {"free", std::stod(balance["available"])}
        };
        emit("balance", parsedBalance);
    }
}

void BitgetWS::handleOrder(const nlohmann::json& data) {
    for (const auto& order : data) {
        std::string status;
        if (order["status"] == "new") {
            status = "open";
        } else if (order["status"] == "filled") {
            status = "closed";
        } else if (order["status"] == "canceled") {
            status = "canceled";
        } else if (order["status"] == "partially_filled") {
            status = "open";
        } else {
            status = "unknown";
        }

        nlohmann::json parsedOrder = {
            {"id", order["orderId"]},
            {"clientOrderId", order.value("clientOid", "")},
            {"info", order},
            {"timestamp", std::stoll(order["cTime"])},
            {"datetime", exchange_.iso8601(std::stoll(order["cTime"]))},
            {"lastTradeTimestamp", std::stoll(order["uTime"])},
            {"status", status},
            {"symbol", order["instId"]},
            {"type", order["ordType"]},
            {"side", order["side"]},
            {"price", std::stod(order["px"])},
            {"amount", std::stod(order["sz"])},
            {"filled", std::stod(order["accFillSz"])},
            {"remaining", std::stod(order["sz"]) - std::stod(order["accFillSz"])},
            {"cost", std::stod(order["fillPx"]) * std::stod(order["accFillSz"])},
            {"average", order.value("avgPx", "0") == "0" ? 0.0 : std::stod(order["avgPx"])},
            {"fee", nullptr}  // Fee information not provided in order update
        };
        emit("order", parsedOrder);
    }
}

void BitgetWS::handleMyTrade(const nlohmann::json& data) {
    for (const auto& trade : data) {
        nlohmann::json parsedTrade = {
            {"id", trade["tradeId"]},
            {"order", trade["orderId"]},
            {"info", trade},
            {"timestamp", std::stoll(trade["ts"])},
            {"datetime", exchange_.iso8601(std::stoll(trade["ts"]))},
            {"symbol", trade["instId"]},
            {"type", trade["ordType"]},
            {"side", trade["side"]},
            {"price", std::stod(trade["fillPx"])},
            {"amount", std::stod(trade["fillSz"])},
            {"cost", std::stod(trade["fillPx"]) * std::stod(trade["fillSz"])},
            {"fee", {
                {"cost", std::stod(trade["fee"])},
                {"currency", trade["feeCcy"]}
            }}
        };
        emit("trade", parsedTrade);
    }
}

void BitgetWS::handlePosition(const nlohmann::json& data) {
    for (const auto& position : data) {
        std::string side = position["posSide"];
        if (side == "net") {
            double pos = std::stod(position["pos"]);
            side = (pos > 0) ? "long" : (pos < 0) ? "short" : "closed";
        }

        nlohmann::json parsedPosition = {
            {"info", position},
            {"symbol", position["instId"]},
            {"timestamp", std::stoll(position["uTime"])},
            {"datetime", exchange_.iso8601(std::stoll(position["uTime"]))},
            {"side", side},
            {"contracts", std::abs(std::stod(position["pos"]))},
            {"contractSize", std::stod(position["contractSize"])},
            {"entryPrice", std::stod(position["avgPx"])},
            {"markPrice", std::stod(position["markPx"])},
            {"notional", std::stod(position["notionalUsd"])},
            {"leverage", std::stod(position["leverage"])},
            {"collateral", std::stod(position["margin"])},
            {"initialMargin", std::stod(position["imr"])},
            {"maintenanceMargin", std::stod(position["mmr"])},
            {"marginRatio", std::stod(position["mgnRatio"])},
            {"percentage", std::stod(position["upl"])},
            {"marginMode", position["marginMode"]},
            {"liquidationPrice", std::stod(position["liqPx"])}
        };
        emit("position", parsedPosition);
    }
}

} // namespace ccxt
