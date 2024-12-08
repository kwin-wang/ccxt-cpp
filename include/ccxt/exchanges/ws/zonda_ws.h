#ifndef CCXT_EXCHANGE_ZONDA_WS_H
#define CCXT_EXCHANGE_ZONDA_WS_H

#include "../websocket_client.h"
#include "zonda.h"

namespace ccxt {

class ZondaWS : public WebSocketClient {
public:
    ZondaWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Zonda& exchange);
    ~ZondaWS() = default;

    void subscribeTicker(const std::string& symbol);
    void subscribeOrderBook(const std::string& symbol);
    void subscribeTrades(const std::string& symbol);

private:
    Zonda& exchange_;

    void handleMessage(const std::string& message);
};

} // namespace ccxt

#endif // CCXT_EXCHANGE_ZONDA_WS_H
