#include "../../../include/ccxt/exchanges/ws/idex_ws.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <chrono>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

namespace ccxt {

IDEXWS::IDEXWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, IDEX& exchange)
    : WebSocketClient(ioc, ctx), exchange_(exchange) {
    options_ = {
        {"watchOrderBookLimit", 100},
        {"tradesLimit", 1000},
        {"OHLCVLimit", 1000}
    };
}

std::string IDEXWS::getEndpoint() {
    return "wss://websocket.idex.io/v1";
}

void IDEXWS::authenticate() {
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
        {"method", "authenticate"},
        {"params", {
            {"key", apiKey},
            {"timestamp", timestamp},
            {"signature", signature}
        }},
        {"id", getNextRequestId()}
    };

    send(request.dump());
}

void IDEXWS::watchTicker(const std::string& symbol) {
    std::string marketId = getMarketId(symbol);
    subscribe("tickers", marketId);
}

void IDEXWS::watchTickers(const std::vector<std::string>& symbols) {
    if (symbols.empty()) {
        subscribe("tickers");
    } else {
        for (const auto& symbol : symbols) {
            watchTicker(symbol);
        }
    }
}

void IDEXWS::watchOrderBook(const std::string& symbol, const std::string& limit) {
    std::string marketId = getMarketId(symbol);
    subscribe("orderbook", marketId);
}

void IDEXWS::watchTrades(const std::string& symbol) {
    std::string marketId = getMarketId(symbol);
    subscribe("trades", marketId);
}

void IDEXWS::watchOHLCV(const std::string& symbol, const std::string& timeframe) {
    std::string marketId = getMarketId(symbol);
    subscribe("candles", marketId + ":" + timeframe);
}

void IDEXWS::watchStatus() {
    subscribe("status");
}

void IDEXWS::watchTime() {
    subscribe("time");
}

void IDEXWS::watchBalance() {
    if (!authenticated_) {
        authenticate();
    }
    subscribe("balances");
}

void IDEXWS::watchOrders(const std::string& symbol) {
    if (!authenticated_) {
        authenticate();
    }
    if (!symbol.empty()) {
        std::string marketId = getMarketId(symbol);
        subscribe("orders", marketId);
    } else {
        subscribe("orders");
    }
}

void IDEXWS::watchMyTrades(const std::string& symbol) {
    if (!authenticated_) {
        authenticate();
    }
    if (!symbol.empty()) {
        std::string marketId = getMarketId(symbol);
        subscribe("trades", marketId);
    } else {
        subscribe("trades");
    }
}

void IDEXWS::subscribe(const std::string& channel, const std::string& symbol) {
    nlohmann::json request = {
        {"method", "subscribe"},
        {"params", {
            {"channel", channel}
        }},
        {"id", getNextRequestId()}
    };

    if (!symbol.empty()) {
        request["params"]["market"] = symbol;
    }

    send(request.dump());
    subscriptions_[channel + (symbol.empty() ? "" : ":" + symbol)] = symbol;
}

void IDEXWS::unsubscribe(const std::string& channel, const std::string& symbol) {
    nlohmann::json request = {
        {"method", "unsubscribe"},
        {"params", {
            {"channel", channel}
        }},
        {"id", getNextRequestId()}
    };

    if (!symbol.empty()) {
        request["params"]["market"] = symbol;
    }

    send(request.dump());
    subscriptions_.erase(channel + (symbol.empty() ? "" : ":" + symbol));
}

