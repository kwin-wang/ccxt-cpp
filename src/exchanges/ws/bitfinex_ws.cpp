#include "../../../include/ccxt/exchanges/ws/bitfinex_ws.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <chrono>
#include <openssl/hmac.h>

namespace ccxt {

BitfinexWS::BitfinexWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Bitfinex& exchange)
    : WebSocketClient(ioc, ctx), exchange_(exchange) {
    options_ = {
        {"watchOrderBook", {
            {"prec", "P0"},
            {"freq", "F0"}
        }},
        {"ordersLimit", 1000}
    };
}

std::string BitfinexWS::getEndpoint() {
    return "wss://api-pub.bitfinex.com/ws/1";
}

std::string BitfinexWS::getPrivateEndpoint() {
    return "wss://api.bitfinex.com/ws/1";
}

void BitfinexWS::authenticate() {
    auto apiKey = exchange_.getApiKey();
    auto apiSecret = exchange_.getApiSecret();
    
    if (apiKey.empty() || apiSecret.empty()) {
        throw std::runtime_error("API key and secret required for private endpoints");
    }
    
    uint64_t nonce = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    std::string payload = "AUTH" + std::to_string(nonce);
    std::string signature = exchange_.hmac(payload, apiSecret, "sha384", "hex");
    
    nlohmann::json request = {
        {"event", "auth"},
        {"apiKey", apiKey},
        {"authSig", signature},
        {"authNonce", nonce},
        {"authPayload", payload}
    };
    
    send(request.dump());
}

void BitfinexWS::subscribe(const std::string& channel, const std::string& symbol,
                          const nlohmann::json& params) {
    auto market = exchange_.market(symbol);
    
    nlohmann::json request = {
        {"event", "subscribe"},
        {"channel", channel},
        {"symbol", market.id}
    };
    
    if (!params.empty()) {
        for (auto& [key, value] : params.items()) {
            request[key] = value;
        }
    }
    
    send(request.dump());
}

void BitfinexWS::watchTicker(const std::string& symbol) {
    subscribe("ticker", symbol);
}

void BitfinexWS::watchOrderBook(const std::string& symbol, const std::string& prec,
                               const std::string& freq) {
    nlohmann::json params = {
        {"prec", prec},
        {"freq", freq}
    };
    
    subscribe("book", symbol, params);
}

void BitfinexWS::watchTrades(const std::string& symbol) {
    subscribe("trades", symbol);
}

void BitfinexWS::watchBalance() {
    if (!authenticated_) {
        authenticate();
    }
    
    nlohmann::json request = {
        {"event", "subscribe"},
        {"channel", "wallet"}
    };
    
    send(request.dump());
}

void BitfinexWS::watchOrders() {
    if (!authenticated_) {
        authenticate();
    }
    
    nlohmann::json request = {
        {"event", "subscribe"},
        {"channel", "orders"}
    };
    
    send(request.dump());
}

void BitfinexWS::watchMyTrades() {
    if (!authenticated_) {
        authenticate();
    }
    
    nlohmann::json request = {
        {"event", "subscribe"},
        {"channel", "trades"}
    };
    
    send(request.dump());
}

void BitfinexWS::createOrder(const std::string& symbol, const std::string& type,
                            const std::string& side, double amount, double price) {
    if (!authenticated_) {
        authenticate();
    }
    
    auto market = exchange_.market(symbol);
    
    nlohmann::json request = {
        {"event", "submit_order"},
        {"symbol", market.id},
        {"type", type},
        {"side", side},
        {"amount", std::to_string(amount)},
        {"price", std::to_string(price)}
    };
    
    send(request.dump());
}

void BitfinexWS::cancelOrder(const std::string& id) {
    if (!authenticated_) {
        authenticate();
    }
    
    nlohmann::json request = {
        {"event", "cancel_order"},
        {"id", id}
    };
    
    send(request.dump());
}

void BitfinexWS::cancelAllOrders() {
    if (!authenticated_) {
        authenticate();
    }
    
    nlohmann::json request = {
        {"event", "cancel_all"}
    };
    
    send(request.dump());
}

