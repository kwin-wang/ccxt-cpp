#include "../../../include/ccxt/exchanges/ws/hyperliquid_ws.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <chrono>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

namespace ccxt {

HyperliquidWS::HyperliquidWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Hyperliquid& exchange)
    : WebSocketClient(ioc, ctx), exchange_(exchange) {
    options_ = {
        {"watchOrderBookLimit", 100},
        {"tradesLimit", 1000},
        {"OHLCVLimit", 1000}
    };
}

std::string HyperliquidWS::getEndpoint() {
    return "wss://api.hyperliquid.xyz/ws";
}

void HyperliquidWS::authenticate() {
    auto apiKey = exchange_.getApiKey();
    auto apiSecret = exchange_.getApiSecret();
    
    if (apiKey.empty() || apiSecret.empty()) {
        throw std::runtime_error("API key and secret required for private endpoints");
    }

    uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    std::string payload = std::to_string(timestamp) + apiKey;
    std::string signature = sign(payload);

    nlohmann::json request = {
        {"op", "auth"},
        {"data", {
            {"apiKey", apiKey},
            {"timestamp", timestamp},
            {"signature", signature}
        }}
    };

    send(request.dump());
}

void HyperliquidWS::watchTicker(const std::string& symbol) {
    std::string marketId = getMarketId(symbol);
    subscribe("ticker", marketId);
}

void HyperliquidWS::watchOrderBook(const std::string& symbol, const std::string& limit) {
    std::string marketId = getMarketId(symbol);
    subscribe("orderbook", marketId);
}

void HyperliquidWS::watchTrades(const std::string& symbol) {
    std::string marketId = getMarketId(symbol);
    subscribe("trades", marketId);
}

void HyperliquidWS::watchOHLCV(const std::string& symbol, const std::string& timeframe) {
    std::string marketId = getMarketId(symbol);
    subscribe("kline_" + timeframe, marketId);
}

void HyperliquidWS::watchMarkPrice(const std::string& symbol) {
    std::string marketId = getMarketId(symbol);
    subscribe("markPrice", marketId);
}

void HyperliquidWS::watchFundingRate(const std::string& symbol) {
    std::string marketId = getMarketId(symbol);
    subscribe("fundingRate", marketId);
}

void HyperliquidWS::watchBalance() {
    if (!authenticated_) {
        authenticate();
    }
    subscribe("balance");
}

void HyperliquidWS::watchOrders(const std::string& symbol) {
    if (!authenticated_) {
        authenticate();
    }
    subscribe("orders", symbol);
}

void HyperliquidWS::watchMyTrades(const std::string& symbol) {
    if (!authenticated_) {
        authenticate();
    }
    subscribe("trades", symbol);
}

void HyperliquidWS::watchPositions(const std::string& symbol) {
    if (!authenticated_) {
        authenticate();
    }
    subscribe("positions", symbol);
}

void HyperliquidWS::watchLeverage(const std::string& symbol) {
    if (!authenticated_) {
        authenticate();
    }
    subscribe("leverage", symbol);
}

void HyperliquidWS::subscribe(const std::string& channel, const std::string& symbol) {
    nlohmann::json request = {
        {"op", "subscribe"},
        {"channel", channel}
    };

    if (!symbol.empty()) {
        request["symbol"] = symbol;
    }

    send(request.dump());
    subscriptions_[channel + (symbol.empty() ? "" : ":" + symbol)] = symbol;
}

void HyperliquidWS::unsubscribe(const std::string& channel, const std::string& symbol) {
    nlohmann::json request = {
        {"op", "unsubscribe"},
        {"channel", channel}
    };

    if (!symbol.empty()) {
        request["symbol"] = symbol;
    }

    send(request.dump());
    subscriptions_.erase(channel + (symbol.empty() ? "" : ":" + symbol));
}

