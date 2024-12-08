#include "ccxt/exchanges/ws/coinex_ws.h"
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

CoinexWS::CoinexWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Coinex& exchange)
    : WebSocketClient(ioc, ctx)
    , exchange_(exchange)
    , authenticated_(false)
    , requestId_(0) {
}

void CoinexWS::authenticate() {
    if (authenticated_) {
        return;
    }

    auto timestamp = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());

    std::string access_id = exchange_.apiKey;
    std::string secret_key = exchange_.secret;

    // Create signature string
    std::string signatureString = "access_id=" + access_id + "&timestamp=" + timestamp;
    std::string signature = exchange_.hmac(signatureString, secret_key, "sha256");

    nlohmann::json auth = {
        {"method", "server.sign"},
        {"params", {
            {"access_id", access_id},
            {"timestamp", timestamp},
            {"signature", signature}
        }},
        {"id", getNextRequestId()}
    };

    send(auth.dump());
}

void CoinexWS::subscribe(const std::string& channel, const std::string& symbol, bool isPrivate) {
    if (isPrivate && !authenticated_) {
        authenticate();
    }

    nlohmann::json request = {
        {"method", "state.subscribe"},
        {"params", {
            {"market", symbol},
            {"channel", channel}
        }},
        {"id", getNextRequestId()}
    };

    send(request.dump());
}

void CoinexWS::subscribeMultiple(const std::string& channel, const std::vector<std::string>& symbols, bool isPrivate) {
    if (isPrivate && !authenticated_) {
        authenticate();
    }

    nlohmann::json request = {
        {"method", "state.subscribe"},
        {"params", {
            {"markets", symbols},
            {"channel", channel}
        }},
        {"id", getNextRequestId()}
    };

    send(request.dump());
}

void CoinexWS::unsubscribe(const std::string& channel, const std::string& symbol) {
    nlohmann::json request = {
        {"method", "state.unsubscribe"},
        {"params", {
            {"market", symbol},
            {"channel", channel}
        }},
        {"id", getNextRequestId()}
    };

    send(request.dump());
}

void CoinexWS::unsubscribeAll() {
    for (const auto& subscription : subscriptions_) {
        unsubscribe(subscription.first, subscription.second);
    }
    subscriptions_.clear();
}

void CoinexWS::watchTicker(const std::string& symbol, const std::map<std::string, std::string>& params) {
    subscribe("state", symbol);
}

void CoinexWS::watchTickers(const std::vector<std::string>& symbols, const std::map<std::string, std::string>& params) {
    subscribeMultiple("state", symbols);
}

void CoinexWS::watchOrderBook(const std::string& symbol, const std::map<std::string, std::string>& params) {
    subscribe("depth", symbol);
}

void CoinexWS::watchOrderBookForSymbols(const std::vector<std::string>& symbols, const std::map<std::string, std::string>& params) {
    subscribeMultiple("depth", symbols);
}

void CoinexWS::watchTrades(const std::string& symbol, const std::map<std::string, std::string>& params) {
    subscribe("deals", symbol);
}

void CoinexWS::watchTradesForSymbols(const std::vector<std::string>& symbols, const std::map<std::string, std::string>& params) {
    subscribeMultiple("deals", symbols);
}

void CoinexWS::watchBidsAsks(const std::vector<std::string>& symbols, const std::map<std::string, std::string>& params) {
    subscribeMultiple("depth", symbols);
}

void CoinexWS::watchBalance(const std::map<std::string, std::string>& params) {
    subscribe("asset", "", true);
}

void CoinexWS::watchOrders(const std::string& symbol, const std::map<std::string, std::string>& params) {
    subscribe("order", symbol, true);
}

void CoinexWS::watchMyTrades(const std::string& symbol, const std::map<std::string, std::string>& params) {
    subscribe("deals", symbol, true);
}

void CoinexWS::handleMessage(const std::string& message) {
    auto data = nlohmann::json::parse(message);
    
    if (data.contains("error")) {
        handleErrorMessage(data);
        return;
    }

    std::string method = data["method"].get<std::string>();
    
    if (method == "state.update") {
        handleTickerMessage(data);
    } else if (method == "depth.update") {
        handleOrderBookMessage(data);
    } else if (method == "deals.update") {
        handleTradeMessage(data);
    } else if (method == "asset.update") {
        handleBalanceMessage(data);
    } else if (method == "order.update") {
        handleOrderMessage(data);
    } else if (method == "server.sign") {
        handleAuthenticationMessage(data);
    }
}

void CoinexWS::handleTickerMessage(const nlohmann::json& data) {
    auto stateList = data["data"]["state_list"];
    for (const auto& ticker : stateList) {
        std::string marketId = ticker["market"].get<std::string>();
        std::string symbol = getSymbol(marketId);
        
        Ticker parsedTicker = parseWsTicker(ticker);
        tickers_[symbol] = parsedTicker;
        
        std::string messageHash = "ticker:" + symbol;
        emit(messageHash, parsedTicker);
    }
}

