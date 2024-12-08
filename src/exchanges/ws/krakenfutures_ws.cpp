#include "../../../include/ccxt/exchanges/ws/krakenfutures_ws.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <chrono>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <openssl/hmac.h>

namespace ccxt {

KrakenFuturesWS::KrakenFuturesWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, KrakenFutures& exchange)
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

std::string KrakenFuturesWS::getEndpoint() {
    return "wss://futures.kraken.com/ws/v1";
}

std::string KrakenFuturesWS::getPrivateEndpoint() {
    return "wss://futures.kraken.com/ws/v1";
}

void KrakenFuturesWS::authenticate() {
    auto apiKey = exchange_.getApiKey();
    auto apiSecret = exchange_.getApiSecret();
    
    if (apiKey.empty() || apiSecret.empty()) {
        throw std::runtime_error("API key and secret required for private endpoints");
    }

    uint64_t nonce = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    std::string path = "/ws/v1/auth";
    std::string postData = "nonce=" + std::to_string(nonce);
    std::string signature = sign(path, std::to_string(nonce), postData);

    nlohmann::json request = {
        {"event", "subscribe"},
        {"feed", "auth"},
        {"api_key", apiKey},
        {"sign", signature},
        {"timestamp", nonce}
    };

    send(request.dump());
}

void KrakenFuturesWS::watchTicker(const std::string& symbol) {
    auto market = exchange_.market(symbol);
    subscribe("ticker", market.id);
}

void KrakenFuturesWS::watchOrderBook(const std::string& symbol, const std::string& limit) {
    auto market = exchange_.market(symbol);
    subscribe("book", market.id);
}

void KrakenFuturesWS::watchTrades(const std::string& symbol) {
    auto market = exchange_.market(symbol);
    subscribe("trade", market.id);
}

void KrakenFuturesWS::watchOHLCV(const std::string& symbol, const std::string& timeframe) {
    auto market = exchange_.market(symbol);
    subscribe("candles", market.id);
}

void KrakenFuturesWS::watchMarkPrice(const std::string& symbol) {
    auto market = exchange_.market(symbol);
    subscribe("markPrice", market.id);
}

void KrakenFuturesWS::watchFundingRate(const std::string& symbol) {
    auto market = exchange_.market(symbol);
    subscribe("fundingRate", market.id);
}

void KrakenFuturesWS::watchBalance() {
    if (!authenticated_) {
        authenticate();
    }
    subscribe("account_balances");
}

void KrakenFuturesWS::watchOrders() {
    if (!authenticated_) {
        authenticate();
    }
    subscribe("open_orders");
}

void KrakenFuturesWS::watchMyTrades() {
    if (!authenticated_) {
        authenticate();
    }
    subscribe("fills");
}

void KrakenFuturesWS::watchPositions() {
    if (!authenticated_) {
        authenticate();
    }
    subscribe("open_positions");
}

void KrakenFuturesWS::createOrder(const std::string& symbol, const std::string& type,
                                const std::string& side, double amount, double price,
                                const std::unordered_map<std::string, std::string>& params) {
    if (!authenticated_) {
        authenticate();
    }

    nlohmann::json request = {
        {"feed", "order"},
        {"event", "create"},
        {"symbol", symbol},
        {"orderType", type},
        {"side", side},
        {"size", amount}
    };

    if (price > 0) {
        request["limitPrice"] = price;
    }

    for (const auto& [key, value] : params) {
        request[key] = value;
    }

    send(request.dump());
}

void KrakenFuturesWS::editOrder(const std::string& id, const std::string& symbol,
                              const std::string& type, const std::string& side,
                              double amount, double price) {
    if (!authenticated_) {
        authenticate();
    }

    nlohmann::json request = {
        {"feed", "order"},
        {"event", "edit"},
        {"orderId", id},
        {"size", amount}
    };

    if (price > 0) {
        request["limitPrice"] = price;
    }

    send(request.dump());
}

void KrakenFuturesWS::cancelOrder(const std::string& id) {
    if (!authenticated_) {
        authenticate();
    }

    nlohmann::json request = {
        {"feed", "order"},
        {"event", "cancel"},
        {"orderId", id}
    };

    send(request.dump());
}

void KrakenFuturesWS::cancelAllOrders() {
    if (!authenticated_) {
        authenticate();
    }

    nlohmann::json request = {
        {"feed", "order"},
        {"event", "cancelAll"}
    };

    send(request.dump());
}

void KrakenFuturesWS::subscribe(const std::string& channel, const std::string& symbol) {
    nlohmann::json request = {
        {"event", "subscribe"},
        {"feed", channel}
    };

    if (!symbol.empty()) {
        request["product_ids"] = {symbol};
    }

    send(request.dump());
    subscriptions_[channel + (symbol.empty() ? "" : ":" + symbol)] = symbol;
}