std::string HyperliquidWS::sign(const std::string& payload) {
    auto apiSecret = exchange_.getApiSecret();
    unsigned char hmac[32];
    HMAC(EVP_sha256(), apiSecret.c_str(), apiSecret.length(),
         reinterpret_cast<const unsigned char*>(payload.c_str()), payload.length(),
         hmac, nullptr);
    
    std::stringstream ss;
    for (int i = 0; i < 32; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hmac[i]);
    }
    return ss.str();
}

std::string HyperliquidWS::getMarketId(const std::string& symbol) {
    auto market = exchange_.market(symbol);
    return market.id;
}

std::string HyperliquidWS::getSymbol(const std::string& marketId) {
    for (const auto& market : exchange_.markets) {
        if (market.second.id == marketId) {
            return market.first;
        }
    }
    return marketId;
}

void HyperliquidWS::handleMessage(const std::string& message) {
    try {
        auto j = nlohmann::json::parse(message);

        // Handle ping-pong
        if (j.contains("ping")) {
            nlohmann::json pong = {{"pong", j["ping"]}};
            send(pong.dump());
            return;
        }

        // Handle authentication response
        if (j.contains("op") && j["op"] == "auth") {
            if (j["success"].get<bool>()) {
                authenticated_ = true;
                std::cout << "Successfully authenticated" << std::endl;
            } else {
                std::cerr << "Authentication failed: " << j["message"] << std::endl;
            }
            return;
        }

        // Handle subscription responses
        if (j.contains("op") && j["op"] == "subscribe") {
            if (j["success"].get<bool>()) {
                std::cout << "Successfully subscribed to " << j["channel"] << std::endl;
            } else {
                std::cerr << "Subscription failed: " << j["message"] << std::endl;
            }
            return;
        }

        // Handle data updates
        if (j.contains("channel")) {
            std::string channel = j["channel"];
            auto data = j["data"];

            if (channel == "ticker") {
                handleTicker(data);
            } else if (channel == "orderbook") {
                handleOrderBook(data);
            } else if (channel == "trades") {
                handleTrade(data);
            } else if (channel.find("kline_") == 0) {
                handleOHLCV(data);
            } else if (channel == "markPrice") {
                handleMarkPrice(data);
            } else if (channel == "fundingRate") {
                handleFundingRate(data);
            } else if (channel == "balance") {
                handleBalance(data);
            } else if (channel == "orders") {
                handleOrder(data);
            } else if (channel == "positions") {
                handlePosition(data);
            } else if (channel == "leverage") {
                handleLeverage(data);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling message: " << e.what() << std::endl;
    }
}

void HyperliquidWS::handleTicker(const nlohmann::json& data) {
    try {
        Ticker ticker;
        ticker.symbol = getSymbol(data["symbol"].get<std::string>());
        ticker.high = std::stod(data["high24h"].get<std::string>());
        ticker.low = std::stod(data["low24h"].get<std::string>());
        ticker.bid = std::stod(data["bestBid"].get<std::string>());
        ticker.ask = std::stod(data["bestAsk"].get<std::string>());
        ticker.last = std::stod(data["lastPrice"].get<std::string>());
        ticker.volume = std::stod(data["volume24h"].get<std::string>());
        ticker.timestamp = data["timestamp"].get<uint64_t>();
        
        exchange_.emitTicker(ticker);
    } catch (const std::exception& e) {
        std::cerr << "Error handling ticker: " << e.what() << std::endl;
    }
}

void HyperliquidWS::handleOrderBook(const nlohmann::json& data) {
    try {
        OrderBook orderBook;
        orderBook.symbol = getSymbol(data["symbol"].get<std::string>());
        orderBook.timestamp = data["timestamp"].get<uint64_t>();
        
        const auto& bids = data["bids"];
        const auto& asks = data["asks"];
        
        for (const auto& bid : bids) {
            orderBook.bids.emplace_back(
                std::stod(bid[0].get<std::string>()),
                std::stod(bid[1].get<std::string>())
            );
        }
        
        for (const auto& ask : asks) {
            orderBook.asks.emplace_back(
                std::stod(ask[0].get<std::string>()),
                std::stod(ask[1].get<std::string>())
            );
        }
        
        exchange_.emitOrderBook(orderBook);
    } catch (const std::exception& e) {
        std::cerr << "Error handling order book: " << e.what() << std::endl;
    }
}

void HyperliquidWS::handleTrade(const nlohmann::json& data) {
    try {
        Trade trade;
        trade.id = data["id"].get<std::string>();
        trade.symbol = getSymbol(data["symbol"].get<std::string>());
        trade.price = std::stod(data["price"].get<std::string>());
        trade.amount = std::stod(data["size"].get<std::string>());
        trade.side = data["side"].get<std::string>();
        trade.timestamp = data["timestamp"].get<uint64_t>();
        
        exchange_.emitTrade(trade);
    } catch (const std::exception& e) {
        std::cerr << "Error handling trade: " << e.what() << std::endl;
    }
}

void HyperliquidWS::handleOHLCV(const nlohmann::json& data) {
    try {
        OHLCV ohlcv;
        ohlcv.timestamp = data["timestamp"].get<uint64_t>();
        ohlcv.open = std::stod(data["open"].get<std::string>());
        ohlcv.high = std::stod(data["high"].get<std::string>());
        ohlcv.low = std::stod(data["low"].get<std::string>());
        ohlcv.close = std::stod(data["close"].get<std::string>());
        ohlcv.volume = std::stod(data["volume"].get<std::string>());
        
        exchange_.emitOHLCV(ohlcv);
    } catch (const std::exception& e) {
        std::cerr << "Error handling OHLCV: " << e.what() << std::endl;
    }
}

void HyperliquidWS::handleMarkPrice(const nlohmann::json& data) {
    try {
        // Implementation depends on exchange-specific data structure
        // Emit mark price update event
    } catch (const std::exception& e) {
        std::cerr << "Error handling mark price: " << e.what() << std::endl;
    }
}

void HyperliquidWS::handleFundingRate(const nlohmann::json& data) {
    try {
        // Implementation depends on exchange-specific data structure
        // Emit funding rate update event
    } catch (const std::exception& e) {
        std::cerr << "Error handling funding rate: " << e.what() << std::endl;
    }
}

void HyperliquidWS::handleBalance(const nlohmann::json& data) {
    try {
        Balance balance;
        balance.currency = data["currency"].get<std::string>();
        balance.free = std::stod(data["available"].get<std::string>());
        balance.used = std::stod(data["used"].get<std::string>());
        balance.total = std::stod(data["total"].get<std::string>());
        balance.timestamp = data["timestamp"].get<uint64_t>();
        
        exchange_.emitBalance(balance);
    } catch (const std::exception& e) {
        std::cerr << "Error handling balance: " << e.what() << std::endl;
    }
}

void HyperliquidWS::handleOrder(const nlohmann::json& data) {
    try {
        Order order;
        order.id = data["orderId"].get<std::string>();
        order.symbol = getSymbol(data["symbol"].get<std::string>());
        order.type = data["type"].get<std::string>();
        order.side = data["side"].get<std::string>();
        order.price = std::stod(data["price"].get<std::string>());
        order.amount = std::stod(data["size"].get<std::string>());
        order.filled = std::stod(data["filled"].get<std::string>());
        order.remaining = order.amount - order.filled;
        order.status = data["status"].get<std::string>();
        order.timestamp = data["timestamp"].get<uint64_t>();
        
        exchange_.emitOrder(order);
    } catch (const std::exception& e) {
        std::cerr << "Error handling order: " << e.what() << std::endl;
    }
}

void HyperliquidWS::handlePosition(const nlohmann::json& data) {
    try {
        // Implementation depends on exchange-specific data structure
        // Emit position update event
    } catch (const std::exception& e) {
        std::cerr << "Error handling position: " << e.what() << std::endl;
    }
}

void HyperliquidWS::handleLeverage(const nlohmann::json& data) {
    try {
        // Implementation depends on exchange-specific data structure
        // Emit leverage update event
    } catch (const std::exception& e) {
        std::cerr << "Error handling leverage: " << e.what() << std::endl;
    }
}

} // namespace ccxt
