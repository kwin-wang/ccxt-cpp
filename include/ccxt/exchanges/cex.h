#ifndef CCXT_EXCHANGE_CEX_H
#define CCXT_EXCHANGE_CEX_H

#include "ccxt/base/exchange.h"
#include "ccxt/base/exchange_impl.h"

namespace ccxt {

class cex : public ExchangeImpl {
public:
    cex(const Config& config = Config());
    ~cex() = default;

    static Exchange* create(const Config& config = Config()) {
        return new cex(config);
    }

protected:
    void init() override;
    Json describeImpl() const override;

    // Market Data
    Json fetchMarketsImpl() const override;
    Json fetchCurrenciesImpl() const override;
    Json fetchTickerImpl(const std::string& symbol) const override;
    Json fetchTickersImpl(const std::vector<std::string>& symbols = std::vector<std::string>()) const override;
    Json fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchTradesImpl(const std::string& symbol, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchTimeImpl() const override;
    Json fetchTradingFeesImpl() const override;

    // Trading
    Json createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side, double amount, const std::optional<double>& price = std::nullopt) override;
    Json createStopOrderImpl(const std::string& symbol, const std::string& type, const std::string& side, double amount, double price, const std::optional<Json>& params = std::nullopt) override;
    Json cancelOrderImpl(const std::string& id, const std::string& symbol) override;
    Json cancelAllOrdersImpl(const std::optional<std::string>& symbol = std::nullopt) override;
    Json fetchOrderImpl(const std::string& id, const std::string& symbol) const override;
    Json fetchOpenOrderImpl(const std::string& id, const std::string& symbol) const override;
    Json fetchClosedOrderImpl(const std::string& id, const std::string& symbol) const override;
    Json fetchOrdersImpl(const std::optional<std::string>& symbol = std::nullopt, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchOpenOrdersImpl(const std::optional<std::string>& symbol = std::nullopt, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchClosedOrdersImpl(const std::optional<std::string>& symbol = std::nullopt, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;

    // Account
    Json fetchAccountsImpl() const override;
    Json fetchBalanceImpl() const override;
    Json fetchLedgerImpl(const std::optional<std::string>& code = std::nullopt, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchDepositAddressImpl(const std::string& code, const std::optional<std::string>& network = std::nullopt) const override;
    Json fetchDepositsWithdrawalsImpl(const std::optional<std::string>& code = std::nullopt, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json transferImpl(const std::string& code, double amount, const std::string& fromAccount, const std::string& toAccount, const std::optional<Json>& params = std::nullopt) override;

private:
    static Exchange* createInstance(const Config& config) {
        return new cex(config);
    }

    static const std::string defaultBaseURL;
    static const std::string defaultVersion;
    static const int defaultRateLimit;
    static const bool defaultPro;

    static ExchangeRegistry::Factory factory;

    // Helper methods for parsing responses
    Json parseTicker(const Json& ticker, const Json& market = Json()) const;
    Json parseTrade(const Json& trade, const Json& market = Json()) const;
    Json parseOrder(const Json& order, const Json& market = Json()) const;
    Json parseTransaction(const Json& transaction, const Json& currency = Json()) const;
    Json parseLedgerEntry(const Json& item, const Json& currency = Json()) const;
    Json parseOHLCV(const Json& ohlcv, const Json& market = Json()) const;
    Json parseTransferStatus(const std::string& status) const;

    // Authentication helpers
    std::string sign(const std::string& path, const std::string& api = "public", const std::string& method = "GET",
                    const Json& params = Json::object(), const Json& headers = Json::object(), const Json& body = Json::object()) const;

    // Error handling
    void handleErrors(const std::string& code, const std::string& reason, const std::string& url, const std::string& method,
                     const Json& headers, const Json& body, const Json& response, const std::string& requestHeaders,
                     const std::string& requestBody) const;

    // Additional helper methods
    Json getMyCurrentFee(const std::string& symbol) const;
    Json getMyVolume() const;
};

} // namespace ccxt

#endif // CCXT_EXCHANGE_CEX_H
