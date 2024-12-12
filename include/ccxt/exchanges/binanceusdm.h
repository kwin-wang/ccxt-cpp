#pragma once

#include "binance.h"

namespace ccxt {

class BinanceUSDM : public Binance {
public:
    static const std::string defaultHostname;
    static const int defaultRateLimit;
    static const bool defaultPro;
    

    explicit BinanceUSDM(const Config& config = Config());
    virtual ~BinanceUSDM() = default;

    void init() override;

    // Transfer methods
    json transferIn(const std::string& code, double amount, const json& params = json::object());
    json transferOut(const std::string& code, double amount, const json& params = json::object());

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
    json fetchFundingRateImpl(const std::string& symbol) const override;
    json fetchFundingRatesImpl(const std::vector<std::string>& symbols = {}) const override;
    json fetchFundingRateHistoryImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt,
                                  const std::optional<int>& limit = std::nullopt) const override;

    // Trading API
    json fetchBalanceImpl(const json& params = json::object()) const override;
    json createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side,
                      double amount, double price = 0, const json& params = json::object()) override;
    json cancelOrderImpl(const std::string& id, const std::string& symbol = "", const json& params = json::object()) override;
    json fetchOrderImpl(const std::string& id, const std::string& symbol = "", const json& params = json::object()) const override;
    json fetchOrdersImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt,
                      const std::optional<int>& limit = std::nullopt, const json& params = json::object()) const override;
    json fetchOpenOrdersImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt,
                         const std::optional<int>& limit = std::nullopt, const json& params = json::object()) const override;
    json fetchClosedOrdersImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt,
                           const std::optional<int>& limit = std::nullopt, const json& params = json::object()) const override;
    json fetchPositionsImpl(const std::vector<std::string>& symbols = {}) const override;
    json fetchPositionRiskImpl(const std::vector<std::string>& symbols = {}) const override;
    json setLeverageImpl(int leverage, const std::string& symbol) override;
    json setMarginModeImpl(const std::string& marginMode, const std::string& symbol = "") override;
    json setPositionModeImpl(bool hedged) override;

private:
    // Helper methods
    std::string sign(const std::string& path, const std::string& api = "public",
                  const std::string& method = "GET", const json& params = json::object(),
                  const std::map<std::string, std::string>& headers = {},
                  const json& body = nullptr) const override;
    json parseTrade(const json& trade, const Market& market = Market()) const;
    json parseOrder(const json& order, const Market& market = Market()) const;
    json parseTicker(const json& ticker, const Market& market = Market()) const;
    json parseOHLCV(const json& ohlcv, const Market& market = Market(), const std::string& timeframe = "1m") const;
    json parseFundingRate(const json& fundingRate, const Market& market = Market()) const;
    json parsePosition(const json& position, const Market& market = Market()) const;
    json parseLeverageBrackets(const json& response, const Market& market = Market()) const;
};

} // namespace ccxt
