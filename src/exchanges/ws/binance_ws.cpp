#include <ccxt/exchanges/ws/binance_ws.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <chrono>
#include <boost/crc.hpp>
#include <boost/algorithm/string.hpp>

namespace ccxt {

BinanceWS::BinanceWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Binance& exchange)
    : WebSocketClient(ioc, ctx), exchange_(exchange) {
    checksumEnabled_ = true;
    streamLimits_ = {
        {"spot", 50},      // max 1024
        {"margin", 50},    // max 1024
        {"future", 50},    // max 200
        {"delivery", 50}   // max 200
    };
    subscriptionLimits_ = {
        {"spot", 200},
        {"margin", 200},
        {"future", 200},
        {"delivery", 200}
    };
    options_ = {
        {"watchOrderBookRate", 100},
        {"liquidationsLimit", 1000},
        {"tradesLimit", 1000},
        {"ordersLimit", 1000},
        {"OHLCVLimit", 1000},
        {"watchOrderBookLimit", 1000},
        {"listenKeyRefreshRate", 1200000},  // 20 mins
        {"watchOrderBook", {
            {"maxRetries", 3},
            {"checksum", true}
        }}
    };
}

std::string BinanceWS::getEndpoint() {
    return "wss://stream.binance.com:9443/ws";
}

void BinanceWS::authenticate() {
    // Get listen key from REST API
    auto listenKey = "";//exchange_.getListenKey();
    
    nlohmann::json request = {
        {"method", "SUBSCRIBE"},
        {"params", {listenKey}},
        {"id", nextRequestId_++}
    };
    
    send(request.dump());
}

void BinanceWS::watchTicker(const std::string& symbol) {
    auto market = exchange_.markets[symbol];
    std::string stream = boost::algorithm::to_lower_copy(market.id) + "@ticker";
    
    nlohmann::json request = {
        {"method", "SUBSCRIBE"},
        {"params", {stream}},
        {"id", nextRequestId_++}
    };
    
    send(request.dump());
}

void BinanceWS::watchOrderBook(const std::string& symbol, const std::string& limit) {
    auto market = exchange_.markets[symbol];
    std::string stream = boost::algorithm::to_lower_copy(market.id) + "@depth" + (limit.empty() ? "" : limit);
    
    nlohmann::json request = {
        {"method", "SUBSCRIBE"},
        {"params", {stream}},
        {"id", nextRequestId_++}
    };
    
    send(request.dump());
}

void BinanceWS::watchTrades(const std::string& symbol) {
    auto market = exchange_.markets[symbol];
    std::string stream = boost::algorithm::to_lower_copy(market.id) + "@trade";
    
    nlohmann::json request = {
        {"method", "SUBSCRIBE"},
        {"params", {stream}},
        {"id", nextRequestId_++}
    };
    
    send(request.dump());
}

void BinanceWS::watchOHLCV(const std::string& symbol, const std::string& timeframe) {
    auto market = exchange_.markets[symbol];
    std::string interval = "1m";//exchange_.timeframes[timeframe];
    std::string stream = boost::algorithm::to_lower_copy(market.id) + "@kline_" + interval;
    
    nlohmann::json request = {
        {"method", "SUBSCRIBE"},
        {"params", {stream}},
        {"id", nextRequestId_++}
    };
    
    send(request.dump());
}

void BinanceWS::watchBalance() {
    if (!authenticated_) {
        authenticate();
    }
    
    // Balance updates come through the user data stream automatically
    // after authentication
}

void BinanceWS::watchOrders() {
    if (!authenticated_) {
        authenticate();
    }
    
    // Order updates come through the user data stream automatically
    // after authentication
}

void BinanceWS::watchMyTrades() {
    if (!authenticated_) {
        authenticate();
    }
    
    // Trade updates come through the user data stream automatically
    // after authentication
}

void BinanceWS::watchMarkPrice(const std::string& symbol) {
    auto market = exchange_.markets[symbol];
    std::string stream = boost::algorithm::to_lower_copy(market.id) + "@markPrice";
    
    nlohmann::json request = {
        {"method", "SUBSCRIBE"},
        {"params", {stream}},
        {"id", nextRequestId_++}
    };
    
    send(request.dump());
}

void BinanceWS::watchPositions() {
    if (!authenticated_) {
        authenticate();
    }
    
    // Position updates come through the user data stream automatically
    // after authentication
}

