#ifndef CCXT_EXCHANGE_BLOCKCHAINCOM_H
#define CCXT_EXCHANGE_BLOCKCHAINCOM_H

#include "ccxt/base/exchange.h"


namespace ccxt {

class blockchaincom : public Exchange {
public:
    blockchaincom(const Config& config = Config());
    ~blockchaincom() = default;

    static Exchange* create(const Config& config = Config()) {
        return new blockchaincom(config);
    }

protected:
    void init() override;
    Json describeImpl() const override;

    // Market Data
    Json fetchMarketsImpl() const override;
    Json fetchTickerImpl(const std::string& symbol) const override;
    Json fetchTickersImpl(const std::vector<std::string>& symbols = std::vector<std::string>()) const override;
    Json fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchL2OrderBookImpl(const std::string& symbol, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchL3OrderBookImpl(const std::string& symbol, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchTradingFeesImpl() const override;

    // Trading
    Json createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side, double amount, const std::optional<double>& price = std::nullopt) override;
    Json createStopOrderImpl(const std::string& symbol, const std::string& type, const std::string& side, double amount, 
                           double price, const std::optional<Json>& params = std::nullopt) override;
    Json cancelOrderImpl(const std::string& id, const std::string& symbol) override;
    Json cancelAllOrdersImpl(const std::optional<std::string>& symbol = std::nullopt) override;
    Json fetchOrderImpl(const std::string& id, const std::string& symbol) const override;
    Json fetchOrdersImpl(const std::optional<std::string>& symbol = std::nullopt, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchOpenOrdersImpl(const std::optional<std::string>& symbol = std::nullopt, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchClosedOrdersImpl(const std::optional<std::string>& symbol = std::nullopt, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchCanceledOrdersImpl(const std::optional<std::string>& symbol = std::nullopt, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchMyTradesImpl(const std::optional<std::string>& symbol = std::nullopt, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;

    // Account
    Json fetchBalanceImpl() const override;
    Json fetchDepositAddressImpl(const std::string& code, const std::optional<std::string>& network = std::nullopt) const override;
    Json fetchDepositsImpl(const std::optional<std::string>& code = std::nullopt, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchWithdrawalsImpl(const std::optional<std::string>& code = std::nullopt, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchDepositImpl(const std::string& id, const std::string& code) const override;
    Json fetchWithdrawalImpl(const std::string& id, const std::string& code) const override;
    Json fetchWithdrawalWhitelistImpl() const override;
    Json withdrawImpl(const std::string& code, double amount, const std::string& address, const std::optional<std::string>& tag = std::nullopt) override;

private:
    static Exchange* createInstance(const Config& config) {
        return new blockchaincom(config);
    }

    static const std::string defaultBaseURL;
    static const std::string defaultVersion;
    static const int defaultRateLimit;
    static const bool defaultPro;

    

    // Helper methods for parsing responses
    Json parseTicker(const Json& ticker, const Json& market = Json()) const;
    Json parseTrade(const Json& trade, const Json& market = Json()) const;
    Json parseOrder(const Json& order, const Json& market = Json()) const;
    Json parseTransaction(const Json& transaction, const Json& currency = Json()) const;
    Json parseDepositAddress(const Json& depositAddress, const Json& currency = Json()) const;

    // Authentication helpers
    std::string sign(const std::string& path, const std::string& api = "public", const std::string& method = "GET",
                    const Json& params = Json::object(), const Json& headers = Json::object(), const Json& body = Json::object()) const;

    // Error handling
    void handleErrors(const std::string& code, const std::string& reason, const std::string& url, const std::string& method,
                     const Json& headers, const Json& body, const Json& response, const std::string& requestHeaders,
                     const std::string& requestBody) const;
};

} // namespace ccxt

#endif // CCXT_EXCHANGE_BLOCKCHAINCOM_H
