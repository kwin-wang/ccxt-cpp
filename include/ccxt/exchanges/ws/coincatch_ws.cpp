#include "coincatch_ws.h"
#include "../../base/json.hpp"
#include "../../base/functions.h"
#include <chrono>
#include <sstream>
#include <iomanip>

namespace ccxt {

CoincatchWS::CoincatchWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Coincatch& exchange)
    : WebSocketClient(ioc, ctx)
    , exchange_(exchange) {
}

// Public API Methods
void CoincatchWS::watchTicker(const std::string& symbol, const std::map<std::string, std::string>& params) {
    std::string instType = getInstType(symbol);
    std::string instId = getInstId(symbol);
    subscribe("ticker", instId, instType);
}

void CoincatchWS::watchTickers(const std::vector<std::string>& symbols, const std::map<std::string, std::string>& params) {
    for (const auto& symbol : symbols) {
        watchTicker(symbol, params);
    }
}

void CoincatchWS::watchOrderBook(const std::string& symbol, int limit, const std::map<std::string, std::string>& params) {
    std::string instType = getInstType(symbol);
    std::string instId = getInstId(symbol);
    std::string channel = limit > 0 ? "books" + std::to_string(limit) : "books";
    subscribe(channel, instId, instType);
}

void CoincatchWS::watchOrderBookForSymbols(const std::vector<std::string>& symbols, int limit, const std::map<std::string, std::string>& params) {
    for (const auto& symbol : symbols) {
        watchOrderBook(symbol, limit, params);
    }
}

void CoincatchWS::watchTrades(const std::string& symbol, const std::map<std::string, std::string>& params) {
    std::string instType = getInstType(symbol);
    std::string instId = getInstId(symbol);
    subscribe("trade", instId, instType);
}

void CoincatchWS::watchTradesForSymbols(const std::vector<std::string>& symbols, const std::map<std::string, std::string>& params) {
    for (const auto& symbol : symbols) {
        watchTrades(symbol, params);
    }
}

void CoincatchWS::watchOHLCV(const std::string& symbol, const std::string& timeframe, const std::map<std::string, std::string>& params) {
    std::string instType = getInstType(symbol);
    std::string instId = getInstId(symbol);
    std::string channel = "candle" + timeframe;
    subscribe(channel, instId, instType);
}

// Private API Methods
void CoincatchWS::watchBalance(const std::map<std::string, std::string>& params) {
    authenticate();
    std::string instType = params.count("instType") ? params.at("instType") : "umcbl";
    subscribePrivate("account", instType);
}

void CoincatchWS::watchOrders(const std::string& symbol, const std::map<std::string, std::string>& params) {
    authenticate();
    std::string instType;
    std::string instId;
    if (!symbol.empty()) {
        instType = getInstType(symbol);
        instId = getInstId(symbol);
    } else {
        instType = params.count("instType") ? params.at("instType") : "umcbl";
        instId = "default";
    }
    std::string channel = params.count("trigger") && params.at("trigger") == "true" ? "ordersAlgo" : "orders";
    subscribePrivate(channel, instType);
}

void CoincatchWS::watchPositions(const std::vector<std::string>& symbols, const std::map<std::string, std::string>& params) {
    authenticate();
    std::vector<std::string> instTypes;
    if (!symbols.empty()) {
        for (const auto& symbol : symbols) {
            std::string instType = getInstType(symbol);
            if (std::find(instTypes.begin(), instTypes.end(), instType) == instTypes.end()) {
                instTypes.push_back(instType);
            }
        }
    } else {
        instTypes = {"umcbl", "dmcbl"};
    }
    for (const auto& instType : instTypes) {
        subscribePrivate("positions", instType);
    }
}

// Unsubscribe methods
void CoincatchWS::unwatchTicker(const std::string& symbol) {
    std::string instType = getInstType(symbol);
    std::string instId = getInstId(symbol);
    unsubscribe("ticker", instId, instType);
}

void CoincatchWS::unwatchOrderBook(const std::string& symbol) {
    std::string instType = getInstType(symbol);
    std::string instId = getInstId(symbol);
    unsubscribe("books", instId, instType);
}

