#include "ccxt/exchanges/ws/gemini_ws.h"
#include "ccxt/base/json.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

namespace ccxt {

GeminiWS::GeminiWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Gemini& exchange)
    : WebSocketClient(ioc, ctx)
    , exchange_(exchange)
    , newUpdates_(false) {
}

void GeminiWS::watchOrderBook(const std::string& symbol) {
    auto market = exchange_.market(symbol);
    std::string messageHash = "orderbook:" + market.symbol;
    std::string marketId = market.id;

    nlohmann::json request = {
        {"type", "subscribe"},
        {"subscriptions", {{
            {"name", "l2"},
            {"symbols", {marketId}},
        }}}
    };

    std::string subscribeHash = "l2:" + market.symbol;
    std::string url = exchange_.urls["api"]["ws"] + "/v2/marketdata";
    
    connect(url);
    send(request.dump());
}

void GeminiWS::watchOrderBookForSymbols(const std::vector<std::string>& symbols) {
    std::vector<std::string> marketIds;
    for (const auto& symbol : symbols) {
        auto market = exchange_.market(symbol);
        marketIds.push_back(market.id);
    }

    nlohmann::json request = {
        {"type", "subscribe"},
        {"subscriptions", {{
            {"name", "l2"},
            {"symbols", marketIds},
        }}}
    };

    std::string url = exchange_.urls["api"]["ws"] + "/v2/marketdata";
    
    connect(url);
    send(request.dump());
}

void GeminiWS::watchTrades(const std::string& symbol) {
    auto market = exchange_.market(symbol);
    std::string messageHash = "trades:" + market.symbol;
    std::string marketId = market.id;

    nlohmann::json request = {
        {"type", "subscribe"},
        {"subscriptions", {{
            {"name", "l2"},
            {"symbols", {marketId}},
        }}}
    };

    std::string subscribeHash = "l2:" + market.symbol;
    std::string url = exchange_.urls["api"]["ws"] + "/v2/marketdata";
    
    connect(url);
    send(request.dump());
}

void GeminiWS::watchTradesForSymbols(const std::vector<std::string>& symbols) {
    std::vector<std::string> marketIds;
    for (const auto& symbol : symbols) {
        auto market = exchange_.market(symbol);
        marketIds.push_back(market.id);
    }

    nlohmann::json request = {
        {"type", "subscribe"},
        {"subscriptions", {{
            {"name", "l2"},
            {"symbols", marketIds},
        }}}
    };

    std::string url = exchange_.urls["api"]["ws"] + "/v2/marketdata";
    
    connect(url);
    send(request.dump());
}

void GeminiWS::watchBidsAsks(const std::string& symbol) {
    watchOrderBook(symbol);
}

void GeminiWS::watchOHLCV(const std::string& symbol, const std::string& timeframe) {
    throw NotSupported("OHLCV data is not supported via WebSocket for Gemini");
}

void GeminiWS::watchOrders() {
    authenticate();
    std::string url = exchange_.urls["api"]["ws"] + "/v1/order/events";
    connect(url);
}

void GeminiWS::authenticate() {
    auto timestamp = std::to_string(exchange_.milliseconds());
    std::string payload = "REQUEST" + timestamp + "v1/order/events";
    
    std::string signature = exchange_.hmac(payload, exchange_.secret, "sha384", "hex");
    
    nlohmann::json request = {
        {"request", "/v1/order/events"},
        {"nonce", timestamp},
        {"apiKey", exchange_.apiKey},
        {"signature", signature},
    };
    
    send(request.dump());
}

void GeminiWS::handleMessage(const std::string& message) {
    auto j = nlohmann::json::parse(message);
    
    std::string type = j["type"].get<std::string>();
    
    if (type == "update") {
        if (j.contains("trades")) {
            handleTradeMessage(j);
        } else if (j.contains("changes")) {
            handleOrderBookMessage(j);
        }
    } else if (type == "subscription_ack") {
        // Handle subscription acknowledgment
    } else if (type == "heartbeat") {
        // Handle heartbeat
    } else if (type == "order") {
        handleOrderMessage(j);
    }
}