void KrakenFuturesWS::unsubscribe(const std::string& channel, const std::string& symbol) {
    nlohmann::json request = {
        {"event", "unsubscribe"},
        {"feed", channel}
    };

    if (!symbol.empty()) {
        request["product_ids"] = {symbol};
    }

    send(request.dump());
    subscriptions_.erase(channel + (symbol.empty() ? "" : ":" + symbol));
}

std::string KrakenFuturesWS::sign(const std::string& path, const std::string& nonce, const std::string& postData) {
    auto apiSecret = exchange_.getApiSecret();
    std::string message = path + nonce + postData;
    
    unsigned char hmac[32];
    HMAC(EVP_sha256(), apiSecret.c_str(), apiSecret.length(),
         reinterpret_cast<const unsigned char*>(message.c_str()), message.length(),
         hmac, nullptr);

    std::stringstream ss;
    for (int i = 0; i < 32; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hmac[i]);
    }
    return ss.str();
}

std::string KrakenFuturesWS::getMarketId(const std::string& symbol) {
    auto market = exchange_.market(symbol);
    return market.id;
}

std::string KrakenFuturesWS::getSymbol(const std::string& marketId) {
    for (const auto& market : exchange_.markets) {
        if (market.second.id == marketId) {
            return market.first;
        }
    }
    return marketId;
}

