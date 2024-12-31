#ifndef CCXT_BITBAY_H
#define CCXT_BITBAY_H

#include "zonda.h"
#include <boost/thread/future.hpp>

namespace ccxt {

class BitBay : public Zonda {
public:
    explicit BitBay(const Config& config = Config());
    ~BitBay() override = default;

    // Async Market Data API
    boost::future<json> fetchMarketsAsync(const json& params = json::object());
    boost::future<json> fetchTickerAsync(const std::string& symbol, const json& params = json::object());
    boost::future<json> fetchOrderBookAsync(const std::string& symbol, int limit = 0, const json& params = json::object());
    boost::future<json> fetchTradesAsync(const std::string& symbol, int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchOHLCVAsync(const std::string& symbol, const std::string& timeframe = "1m",
                                      int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchTradingFeesAsync(const json& params = json::object());

    // Async Trading API
    boost::future<json> fetchBalanceAsync(const json& params = json::object());
    boost::future<json> createOrderAsync(const std::string& symbol, const std::string& type, const std::string& side,
                                      double amount, double price = 0, const json& params = json::object());
    boost::future<json> cancelOrderAsync(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    boost::future<json> fetchOrderAsync(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    boost::future<json> fetchOpenOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchMyTradesAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());

    // Async Account API
    boost::future<json> fetchDepositAddressAsync(const std::string& code, const json& params = json::object());
    boost::future<json> withdrawAsync(const std::string& code, double amount, const std::string& address,
                                   const std::string& tag = "", const json& params = json::object());

protected:
    json describe() override {
        return this->deepExtend(Zonda::describe(), {
            {"id", "bitbay"},
            {"name", "BitBay"},
            {"alias", true}
        });
    }
};

} // namespace ccxt

#endif // CCXT_BITBAY_H
