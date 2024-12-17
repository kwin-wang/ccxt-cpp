#pragma once

#include "ccxt/base/exchange.h"
#include <future>

namespace ccxt {

class HuobiJP : public Exchange {
public:
    HuobiJP(const Config& config = Config());
    ~HuobiJP() override = default;

    static Exchange* create(const Config& config = Config()) {
        return new HuobiJP(config);
    }

    // Market Data API - Sync
    Json fetchMarketsImpl(const Json& params = Json::object()) const override;
    Json fetchTickerImpl(const std::string& symbol, const Json& params = Json::object()) const override;
    Json fetchTickersImpl(const std::vector<std::string>& symbols = {}, const Json& params = Json::object()) const override;
    Json fetchOrderBookImpl(const std::string& symbol, int limit = 0, const Json& params = Json::object()) const override;
    Json fetchTradesImpl(const std::string& symbol, int since = 0, int limit = 0, const Json& params = Json::object()) const override;
    Json fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe = "1m",
                        int since = 0, int limit = 0, const Json& params = Json::object()) const override;

    // Trading API - Sync
    Json fetchBalanceImpl(const Json& params = Json::object()) const override;
    Json createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side,
                        double amount, double price = 0, const Json& params = Json::object()) override;
    Json cancelOrderImpl(const std::string& id, const std::string& symbol = "", const Json& params = Json::object()) override;
    Json fetchOrderImpl(const std::string& id, const std::string& symbol = "", const Json& params = Json::object()) override;
    Json fetchOrdersImpl(const std::string& symbol = "", int since = 0, int limit = 0, const Json& params = Json::object()) override;
    Json fetchOpenOrdersImpl(const std::string& symbol = "", int since = 0, int limit = 0, const Json& params = Json::object()) override;
    Json fetchClosedOrdersImpl(const std::string& symbol = "", int since = 0, int limit = 0, const Json& params = Json::object()) override;

    // Market Data API - Async
    AsyncPullType fetchMarketsAsync(const Json& params = Json::object());
    AsyncPullType fetchTickerAsync(const std::string& symbol, const Json& params = Json::object());
    AsyncPullType fetchTickersAsync(const std::vector<std::string>& symbols = {}, const Json& params = Json::object());
    AsyncPullType fetchOrderBookAsync(const std::string& symbol, int limit = 0, const Json& params = Json::object());
    AsyncPullType fetchTradesAsync(const std::string& symbol, int since = 0, int limit = 0, const Json& params = Json::object());
    AsyncPullType fetchOHLCVAsync(const std::string& symbol, const std::string& timeframe = "1m",
                                     int since = 0, int limit = 0, const Json& params = Json::object());

    // Trading API - Async
    AsyncPullType fetchBalanceAsync(const Json& params = Json::object());
    AsyncPullType createOrderAsync(const std::string& symbol, const std::string& type, const std::string& side,
                                     double amount, double price = 0, const Json& params = Json::object());
    AsyncPullType cancelOrderAsync(const std::string& id, const std::string& symbol = "", const Json& params = Json::object());
    AsyncPullType fetchOrderAsync(const std::string& id, const std::string& symbol = "", const Json& params = Json::object());
    AsyncPullType fetchOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const Json& params = Json::object());
    AsyncPullType fetchOpenOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const Json& params = Json::object());
    AsyncPullType fetchClosedOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const Json& params = Json::object());

protected:
    void init() override;
    Json describeImpl() const override;

    String sign(const String& path, const String& api = "public",
               const String& method = "GET", const Json& params = Json::object(),
               const std::map<String, String>& headers = {}, const Json& body = nullptr) override;

private:
    void initializeApiEndpoints();
    String getTimestamp();
    String createSignature(const String& method, const String& host,
                         const String& path, const std::map<String, String>& params);
    String getAccountId();

    String accountId;
    std::map<String, String> timeframes;

    static const std::string defaultBaseURL;
    static const std::string defaultVersion;
    static const int defaultRateLimit;
    static const bool defaultPro;

    

    // Helper methods for parsing responses
    Json parseTicker(const Json& ticker, const Json& market = Json()) const;
    Json parseTrade(const Json& trade, const Json& market = Json()) const;
    Json parseOrder(const Json& order, const Json& market = Json()) const;
    Json parseTransaction(const Json& transaction, const Json& currency = Json()) const;

    // Authentication helpers
    std::string sign(const std::string& path, const std::string& api = "public", const std::string& method = "GET",
                    const Json& params = Json::object(), const Json& headers = Json::object(), const Json& body = Json::object()) const;

    // Error handling
    void handleErrors(const std::string& code, const std::string& reason, const std::string& url, const std::string& method,
                     const Json& headers, const Json& body, const Json& response, const std::string& requestHeaders,
                     const std::string& requestBody) const;
};

} // namespace ccxt
