#ifndef CCXT_EXCHANGE_BITFINEX2_H
#define CCXT_EXCHANGE_BITFINEX2_H

#include "ccxt/base/exchange.h"


namespace ccxt {

class bitfinex2 : public Exchange {
public:
    bitfinex2(const Config& config = Config());
    ~bitfinex2() = default;

    static Exchange* create(const Config& config = Config()) {
        return new bitfinex2(config);
    }

protected:
    void init() override;
    Json describeImpl() const override;

    // Market Data
    Json fetchMarketsImpl() const override;
    Json fetchCurrenciesImpl() const override;
    Json fetchTickerImpl(const std::string& symbol) const override;
    Json fetchTickersImpl(const std::vector<std::string>& symbols = {}) const override;
    Json fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchTradesImpl(const std::string& symbol, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;

    // Trading
    Json createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side, double amount, const std::optional<double>& price = std::nullopt) override;
    Json editOrderImpl(const std::string& id, const std::string& symbol, const std::string& type, const std::string& side, const std::optional<double>& amount = std::nullopt, const std::optional<double>& price = std::nullopt) override;
    Json cancelOrderImpl(const std::string& id, const std::string& symbol) override;
    Json cancelOrdersImpl(const std::vector<std::string>& ids, const std::string& symbol = "") override;
    Json cancelAllOrdersImpl(const std::string& symbol = "") override;
    Json fetchOrderImpl(const std::string& id, const std::string& symbol) const override;
    Json fetchOpenOrdersImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchClosedOrdersImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchMyTradesImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;

    // Account
    Json fetchBalanceImpl() const override;
    Json fetchLedgerImpl(const std::string& code = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchDepositAddressImpl(const std::string& code, const std::optional<std::string>& network = std::nullopt) const override;
    Json createDepositAddressImpl(const std::string& code, const std::optional<std::string>& network = std::nullopt) override;
    Json fetchDepositsWithdrawalsImpl(const std::string& code = "", const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;

    // Funding
    Json fetchFundingRateImpl(const std::string& symbol) const override;
    Json fetchFundingRatesImpl(const std::vector<std::string>& symbols = {}) const override;
    Json fetchFundingRateHistoryImpl(const std::string& symbol, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;

    // Trading Fees
    Json fetchTradingFeesImpl() const override;

private:
    static Exchange* createInstance(const Config& config) {
        return new bitfinex2(config);
    }

    static const std::string defaultBaseURL;
    static const std::string defaultVersion;
    static const int defaultRateLimit;
    static const bool defaultPro;

    

    // Helper methods for parsing responses
    Json parseMarket(const Json& market) const;
    Json parseTicker(const Json& ticker, const Json& market = Json()) const;
    Json parseTrade(const Json& trade, const Json& market = Json()) const;
    Json parseOHLCV(const Json& ohlcv, const Json& market = Json(), const std::string& timeframe = "1m", 
                    const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const;
    Json parseOrder(const Json& order, const Json& market = Json()) const;
    Json parseLedgerEntry(const Json& item, const Json& currency = Json()) const;
    Json parseTransaction(const Json& transaction, const Json& currency = Json()) const;
    Json parseFundingRate(const Json& fundingRate, const Json& market = Json()) const;

    // Authentication helpers
    std::string sign(const std::string& path, const std::string& api = "public", const std::string& method = "GET",
                    const Json& params = Json::object(), const Json& headers = Json::object(), const Json& body = Json::object()) const;
};

} // namespace ccxt

#endif // CCXT_EXCHANGE_BITFINEX2_H
