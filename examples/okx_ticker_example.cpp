#include "ccxt/exchanges/okx.h"  
#include "ccxt/exchanges/ws/okx_ws.h"  
#include <boost/asio.hpp>  
#include <boost/asio/ssl.hpp>  
#include <iostream>  
  
int main() {  
    try {  
        // 创建IO上下文和SSL上下文  
        boost::asio::io_context ioc;  
        boost::asio::ssl::context ctx{boost::asio::ssl::context::tlsv12_client};  
          
        // 创建OKX交易所实例  
        ccxt::OKX okx;  
          
        // 创建WebSocket客户端  
        ccxt::OKXWS ws(ioc, ctx, okx);  
          
        // 设置ticker事件处理器  
        ws.on("ticker", [](const std::string& symbol, const nlohmann::json& ticker) {  
            std::cout << "=== Ticker Update ===" << std::endl;  
            std::cout << "Symbol: " << ticker["symbol"] << std::endl;  
            std::cout << "Last Price: " << ticker["last"] << std::endl;  
            std::cout << "Bid: " << ticker["bid"] << " @ " << ticker["bidVolume"] << std::endl;  
            std::cout << "Ask: " << ticker["ask"] << " @ " << ticker["askVolume"] << std::endl;  
            std::cout << "24h High: " << ticker["high"] << std::endl;  
            std::cout << "24h Low: " << ticker["low"] << std::endl;  
            std::cout << "24h Volume: " << ticker["baseVolume"] << std::endl;  
            std::cout << "24h Change: " << ticker["change"] << " ("   
                      << ticker["percentage"] << "%)" << std::endl;  
            std::cout << "Timestamp: " << ticker["datetime"] << std::endl;  
            std::cout << "========================" << std::endl;  
        });  
          
        // 连接WebSocket  
        ws.connect();  
          
        // 订阅BTC/USDT ticker数据  
        ws.watchTicker("BTC-USDT");  
          
        // 运行事件循环  
        ioc.run();  
          
    } catch (const std::exception& e) {  
        std::cerr << "Error: " << e.what() << std::endl;  
        return 1;  
    }  
      
    return 0;  
}