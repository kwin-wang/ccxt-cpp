#ifndef CCXT_EXCHANGE_VERTEX_WS_H
#define CCXT_EXCHANGE_VERTEX_WS_H

#include "../websocket_client.h"
#include "vertex.h"

namespace ccxt {

class VertexWS : public WebSocketClient {
public:
    VertexWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Vertex& exchange);
    ~VertexWS() = default;

    void subscribeTicker(const std::string& symbol);
    void subscribeOrderBook(const std::string& symbol);
    void subscribeTrades(const std::string& symbol);

private:
    Vertex& exchange_;

    void handleMessage(const std::string& message);
};

} // namespace ccxt

#endif // CCXT_EXCHANGE_VERTEX_WS_H