void BinanceWS::handleMessage(const std::string& message) {
    try {
        auto j = nlohmann::json::parse(message);
        
        // Handle subscription responses
        if (j.contains("result") && j.contains("id")) {
            if (j["result"].is_null()) {
                std::cout << "Successfully subscribed to stream" << std::endl;
            } else {
                std::cerr << "Subscription failed: " << j["result"] << std::endl;
            }
            return;
        }
        
        // Handle stream data
        if (j.contains("stream")) {
            std::string stream = j["stream"];
            auto data = j["data"];
            
            if (stream.find("@ticker") != std::string::npos) {
                handleTicker(data);
            } else if (stream.find("@depth") != std::string::npos) {
                handleOrderBook(data);
            } else if (stream.find("@trade") != std::string::npos) {
                handleTrade(data);
            } else if (stream.find("@kline") != std::string::npos) {
                handleOHLCV(data);
            } else if (stream.find("@markPrice") != std::string::npos) {
                handleMarkPrice(data);
            }
        }
        // Handle user data stream
        else if (j.contains("e")) {
            std::string eventType = j["e"];
            
            if (eventType == "outboundAccountPosition") {
                handleBalance(j);
            } else if (eventType == "executionReport") {
                handleOrder(j);
                if (j["x"] == "TRADE") {
                    handleMyTrade(j);
                }
            } else if (eventType == "ACCOUNT_UPDATE") {
                handlePosition(j);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling message: " << e.what() << std::endl;
    }
}

void BinanceWS::handleTicker(const nlohmann::json& data) {
    Ticker ticker;
    ticker.symbol = data["s"].get<std::string>();
    ticker.high = std::stod(data["h"].get<std::string>());
    ticker.low = std::stod(data["l"].get<std::string>());
    ticker.bid = std::stod(data["b"].get<std::string>());
    ticker.ask = std::stod(data["a"].get<std::string>());
    ticker.last = std::stod(data["c"].get<std::string>());
    ticker.volume = std::stod(data["v"].get<std::string>());
    ticker.timestamp = data["E"].get<uint64_t>();

    //exchange_.emitTicker(ticker);
}

void BinanceWS::handleOrderBook(const nlohmann::json& data) {
    OrderBook orderBook;
    orderBook.symbol = data["s"].get<std::string>();
    orderBook.timestamp = data["E"].get<uint64_t>();

    // Process bids
    for (const auto& bid : data["b"]) {
        double price = std::stod(bid[0].get<std::string>());
        double amount = std::stod(bid[1].get<std::string>());
        orderBook.bids.emplace_back(price, amount);
    }

    // Process asks
    for (const auto& ask : data["a"]) {
        double price = std::stod(ask[0].get<std::string>());
        double amount = std::stod(ask[1].get<std::string>());
        orderBook.asks.emplace_back(price, amount);
    }

    //exchange_.emitOrderBook(orderBook);
}

void BinanceWS::handleTrade(const nlohmann::json& data) {
    Trade trade;
    trade.symbol = data["s"].get<std::string>();
    trade.id = data["t"].get<std::string>();
    trade.price = std::stod(data["p"].get<std::string>());
    trade.amount = std::stod(data["q"].get<std::string>());
    trade.timestamp = data["E"].get<uint64_t>();
    trade.side = data["m"].get<bool>() ? "sell" : "buy";

    //exchange_.emitTrade(trade);
}

void BinanceWS::handleOHLCV(const nlohmann::json& data) {
    OHLCV ohlcv;
    const auto& k = data["k"];
    
    ohlcv.timestamp = k["t"].get<uint64_t>();
    ohlcv.open = std::stod(k["o"].get<std::string>());
    ohlcv.high = std::stod(k["h"].get<std::string>());
    ohlcv.low = std::stod(k["l"].get<std::string>());
    ohlcv.close = std::stod(k["c"].get<std::string>());
    ohlcv.volume = std::stod(k["v"].get<std::string>());

    //exchange_.emitOHLCV(ohlcv);
}

void BinanceWS::handleMarkPrice(const nlohmann::json& data) {
    try {
        MarkPrice markPrice;
        markPrice.symbol = data["s"].get<std::string>();
        markPrice.markPrice = std::stod(data["p"].get<std::string>());
        markPrice.timestamp = data["E"].get<uint64_t>();
        markPrice.fundingRate = data.contains("r") ? std::stod(data["r"].get<std::string>()) : 0.0;
        markPrice.nextFundingTime = data.contains("T") ? data["T"].get<uint64_t>() : 0;

        //exchange_.emitMarkPrice(markPrice);
    } catch (const std::exception& e) {
        std::cerr << "Error handling mark price: " << e.what() << std::endl;
    }
}

void BinanceWS::handleBalance(const nlohmann::json& data) {
    try {
        Balance balance;
        balance.currency = data["a"].get<std::string>();
        balance.free = std::stod(data["f"].get<std::string>());
        balance.used = std::stod(data["l"].get<std::string>());
        balance.total = balance.free + balance.used;
        balance.timestamp = data["E"].get<uint64_t>();

        //exchange_.emitBalance(balance);
    } catch (const std::exception& e) {
        std::cerr << "Error handling balance: " << e.what() << std::endl;
    }
}

void BinanceWS::handleOrder(const nlohmann::json& data) {
    try {
        Order order;
        order.id = data["i"].get<std::string>();
        order.clientOrderId = data["c"].get<std::string>();
        order.symbol = data["s"].get<std::string>();
        order.side = data["S"].get<std::string>();
        order.type = data["o"].get<std::string>();
        order.price = std::stod(data["p"].get<std::string>());
        order.amount = std::stod(data["q"].get<std::string>());
        order.filled = std::stod(data["z"].get<std::string>());
        order.remaining = order.amount - order.filled;
        order.status = data["X"].get<std::string>();
        order.timestamp = data["E"].get<uint64_t>();

        //exchange_.emitOrder(order);
    } catch (const std::exception& e) {
        std::cerr << "Error handling order: " << e.what() << std::endl;
    }
}

void BinanceWS::handleMyTrade(const nlohmann::json& data) {
    try {
        Trade trade;
        trade.id = data["t"].get<std::string>();
        trade.orderId = data["i"].get<std::string>();
        trade.symbol = data["s"].get<std::string>();
        trade.side = data["S"].get<std::string>();
        trade.price = std::stod(data["p"].get<std::string>());
        trade.amount = std::stod(data["q"].get<std::string>());
        trade.cost = trade.price * trade.amount;
        trade.fee = std::stod(data["n"].get<std::string>());
        trade.feeCurrency = data["N"].get<std::string>();
        trade.timestamp = data["E"].get<uint64_t>();

        //exchange_.emitMyTrade(trade);
    } catch (const std::exception& e) {
        std::cerr << "Error handling my trade: " << e.what() << std::endl;
    }
}

void BinanceWS::handlePosition(const nlohmann::json& data) {
    try {
        Position position;
        position.symbol = data["s"].get<std::string>();
        position.side = data["ps"].get<std::string>();
        position.amount = std::stod(data["pa"].get<std::string>());
        position.entryPrice = std::stod(data["ep"].get<std::string>());
        position.unrealizedPnl = std::stod(data["up"].get<std::string>());
        position.leverage = std::stod(data["l"].get<std::string>());
        position.marginType = data["mt"].get<std::string>();
        position.timestamp = data["E"].get<uint64_t>();

        //exchange_.emitPosition(position);
    } catch (const std::exception& e) {
        std::cerr << "Error handling position: " << e.what() << std::endl;
    }
}

std::string BinanceWS::getStream(const std::string& type, const std::string& subscriptionHash, int numSubscriptions) {
    auto it = streamBySubscriptionsHash_.find(subscriptionHash);
    if (it != streamBySubscriptionsHash_.end()) {
        return it->second;
    }

    // Create new stream
    streamIndex_++;
    auto streamLimit = streamLimits_[type];
    int normalizedIndex = streamIndex_ % streamLimit;
    std::string stream = std::to_string(normalizedIndex);
    
    streamBySubscriptionsHash_[subscriptionHash] = stream;
    checkSubscriptionLimit(type, stream, numSubscriptions);
    
    return stream;
}

void BinanceWS::checkSubscriptionLimit(const std::string& type, const std::string& stream, int numSubscriptions) {
    static std::unordered_map<std::string, int> subscriptionsByStream;
    
    auto it = subscriptionsByStream.find(stream);
    int currentSubscriptions = (it != subscriptionsByStream.end()) ? it->second : 0;
    int newNumSubscriptions = currentSubscriptions + numSubscriptions;
    
    auto limitIt = this->subscriptionLimits_.find(type);
    int subscriptionLimit = (limitIt != subscriptionLimits_.end()) ? limitIt->second : 200;
    
    if (newNumSubscriptions > subscriptionLimit) {
        throw std::runtime_error("Reached the limit of subscriptions by stream. Increase the number of streams, or increase the stream limit or subscription limit by stream if the exchange allows.");
    }
    
    subscriptionsByStream[stream] = newNumSubscriptions;
}

 
}// namespace ccxt
