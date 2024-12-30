#ifndef CCXT_WEBSOCKET_CLIENT_H
#define CCXT_WEBSOCKET_CLIENT_H

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <string>
#include <functional>
#include <memory>

namespace ccxt {

class WebSocketClient : public std::enable_shared_from_this<WebSocketClient> {
public:
    using MessageHandler = std::function<void(const std::string&)>;

    WebSocketClient(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx);
    ~WebSocketClient();

    void connect(const std::string& host, const std::string& port, const std::string& path);
    void send(const std::string& message);
    void close();

    void setMessageHandler(MessageHandler handler);
protected:
    virtual void handleMessage(const std::string& message) {}
private:
    void onResolve(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type results);
    void onConnect(const boost::system::error_code& ec, const boost::asio::ip::tcp::endpoint& endpoint);
    void onHandshake(boost::beast::error_code ec);
    void onWrite(boost::beast::error_code ec, std::size_t bytes_transferred);
    void onRead(boost::beast::error_code ec, std::size_t bytes_transferred);
    void onClose(boost::beast::error_code ec);

    boost::beast::websocket::stream<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>> ws_;
    boost::beast::flat_buffer buffer_;
    boost::asio::ip::tcp::resolver resolver_;
    MessageHandler messageHandler_;
};

} // namespace ccxt

#endif // CCXT_WEBSOCKET_CLIENT_H
