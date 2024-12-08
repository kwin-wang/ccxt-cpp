#include "../include/ccxt/websocket_client.h"

namespace ccxt {

WebSocketClient::WebSocketClient(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx)
    : ws_(ioc, ctx), resolver_(ioc) {}

WebSocketClient::~WebSocketClient() {
    close();
}

void WebSocketClient::connect(const std::string& host, const std::string& port, const std::string& path) {
    auto self(shared_from_this());
    resolver_.async_resolve(host, port,
        [this, self](boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type results) {
            if (!ec) {
                onResolve(ec, results);
            }
        });
}

void WebSocketClient::onResolve(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type results) {
    if (ec) return;
    auto self(shared_from_this());
    boost::asio::async_connect(ws_.next_layer().next_layer(), results.begin(), results.end(),
        [this, self](boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type::endpoint_type endpoint) {
            if (!ec) {
                onConnect(ec, endpoint);
            }
        });
}

void WebSocketClient::onConnect(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type::endpoint_type endpoint) {
    if (ec) return;
    auto self(shared_from_this());
    ws_.next_layer().async_handshake(boost::asio::ssl::stream_base::client,
        [this, self](boost::beast::error_code ec) {
            if (!ec) {
                onHandshake(ec);
            }
        });
}

void WebSocketClient::onHandshake(boost::beast::error_code ec) {
    if (ec) return;
    auto self(shared_from_this());
    ws_.async_handshake(ws_.next_layer().next_layer().remote_endpoint().address().to_string(), "",
        [this, self](boost::beast::error_code ec) {
            if (!ec) {
                ws_.async_read(buffer_,
                    [this, self](boost::beast::error_code ec, std::size_t bytes_transferred) {
                        onRead(ec, bytes_transferred);
                    });
            }
        });
}

void WebSocketClient::send(const std::string& message) {
    auto self(shared_from_this());
    ws_.async_write(boost::asio::buffer(message),
        [this, self](boost::beast::error_code ec, std::size_t bytes_transferred) {
            onWrite(ec, bytes_transferred);
        });
}

void WebSocketClient::onWrite(boost::beast::error_code ec, std::size_t bytes_transferred) {
    if (ec) return;
    // Handle write completion
}

void WebSocketClient::onRead(boost::beast::error_code ec, std::size_t bytes_transferred) {
    if (ec) return;
    if (messageHandler_) {
        messageHandler_(boost::beast::buffers_to_string(buffer_.data()));
    }
    buffer_.consume(bytes_transferred);
    auto self(shared_from_this());
    ws_.async_read(buffer_,
        [this, self](boost::beast::error_code ec, std::size_t bytes_transferred) {
            onRead(ec, bytes_transferred);
        });
}

void WebSocketClient::close() {
    auto self(shared_from_this());
    ws_.async_close(boost::beast::websocket::close_code::normal,
        [this, self](boost::beast::error_code ec) {
            onClose(ec);
        });
}

void WebSocketClient::onClose(boost::beast::error_code ec) {
    if (ec) return;
    // Handle close completion
}

void WebSocketClient::setMessageHandler(MessageHandler handler) {
    messageHandler_ = handler;
}

} // namespace ccxt
