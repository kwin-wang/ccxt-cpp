#include "../../../include/ccxt/exchanges/ws/independentreserve_ws.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <chrono>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

namespace ccxt {

IndependentReserveWS::IndependentReserveWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, IndependentReserve& exchange)
    : WebSocketClient(ioc, ctx), exchange_(exchange) {
    options_ = {
        {"watchOrderBookLimit", 100},
        {"tradesLimit", 1000}
    };
}

std::string IndependentReserveWS::getEndpoint() {
    return "wss://websockets.independentreserve.com";
}

void IndependentReserveWS::authenticate() {
    auto apiKey = exchange_.getApiKey();
    auto apiSecret = exchange_.getApiSecret();
    
    if (apiKey.empty() || apiSecret.empty()) {
        throw std::runtime_error("API key and secret required for private endpoints");
    }

    uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    std::string payload = apiKey + std::to_string(timestamp);
    std::string signature = sign(payload);

    nlohmann::json request = {
        {"type", "Authenticate"},
        {"data", {
            {"apiKey", apiKey},
            {"nonce", timestamp},
            {"signature", signature}
        }},
        {"requestId", getNextRequestId()}
    };

    send(request.dump());
}

void IndependentReserveWS::watchTicker(const std::string& symbol) {
    std::string marketId = getMarketId(symbol);
    subscribe("TickerData", marketId);
}

void IndependentReserveWS::watchOrderBook(const std::string& symbol, const std::string& limit) {
    std::string marketId = getMarketId(symbol);
    subscribe("OrderBook", marketId);
}

void IndependentReserveWS::watchTrades(const std::string& symbol) {
    std::string marketId = getMarketId(symbol);
    subscribe("TradeData", marketId);
}

void IndependentReserveWS::watchStatus() {
    subscribe("Status");
}

void IndependentReserveWS::watchBalance() {
    if (!authenticated_) {
        authenticate();
    }
    subscribe("AccountBalance");
}

void IndependentReserveWS::watchOrders(const std::string& symbol) {
    if (!authenticated_) {
        authenticate();
    }
    if (!symbol.empty()) {
        std::string marketId = getMarketId(symbol);
        subscribe("OrderData", marketId);
    } else {
        subscribe("OrderData");
    }
}

void IndependentReserveWS::watchMyTrades(const std::string& symbol) {
    if (!authenticated_) {
        authenticate();
    }
    if (!symbol.empty()) {
        std::string marketId = getMarketId(symbol);
        subscribe("TradeData", marketId);
    } else {
        subscribe("TradeData");
    }
}

void IndependentReserveWS::subscribe(const std::string& channel, const std::string& symbol) {
    nlohmann::json request = {
        {"type", "Subscribe"},
        {"data", {
            {"channel", channel}
        }},
        {"requestId", getNextRequestId()}
    };

    if (!symbol.empty()) {
        request["data"]["currencyPair"] = symbol;
    }

    send(request.dump());
    subscriptions_[channel + (symbol.empty() ? "" : ":" + symbol)] = symbol;
}

void IndependentReserveWS::unsubscribe(const std::string& channel, const std::string& symbol) {
    nlohmann::json request = {
        {"type", "Unsubscribe"},
        {"data", {
            {"channel", channel}
        }},
        {"requestId", getNextRequestId()}
    };

    if (!symbol.empty()) {
        request["data"]["currencyPair"] = symbol;
    }

    send(request.dump());
    subscriptions_.erase(channel + (symbol.empty() ? "" : ":" + symbol));
}