void CoincatchWS::unwatchTrades(const std::string& symbol) {
    std::string instType = getInstType(symbol);
    std::string instId = getInstId(symbol);
    unsubscribe("trade", instId, instType);
}

void CoincatchWS::unwatchOHLCV(const std::string& symbol, const std::string& timeframe) {
    std::string instType = getInstType(symbol);
    std::string instId = getInstId(symbol);
    std::string channel = "candle" + timeframe;
    unsubscribe(channel, instId, instType);
}

// Private helper methods
std::string CoincatchWS::getEndpoint(const std::string& type) {
    if (type == "public") {
        return "wss://ws.coincatch.com/public/v1/stream";
    }
    return "wss://ws.coincatch.com/private/v1/stream";
}

void CoincatchWS::subscribe(const std::string& channel, const std::string& symbol, const std::string& instType) {
    nlohmann::json args = {
        {"instType", instType},
        {"channel", channel},
        {"instId", symbol}
    };
    nlohmann::json request = {
        {"op", "subscribe"},
        {"args", {args}}
    };
    send(request.dump());
}

void CoincatchWS::subscribePrivate(const std::string& channel, const std::string& instType) {
    nlohmann::json args = {
        {"instType", instType},
        {"channel", channel},
        {"instId", "default"}
    };
    nlohmann::json request = {
        {"op", "subscribe"},
        {"args", {args}}
    };
    send(request.dump());
}

void CoincatchWS::unsubscribe(const std::string& channel, const std::string& symbol, const std::string& instType) {
    nlohmann::json args = {
        {"instType", instType},
        {"channel", channel},
        {"instId", symbol}
    };
    nlohmann::json request = {
        {"op", "unsubscribe"},
        {"args", {args}}
    };
    send(request.dump());
}

void CoincatchWS::authenticate() {
    std::string timestamp = std::to_string(std::chrono::system_clock::now().time_since_epoch().count() / 1000);
    std::string auth = timestamp + "GET/user/verify";
    std::string signature = exchange_.hmac(auth, exchange_.secret, "sha256", true);
    
    nlohmann::json args = {
        {"apiKey", exchange_.apiKey},
        {"passphrase", exchange_.password},
        {"timestamp", timestamp},
        {"sign", signature}
    };
    nlohmann::json request = {
        {"op", "login"},
        {"args", {args}}
    };
    send(request.dump());
}

std::string CoincatchWS::getInstType(const std::string& symbol) {
    auto market = parseMarket(symbol);
    if (market["type"] == "spot") {
        return "spbl";
    }
    return market["settle"] == "USDT" ? "umcbl" : "dmcbl";
}

std::string CoincatchWS::getInstId(const std::string& symbol) {
    return parseMarketId(symbol);
}

// Message handlers
void CoincatchWS::handleMessage(const std::string& message) {
    auto data = nlohmann::json::parse(message);
    
    if (data.contains("event")) {
        std::string event = data["event"];
        if (event == "error") {
            handleErrorMessage(data);
        } else if (event == "login") {
            handleAuthenticate(data);
        } else if (event == "subscribe") {
            handleSubscriptionStatus(data);
        } else if (event == "unsubscribe") {
            handleUnsubscriptionStatus(data);
        }
        return;
    }

    if (message == "pong" || data["message"] == "pong") {
        handlePong(data);
        return;
    }

    auto arg = data["arg"];
    std::string channel = arg["channel"];
    
    if (channel == "ticker") {
        handleTickerMessage(data);
    } else if (channel.find("books") != std::string::npos) {
        handleOrderBookMessage(data);
    } else if (channel == "trade") {
        handleTradeMessage(data);
    } else if (channel.find("candle") != std::string::npos) {
        handleOHLCVMessage(data);
    } else if (channel == "account") {
        handleBalanceMessage(data);
    } else if (channel == "orders" || channel == "ordersAlgo") {
        handleOrderMessage(data);
    } else if (channel == "positions") {
        handlePositionMessage(data);
    }
}