void BitfinexWS::handleMessage(const std::string& message) {
    try {
        auto j = nlohmann::json::parse(message);
        
        if (j.is_object()) {
            if (j.contains("event")) {
                std::string event = j["event"];
                
                if (event == "subscribed") {
                    handleSubscribed(j);
                } else if (event == "auth") {
                    if (j["status"] == "OK") {
                        authenticated_ = true;
                        std::cout << "Successfully authenticated" << std::endl;
                    } else {
                        std::cerr << "Authentication failed: " << j["message"] << std::endl;
                    }
                } else if (event == "error") {
                    handleError(j);
                }
            }
        } else if (j.is_array()) {
            int channelId = j[0];
            
            if (j[1] == "hb") {
                handleHeartbeat(channelId);
                return;
            }
            
            auto channelIt = channelMap_.find(channelId);
            if (channelIt == channelMap_.end()) {
                return;
            }
            
            std::string channel = channelIt->second;
            
            if (channel == "ticker") {
                handleTicker(channelId, j[1]);
            } else if (channel == "book") {
                handleOrderBook(channelId, j[1]);
            } else if (channel == "trades") {
                handleTrade(channelId, j[1]);
            } else if (channel == "wallet") {
                handleBalance(channelId, j[1]);
            } else if (channel == "orders") {
                handleOrder(channelId, j[1]);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling message: " << e.what() << std::endl;
    }
}

void BitfinexWS::handleTicker(int channelId, const nlohmann::json& data) {
    try {
        if (!data.is_array()) {
            return;
        }
        
        Ticker ticker;
        ticker.bid = std::stod(data[0].get<std::string>());
        ticker.bidVolume = std::stod(data[1].get<std::string>());
        ticker.ask = std::stod(data[2].get<std::string>());
        ticker.askVolume = std::stod(data[3].get<std::string>());
        ticker.dailyChange = std::stod(data[4].get<std::string>());
        ticker.dailyChangePercentage = std::stod(data[5].get<std::string>());
        ticker.last = std::stod(data[6].get<std::string>());
        ticker.volume = std::stod(data[7].get<std::string>());
        ticker.high = std::stod(data[8].get<std::string>());
        ticker.low = std::stod(data[9].get<std::string>());
        ticker.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        
        exchange_.emitTicker(ticker);
    } catch (const std::exception& e) {
        std::cerr << "Error handling ticker: " << e.what() << std::endl;
    }
}

void BitfinexWS::handleOrderBook(int channelId, const nlohmann::json& data) {
    try {
        OrderBook orderBook;
        
        if (data.is_array() && data[0].is_array()) {
            // Snapshot
            for (const auto& item : data) {
                double price = std::stod(item[0].get<std::string>());
                int count = item[1].get<int>();
                double amount = std::stod(item[2].get<std::string>());
                
                if (amount > 0) {
                    orderBook.bids.emplace_back(price, amount);
                } else {
                    orderBook.asks.emplace_back(price, -amount);
                }
            }
        } else {
            // Update
            double price = std::stod(data[0].get<std::string>());
            int count = data[1].get<int>();
            double amount = std::stod(data[2].get<std::string>());
            
            if (count > 0) {
                if (amount > 0) {
                    orderBook.bids.emplace_back(price, amount);
                } else {
                    orderBook.asks.emplace_back(price, -amount);
                }
            }
        }
        
        exchange_.emitOrderBook(orderBook);
    } catch (const std::exception& e) {
        std::cerr << "Error handling order book: " << e.what() << std::endl;
    }
}

void BitfinexWS::handleTrade(int channelId, const nlohmann::json& data) {
    try {
        if (data[0] == "te") {
            Trade trade;
            auto tradeData = data[1];
            
            trade.id = std::to_string(tradeData[0].get<int64_t>());
            trade.timestamp = tradeData[1].get<int64_t>() * 1000;
            trade.amount = std::stod(tradeData[2].get<std::string>());
            trade.price = std::stod(tradeData[3].get<std::string>());
            trade.side = trade.amount > 0 ? "buy" : "sell";
            trade.amount = std::abs(trade.amount);
            
            exchange_.emitTrade(trade);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling trade: " << e.what() << std::endl;
    }
}

void BitfinexWS::handleBalance(int channelId, const nlohmann::json& data) {
    try {
        if (data[0] == "wu") {
            auto balanceData = data[1];
            
            Balance balance;
            balance.currency = balanceData[1].get<std::string>();
            balance.total = std::stod(balanceData[2].get<std::string>());
            balance.available = std::stod(balanceData[4].get<std::string>());
            balance.used = balance.total - balance.available;
            balance.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
            
            exchange_.emitBalance(balance);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling balance: " << e.what() << std::endl;
    }
}

void BitfinexWS::handleOrder(int channelId, const nlohmann::json& data) {
    try {
        if (data[0] == "on") {
            auto orderData = data[1];
            
            Order order;
            order.id = std::to_string(orderData[0].get<int64_t>());
            order.symbol = orderData[3].get<std::string>();
            order.timestamp = orderData[4].get<int64_t>() * 1000;
            order.amount = std::stod(orderData[6].get<std::string>());
            order.remaining = std::stod(orderData[7].get<std::string>());
            order.filled = order.amount - order.remaining;
            order.price = std::stod(orderData[16].get<std::string>());
            order.status = order.remaining == 0 ? "closed" : "open";
            
            exchange_.emitOrder(order);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling order: " << e.what() << std::endl;
    }
}

void BitfinexWS::handleMyTrade(int channelId, const nlohmann::json& data) {
    try {
        if (data[0] == "te") {
            auto tradeData = data[1];
            
            Trade trade;
            trade.id = std::to_string(tradeData[0].get<int64_t>());
            trade.orderId = std::to_string(tradeData[3].get<int64_t>());
            trade.timestamp = tradeData[2].get<int64_t>() * 1000;
            trade.symbol = tradeData[1].get<std::string>();
            trade.amount = std::stod(tradeData[4].get<std::string>());
            trade.price = std::stod(tradeData[5].get<std::string>());
            trade.fee = std::stod(tradeData[9].get<std::string>());
            trade.side = trade.amount > 0 ? "buy" : "sell";
            trade.amount = std::abs(trade.amount);
            
            exchange_.emitMyTrade(trade);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling my trade: " << e.what() << std::endl;
    }
}

void BitfinexWS::handleHeartbeat(int channelId) {
    nlohmann::json response = {
        {channelId, "hb"}
    };
    send(response.dump());
}

void BitfinexWS::handleSubscribed(const nlohmann::json& data) {
    try {
        int channelId = data["chanId"];
        std::string channel = data["channel"];
        channelMap_[channelId] = channel;
        
        std::cout << "Successfully subscribed to " << channel << " (Channel ID: " << channelId << ")" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error handling subscription: " << e.what() << std::endl;
    }
}

void BitfinexWS::handleError(const nlohmann::json& data) {
    std::cerr << "Error: " << data["msg"] << std::endl;
}

} // namespace ccxt
