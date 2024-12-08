#include "../../../include/ccxt/exchanges/ws/bybit_ws.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <chrono>
#include <boost/crc.hpp>

namespace ccxt {

BybitWS::BybitWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Bybit& exchange)
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

std::string BybitWS::getEndpoint() {
    std::string hostname = exchange_.getHostname();
    return "wss://stream." + hostname + "/v5/public/spot";
}

void BybitWS::authenticate() {
    auto apiKey = exchange_.getApiKey();
    auto apiSecret = exchange_.getApiSecret();
    
    if (apiKey.empty() || apiSecret.empty()) {
        throw std::runtime_error("API key and secret required for private endpoints");
    }
    
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    std::string expires = std::to_string(timestamp + 10000);
    std::string signature = sign("GET/realtime" + expires);
    
    nlohmann::json request = {
        {"op", "auth"},
        {"args", {apiKey, expires, signature}}
    };
    
    send(request.dump());
}

void BybitWS::watchTicker(const std::string& symbol) {
    auto market = exchange_.market(symbol);
    std::string topic = "tickers." + market.id;
    
    nlohmann::json request = {
        {"op", "subscribe"},
        {"args", {topic}}
    };
    
    send(request.dump());
}

void BybitWS::watchOrderBook(const std::string& symbol, const std::string& limit) {
    auto market = exchange_.market(symbol);
    std::string topic = "orderbook." + limit + "." + market.id;
    
    nlohmann::json request = {
        {"op", "subscribe"},
        {"args", {topic}}
    };
    
    send(request.dump());
}

void BybitWS::watchTrades(const std::string& symbol) {
    auto market = exchange_.market(symbol);
    std::string topic = "trades." + market.id;
    
    nlohmann::json request = {
        {"op", "subscribe"},
        {"args", {topic}}
    };
    
    send(request.dump());
}

void BybitWS::watchOHLCV(const std::string& symbol, const std::string& timeframe) {
    auto market = exchange_.market(symbol);
    std::string interval = exchange_.timeframes[timeframe];
    std::string topic = "kline." + interval + "." + market.id;
    
    nlohmann::json request = {
        {"op", "subscribe"},
        {"args", {topic}}
    };
    
    send(request.dump());
}

void BybitWS::watchBalance() {
    if (!authenticated_) {
        authenticate();
    }
    
    nlohmann::json request = {
        {"op", "subscribe"},
        {"args", {"wallet"}}
    };
    
    send(request.dump());
}

void BybitWS::watchOrders() {
    if (!authenticated_) {
        authenticate();
    }
    
    nlohmann::json request = {
        {"op", "subscribe"},
        {"args", {"order"}}
    };
    
    send(request.dump());
}

void BybitWS::watchMyTrades() {
    if (!authenticated_) {
        authenticate();
    }
    
    nlohmann::json request = {
        {"op", "subscribe"},
        {"args", {"execution"}}
    };
    
    send(request.dump());
}

void BybitWS::watchPositions() {
    if (!authenticated_) {
        authenticate();
    }
    
    nlohmann::json request = {
        {"op", "subscribe"},
        {"args", {"position"}}
    };
    
    send(request.dump());
}

