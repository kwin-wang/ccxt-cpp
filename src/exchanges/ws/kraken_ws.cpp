#include "../../../include/ccxt/exchanges/ws/kraken_ws.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <chrono>
#include <boost/crc.hpp>

namespace ccxt {

KrakenWS::KrakenWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Kraken& exchange)
    : WebSocketClient(ioc, ctx), exchange_(exchange) {
    options_ = {
        {"tradesLimit", 1000},
        {"OHLCVLimit", 1000},
        {"ordersLimit", 1000},
        {"watchOrderBook", {
            {"checksum", true}
        }}
    };
}

std::string KrakenWS::getEndpoint() {
    return "wss://ws.kraken.com";
}

std::string KrakenWS::getPrivateEndpoint() {
    return "wss://ws-auth.kraken.com";
}

void KrakenWS::authenticate() {
    auto apiKey = exchange_.getApiKey();
    auto apiSecret = exchange_.getApiSecret();
    
    if (apiKey.empty() || apiSecret.empty()) {
        throw std::runtime_error("API key and secret required for private endpoints");
    }
    
    uint64_t nonce = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    std::string token = exchange_.getToken();
    
    nlohmann::json request = {
        {"event", "subscribe"},
        {"subscription", {
            {"name", "ownTrades"},
            {"token", token}
        }}
    };
    
    send(request.dump());
}

void KrakenWS::watchTicker(const std::string& symbol) {
    auto market = exchange_.market(symbol);
    
    nlohmann::json request = {
        {"event", "subscribe"},
        {"pair", {market.id}},
        {"subscription", {
            {"name", "ticker"}
        }}
    };
    
    send(request.dump());
}

void KrakenWS::watchOrderBook(const std::string& symbol, const std::string& limit) {
    auto market = exchange_.market(symbol);
    
    nlohmann::json request = {
        {"event", "subscribe"},
        {"pair", {market.id}},
        {"subscription", {
            {"name", "book"},
            {"depth", limit.empty() ? 10 : std::stoi(limit)}
        }}
    };
    
    send(request.dump());
}

void KrakenWS::watchTrades(const std::string& symbol) {
    auto market = exchange_.market(symbol);
    
    nlohmann::json request = {
        {"event", "subscribe"},
        {"pair", {market.id}},
        {"subscription", {
            {"name", "trade"}
        }}
    };
    
    send(request.dump());
}

void KrakenWS::watchOHLCV(const std::string& symbol, const std::string& timeframe) {
    auto market = exchange_.market(symbol);
    int interval = std::stoi(exchange_.timeframes[timeframe]);
    
    nlohmann::json request = {
        {"event", "subscribe"},
        {"pair", {market.id}},
        {"subscription", {
            {"name", "ohlc"},
            {"interval", interval}
        }}
    };
    
    send(request.dump());
}

void KrakenWS::watchBalance() {
    if (!authenticated_) {
        authenticate();
    }
    
    nlohmann::json request = {
        {"event", "subscribe"},
        {"subscription", {
            {"name", "balances"}
        }}
    };
    
    send(request.dump());
}

void KrakenWS::watchOrders() {
    if (!authenticated_) {
        authenticate();
    }
    
    nlohmann::json request = {
        {"event", "subscribe"},
        {"subscription", {
            {"name", "openOrders"}
        }}
    };
    
    send(request.dump());
}

void KrakenWS::watchMyTrades() {
    if (!authenticated_) {
        authenticate();
    }
    
    nlohmann::json request = {
        {"event", "subscribe"},
        {"subscription", {
            {"name", "ownTrades"}
        }}
    };
    
    send(request.dump());
}

void KrakenWS::createOrder(const std::string& symbol, const std::string& type,
                          const std::string& side, double amount, double price) {
    if (!authenticated_) {
        authenticate();
    }
    
    nlohmann::json request = {
        {"event", "addOrder"},
        {"ordertype", type},
        {"pair", symbol},
        {"type", side},
        {"volume", std::to_string(amount)}
    };
    
    if (price > 0) {
        request["price"] = std::to_string(price);
    }
    
    send(request.dump());
}