void CoincatchWS::handleTickerMessage(const nlohmann::json& data) {
    // Implementation of ticker message handling
}

void CoincatchWS::handleOrderBookMessage(const nlohmann::json& data) {
    // Implementation of orderbook message handling
}

void CoincatchWS::handleTradeMessage(const nlohmann::json& data) {
    // Implementation of trade message handling
}

void CoincatchWS::handleOHLCVMessage(const nlohmann::json& data) {
    // Implementation of OHLCV message handling
}

void CoincatchWS::handleBalanceMessage(const nlohmann::json& data) {
    // Implementation of balance message handling
}

void CoincatchWS::handleOrderMessage(const nlohmann::json& data) {
    // Implementation of order message handling
}

void CoincatchWS::handlePositionMessage(const nlohmann::json& data) {
    // Implementation of position message handling
}

void CoincatchWS::handleErrorMessage(const nlohmann::json& data) {
    // Implementation of error message handling
}

void CoincatchWS::handlePong(const nlohmann::json& data) {
    // Implementation of pong message handling
}

void CoincatchWS::handleSubscriptionStatus(const nlohmann::json& data) {
    // Implementation of subscription status handling
}

void CoincatchWS::handleUnsubscriptionStatus(const nlohmann::json& data) {
    // Implementation of unsubscription status handling
}

void CoincatchWS::handleAuthenticate(const nlohmann::json& data) {
    // Implementation of authentication response handling
}

// Market parsing methods
std::string CoincatchWS::parseMarketId(const std::string& symbol) {
    auto parts = split(symbol, '/');
    std::string base = parts[0];
    std::string quote = split(parts[1], ':')[0];
    std::string settle = parts[1].find(':') != std::string::npos ? split(parts[1], ':')[1] : "";
    
    std::string suffix;
    if (settle.empty()) {
        suffix = "_SPBL";
    } else if (settle == "USDT") {
        suffix = "_UMCBL";
    } else {
        suffix = "_DMCBL";
    }
    
    return base + quote + suffix;
}

std::string CoincatchWS::parseSymbol(const std::string& marketId) {
    auto parts = split(marketId, '_');
    if (parts.size() < 2) return "";
    
    std::string baseQuote = parts[0];
    std::string suffix = parts[1];
    
    std::string base;
    std::string quote;
    if (baseQuote.find("USDT") != std::string::npos) {
        base = baseQuote.substr(0, baseQuote.find("USDT"));
        quote = "USDT";
    } else if (baseQuote.find("USD") != std::string::npos) {
        base = baseQuote.substr(0, baseQuote.find("USD"));
        quote = "USD";
    } else {
        return "";
    }
    
    std::string symbol = base + "/" + quote;
    if (suffix == "UMCBL") {
        symbol += ":USDT";
    } else if (suffix == "DMCBL") {
        symbol += ":USD";
    }
    
    return symbol;
}

std::map<std::string, std::string> CoincatchWS::parseMarket(const std::string& marketId) {
    std::map<std::string, std::string> result;
    
    auto parts = split(marketId, '_');
    if (parts.size() < 2) return result;
    
    std::string baseQuote = parts[0];
    std::string suffix = parts[1];
    
    std::string base;
    std::string quote;
    if (baseQuote.find("USDT") != std::string::npos) {
        base = baseQuote.substr(0, baseQuote.find("USDT"));
        quote = "USDT";
    } else if (baseQuote.find("USD") != std::string::npos) {
        base = baseQuote.substr(0, baseQuote.find("USD"));
        quote = "USD";
    } else {
        return result;
    }
    
    result["id"] = marketId;
    result["base"] = base;
    result["quote"] = quote;
    
    if (suffix == "SPBL") {
        result["type"] = "spot";
        result["settle"] = "";
    } else if (suffix == "UMCBL") {
        result["type"] = "swap";
        result["settle"] = "USDT";
    } else if (suffix == "DMCBL") {
        result["type"] = "swap";
        result["settle"] = "USD";
    }
    
    return result;
}

} // namespace ccxt