std::string BybitWS::sign(const std::string& payload) {
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

void BybitWS::handleMessage(const std::string& message) {
    try {
        auto j = nlohmann::json::parse(message);
        
        // Handle subscription responses
        if (j.contains("success")) {
            if (j["success"].get<bool>()) {
                std::cout << "Successfully subscribed to topic" << std::endl;
            } else {
                std::cerr << "Subscription failed: " << j["ret_msg"] << std::endl;
            }
            return;
        }
        
        // Handle stream data
        if (j.contains("topic")) {
            std::string topic = j["topic"];
            auto data = j["data"];
            
            if (topic.find("tickers") == 0) {
                handleTicker(data);
            } else if (topic.find("orderbook") == 0) {
                handleOrderBook(data);
            } else if (topic.find("trades") == 0) {
                handleTrade(data);
            } else if (topic.find("kline") == 0) {
                handleOHLCV(data);
            } else if (topic == "wallet") {
                handleBalance(data);
            } else if (topic == "order") {
                handleOrder(data);
            } else if (topic == "execution") {
                handleMyTrade(data);
            } else if (topic == "position") {
                handlePosition(data);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling message: " << e.what() << std::endl;
    }
}

void BybitWS::handleTicker(const nlohmann::json& data) {
    Ticker ticker;
    ticker.symbol = data["s"].get<std::string>();
    ticker.high = std::stod(data["h"].get<std::string>());
    ticker.low = std::stod(data["l"].get<std::string>());
    ticker.bid = std::stod(data["b"].get<std::string>());
    ticker.ask = std::stod(data["a"].get<std::string>());
    ticker.last = std::stod(data["lp"].get<std::string>());
    ticker.volume = std::stod(data["v"].get<std::string>());
    ticker.timestamp = data["t"].get<uint64_t>();
    
    exchange_.emitTicker(ticker);
}

void BybitWS::handleOrderBook(const nlohmann::json& data) {
    OrderBook orderBook;
    orderBook.symbol = data["s"].get<std::string>();
    orderBook.timestamp = data["t"].get<uint64_t>();
    
    const auto& bids = data["b"];
    const auto& asks = data["a"];
    
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
}

void BybitWS::handleTrade(const nlohmann::json& data) {
    Trade trade;
    trade.symbol = data["s"].get<std::string>();
    trade.id = data["i"].get<std::string>();
    trade.price = std::stod(data["p"].get<std::string>());
    trade.amount = std::stod(data["v"].get<std::string>());
    trade.side = data["S"].get<std::string>();
    trade.timestamp = data["t"].get<uint64_t>();
    
    exchange_.emitTrade(trade);
}

void BybitWS::handleOHLCV(const nlohmann::json& data) {
    OHLCV ohlcv;
    ohlcv.timestamp = data["t"].get<uint64_t>();
    ohlcv.open = std::stod(data["o"].get<std::string>());
    ohlcv.high = std::stod(data["h"].get<std::string>());
    ohlcv.low = std::stod(data["l"].get<std::string>());
    ohlcv.close = std::stod(data["c"].get<std::string>());
    ohlcv.volume = std::stod(data["v"].get<std::string>());
    
    exchange_.emitOHLCV(ohlcv);
}

void BybitWS::handleBalance(const nlohmann::json& data) {
    try {
        Balance balance;
        balance.currency = data["coin"].get<std::string>();
        balance.free = std::stod(data["free"].get<std::string>());
        balance.used = std::stod(data["locked"].get<std::string>());
        balance.total = balance.free + balance.used;
        balance.timestamp = data["t"].get<uint64_t>();
        
        exchange_.emitBalance(balance);
    } catch (const std::exception& e) {
        std::cerr << "Error handling balance: " << e.what() << std::endl;
    }
}

void BybitWS::handleOrder(const nlohmann::json& data) {
    try {
        Order order;
        order.id = data["i"].get<std::string>();
        order.symbol = data["s"].get<std::string>();
        order.type = data["o"].get<std::string>();
        order.side = data["S"].get<std::string>();
        order.price = std::stod(data["p"].get<std::string>());
        order.amount = std::stod(data["q"].get<std::string>());
        order.filled = std::stod(data["z"].get<std::string>());
        order.remaining = order.amount - order.filled;
        order.status = data["X"].get<std::string>();
        order.timestamp = data["t"].get<uint64_t>();
        
        exchange_.emitOrder(order);
    } catch (const std::exception& e) {
        std::cerr << "Error handling order: " << e.what() << std::endl;
    }
}

void BybitWS::handleMyTrade(const nlohmann::json& data) {
    try {
        Trade trade;
        trade.id = data["i"].get<std::string>();
        trade.orderId = data["c"].get<std::string>();
        trade.symbol = data["s"].get<std::string>();
        trade.side = data["S"].get<std::string>();
        trade.price = std::stod(data["p"].get<std::string>());
        trade.amount = std::stod(data["q"].get<std::string>());
        trade.cost = trade.price * trade.amount;
        trade.fee = std::stod(data["n"].get<std::string>());
        trade.feeCurrency = data["N"].get<std::string>();
        trade.timestamp = data["t"].get<uint64_t>();
        
        exchange_.emitMyTrade(trade);
    } catch (const std::exception& e) {
        std::cerr << "Error handling my trade: " << e.what() << std::endl;
    }
}

void BybitWS::handlePosition(const nlohmann::json& data) {
    try {
        Position position;
        position.symbol = data["s"].get<std::string>();
        position.side = data["S"].get<std::string>();
        position.amount = std::stod(data["sz"].get<std::string>());
        position.entryPrice = std::stod(data["ep"].get<std::string>());
        position.unrealizedPnl = std::stod(data["up"].get<std::string>());
        position.leverage = std::stod(data["l"].get<std::string>());
        position.marginType = data["mt"].get<std::string>();
        position.timestamp = data["t"].get<uint64_t>();
        
        exchange_.emitPosition(position);
    } catch (const std::exception& e) {
        std::cerr << "Error handling position: " << e.what() << std::endl;
    }
}

} // namespace ccxt