void KrakenFuturesWS::handleMessage(const std::string& message) {
    try {
        auto j = nlohmann::json::parse(message);

        // Handle authentication response
        if (j.contains("event") && j["event"] == "auth") {
            if (j["success"].get<bool>()) {
                authenticated_ = true;
                std::cout << "Successfully authenticated" << std::endl;
            } else {
                std::cerr << "Authentication failed: " << j["message"] << std::endl;
            }
            return;
        }

        // Handle subscription responses
        if (j.contains("event") && j["event"] == "subscribed") {
            std::cout << "Successfully subscribed to " << j["feed"] << std::endl;
            return;
        }

        // Handle data updates
        if (j.contains("feed")) {
            std::string feed = j["feed"];

            if (feed == "ticker") {
                handleTicker(j);
            } else if (feed == "trade") {
                handleTrade(j);
            } else if (feed == "book") {
                handleOrderBook(j);
            } else if (feed == "candles") {
                handleOHLCV(j);
            } else if (feed == "markPrice") {
                handleMarkPrice(j);
            } else if (feed == "fundingRate") {
                handleFundingRate(j);
            } else if (feed == "fills") {
                handleMyTrade(j);
            } else if (feed == "open_orders") {
                handleOrder(j);
            } else if (feed == "open_positions") {
                handlePosition(j);
            } else if (feed == "account_balances") {
                handleBalance(j);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling message: " << e.what() << std::endl;
    }
}

void KrakenFuturesWS::handleTicker(const nlohmann::json& data) {
    try {
        Ticker ticker;
        ticker.symbol = getSymbol(data["product_id"].get<std::string>());
        ticker.bid = std::stod(data["bid"].get<std::string>());
        ticker.ask = std::stod(data["ask"].get<std::string>());
        ticker.last = std::stod(data["last"].get<std::string>());
        ticker.volume = std::stod(data["volume24h"].get<std::string>());
        ticker.high = std::stod(data["high24h"].get<std::string>());
        ticker.low = std::stod(data["low24h"].get<std::string>());
        ticker.timestamp = data["time"].get<uint64_t>();

        exchange_.emitTicker(ticker);
    } catch (const std::exception& e) {
        std::cerr << "Error handling ticker: " << e.what() << std::endl;
    }
}

void KrakenFuturesWS::handleOrderBook(const nlohmann::json& data) {
    try {
        OrderBook orderBook;
        orderBook.symbol = getSymbol(data["product_id"].get<std::string>());
        orderBook.timestamp = data["time"].get<uint64_t>();

        if (data.contains("bids")) {
            for (const auto& bid : data["bids"]) {
                orderBook.bids.emplace_back(
                    std::stod(bid[0].get<std::string>()),
                    std::stod(bid[1].get<std::string>())
                );
            }
        }

        if (data.contains("asks")) {
            for (const auto& ask : data["asks"]) {
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

void KrakenFuturesWS::handleTrade(const nlohmann::json& data) {
    try {
        Trade trade;
        trade.symbol = getSymbol(data["product_id"].get<std::string>());
        trade.id = data["uid"].get<std::string>();
        trade.price = std::stod(data["price"].get<std::string>());
        trade.amount = std::stod(data["size"].get<std::string>());
        trade.cost = trade.price * trade.amount;
        trade.side = data["side"].get<std::string>();
        trade.timestamp = data["time"].get<uint64_t>();

        exchange_.emitTrade(trade);
    } catch (const std::exception& e) {
        std::cerr << "Error handling trade: " << e.what() << std::endl;
    }
}

void KrakenFuturesWS::handleOHLCV(const nlohmann::json& data) {
    try {
        OHLCV ohlcv;
        ohlcv.timestamp = data["time"].get<uint64_t>();
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

void KrakenFuturesWS::handleMarkPrice(const nlohmann::json& data) {
    try {
        MarkPrice markPrice;
        markPrice.symbol = getSymbol(data["product_id"].get<std::string>());
        markPrice.price = std::stod(data["markPrice"].get<std::string>());
        markPrice.timestamp = data["time"].get<uint64_t>();

        exchange_.emitMarkPrice(markPrice);
    } catch (const std::exception& e) {
        std::cerr << "Error handling mark price: " << e.what() << std::endl;
    }
}

void KrakenFuturesWS::handleFundingRate(const nlohmann::json& data) {
    try {
        FundingRate fundingRate;
        fundingRate.symbol = getSymbol(data["product_id"].get<std::string>());
        fundingRate.rate = std::stod(data["fundingRate"].get<std::string>());
        fundingRate.timestamp = data["time"].get<uint64_t>();
        fundingRate.nextTimestamp = data["nextFundingTime"].get<uint64_t>();

        exchange_.emitFundingRate(fundingRate);
    } catch (const std::exception& e) {
        std::cerr << "Error handling funding rate: " << e.what() << std::endl;
    }
}

void KrakenFuturesWS::handleBalance(const nlohmann::json& data) {
    try {
        for (const auto& [currency, value] : data["balances"].items()) {
            Balance balance;
            balance.currency = currency;
            balance.free = std::stod(value["available"].get<std::string>());
            balance.used = std::stod(value["initial_margin"].get<std::string>());
            balance.total = std::stod(value["total"].get<std::string>());
            balance.timestamp = data["time"].get<uint64_t>();

            exchange_.emitBalance(balance);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling balance: " << e.what() << std::endl;
    }
}

void KrakenFuturesWS::handleOrder(const nlohmann::json& data) {
    try {
        Order order;
        order.id = data["order_id"].get<std::string>();
        order.symbol = getSymbol(data["product_id"].get<std::string>());
        order.type = data["order_type"].get<std::string>();
        order.side = data["side"].get<std::string>();
        order.price = std::stod(data["limit_price"].get<std::string>());
        order.amount = std::stod(data["size"].get<std::string>());
        order.filled = std::stod(data["filled"].get<std::string>());
        order.remaining = order.amount - order.filled;
        order.status = data["status"].get<std::string>();
        order.timestamp = data["time"].get<uint64_t>();

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

void KrakenFuturesWS::handleMyTrade(const nlohmann::json& data) {
    try {
        Trade trade;
        trade.id = data["fill_id"].get<std::string>();
        trade.orderId = data["order_id"].get<std::string>();
        trade.symbol = getSymbol(data["product_id"].get<std::string>());
        trade.type = data["order_type"].get<std::string>();
        trade.side = data["side"].get<std::string>();
        trade.price = std::stod(data["price"].get<std::string>());
        trade.amount = std::stod(data["size"].get<std::string>());
        trade.cost = trade.price * trade.amount;
        trade.timestamp = data["time"].get<uint64_t>();

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

void KrakenFuturesWS::handlePosition(const nlohmann::json& data) {
    try {
        Position position;
        position.symbol = getSymbol(data["product_id"].get<std::string>());
        position.size = std::stod(data["size"].get<std::string>());
        position.side = data["side"].get<std::string>();
        position.entryPrice = std::stod(data["entry_price"].get<std::string>());
        position.markPrice = std::stod(data["mark_price"].get<std::string>());
        position.liquidationPrice = std::stod(data["liquidation_price"].get<std::string>());
        position.margin = std::stod(data["initial_margin"].get<std::string>());
        position.leverage = std::stod(data["leverage"].get<std::string>());
        position.unrealizedPnl = std::stod(data["unrealized_pnl"].get<std::string>());
        position.timestamp = data["time"].get<uint64_t>();

        exchange_.emitPosition(position);
    } catch (const std::exception& e) {
        std::cerr << "Error handling position: " << e.what() << std::endl;
    }
}

void KrakenFuturesWS::handleOrderResponse(const nlohmann::json& data) {
    try {
        if (data["status"] == "success") {
            std::cout << "Order operation successful: " << data["message"] << std::endl;
        } else {
            std::cerr << "Order operation failed: " << data["message"] << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling order response: " << e.what() << std::endl;
    }
}

} // namespace ccxt
