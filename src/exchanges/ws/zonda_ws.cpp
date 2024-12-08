#include "../../include/ccxt/exchanges/zonda_ws.h"

namespace ccxt {

ZondaWS::ZondaWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Zonda& exchange)
    : WebSocketClient(ioc, ctx), exchange_(exchange) {
    setMessageHandler(std::bind(&ZondaWS::handleMessage, this, std::placeholders::_1));
}

void ZondaWS::subscribeTicker(const std::string& symbol) {
    std::string message = "{\"action\": \"subscribe\", \"channel\": \"ticker\", \"symbol\": \"" + symbol + "\"}";
    send(message);
}

void ZondaWS::subscribeOrderBook(const std::string& symbol) {
    std::string message = "{\"action\": \"subscribe\", \"channel\": \"orderBook\", \"symbol\": \"" + symbol + "\"}";
    send(message);
}

void ZondaWS::subscribeTrades(const std::string& symbol) {
    std::string message = "{\"action\": \"subscribe\", \"channel\": \"trades\", \"symbol\": \"" + symbol + "\"}";
    send(message);
}

void ZondaWS::handleMessage(const std::string& message) {
    // Parse the JSON message
    auto jsonMessage = nlohmann::json::parse(message);

    // Determine the type of message
    std::string channel = jsonMessage["channel"];

    if (channel == "ticker") {
        // Handle ticker updates
        auto ticker = jsonMessage["data"];
        std::cout << "Ticker update: " << ticker.dump() << std::endl;
        // Update exchange's ticker data
        exchange_.updateTicker(ticker);
    } else if (channel == "orderBook") {
        // Handle order book updates
        auto orderBook = jsonMessage["data"];
        std::cout << "Order book update: " << orderBook.dump() << std::endl;
        // Update exchange's order book data
        exchange_.updateOrderBook(orderBook);
    } else if (channel == "trades") {
        // Handle trade updates
        auto trades = jsonMessage["data"];
        std::cout << "Trade update: " << trades.dump() << std::endl;
        // Update exchange's trade data
        exchange_.updateTrades(trades);
    } else {
        std::cout << "Unknown channel: " << channel << std::endl;
    }
}

} // namespace ccxt
