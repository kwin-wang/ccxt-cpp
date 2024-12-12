#pragma once

#include "binance.h"

namespace ccxt {

class BinanceUS : public Binance {
public:
    static const std::string defaultHostname;
    static const int defaultRateLimit;
    static const bool defaultPro;
    

    explicit BinanceUS(const Config& config = Config());
    virtual ~BinanceUS() = default;

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
};

} // namespace ccxt
