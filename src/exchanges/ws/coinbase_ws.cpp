#include "ccxt/exchanges/ws/coinbase_ws.h"
#include "ccxt/base/json.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

namespace ccxt {

CoinbaseWS::CoinbaseWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Coinbase& exchange)
    : WebSocketClient(ioc, ctx)
    , exchange_(exchange) {
}

void CoinbaseWS::watchTicker(const std::string& symbol) {
    subscribe("ticker", symbol);
}

void CoinbaseWS::watchTickers(const std::vector<std::string>& symbols) {
    subscribeMultiple("ticker", symbols);
}

void CoinbaseWS::watchOrderBook(const std::string& symbol) {
    subscribe("level2", symbol);
}

void CoinbaseWS::watchOrderBookForSymbols(const std::vector<std::string>& symbols) {
    subscribeMultiple("level2", symbols);
}

void CoinbaseWS::watchTrades(const std::string& symbol) {
    subscribe("matches", symbol);
}

void CoinbaseWS::watchTradesForSymbols(const std::vector<std::string>& symbols) {
    subscribeMultiple("matches", symbols);
}

void CoinbaseWS::watchOrders() {
    subscribe("orders", "", true);
}

void CoinbaseWS::subscribe(const std::string& channel, const std::string& symbol, bool isPrivate) {
    std::vector<std::string> productIds;
    if (!symbol.empty()) {
        auto market = exchange_.market(symbol);
        productIds.push_back(market.id);
    }
    
    nlohmann::json request = {
        {"type", "subscribe"},
        {"product_ids", productIds},
        {"channel", channel}
    };

    if (isPrivate) {
        authenticate(channel, productIds);
    }

    connect("wss://advanced-trade-ws.coinbase.com");
    send(request.dump());
}

void CoinbaseWS::subscribeMultiple(const std::string& channel, const std::vector<std::string>& symbols, bool isPrivate) {
    std::vector<std::string> productIds;
    for (const auto& symbol : symbols) {
        auto market = exchange_.market(symbol);
        productIds.push_back(market.id);
    }
    
    nlohmann::json request = {
        {"type", "subscribe"},
        {"product_ids", productIds},
        {"channel", channel}
    };

    if (isPrivate) {
        authenticate(channel, productIds);
    }

    connect("wss://advanced-trade-ws.coinbase.com");
    send(request.dump());
}

void CoinbaseWS::authenticate(const std::string& channel, const std::vector<std::string>& productIds) {
    long long timestamp = std::time(nullptr);
    std::string message = std::to_string(timestamp) + channel;
    for (const auto& id : productIds) {
        message += id;
    }
    
    std::string signature = exchange_.hmac(message, exchange_.secret, "sha256");
    
    nlohmann::json auth = {
        {"api_key", exchange_.apiKey},
        {"timestamp", timestamp},
        {"signature", signature}
    };
    
    send(auth.dump());
}

void CoinbaseWS::handleMessage(const std::string& message) {
    try {
        auto j = nlohmann::json::parse(message);
        
        if (j.contains("type")) {
            std::string type = j["type"];
            
            if (type == "ticker") {
                handleTickerMessage(j);
            } else if (type == "snapshot" || type == "l2update") {
                handleOrderBookMessage(j);
            } else if (type == "match") {
                handleTradeMessage(j);
            } else if (type == "order") {
                handleOrderMessage(j);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling message: " << e.what() << std::endl;
    }
}

void CoinbaseWS::handleTickerMessage(const nlohmann::json& data) {
    nlohmann::json ticker = {
        {"symbol", data["product_id"]},
        {"last", data["price"]},
        {"bid", data["best_bid"]},
        {"ask", data["best_ask"]},
        {"volume", data["volume_24h"]},
        {"timestamp", data["time"]},
        {"info", data}
    };
    
    emit("ticker::" + std::string(data["product_id"]), ticker);
}

void CoinbaseWS::handleOrderBookMessage(const nlohmann::json& data) {
    std::string symbol = data["product_id"];
    std::string type = data["type"];
    
    if (type == "snapshot") {
        nlohmann::json orderbook = {
            {"symbol", symbol},
            {"bids", data["bids"]},
            {"asks", data["asks"]},
            {"timestamp", data["time"]},
            {"info", data}
        };
        emit("orderbook::" + symbol, orderbook);
    } else if (type == "l2update") {
        nlohmann::json update = {
            {"symbol", symbol},
            {"changes", data["changes"]},
            {"timestamp", data["time"]},
            {"info", data}
        };
        emit("orderbook::" + symbol, update);
    }
}

void CoinbaseWS::handleTradeMessage(const nlohmann::json& data) {
    nlohmann::json trade = {
        {"id", data["trade_id"]},
        {"symbol", data["product_id"]},
        {"timestamp", data["time"]},
        {"side", data["side"]},
        {"price", data["price"]},
        {"amount", data["size"]},
        {"info", data}
    };
    
    emit("trades::" + std::string(data["product_id"]), trade);
}

void CoinbaseWS::handleOrderMessage(const nlohmann::json& data) {
    nlohmann::json order = {
        {"id", data["order_id"]},
        {"symbol", data["product_id"]},
        {"type", data["order_type"]},
        {"side", data["side"]},
        {"price", data["price"]},
        {"amount", data["size"]},
        {"status", data["status"]},
        {"timestamp", data["time"]},
        {"info", data}
    };
    
    emit("orders::" + std::string(data["product_id"]), order);
}

} // namespace ccxt
