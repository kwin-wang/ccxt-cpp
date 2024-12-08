#include "../../../include/ccxt/exchanges/ws/huobijp_ws.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <chrono>
#include <boost/crc.hpp>
#include <gzip/decompress.hpp>

namespace ccxt {

HuobiJPWS::HuobiJPWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, HuobiJP& exchange)
    : WebSocketClient(ioc, ctx), exchange_(exchange) {
    options_ = {
        {"watchOrderBookRate", 100},
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

std::string HuobiJPWS::getEndpoint() {
    std::string hostname = exchange_.getHostname();
    return "wss://" + hostname + "/ws";
}

void HuobiJPWS::authenticate() {
    auto apiKey = exchange_.getApiKey();
    auto apiSecret = exchange_.getApiSecret();
    
    if (apiKey.empty() || apiSecret.empty()) {
        throw std::runtime_error("API key and secret required for private endpoints");
    }
    
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    std::string authParams = "accessKey=" + apiKey + "&signatureMethod=HmacSHA256&signatureVersion=2.1&timestamp=" + std::to_string(timestamp);
    std::string signature = sign("GET\n" + exchange_.getHostname() + "\n/ws/v2\n" + authParams);
    
    nlohmann::json request = {
        {"action", "req"},
        {"ch", "auth"},
        {"params", {
            {"authType", "api"},
            {"accessKey", apiKey},
            {"signatureMethod", "HmacSHA256"},
            {"signatureVersion", "2.1"},
            {"timestamp", timestamp},
            {"signature", signature}
        }}
    };
    
    send(request.dump());
}

void HuobiJPWS::watchTicker(const std::string& symbol) {
    auto market = exchange_.market(symbol);
    std::string topic = "market." + market.id + ".detail";
    
    nlohmann::json request = {
        {"sub", topic},
        {"id", nextRequestId_++}
    };
    
    send(request.dump());
}

void HuobiJPWS::watchOrderBook(const std::string& symbol, const std::string& limit) {
    auto market = exchange_.market(symbol);
    std::string topic = "market." + market.id + ".depth.step0";
    
    nlohmann::json request = {
        {"sub", topic},
        {"id", nextRequestId_++}
    };
    
    send(request.dump());
}

void HuobiJPWS::watchTrades(const std::string& symbol) {
    auto market = exchange_.market(symbol);
    std::string topic = "market." + market.id + ".trade.detail";
    
    nlohmann::json request = {
        {"sub", topic},
        {"id", nextRequestId_++}
    };
    
    send(request.dump());
}

void HuobiJPWS::watchOHLCV(const std::string& symbol, const std::string& timeframe) {
    auto market = exchange_.market(symbol);
    std::string period = exchange_.timeframes[timeframe];
    std::string topic = "market." + market.id + ".kline." + period;
    
    nlohmann::json request = {
        {"sub", topic},
        {"id", nextRequestId_++}
    };
    
    send(request.dump());
}

void HuobiJPWS::watchBalance() {
    if (!authenticated_) {
        authenticate();
    }
    
    nlohmann::json request = {
        {"action", "sub"},
        {"ch", "accounts.update#2"}
    };
    
    send(request.dump());
}

void HuobiJPWS::watchOrders() {
    if (!authenticated_) {
        authenticate();
    }
    
    nlohmann::json request = {
        {"action", "sub"},
        {"ch", "orders#*"}
    };
    
    send(request.dump());
}

void HuobiJPWS::watchMyTrades() {
    if (!authenticated_) {
        authenticate();
    }
    
    nlohmann::json request = {
        {"action", "sub"},
        {"ch", "trade.clearing#*"}
    };
    
    send(request.dump());
}

std::string HuobiJPWS::sign(const std::string& payload) {
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

void HuobiJPWS::handleMessage(const std::string& message) {
    try {
        auto j = nlohmann::json::parse(message);
        
        // Handle ping-pong
        if (j.contains("ping")) {
            nlohmann::json pong = {{"pong", j["ping"]}};
            send(pong.dump());
            return;
        }
        
        // Handle authentication response
        if (j.contains("action") && j["action"] == "req" && j.contains("ch") && j["ch"] == "auth") {
            if (j["code"].get<int>() == 200) {
                authenticated_ = true;
                std::cout << "Successfully authenticated" << std::endl;
            } else {
                std::cerr << "Authentication failed: " << j["message"] << std::endl;
            }
            return;
        }
        
        // Handle subscription responses
        if (j.contains("subbed")) {
            if (j.contains("status") && j["status"] == "ok") {
                std::cout << "Successfully subscribed to " << j["subbed"] << std::endl;
            } else {
                std::cerr << "Subscription failed: " << j["subbed"] << std::endl;
            }
            return;
        }
        
        // Handle data updates
        if (j.contains("ch")) {
            std::string channel = j["ch"];
            auto data = j["tick"];
            
            if (channel.find("market") == 0) {
                if (channel.find(".detail") != std::string::npos) {
                    handleTicker(data);
                } else if (channel.find(".depth") != std::string::npos) {
                    handleOrderBook(data);
                } else if (channel.find(".trade") != std::string::npos) {
                    handleTrade(data);
                } else if (channel.find(".kline") != std::string::npos) {
                    handleOHLCV(data);
                }
            } else if (channel.find("accounts") == 0) {
                handleBalance(data);
            } else if (channel.find("orders") == 0) {
                handleOrder(data);
            } else if (channel.find("trade.clearing") == 0) {
                handleMyTrade(data);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling message: " << e.what() << std::endl;
    }
}

void HuobiJPWS::handleTicker(const nlohmann::json& data) {
    Ticker ticker;
    ticker.symbol = data["symbol"].get<std::string>();
    ticker.high = std::stod(data["high"].get<std::string>());
    ticker.low = std::stod(data["low"].get<std::string>());
    ticker.bid = std::stod(data["bid"][0].get<std::string>());
    ticker.ask = std::stod(data["ask"][0].get<std::string>());
    ticker.last = std::stod(data["close"].get<std::string>());
    ticker.volume = std::stod(data["vol"].get<std::string>());
    ticker.timestamp = data["ts"].get<uint64_t>();
    
    exchange_.emitTicker(ticker);
}

void HuobiJPWS::handleOrderBook(const nlohmann::json& data) {
    OrderBook orderBook;
    orderBook.symbol = data["symbol"].get<std::string>();
    orderBook.timestamp = data["ts"].get<uint64_t>();
    
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
    
    if (data.contains("version")) {
        orderBook.nonce = data["version"].get<uint64_t>();
    }
    
    exchange_.emitOrderBook(orderBook);
}

void HuobiJPWS::handleTrade(const nlohmann::json& data) {
    const auto& trades = data["data"];
    for (const auto& t : trades) {
        Trade trade;
        trade.id = t["id"].get<std::string>();
        trade.symbol = data["symbol"].get<std::string>();
        trade.price = std::stod(t["price"].get<std::string>());
        trade.amount = std::stod(t["amount"].get<std::string>());
        trade.side = t["direction"].get<std::string>();
        trade.timestamp = t["ts"].get<uint64_t>();
        
        exchange_.emitTrade(trade);
    }
}

void HuobiJPWS::handleOHLCV(const nlohmann::json& data) {
    OHLCV ohlcv;
    ohlcv.timestamp = data["id"].get<uint64_t>() * 1000;
    ohlcv.open = std::stod(data["open"].get<std::string>());
    ohlcv.high = std::stod(data["high"].get<std::string>());
    ohlcv.low = std::stod(data["low"].get<std::string>());
    ohlcv.close = std::stod(data["close"].get<std::string>());
    ohlcv.volume = std::stod(data["vol"].get<std::string>());
    
    exchange_.emitOHLCV(ohlcv);
}

void HuobiJPWS::handleBalance(const nlohmann::json& data) {
    try {
        Balance balance;
        balance.currency = data["currency"].get<std::string>();
        balance.free = std::stod(data["available"].get<std::string>());
        balance.used = std::stod(data["frozen"].get<std::string>());
        balance.total = balance.free + balance.used;
        balance.timestamp = data["ts"].get<uint64_t>();
        
        exchange_.emitBalance(balance);
    } catch (const std::exception& e) {
        std::cerr << "Error handling balance: " << e.what() << std::endl;
    }
}

void HuobiJPWS::handleOrder(const nlohmann::json& data) {
    try {
        Order order;
        order.id = data["orderId"].get<std::string>();
        order.symbol = data["symbol"].get<std::string>();
        order.type = data["type"].get<std::string>();
        order.side = data["side"].get<std::string>();
        order.price = std::stod(data["price"].get<std::string>());
        order.amount = std::stod(data["orderSize"].get<std::string>());
        order.filled = std::stod(data["filledSize"].get<std::string>());
        order.remaining = order.amount - order.filled;
        order.status = data["orderStatus"].get<std::string>();
        order.timestamp = data["orderCreateTime"].get<uint64_t>();
        
        exchange_.emitOrder(order);
    } catch (const std::exception& e) {
        std::cerr << "Error handling order: " << e.what() << std::endl;
    }
}

void HuobiJPWS::handleMyTrade(const nlohmann::json& data) {
    try {
        Trade trade;
        trade.id = data["tradeId"].get<std::string>();
        trade.orderId = data["orderId"].get<std::string>();
        trade.symbol = data["symbol"].get<std::string>();
        trade.side = data["orderSide"].get<std::string>();
        trade.price = std::stod(data["tradePrice"].get<std::string>());
        trade.amount = std::stod(data["tradeVolume"].get<std::string>());
        trade.cost = trade.price * trade.amount;
        trade.fee = std::stod(data["transactFee"].get<std::string>());
        trade.feeCurrency = data["feeCurrency"].get<std::string>();
        trade.timestamp = data["tradeTime"].get<uint64_t>();
        
        exchange_.emitMyTrade(trade);
    } catch (const std::exception& e) {
        std::cerr << "Error handling my trade: " << e.what() << std::endl;
    }
}

} // namespace ccxt
