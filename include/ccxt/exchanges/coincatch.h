#ifndef CCXT_EXCHANGE_COINCATCH_H
#define CCXT_EXCHANGE_COINCATCH_H

#include "ccxt/base/exchange.h"

namespace ccxt {

class coincatch : public Exchange {
public:
    coincatch(const Config& config = Config());
    ~coincatch() = default;

    static Exchange* create(const Config& config = Config()) {
        return new coincatch(config);
    }

protected:
    void init() override;
    Json describeImpl() const override;

    // Market Data
    Json fetchMarketsImpl() const override;
    Json fetchCurrenciesImpl() const override;
    Json fetchTickerImpl(const std::string& symbol) const override;
    Json fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchTradesImpl(const std::string& symbol, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchBalanceImpl() const override;
    Json fetchLedgerImpl(const std::optional<std::string>& code = std::nullopt, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json fetchLeverageImpl(const std::string& symbol, const std::optional<Json>& params = std::nullopt) const override;
    Json fetchFundingRateImpl(const std::string& symbol, const std::optional<Json>& params = std::nullopt) const override;
    Json fetchFundingRateHistoryImpl(const std::string& symbol, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;

    // Async Market Data Methods
    boost::future<Json> fetchMarketsAsync(const std::optional<Json>& params = std::nullopt) const;
    boost::future<Json> fetchCurrenciesAsync(const std::optional<Json>& params = std::nullopt) const;
    boost::future<Json> fetchTickerAsync(const std::string& symbol, const std::optional<Json>& params = std::nullopt) const;
    boost::future<Json> fetchOrderBookAsync(const std::string& symbol, const std::optional<int>& limit = std::nullopt, const std::optional<Json>& params = std::nullopt) const;
    boost::future<Json> fetchTradesAsync(const std::string& symbol, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt, const std::optional<Json>& params = std::nullopt) const;
    boost::future<Json> fetchBalanceAsync(const std::optional<Json>& params = std::nullopt) const;
    boost::future<Json> fetchLedgerAsync(const std::optional<std::string>& code = std::nullopt, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt, const std::optional<Json>& params = std::nullopt) const;
    boost::future<Json> fetchLeverageAsync(const std::string& symbol, const std::optional<Json>& params = std::nullopt) const;
    boost::future<Json> fetchFundingRateAsync(const std::string& symbol, const std::optional<Json>& params = std::nullopt) const;
    boost::future<Json> fetchFundingRateHistoryAsync(const std::string& symbol, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt, const std::optional<Json>& params = std::nullopt) const;

    // Trading
    Json createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side, double amount, const std::optional<double>& price = std::nullopt) override;
    Json createMarketOrderWithCostImpl(const std::string& symbol, const std::string& side, double cost, const std::optional<Json>& params = std::nullopt) override;
    Json createStopLimitOrderImpl(const std::string& symbol, const std::string& side, double amount, double price, double stopPrice, const std::optional<Json>& params = std::nullopt) override;
    Json createStopMarketOrderImpl(const std::string& symbol, const std::string& side, double amount, double stopPrice, const std::optional<Json>& params = std::nullopt) override;
    Json createTakeProfitOrderImpl(const std::string& symbol, const std::string& type, const std::string& side, double amount, const std::optional<double>& price = std::nullopt, const std::optional<Json>& params = std::nullopt) override;
    Json cancelOrderImpl(const std::string& id, const std::string& symbol) override;
    Json cancelOrdersImpl(const std::vector<std::string>& ids, const std::string& symbol, const std::optional<Json>& params = std::nullopt) override;
    Json cancelAllOrdersImpl(const std::optional<std::string>& symbol = std::nullopt) override;

    // Async Trading Methods
    boost::future<Json> createOrderAsync(const std::string& symbol, const std::string& type, const std::string& side, double amount, const std::optional<double>& price = std::nullopt, const std::optional<Json>& params = std::nullopt);
    boost::future<Json> createMarketOrderWithCostAsync(const std::string& symbol, const std::string& side, double cost, const std::optional<Json>& params = std::nullopt);
    boost::future<Json> createStopLimitOrderAsync(const std::string& symbol, const std::string& side, double amount, double price, double stopPrice, const std::optional<Json>& params = std::nullopt);
    boost::future<Json> createStopMarketOrderAsync(const std::string& symbol, const std::string& side, double amount, double stopPrice, const std::optional<Json>& params = std::nullopt);
    boost::future<Json> createTakeProfitOrderAsync(const std::string& symbol, const std::string& type, const std::string& side, double amount, const std::optional<double>& price = std::nullopt, const std::optional<Json>& params = std::nullopt);
    boost::future<Json> cancelOrderAsync(const std::string& id, const std::string& symbol, const std::optional<Json>& params = std::nullopt);
    boost::future<Json> cancelOrdersAsync(const std::vector<std::string>& ids, const std::string& symbol, const std::optional<Json>& params = std::nullopt);
    boost::future<Json> cancelAllOrdersAsync(const std::optional<std::string>& symbol = std::nullopt, const std::optional<Json>& params = std::nullopt);

    // Account
    Json fetchDepositAddressImpl(const std::string& code, const std::optional<std::string>& network = std::nullopt) const override;
    Json fetchDepositsImpl(const std::optional<std::string>& code = std::nullopt, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt) const override;
    Json addMarginImpl(const std::string& symbol, double amount, const std::optional<Json>& params = std::nullopt) override;

    // Async Account Methods
    boost::future<Json> fetchDepositAddressAsync(const std::string& code, const std::optional<std::string>& network = std::nullopt, const std::optional<Json>& params = std::nullopt) const;
    boost::future<Json> fetchDepositsAsync(const std::optional<std::string>& code = std::nullopt, const std::optional<long long>& since = std::nullopt, const std::optional<int>& limit = std::nullopt, const std::optional<Json>& params = std::nullopt) const;
    boost::future<Json> addMarginAsync(const std::string& symbol, double amount, const std::optional<Json>& params = std::nullopt);

private:
    static Exchange* createInstance(const Config& config) {
        return new coincatch(config);
    }

    

    // Helper methods for parsing responses
    Json parseTicker(const Json& ticker, const Json& market = Json()) const;
    Json parseTrade(const Json& trade, const Json& market = Json()) const;
    Json parseOrder(const Json& order, const Json& market = Json()) const;
    Json parseTransaction(const Json& transaction, const Json& currency = Json()) const;
    Json parseLedgerEntry(const Json& item, const Json& currency = Json()) const;
    Json parseFundingRate(const Json& fundingRate, const Json& market = Json()) const;

    // Authentication helpers
    std::string sign(const std::string& path, const std::string& api = "public", const std::string& method = "GET",
                    const Json& params = Json::object(), const Json& headers = Json::object(), const Json& body = Json::object()) const;

    // Error handling
    void handleErrors(const std::string& code, const std::string& reason, const std::string& url, const std::string& method,
                     const Json& headers, const Json& body, const Json& response, const std::string& requestHeaders,
                     const std::string& requestBody) const;
};

} // namespace ccxt

#endif // CCXT_EXCHANGE_COINCATCH_H