void GeminiWS::handleTradeMessage(const nlohmann::json& message) {
    std::string symbol = message["symbol"];
    auto trades = message["trades"];
    
    for (const auto& trade : trades) {
        nlohmann::json parsedTrade = {
            {"id", trade["tid"]},
            {"order", trade["order_id"]},
            {"timestamp", trade["timestampms"]},
            {"datetime", exchange_.iso8601(trade["timestampms"].get<long long>())},
            {"symbol", symbol},
            {"type", trade.value("type", "")},
            {"side", trade["side"]},
            {"price", exchange_.parseNumber(trade["price"])},
            {"amount", exchange_.parseNumber(trade["amount"])},
            {"cost", exchange_.parseNumber(trade["price"].get<std::string>()) * 
                     exchange_.parseNumber(trade["amount"].get<std::string>())},
            {"fee", {
                {"cost", exchange_.parseNumber(trade.value("fee_amount", "0"))},
                {"currency", trade.value("fee_currency", "")}
            }}
        };
        
        exchange_.handleTrade(parsedTrade);
    }
}

void GeminiWS::handleOrderBookMessage(const nlohmann::json& message) {
    std::string symbol = message["symbol"];
    auto changes = message["changes"];
    bool isSnapshot = message.value("snapshot", false);
    
    nlohmann::json orderBook;
    if (isSnapshot) {
        orderBook = {
            {"symbol", symbol},
            {"bids", nlohmann::json::array()},
            {"asks", nlohmann::json::array()},
            {"timestamp", exchange_.milliseconds()},
            {"datetime", exchange_.iso8601(exchange_.milliseconds())},
            {"nonce", message.value("eventId", 0)}
        };
    }
    
    for (const auto& change : changes) {
        std::string side = change[0];
        std::string price = change[1];
        std::string amount = change[2];
        
        nlohmann::json order = {
            {price, amount}
        };
        
        if (side == "buy") {
            if (isSnapshot) {
                orderBook["bids"].push_back({price, amount});
            } else {
                exchange_.handleBidAsk(order, symbol, "bid");
            }
        } else {
            if (isSnapshot) {
                orderBook["asks"].push_back({price, amount});
            } else {
                exchange_.handleBidAsk(order, symbol, "ask");
            }
        }
    }
    
    if (isSnapshot) {
        exchange_.handleOrderBook(orderBook);
    }
}

void GeminiWS::handleOrderMessage(const nlohmann::json& message) {
    if (!message.contains("order_id")) {
        return;
    }
    
    std::string symbol = message["symbol"];
    std::string orderId = message["order_id"].get<std::string>();
    std::string orderType = message["type"];
    std::string side = message["side"];
    std::string price = message.value("price", "0");
    std::string amount = message.value("original_amount", "0");
    std::string remaining = message.value("remaining_amount", "0");
    std::string status = "open";
    
    if (message.value("is_cancelled", false)) {
        status = "canceled";
    } else if (message.value("is_live", false)) {
        status = "open";
    } else if (exchange_.parseNumber(remaining) == 0) {
        status = "closed";
    }
    
    nlohmann::json order = {
        {"id", orderId},
        {"clientOrderId", message.value("client_order_id", "")},
        {"timestamp", message["timestampms"]},
        {"datetime", exchange_.iso8601(message["timestampms"].get<long long>())},
        {"lastTradeTimestamp", nullptr},
        {"status", status},
        {"symbol", symbol},
        {"type", orderType},
        {"side", side},
        {"price", exchange_.parseNumber(price)},
        {"amount", exchange_.parseNumber(amount)},
        {"filled", exchange_.parseNumber(amount) - exchange_.parseNumber(remaining)},
        {"remaining", exchange_.parseNumber(remaining)},
        {"cost", exchange_.parseNumber(price) * (exchange_.parseNumber(amount) - exchange_.parseNumber(remaining))},
        {"trades", nlohmann::json::array()},
        {"fee", nullptr},
        {"info", message}
    };
    
    exchange_.handleOrder(order);
}

} // namespace ccxt
