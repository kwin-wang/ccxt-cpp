#pragma once

#include "binance.h"

namespace ccxt {

class BinanceCoinM : public Binance {
public:
    static const std::string defaultHostname;
    static const int defaultRateLimit;
    static const bool defaultPro;
    

    explicit BinanceCoinM(const Config& config = Config());
    virtual ~BinanceCoinM() = default;

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
    json fetchIndexOHLCVImpl(const std::string& symbol, const std::string& timeframe = "1m",
                          const std::optional<long long>& since = std::nullopt,
                          const std::optional<int>& limit = std::nullopt) const override;
    json fetchMarkOHLCVImpl(const std::string& symbol, const std::string& timeframe = "1m",
                         const std::optional<long long>& since = std::nullopt,
                         const std::optional<int>& limit = std::nullopt) const override;
    json fetchPremiumIndexOHLCVImpl(const std::string& symbol, const std::string& timeframe = "1m",
                                 const std::optional<long long>& since = std::nullopt,
                                 const std::optional<int>& limit = std::nullopt) const override;
    json fetchFundingRateImpl(const std::string& symbol) const override;
    json fetchFundingRatesImpl(const std::vector<std::string>& symbols = {}) const override;
    json fetchFundingRateHistoryImpl(const std::string& symbol, const std::optional<long long>& since = std::nullopt,
                                  const std::optional<int>& limit = std::nullopt) const override;

    // Trading API
    json fetchLeverageBracketsImpl(const std::vector<std::string>& symbols = {}) const override;
    json fetchPositionsImpl(const std::vector<std::string>& symbols = {}) const override;
    json setLeverageImpl(int leverage, const std::string& symbol = "") override;
    json setMarginModeImpl(const std::string& marginMode, const std::string& symbol = "") override;
    json setPositionModeImpl(bool hedged) override;
    json addMarginImpl(const std::string& symbol, double amount) override;
    json reduceMarginImpl(const std::string& symbol, double amount) override;
    json transferInImpl(const std::string& code, double amount, const json& params = json::object()) override;
    json transferOutImpl(const std::string& code, double amount, const json& params = json::object()) override;

private:
    // Helper methods
    std::string sign(const std::string& path, const std::string& api = "public",
                  const std::string& method = "GET", const json& params = json::object(),
                  const std::map<std::string, std::string>& headers = {},
                  const json& body = nullptr) const override;
    json parseFundingRate(const json& fundingRate, const Market& market = Market()) const;
    json parsePosition(const json& position, const Market& market = Market()) const;
    json parseLeverageBrackets(const json& response, const Market& market = Market()) const;
    std::string getSettlementCurrency(const std::string& market) const;
};

} // namespace ccxt