std::string IDEXWS::sign(const std::string& payload) {
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

std::string IDEXWS::getMarketId(const std::string& symbol) {
    auto market = exchange_.market(symbol);
    return market.id;
}

std::string IDEXWS::getSymbol(const std::string& marketId) {
    for (const auto& market : exchange_.markets) {
        if (market.second.id == marketId) {
            return market.first;
        }
    }
    return marketId;
}

int IDEXWS::getNextRequestId() {
    return nextRequestId_++;
}

void IDEXWS::handleMessage(const std::string& message) {
    try {
        auto j = nlohmann::json::parse(message);

        // Handle authentication response
        if (j.contains("method") && j["method"] == "authenticate") {
            if (j["result"] == "success") {
                authenticated_ = true;
                std::cout << "Successfully authenticated" << std::endl;
            } else {
                std::cerr << "Authentication failed: " << j["error"]["message"] << std::endl;
            }
            return;
        }

        // Handle subscription responses
        if (j.contains("method") && j["method"] == "subscribe") {
            if (j["result"] == "success") {
                std::cout << "Successfully subscribed to " << j["params"]["channel"] << std::endl;
            } else {
                std::cerr << "Subscription failed: " << j["error"]["message"] << std::endl;
            }
            return;
        }

        // Handle data updates
        if (j.contains("type")) {
            std::string type = j["type"];
            auto data = j["data"];

            if (type == "ticker") {
                handleTicker(data);
            } else if (type == "l2orderbook") {
                handleOrderBook(data);
            } else if (type == "trades") {
                handleTrade(data);
            } else if (type == "candles") {
                handleOHLCV(data);
            } else if (type == "status") {
                handleStatus(data);
            } else if (type == "time") {
                handleTime(data);
            } else if (type == "balances") {
                handleBalance(data);
            } else if (type == "orders") {
                handleOrder(data);
            } else if (type == "myTrades") {
                handleMyTrade(data);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling message: " << e.what() << std::endl;
    }
}

void IDEXWS::handleTicker(const nlohmann::json& data) {
    try {
        Ticker ticker;
        ticker.symbol = getSymbol(data["market"].get<std::string>());
        ticker.high = std::stod(data["high24h"].get<std::string>());
        ticker.low = std::stod(data["low24h"].get<std::string>());
        ticker.bid = std::stod(data["bestBid"].get<std::string>());
        ticker.ask = std::stod(data["bestAsk"].get<std::string>());
        ticker.last = std::stod(data["lastPrice"].get<std::string>());
        ticker.volume = std::stod(data["baseVolume24h"].get<std::string>());
        ticker.quoteVolume = std::stod(data["quoteVolume24h"].get<std::string>());
        ticker.timestamp = data["timestamp"].get<uint64_t>();
        
        exchange_.emitTicker(ticker);
    } catch (const std::exception& e) {
        std::cerr << "Error handling ticker: " << e.what() << std::endl;
    }
}

void IDEXWS::handleOrderBook(const nlohmann::json& data) {
    try {
        OrderBook orderBook;
        orderBook.symbol = getSymbol(data["market"].get<std::string>());
        orderBook.timestamp = data["timestamp"].get<uint64_t>();
        
        const auto& bids = data["bids"];
        const auto& asks = data["asks"];
        
        for (const auto& bid : bids) {
            orderBook.bids.emplace_back(
                std::stod(bid["price"].get<std::string>()),
                std::stod(bid["size"].get<std::string>())
            );
        }
        
        for (const auto& ask : asks) {
            orderBook.asks.emplace_back(
                std::stod(ask["price"].get<std::string>()),
                std::stod(ask["size"].get<std::string>())
            );
        }
        
        if (data.contains("sequence")) {
            orderBook.nonce = data["sequence"].get<uint64_t>();
        }
        
        exchange_.emitOrderBook(orderBook);
    } catch (const std::exception& e) {
        std::cerr << "Error handling order book: " << e.what() << std::endl;
    }
}

void IDEXWS::handleTrade(const nlohmann::json& data) {
    try {
        Trade trade;
        trade.id = data["id"].get<std::string>();
        trade.symbol = getSymbol(data["market"].get<std::string>());
        trade.price = std::stod(data["price"].get<std::string>());
        trade.amount = std::stod(data["size"].get<std::string>());
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

void IDEXWS::handleOHLCV(const nlohmann::json& data) {
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

void IDEXWS::handleStatus(const nlohmann::json& data) {
    try {
        // Handle exchange status updates
        std::cout << "Exchange status: " << data["status"].get<std::string>() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error handling status: " << e.what() << std::endl;
    }
}

void IDEXWS::handleTime(const nlohmann::json& data) {
    try {
        // Handle server time updates
        std::cout << "Server time: " << data["timestamp"].get<uint64_t>() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error handling time: " << e.what() << std::endl;
    }
}

void IDEXWS::handleBalance(const nlohmann::json& data) {
    try {
        Balance balance;
        balance.currency = data["currency"].get<std::string>();
        balance.free = std::stod(data["available"].get<std::string>());
        balance.used = std::stod(data["held"].get<std::string>());
        balance.total = balance.free + balance.used;
        balance.timestamp = data["timestamp"].get<uint64_t>();
        
        exchange_.emitBalance(balance);
    } catch (const std::exception& e) {
        std::cerr << "Error handling balance: " << e.what() << std::endl;
    }
}

void IDEXWS::handleOrder(const nlohmann::json& data) {
    try {
        Order order;
        order.id = data["orderId"].get<std::string>();
        order.clientOrderId = data.contains("clientOrderId") ? data["clientOrderId"].get<std::string>() : "";
        order.symbol = getSymbol(data["market"].get<std::string>());
        order.type = data["type"].get<std::string>();
        order.side = data["side"].get<std::string>();
        order.price = std::stod(data["price"].get<std::string>());
        order.amount = std::stod(data["size"].get<std::string>());
        order.filled = std::stod(data["filled"].get<std::string>());
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

void IDEXWS::handleMyTrade(const nlohmann::json& data) {
    try {
        Trade trade;
        trade.id = data["tradeId"].get<std::string>();
        trade.orderId = data["orderId"].get<std::string>();
        trade.symbol = getSymbol(data["market"].get<std::string>());
        trade.type = data["type"].get<std::string>();
        trade.side = data["side"].get<std::string>();
        trade.price = std::stod(data["price"].get<std::string>());
        trade.amount = std::stod(data["size"].get<std::string>());
        trade.cost = trade.price * trade.amount;
        trade.timestamp = data["timestamp"].get<uint64_t>();
        
        if (data.contains("fee")) {
            trade.fee = {
                {"cost", std::stod(data["fee"]["amount"].get<std::string>())},
                {"currency", data["fee"]["currency"].get<std::string>()}
            };
        }
        
        exchange_.emitMyTrade(trade);
    } catch (const std::exception& e) {
        std::cerr << "Error handling my trade: " << e.what() << std::endl;
    }
}

} // namespace ccxt
