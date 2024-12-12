#ifndef CCXT_EXCHANGE_BL3P_H
#define CCXT_EXCHANGE_BL3P_H

#include "ccxt/base/exchange.h"
#include <boost/asio.hpp>
#include <boost/future.hpp>

namespace ccxt {

class bl3p : public Exchange {
public:
    bl3p(const Config& config = Config());
    ~bl3p() = default;

    static Exchange* create(const Config& config = Config()) {
        return new bl3p(config);
    }

    // Market Data
    Json fetchTickerImpl(const std::string& symbol) const override;
    Json fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchTradesImpl(const std::string& symbol, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchTradingFeesImpl() const override;

    // Async Market Data
    boost::future<Json> fetchTickerAsync(const std::string& symbol) const;
    boost::future<Json> fetchOrderBookAsync(const std::string& symbol, const std::optional<int>& limit = std::nullopt) const;
    boost::future<Json> fetchTradesAsync(const std::string& symbol, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const;
    boost::future<Json> fetchTradingFeesAsync() const;

    // Trading
    Json createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side, double amount, const std::optional<double>& price = std::nullopt) override;
    Json cancelOrderImpl(const std::string& id, const std::string& symbol) override;
    Json createDepositAddressImpl(const std::string& code, const std::optional<std::string>& network = std::nullopt) override;

    // Async Trading
    boost::future<Json> createOrderAsync(const std::string& symbol, const std::string& type, const std::string& side, double amount, const std::optional<double>& price = std::nullopt);
    boost::future<Json> cancelOrderAsync(const std::string& id, const std::string& symbol);
    boost::future<Json> createDepositAddressAsync(const std::string& code, const std::optional<std::string>& network = std::nullopt);

    // Account
    Json fetchBalanceImpl() const override;

    // Async Account
    boost::future<Json> fetchBalanceAsync() const;

protected:
    void init() override;
    Json describeImpl() const override;

private:
    static Exchange* createInstance(const Config& config) {
        return new bl3p(config);
    }

    static const std::string defaultBaseURL;
    static const std::string defaultVersion;
    static const int defaultRateLimit;
    static const bool defaultPro;

    

    // Helper methods for parsing responses
    Json parseTicker(const Json& ticker, const Json& market = Json()) const;
    Json parseTrade(const Json& trade, const Json& market = Json()) const;
    Json parseOrder(const Json& order, const Json& market = Json()) const;
    Json parseFees(const Json& response) const;

    // Authentication helpers
    std::string sign(const std::string& path, const std::string& api = "public", const std::string& method = "GET",
                    const Json& params = Json::object(), const Json& headers = Json::object(), const Json& body = Json::object()) const;
};

} // namespace ccxt

#endif // CCXT_EXCHANGE_BL3P_H