void CoinexWS::handleOrderBookMessage(const nlohmann::json& data) {
    auto orderBookData = data["data"];
    std::string marketId = orderBookData["market"].get<std::string>();
    std::string symbol = getSymbol(marketId);
    
    OrderBook& orderbook = orderbooks_[symbol];
    orderbook.symbol = symbol;
    
    if (orderBookData.contains("bids")) {
        for (const auto& bid : orderBookData["bids"]) {
            orderbook.bids.push_back({
                std::stod(bid[0].get<std::string>()),
                std::stod(bid[1].get<std::string>())
            });
        }
    }
    
    if (orderBookData.contains("asks")) {
        for (const auto& ask : orderBookData["asks"]) {
            orderbook.asks.push_back({
                std::stod(ask[0].get<std::string>()),
                std::stod(ask[1].get<std::string>())
            });
        }
    }
    
    std::string messageHash = "orderbook:" + symbol;
    emit(messageHash, orderbook);
}

void CoinexWS::handleTradeMessage(const nlohmann::json& data) {
    auto tradeData = data["data"];
    std::string marketId = tradeData["market"].get<std::string>();
    std::string symbol = getSymbol(marketId);
    
    if (trades_.find(symbol) == trades_.end()) {
        trades_[symbol] = ArrayCache<Trade>(1000);
    }
    
    Trade parsedTrade = parseWsTrade(tradeData);
    trades_[symbol].push_back(parsedTrade);
    
    std::string messageHash = "trade:" + symbol;
    emit(messageHash, trades_[symbol]);
}

void CoinexWS::handleBalanceMessage(const nlohmann::json& data) {
    // Implementation for balance message handling
}

void CoinexWS::handleOrderMessage(const nlohmann::json& data) {
    // Implementation for order message handling
}

void CoinexWS::handleMyTradeMessage(const nlohmann::json& data) {
    // Implementation for user trade message handling
}

void CoinexWS::handleErrorMessage(const nlohmann::json& data) {
    int code = data["error"]["code"].get<int>();
    std::string message = data["error"]["message"].get<std::string>();
    
    switch (code) {
        case 20001:
        case 30001:
            throw BadRequest(message);
        case 21001:
        case 21002:
        case 31001:
        case 31002:
            throw AuthenticationError(message);
        case 23001:
        case 33001:
            throw RequestTimeout(message);
        case 23002:
        case 33002:
            throw RateLimitExceeded(message);
        case 24001:
        case 34001:
            throw ExchangeError(message);
        case 24002:
        case 34002:
            throw ExchangeNotAvailable(message);
        default:
            throw ExchangeError(message);
    }
}

void CoinexWS::handleAuthenticationMessage(const nlohmann::json& data) {
    if (!data.contains("error")) {
        authenticated_ = true;
    }
}

Ticker CoinexWS::parseWsTicker(const nlohmann::json& ticker, const Market* market) {
    Ticker result;
    result.symbol = getSymbol(ticker["market"].get<std::string>());
    result.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    result.datetime = exchange_.iso8601(result.timestamp);
    result.high = std::stod(ticker["high"].get<std::string>());
    result.low = std::stod(ticker["low"].get<std::string>());
    result.bid = std::stod(ticker["last"].get<std::string>());
    result.ask = std::stod(ticker["last"].get<std::string>());
    result.last = std::stod(ticker["last"].get<std::string>());
    result.close = std::stod(ticker["close"].get<std::string>());
    result.baseVolume = std::stod(ticker["volume"].get<std::string>());
    result.quoteVolume = std::stod(ticker["value"].get<std::string>());
    
    return result;
}

Trade CoinexWS::parseWsTrade(const nlohmann::json& trade, const Market* market) {
    Trade result;
    result.symbol = getSymbol(trade["market"].get<std::string>());
    result.id = trade["id"].get<std::string>();
    result.timestamp = std::stoll(trade["time"].get<std::string>()) * 1000;
    result.datetime = exchange_.iso8601(result.timestamp);
    result.side = trade["type"].get<std::string>();
    result.price = std::stod(trade["price"].get<std::string>());
    result.amount = std::stod(trade["amount"].get<std::string>());
    result.cost = result.price * result.amount;
    
    return result;
}

std::string CoinexWS::getEndpoint(const std::string& type) {
    return type == "spot" ? "wss://socket.coinex.com/v2/spot/" : "wss://socket.coinex.com/v2/futures/";
}

std::string CoinexWS::getMarketId(const std::string& symbol) {
    return exchange_.market_id(symbol);
}

std::string CoinexWS::getSymbol(const std::string& marketId) {
    return exchange_.market(marketId)["symbol"];
}

std::string CoinexWS::getChannel(const std::string& channel, const std::string& symbol) {
    return channel + (symbol.empty() ? "" : ":" + symbol);
}

int CoinexWS::getNextRequestId() {
    return ++requestId_;
}

std::map<std::string, std::string> CoinexWS::parseMarket(const std::string& marketId) {
    return exchange_.markets_by_id[marketId];
}

} // namespace ccxt