std::string IndependentReserveWS::sign(const std::string& payload) {
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

std::string IndependentReserveWS::getMarketId(const std::string& symbol) {
    auto market = exchange_.market(symbol);
    return market.id;
}

std::string IndependentReserveWS::getSymbol(const std::string& marketId) {
    for (const auto& market : exchange_.markets) {
        if (market.second.id == marketId) {
            return market.first;
        }
    }
    return marketId;
}

int IndependentReserveWS::getNextRequestId() {
    return nextRequestId_++;
}

void IndependentReserveWS::handleMessage(const std::string& message) {
    try {
        auto j = nlohmann::json::parse(message);

        // Handle heartbeat
        if (j.contains("type") && j["type"] == "Heartbeat") {
            handleHeartbeat();
            return;
        }

        // Handle authentication response
        if (j.contains("type") && j["type"] == "AuthenticationResult") {
            handleAuthenticationResponse(j);
            return;
        }

        // Handle subscription responses
        if (j.contains("type") && j["type"] == "SubscriptionResult") {
            handleSubscriptionResponse(j);
            return;
        }

        // Handle data updates
        if (j.contains("type")) {
            std::string type = j["type"];
            auto data = j["data"];

            if (type == "TickerData") {
                handleTicker(data);
            } else if (type == "OrderBook") {
                handleOrderBook(data);
            } else if (type == "TradeData") {
                handleTrade(data);
            } else if (type == "Status") {
                handleStatus(data);
            } else if (type == "AccountBalance") {
                handleBalance(data);
            } else if (type == "OrderData") {
                handleOrder(data);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling message: " << e.what() << std::endl;
    }
}

void IndependentReserveWS::handleHeartbeat() {
    nlohmann::json response = {
        {"type", "Heartbeat"}
    };
    send(response.dump());
}

void IndependentReserveWS::handleSubscriptionResponse(const nlohmann::json& data) {
    if (data["success"].get<bool>()) {
        std::cout << "Successfully subscribed to " << data["data"]["channel"] << std::endl;
    } else {
        std::cerr << "Subscription failed: " << data["error"]["message"] << std::endl;
    }
}

void IndependentReserveWS::handleAuthenticationResponse(const nlohmann::json& data) {
    if (data["success"].get<bool>()) {
        authenticated_ = true;
        std::cout << "Successfully authenticated" << std::endl;
    } else {
        std::cerr << "Authentication failed: " << data["error"]["message"] << std::endl;
    }
}

void IndependentReserveWS::handleTicker(const nlohmann::json& data) {
    try {
        Ticker ticker;
        ticker.symbol = getSymbol(data["currencyPair"].get<std::string>());
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

void IndependentReserveWS::handleOrderBook(const nlohmann::json& data) {
    try {
        OrderBook orderBook;
        orderBook.symbol = getSymbol(data["currencyPair"].get<std::string>());
        orderBook.timestamp = data["timestamp"].get<uint64_t>();
        
        const auto& bids = data["bids"];
        const auto& asks = data["asks"];
        
        for (const auto& bid : bids) {
            orderBook.bids.emplace_back(
                std::stod(bid["price"].get<std::string>()),
                std::stod(bid["volume"].get<std::string>())
            );
        }
        
        for (const auto& ask : asks) {
            orderBook.asks.emplace_back(
                std::stod(ask["price"].get<std::string>()),
                std::stod(ask["volume"].get<std::string>())
            );
        }
        
        exchange_.emitOrderBook(orderBook);
    } catch (const std::exception& e) {
        std::cerr << "Error handling order book: " << e.what() << std::endl;
    }
}

void IndependentReserveWS::handleTrade(const nlohmann::json& data) {
    try {
        Trade trade;
        trade.id = data["tradeId"].get<std::string>();
        trade.symbol = getSymbol(data["currencyPair"].get<std::string>());
        trade.price = std::stod(data["price"].get<std::string>());
        trade.amount = std::stod(data["volume"].get<std::string>());
        trade.cost = trade.price * trade.amount;
        trade.side = data["side"].get<std::string>();
        trade.timestamp = data["timestamp"].get<uint64_t>();
        
        if (data.contains("fee")) {
            trade.fee = {
                {"cost", std::stod(data["fee"]["amount"].get<std::string>())},
                {"currency", data["fee"]["currency"].get<std::string>()}
            };
        }
        
        exchange_.emitTrade(trade);
    } catch (const std::exception& e) {
        std::cerr << "Error handling trade: " << e.what() << std::endl;
    }
}

void IndependentReserveWS::handleStatus(const nlohmann::json& data) {
    try {
        std::cout << "Exchange status: " << data["status"].get<std::string>() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error handling status: " << e.what() << std::endl;
    }
}

void IndependentReserveWS::handleBalance(const nlohmann::json& data) {
    try {
        Balance balance;
        balance.currency = data["currency"].get<std::string>();
        balance.free = std::stod(data["available"].get<std::string>());
        balance.used = std::stod(data["reserved"].get<std::string>());
        balance.total = balance.free + balance.used;
        balance.timestamp = data["timestamp"].get<uint64_t>();
        
        exchange_.emitBalance(balance);
    } catch (const std::exception& e) {
        std::cerr << "Error handling balance: " << e.what() << std::endl;
    }
}

void IndependentReserveWS::handleOrder(const nlohmann::json& data) {
    try {
        Order order;
        order.id = data["orderId"].get<std::string>();
        order.symbol = getSymbol(data["currencyPair"].get<std::string>());
        order.type = data["type"].get<std::string>();
        order.side = data["side"].get<std::string>();
        order.price = std::stod(data["price"].get<std::string>());
        order.amount = std::stod(data["volume"].get<std::string>());
        order.filled = std::stod(data["filledVolume"].get<std::string>());
        order.remaining = order.amount - order.filled;
        order.status = data["status"].get<std::string>();
        order.timestamp = data["timestamp"].get<uint64_t>();
        
        if (data.contains("fee")) {
            order.fee = {
                {"cost", std::stod(data["fee"]["amount"].get<std::string>())},
                {"currency", data["fee"]["currency"].get<std::string>()}
            };
        }
        
        exchange_.emitOrder(order);
    } catch (const std::exception& e) {
        std::cerr << "Error handling order: " << e.what() << std::endl;
    }
}

} // namespace ccxt
