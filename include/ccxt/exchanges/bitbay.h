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
    boost::future<json> fetchTickerAsync(const String& symbol, const json& params = json::object());
    boost::future<json> fetchOrderBookAsync(const String& symbol, int limit = 0, const json& params = json::object());
    boost::future<json> fetchTradesAsync(const String& symbol, int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchOHLCVAsync(const String& symbol, const String& timeframe = "1m",
                                      int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchTradingFeesAsync(const json& params = json::object());

    // Async Trading API
    boost::future<json> fetchBalanceAsync(const json& params = json::object());
    boost::future<json> createOrderAsync(const String& symbol, const String& type, const String& side,
                                      double amount, double price = 0, const json& params = json::object());
    boost::future<json> cancelOrderAsync(const String& id, const String& symbol = "", const json& params = json::object());
    boost::future<json> fetchOrderAsync(const String& id, const String& symbol = "", const json& params = json::object());
    boost::future<json> fetchOpenOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchMyTradesAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());

    // Async Account API
    boost::future<json> fetchDepositAddressAsync(const String& code, const json& params = json::object());
    boost::future<json> withdrawAsync(const String& code, double amount, const String& address,
                                   const String& tag = "", const json& params = json::object());

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
