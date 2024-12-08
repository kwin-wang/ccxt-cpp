#include "ccxt/exchanges/ws/coinmetro_ws.h"
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

CoinmetroWS::CoinmetroWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Coinmetro& exchange)
    : WebSocketClient(ioc, ctx)
    , exchange_(exchange) {
}

void CoinmetroWS::throwNotSupported() {
    throw NotSupported("Coinmetro does not support WebSocket API. Please use REST API instead.");
}

void CoinmetroWS::watchTicker(const std::string& symbol, const std::map<std::string, std::string>& params) {
    throwNotSupported();
}

void CoinmetroWS::watchTickers(const std::vector<std::string>& symbols, const std::map<std::string, std::string>& params) {
    throwNotSupported();
}

void CoinmetroWS::watchOrderBook(const std::string& symbol, const std::map<std::string, std::string>& params) {
    throwNotSupported();
}

void CoinmetroWS::watchTrades(const std::string& symbol, const std::map<std::string, std::string>& params) {
    throwNotSupported();
}

void CoinmetroWS::watchOHLCV(const std::string& symbol, const std::string& timeframe, const std::map<std::string, std::string>& params) {
    throwNotSupported();
}

void CoinmetroWS::watchBalance(const std::map<std::string, std::string>& params) {
    throwNotSupported();
}

void CoinmetroWS::watchOrders(const std::string& symbol, const std::map<std::string, std::string>& params) {
    throwNotSupported();
}

void CoinmetroWS::watchMyTrades(const std::string& symbol, const std::map<std::string, std::string>& params) {
    throwNotSupported();
}

} // namespace ccxt
