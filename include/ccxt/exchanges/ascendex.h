#pragma once

#include "ccxt/base/exchange.h"

namespace ccxt {

class AscendEX : public Exchange {
public:
    static const std::string defaultHostname;
    static const int defaultRateLimit;
    static const bool defaultPro;
    

    explicit AscendEX(const Config& config = Config());
    virtual ~AscendEX() = default;

    void init() override;

protected:
    // Market Data API
    json fetchMarketsImpl() const override;
    json fetchTickerImpl(const std::string& symbol) const override;
    json fetchTickersImpl(const std::vector<std::string>& symbols = {}) const override;
    json fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit = std::nullopt) const override;
    json fetchTradesImpl(const std::string& symbol, const std::optional<long long>& since = std::nullopt,
                      const std::optional<int>& limit = std::nullopt) const override;
    json fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe = "1m",
                     const std::optional<long long>& since = std::nullopt,
                     const std::optional<int>& limit = std::nullopt) const override;
    json fetchTimeImpl() const override;
    json fetchCurrenciesImpl() const override;
    json fetchTradingFeesImpl() const override;
    json fetchBalanceImpl() const override;
    json fetchAccountsImpl() const override;
    json fetchDepositAddressImpl(const std::string& code, const json& params = json::object()) const override;
    json fetchDepositsImpl(const std::string& code = "", const std::optional<long long>& since = std::nullopt,
                        const std::optional<int>& limit = std::nullopt) const override;
    json fetchWithdrawalsImpl(const std::string& code = "", const std::optional<long long>& since = std::nullopt,
                          const std::optional<int>& limit = std::nullopt) const override;
    json fetchDepositsWithdrawalsImpl(const std::string& code = "", const std::optional<long long>& since = std::nullopt,
                                   const std::optional<int>& limit = std::nullopt) const override;
    json fetchDepositWithdrawFeesImpl() const override;
    json fetchFundingRatesImpl(const std::vector<std::string>& symbols = {}) const override;
    json fetchFundingRateHistoryImpl(const std::string& symbol, const std::optional<long long>& since = std::nullopt,
                                  const std::optional<int>& limit = std::nullopt) const override;
    json fetchLeverageImpl(const std::string& symbol) const override;
    json fetchLeveragesImpl(const std::vector<std::string>& symbols = {}) const override;
    json fetchMarginModesImpl(const std::vector<std::string>& symbols = {}) const override;
    json fetchPositionsImpl(const std::vector<std::string>& symbols = {}) const override;

    // Trading API
    json createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side,
                      double amount, const std::optional<double>& price = std::nullopt) override;
    json createOrdersImpl(const std::vector<json>& orders) override;
    json cancelOrderImpl(const std::string& id, const std::string& symbol = "") override;
    json cancelAllOrdersImpl(const std::string& symbol = "") override;
    json fetchOrderImpl(const std::string& id, const std::string& symbol = "") const override;
    json fetchOrdersImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt,
                      const std::optional<int>& limit = std::nullopt) const override;
    json fetchOpenOrdersImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt,
                         const std::optional<int>& limit = std::nullopt) const override;
    json fetchClosedOrdersImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt,
                           const std::optional<int>& limit = std::nullopt) const override;
    json setLeverageImpl(int leverage, const std::string& symbol = "") override;
    json setMarginModeImpl(const std::string& marginMode, const std::string& symbol = "") override;
    json addMarginImpl(const std::string& symbol, double amount) override;
    json reduceMarginImpl(const std::string& symbol, double amount) override;
    json transferImpl(const std::string& code, double amount, const std::string& fromAccount,
                   const std::string& toAccount) override;

private:
    // Helper methods
    json parseMarket(const json& market) const;
    json parseTicker(const json& ticker, const std::optional<json>& market = std::nullopt) const;
    json parseOrderBook(const json& orderbook, const std::string& symbol,
                     const std::optional<int>& limit = std::nullopt) const;
    json parseOHLCV(const json& ohlcv, const std::optional<json>& market = std::nullopt) const;
    json parseTrade(const json& trade, const std::optional<json>& market = std::nullopt) const;
    json parseOrder(const json& order, const std::optional<json>& market = std::nullopt) const;
    json parseOrders(const json& orders, const std::string& symbol = "",
                  const std::optional<long long>& since = std::nullopt,
                  const std::optional<int>& limit = std::nullopt) const;
    json parseTrades(const json& trades, const std::string& symbol = "",
                  const std::optional<long long>& since = std::nullopt,
                  const std::optional<int>& limit = std::nullopt) const;
    json parseBalance(const json& response) const;
    json parseMarginBalance(const json& response) const;
    json parseSwapBalance(const json& response) const;
    json parsePosition(const json& position, const std::optional<json>& market = std::nullopt) const;
    json parseFundingRate(const json& fundingRate, const std::optional<json>& market = std::nullopt) const;
    json parseDepositAddress(const json& depositAddress, const std::optional<json>& currency = std::nullopt) const;
    json parseTransaction(const json& transaction, const std::optional<json>& currency = std::nullopt) const;
    json parseTransfer(const json& transfer, const std::optional<json>& currency = std::nullopt) const;
    json parseMarginMode(const json& marginMode, const std::optional<json>& market = std::nullopt) const;
    json parseLeverage(const json& leverage, const std::optional<json>& market = std::nullopt) const;
    json parseMarketLeverageTiers(const json& info, const std::optional<json>& market = std::nullopt) const;
    json parseDepositWithdrawFee(const json& fee, const std::optional<json>& currency = std::nullopt) const;
    json parseIncome(const json& income, const std::optional<json>& market = std::nullopt) const;
    std::string parseOrderStatus(const std::string& status) const;
    std::string parseTransactionStatus(const std::string& status) const;
    std::string parseTransferStatus(const std::string& status) const;
    std::string getAccountCategory(const json& params = json::object()) const;
    json createOrderRequest(const std::string& symbol, const std::string& type, const std::string& side,
                        double amount, const std::optional<double>& price = std::nullopt,
                        const json& params = json::object()) const;
    json modifyMarginHelper(const std::string& symbol, double amount, const std::string& type,
                        const json& params = json::object()) const;
    json parseMarginModification(const json& data, const std::optional<json>& market = std::nullopt) const;
};

} // namespace ccxt
