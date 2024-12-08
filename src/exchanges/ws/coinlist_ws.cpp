#include "ccxt/exchanges/ws/coinlist_ws.h"
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

CoinlistWS::CoinlistWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Coinlist& exchange)
    : WebSocketClient(ioc, ctx)
    , exchange_(exchange) {
}

void CoinlistWS::throwNotSupported() {
    throw NotSupported("Coinlist does not support WebSocket API. Please use REST API instead.");
}

void CoinlistWS::watchTicker(const std::string& symbol, const std::map<std::string, std::string>& params) {
    throwNotSupported();
}

void CoinlistWS::watchTickers(const std::vector<std::string>& symbols, const std::map<std::string, std::string>& params) {
    throwNotSupported();
}

void CoinlistWS::watchOrderBook(const std::string& symbol, const std::map<std::string, std::string>& params) {
    throwNotSupported();
}

void CoinlistWS::watchTrades(const std::string& symbol, const std::map<std::string, std::string>& params) {
    throwNotSupported();
}

void CoinlistWS::watchOHLCV(const std::string& symbol, const std::string& timeframe, const std::map<std::string, std::string>& params) {
    throwNotSupported();
}

void CoinlistWS::watchBalance(const std::map<std::string, std::string>& params) {
    throwNotSupported();
}

void CoinlistWS::watchOrders(const std::string& symbol, const std::map<std::string, std::string>& params) {
    throwNotSupported();
}

void CoinlistWS::watchMyTrades(const std::string& symbol, const std::map<std::string, std::string>& params) {
    throwNotSupported();
}

} // namespace ccxt
