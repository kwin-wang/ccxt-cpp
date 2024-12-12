#ifndef CCXT_EXCHANGE_BITHUMB_H
#define CCXT_EXCHANGE_BITHUMB_H

#include "ccxt/base/exchange.h"
#include <boost/asio.hpp>
#include <boost/thread/future.hpp>

namespace ccxt {

class bithumb : public Exchange {
public:
    bithumb(const Config& config = Config());
    ~bithumb() = default;

    // Market Data Methods
    Json fetchMarketsImpl() const override;
    Json fetchTickerImpl(const std::string& symbol) const override;
    Json fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchTradesImpl(const std::string& symbol, const std::optional<long long>& since = std::nullopt, 
                        const std::optional<int>& limit = std::nullopt) const override;
    Json fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe = "1m",
                       const std::optional<long long>& since = std::nullopt,
                       const std::optional<int>& limit = std::nullopt) const override;

    // Trading Methods
    Json createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side,
                        double amount, const std::optional<double>& price = std::nullopt) override;
    Json cancelOrderImpl(const std::string& id, const std::string& symbol) override;
    Json fetchOrderImpl(const std::string& id, const std::string& symbol) const override;
    Json fetchOrdersImpl(const std::string& symbol, const std::optional<long long>& since = std::nullopt,
                        const std::optional<int>& limit = std::nullopt) const override;
    Json fetchOpenOrdersImpl(const std::string& symbol, const std::optional<long long>& since = std::nullopt,
                           const std::optional<int>& limit = std::nullopt) const override;
    Json fetchClosedOrdersImpl(const std::string& symbol, const std::optional<long long>& since = std::nullopt,
                             const std::optional<int>& limit = std::nullopt) const override;

    // Account Methods
    Json fetchBalanceImpl() const override;
    Json fetchMyTradesImpl(const std::string& symbol, const std::optional<long long>& since = std::nullopt,
                          const std::optional<int>& limit = std::nullopt) const override;
    Json fetchDepositsImpl(const std::string& code, const std::optional<long long>& since = std::nullopt,
                          const std::optional<int>& limit = std::nullopt) const override;
    Json fetchWithdrawalsImpl(const std::string& code, const std::optional<long long>& since = std::nullopt,
                             const std::optional<int>& limit = std::nullopt) const override;
    Json withdrawImpl(const std::string& code, double amount, const std::string& address,
                     const std::string& tag = "", const Json& params = Json::object()) override;

    // Async Methods
    boost::future<Json> fetchMarketsAsync() const;
    boost::future<Json> fetchTickerAsync(const std::string& symbol) const;
    boost::future<Json> fetchOrderBookAsync(const std::string& symbol, const std::optional<int>& limit = std::nullopt) const;
    boost::future<Json> fetchTradesAsync(const std::string& symbol, const std::optional<long long>& since = std::nullopt,
                                       const std::optional<int>& limit = std::nullopt) const;
    boost::future<Json> fetchOHLCVAsync(const std::string& symbol, const std::string& timeframe = "1m",
                                       const std::optional<long long>& since = std::nullopt,
                                       const std::optional<int>& limit = std::nullopt) const;

    boost::future<Json> createOrderAsync(const std::string& symbol, const std::string& type, const std::string& side,
                                       double amount, const std::optional<double>& price = std::nullopt);
    boost::future<Json> cancelOrderAsync(const std::string& id, const std::string& symbol);
    boost::future<Json> fetchOrderAsync(const std::string& id, const std::string& symbol) const;
    boost::future<Json> fetchOrdersAsync(const std::string& symbol, const std::optional<long long>& since = std::nullopt,
                                       const std::optional<int>& limit = std::nullopt) const;
    boost::future<Json> fetchOpenOrdersAsync(const std::string& symbol, const std::optional<long long>& since = std::nullopt,
                                           const std::optional<int>& limit = std::nullopt) const;
    boost::future<Json> fetchClosedOrdersAsync(const std::string& symbol, const std::optional<long long>& since = std::nullopt,
                                             const std::optional<int>& limit = std::nullopt) const;

    boost::future<Json> fetchBalanceAsync() const;
    boost::future<Json> fetchMyTradesAsync(const std::string& symbol, const std::optional<long long>& since = std::nullopt,
                                          const std::optional<int>& limit = std::nullopt) const;
    boost::future<Json> fetchDepositsAsync(const std::string& code, const std::optional<long long>& since = std::nullopt,
                                          const std::optional<int>& limit = std::nullopt) const;
    boost::future<Json> fetchWithdrawalsAsync(const std::string& code, const std::optional<long long>& since = std::nullopt,
                                             const std::optional<int>& limit = std::nullopt) const;
    boost::future<Json> withdrawAsync(const std::string& code, double amount, const std::string& address,
                                    const std::string& tag = "", const Json& params = Json::object());

protected:
    boost::future<Json> fetchAsync(const std::string& path, const std::string& api = "public",
                                 const std::string& method = "GET", const Json& params = Json::object(),
                                 const std::map<std::string, std::string>& headers = {}) const;

    std::string getOrderId() const;
};

} // namespace ccxt

#endif // CCXT_EXCHANGE_BITHUMB_H
