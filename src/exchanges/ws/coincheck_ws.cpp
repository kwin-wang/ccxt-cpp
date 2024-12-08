#include "ccxt/exchanges/ws/coincheck_ws.h"
#include "ccxt/error.h"
#include "ccxt/json.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <chrono>
#include <ctime>
#include <sstream>

namespace ccxt {

CoincheckWS::CoincheckWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Coincheck& exchange)
    : WebSocketClient(ioc, ctx)
    , exchange_(exchange) {
}

void CoincheckWS::subscribe(const std::string& channel, const std::string& symbol) {
    nlohmann::json request = {
        {"type", "subscribe"},
        {"channel", getMarketId(symbol) + "-" + channel}
    };

    send(request.dump());
}

void CoincheckWS::unsubscribe(const std::string& channel, const std::string& symbol) {
    nlohmann::json request = {
        {"type", "unsubscribe"},
        {"channel", getMarketId(symbol) + "-" + channel}
    };

    send(request.dump());
}

void CoincheckWS::unsubscribeAll() {
    for (const auto& subscription : subscriptions_) {
        unsubscribe(subscription.first, subscription.second);
    }
    subscriptions_.clear();
}

void CoincheckWS::watchOrderBook(const std::string& symbol, const std::map<std::string, std::string>& params) {
    subscribe("orderbook", symbol);
}

void CoincheckWS::watchTrades(const std::string& symbol, const std::map<std::string, std::string>& params) {
    subscribe("trades", symbol);
}

void CoincheckWS::handleMessage(const std::string& message) {
    auto data = nlohmann::json::parse(message);
    
    if (!data.is_array() || data.empty()) {
        return;
    }

    auto firstElement = data[0];
    if (firstElement.is_array()) {
        handleTradeMessage(data);
    } else {
        handleOrderBookMessage(data);
    }
}

void CoincheckWS::handleOrderBookMessage(const nlohmann::json& data) {
    //
    //     [
    //         "btc_jpy",
    //         {
    //             "bids": [
    //                 [
    //                     "6288279.0",
    //                     "0"
    //                 ]
    //             ],
    //             "asks": [
    //                 [
    //                     "6290314.0",
    //                     "0"
    //                 ]
    //             ],
    //             "last_update_at": "1705396097"
    //         }
    //     ]
    //
    std::string marketId = data[0].get<std::string>();
    std::string symbol = getSymbol(marketId);
    auto orderBookData = data[1];
    
    long long timestamp = orderBookData["last_update_at"].get<long long>();
    
    OrderBook orderbook;
    orderbook.symbol = symbol;
    orderbook.timestamp = timestamp * 1000; // convert to milliseconds
    orderbook.datetime = exchange_.iso8601(orderbook.timestamp);
    
    for (const auto& bid : orderBookData["bids"]) {
        orderbook.bids.push_back({
            std::stod(bid[0].get<std::string>()),
            std::stod(bid[1].get<std::string>())
        });
    }
    
    for (const auto& ask : orderBookData["asks"]) {
        orderbook.asks.push_back({
            std::stod(ask[0].get<std::string>()),
            std::stod(ask[1].get<std::string>())
        });
    }
    
    orderbooks_[symbol] = orderbook;
    std::string messageHash = "orderbook:" + symbol;
    emit(messageHash, orderbook);
}

void CoincheckWS::handleTradeMessage(const nlohmann::json& data) {
    //
    //     [
    //         [
    //             "1663318663",  # transaction timestamp(unix time)
    //             "2357062",  # transaction ID
    //             "btc_jpy",  # pair
    //             "2820896.0",  # transaction rate
    //             "5.0",  # transaction amount
    //             "sell",  # order side
    //             "1193401",  # ID of the Taker
    //             "2078767"  # ID of the Maker
    //         ]
    //     ]
    //
    for (const auto& trade : data) {
        std::string marketId = trade[2].get<std::string>();
        std::string symbol = getSymbol(marketId);
        
        if (trades_.find(symbol) == trades_.end()) {
            trades_[symbol] = ArrayCache<Trade>(1000); // tradesLimit from options
        }
        
        Trade parsedTrade = parseWsTrade(trade);
        trades_[symbol].push_back(parsedTrade);
        
        std::string messageHash = "trade:" + symbol;
        emit(messageHash, trades_[symbol]);
    }
}

Trade CoincheckWS::parseWsTrade(const nlohmann::json& trade, const Market* market) {
    //
    //     [
    //         "1663318663",  # transaction timestamp(unix time)
    //         "2357062",  # transaction ID
    //         "btc_jpy",  # pair
    //         "2820896.0",  # transaction rate
    //         "5.0",  # transaction amount
    //         "sell",  # order side
    //         "1193401",  # ID of the Taker
    //         "2078767"  # ID of the Maker
    //     ]
    //
    long long timestamp = std::stoll(trade[0].get<std::string>()) * 1000; // convert to milliseconds
    std::string marketId = trade[2].get<std::string>();
    std::string symbol = getSymbol(marketId);
    
    Trade result;
    result.id = trade[1].get<std::string>();
    result.timestamp = timestamp;
    result.datetime = exchange_.iso8601(timestamp);
    result.symbol = symbol;
    result.price = std::stod(trade[3].get<std::string>());
    result.amount = std::stod(trade[4].get<std::string>());
    result.side = trade[5].get<std::string>();
    result.cost = result.price * result.amount;
    
    return result;
}

void CoincheckWS::handleErrorMessage(const nlohmann::json& data) {
    if (data.contains("message")) {
        throw ExchangeError(data["message"].get<std::string>());
    }
}

void CoincheckWS::handleSubscriptionMessage(const nlohmann::json& data) {
    auto channel = data["channel"].get<std::string>();
    if (data.contains("market")) {
        subscriptions_[channel] = data["market"].get<std::string>();
    } else {
        subscriptions_[channel] = "";
    }
}

void CoincheckWS::handleUnsubscriptionMessage(const nlohmann::json& data) {
    auto channel = data["channel"].get<std::string>();
    subscriptions_.erase(channel);
}

std::string CoincheckWS::getEndpoint() {
    return "wss://ws-api.coincheck.com/";
}

std::string CoincheckWS::getMarketId(const std::string& symbol) {
    return exchange_.market_id(symbol);
}

std::string CoincheckWS::getSymbol(const std::string& marketId) {
    return exchange_.market(marketId)["symbol"];
}

std::string CoincheckWS::getChannel(const std::string& channel, const std::string& symbol) {
    return channel + (symbol.empty() ? "" : ":" + symbol);
}

std::map<std::string, std::string> CoincheckWS::parseMarket(const std::string& marketId) {
    return exchange_.markets_by_id[marketId];
}

} // namespace ccxt
