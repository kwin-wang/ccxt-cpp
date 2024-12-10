#pragma once

#include <string>
#include <map>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include "types.h"
#include "config.h"

namespace ccxt {

using json = nlohmann::json;
using String = std::string;

class Exchange {
public:
    Exchange(const Config& config = Config());
    virtual ~Exchange() = default;

    // Public properties
    std::string id;
    std::string name;
    std::vector<std::string> countries;
    std::string version;
    int rateLimit;
    bool pro;
    bool certified;
    std::map<std::string, std::map<std::string, std::string>> urls;
    std::map<std::string, std::optional<bool>> has;
    std::map<std::string, std::string> timeframes;
    long long lastRestRequestTimestamp;
    
    // Public API methods
    virtual void init();
    virtual json describe() const;
    virtual json fetchMarkets() const;
    virtual json fetchTicker(const std::string& symbol) const;
    virtual json fetchTickers(const std::vector<std::string>& symbols = {}) const;
    virtual json fetchOrderBook(const std::string& symbol, const std::optional<int>& limit = std::nullopt) const;
    virtual json fetchOHLCV(const std::string& symbol, const std::string& timeframe,
                          const std::optional<long long>& since = std::nullopt,
                          const std::optional<int>& limit = std::nullopt) const;
    virtual json createOrder(const std::string& symbol, const std::string& type,
                           const std::string& side, double amount,
                           const std::optional<double>& price = std::nullopt);
    virtual json cancelOrder(const std::string& id, const std::string& symbol);
    virtual json fetchOrder(const std::string& id, const std::string& symbol) const;
    virtual json fetchOpenOrders(const std::string& symbol = "",
                               const std::optional<long long>& since = std::nullopt,
                               const std::optional<int>& limit = std::nullopt) const;
    virtual json fetchMyTrades(const std::string& symbol = "",
                             const std::optional<long long>& since = std::nullopt,
                             const std::optional<int>& limit = std::nullopt) const;
    virtual json fetchOrderTrades(const std::string& id, const std::string& symbol) const;
    virtual json fetchBalance() const;

protected:
    // Protected implementation methods
    virtual json describeImpl() const { return json(); }
    virtual json fetchMarketsImpl() const { return json(); }
    virtual json fetchTickerImpl(const std::string& symbol) const { return json(); }
    virtual json fetchTickersImpl(const std::vector<std::string>& symbols) const { return json(); }
    virtual json fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const { return json(); }
    virtual json fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe,
                             const std::optional<long long>& since,
                             const std::optional<int>& limit) const { return json(); }
    virtual json createOrderImpl(const std::string& symbol, const std::string& type,
                             const std::string& side, double amount,
                             const std::optional<double>& price) { return json(); }
    virtual json cancelOrderImpl(const std::string& id, const std::string& symbol) { return json(); }
    virtual json fetchOrderImpl(const std::string& id, const std::string& symbol) const { return json(); }
    virtual json fetchOpenOrdersImpl(const std::string& symbol,
                                 const std::optional<long long>& since,
                                 const std::optional<int>& limit) const { return json(); }
    virtual json fetchMyTradesImpl(const std::string& symbol,
                                const std::optional<long long>& since,
                                const std::optional<int>& limit) const { return json(); }
    virtual json fetchOrderTradesImpl(const std::string& id, const std::string& symbol) const { return json(); }
    virtual json fetchBalanceImpl() const { return json(); }

    // Utility methods
    String sign(const String& path, const String& api = "public",
              const String& method = "GET", const json& params = json::object(),
              const std::map<String, String>& headers = {},
              const json& body = nullptr);
    
    void checkRequiredCredentials();
    String implodeParams(const String& path, const json& params);
    json omit(const json& params, const std::vector<String>& keys);
    std::vector<String> extractParams(const String& path);
    String urlencode(const json& params);
    String encode(const String& string);
    String hmac(const String& message, const String& secret,
               const String& algorithm = "sha256", const String& digest = "hex");
    long long milliseconds();
    String uuid();
    String iso8601(long long timestamp);
    long long parse8601(const String& datetime);
    
    // Market utilities
    Market market(const String& symbol);
    void loadMarkets(bool reload = false);
    String marketId(const String& symbol);
    String symbol(const String& marketId);
    String amountToPrecision(const String& symbol, double amount);
    String priceToPrecision(const String& symbol, double price);
    String feeToPrecision(const String& symbol, double fee);
    String currencyToPrecision(const String& currency, double fee);
    String costToPrecision(const String& symbol, double cost);

    Config config_;
    std::map<String, Market> markets;
    std::map<String, Market> markets_by_id;
};

} // namespace ccxt