void KrakenWS::editOrder(const std::string& id, const std::string& symbol,
                        const std::string& type, const std::string& side,
                        double amount, double price) {
    if (!authenticated_) {
        authenticate();
    }
    
    nlohmann::json request = {
        {"event", "editOrder"},
        {"orderid", id},
        {"pair", symbol},
        {"volume", std::to_string(amount)}
    };
    
    if (price > 0) {
        request["price"] = std::to_string(price);
    }
    
    send(request.dump());
}

void KrakenWS::cancelOrder(const std::string& id) {
    if (!authenticated_) {
        authenticate();
    }
    
    nlohmann::json request = {
        {"event", "cancelOrder"},
        {"orderid", id}
    };
    
    send(request.dump());
}

void KrakenWS::cancelAllOrders() {
    if (!authenticated_) {
        authenticate();
    }
    
    nlohmann::json request = {
        {"event", "cancelAll"}
    };
    
    send(request.dump());
}

void KrakenWS::handleMessage(const std::string& message) {
    try {
        auto j = nlohmann::json::parse(message);
        
        // Handle subscription responses
        if (j.contains("event")) {
            std::string event = j["event"];
            
            if (event == "subscriptionStatus") {
                if (j["status"] == "subscribed") {
                    std::cout << "Successfully subscribed to " << j["subscription"]["name"] << std::endl;
                } else {
                    std::cerr << "Subscription failed: " << j["errorMessage"] << std::endl;
                }
                return;
            } else if (event == "addOrderStatus") {
                handleOrderResponse(j);
                return;
            } else if (event == "editOrderStatus") {
                handleOrderResponse(j);
                return;
            } else if (event == "cancelOrderStatus") {
                handleOrderResponse(j);
                return;
            }
        }
        
        // Handle data updates
        if (j.is_array()) {
            auto channelName = j[2];
            auto channelId = j[0].get<int>();
            auto data = j[1];
            
            if (channelName == "ticker") {
                handleTicker(data);
            } else if (channelName == "trade") {
                handleTrade(data);
            } else if (channelName == "ohlc") {
                handleOHLCV(data);
            } else if (channelName == "book") {
                handleOrderBook(data);
            } else if (channelName == "ownTrades") {
                handleMyTrade(data);
            } else if (channelName == "openOrders") {
                handleOrder(data);
            } else if (channelName == "balances") {
                handleBalance(data);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling message: " << e.what() << std::endl;
    }
}

void KrakenWS::handleTicker(const nlohmann::json& data) {
    try {
        Ticker ticker;
        ticker.symbol = data["p"][0];
        ticker.bid = std::stod(data["b"][0].get<std::string>());
        ticker.ask = std::stod(data["a"][0].get<std::string>());
        ticker.last = std::stod(data["c"][0].get<std::string>());
        ticker.volume = std::stod(data["v"][1].get<std::string>());
        ticker.high = std::stod(data["h"][1].get<std::string>());
        ticker.low = std::stod(data["l"][1].get<std::string>());
        ticker.vwap = std::stod(data["p"][1].get<std::string>());
        ticker.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        
        exchange_.emitTicker(ticker);
    } catch (const std::exception& e) {
        std::cerr << "Error handling ticker: " << e.what() << std::endl;
    }
}

void KrakenWS::handleOrderBook(const nlohmann::json& data) {
    try {
        OrderBook orderBook;
        orderBook.symbol = data["p"][0];
        
        if (data.contains("bs")) {
            for (const auto& bid : data["bs"]) {
                orderBook.bids.emplace_back(
                    std::stod(bid[0].get<std::string>()),
                    std::stod(bid[1].get<std::string>())
                );
            }
        }
        
        if (data.contains("as")) {
            for (const auto& ask : data["as"]) {
                orderBook.asks.emplace_back(
                    std::stod(ask[0].get<std::string>()),
                    std::stod(ask[1].get<std::string>())
                );
            }
        }
        
        exchange_.emitOrderBook(orderBook);
    } catch (const std::exception& e) {
        std::cerr << "Error handling order book: " << e.what() << std::endl;
    }
}

void KrakenWS::handleTrade(const nlohmann::json& data) {
    try {
        for (const auto& t : data) {
            Trade trade;
            trade.symbol = data["p"][0];
            trade.price = std::stod(t[0].get<std::string>());
            trade.amount = std::stod(t[1].get<std::string>());
            trade.timestamp = std::stoll(t[2].get<std::string>()) * 1000;
            trade.side = t[3] == "b" ? "buy" : "sell";
            trade.type = t[4] == "l" ? "limit" : "market";
            
            exchange_.emitTrade(trade);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling trade: " << e.what() << std::endl;
    }
}

void KrakenWS::handleOHLCV(const nlohmann::json& data) {
    try {
        OHLCV ohlcv;
        ohlcv.timestamp = std::stoll(data[0].get<std::string>()) * 1000;
        ohlcv.open = std::stod(data[1].get<std::string>());
        ohlcv.high = std::stod(data[2].get<std::string>());
        ohlcv.low = std::stod(data[3].get<std::string>());
        ohlcv.close = std::stod(data[4].get<std::string>());
        ohlcv.volume = std::stod(data[6].get<std::string>());
        
        exchange_.emitOHLCV(ohlcv);
    } catch (const std::exception& e) {
        std::cerr << "Error handling OHLCV: " << e.what() << std::endl;
    }
}

void KrakenWS::handleBalance(const nlohmann::json& data) {
    try {
        for (const auto& [currency, value] : data.items()) {
            Balance balance;
            balance.currency = currency;
            balance.free = std::stod(value.get<std::string>());
            balance.used = 0.0;  // Kraken doesn't provide used balance in WS
            balance.total = balance.free;
            balance.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
            
            exchange_.emitBalance(balance);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling balance: " << e.what() << std::endl;
    }
}

void KrakenWS::handleOrder(const nlohmann::json& data) {
    try {
        for (const auto& [orderId, orderData] : data.items()) {
            Order order;
            order.id = orderId;
            order.symbol = orderData["descr"]["pair"].get<std::string>();
            order.type = orderData["descr"]["ordertype"].get<std::string>();
            order.side = orderData["descr"]["type"].get<std::string>();
            order.price = std::stod(orderData["descr"]["price"].get<std::string>());
            order.amount = std::stod(orderData["vol"].get<std::string>());
            order.filled = std::stod(orderData["vol_exec"].get<std::string>());
            order.remaining = order.amount - order.filled;
            order.status = orderData["status"].get<std::string>();
            order.timestamp = std::stoll(orderData["opentm"].get<std::string>()) * 1000;
            
            exchange_.emitOrder(order);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling order: " << e.what() << std::endl;
    }
}

void KrakenWS::handleMyTrade(const nlohmann::json& data) {
    try {
        for (const auto& [tradeId, tradeData] : data.items()) {
            Trade trade;
            trade.id = tradeId;
            trade.orderId = tradeData["ordertxid"].get<std::string>();
            trade.symbol = tradeData["pair"].get<std::string>();
            trade.type = tradeData["ordertype"].get<std::string>();
            trade.side = tradeData["type"].get<std::string>();
            trade.price = std::stod(tradeData["price"].get<std::string>());
            trade.amount = std::stod(tradeData["vol"].get<std::string>());
            trade.cost = trade.price * trade.amount;
            trade.fee = std::stod(tradeData["fee"].get<std::string>());
            trade.timestamp = std::stoll(tradeData["time"].get<std::string>()) * 1000;
            
            exchange_.emitMyTrade(trade);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling my trade: " << e.what() << std::endl;
    }
}

void KrakenWS::handleOrderResponse(const nlohmann::json& data) {
    try {
        if (data["status"] == "ok") {
            std::cout << "Order operation successful: " << data["txid"][0] << std::endl;
        } else {
            std::cerr << "Order operation failed: " << data["errorMessage"] << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling order response: " << e.what() << std::endl;
    }
}

} // namespace ccxt
