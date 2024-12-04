#pragma once

#include <string>
#include <map>
#include <vector>
#include <nlohmann/json.hpp>
#include "types.h"

namespace ccxt {

using json = nlohmann::json;
using String = std::string;

class Exchange {
public:
    Exchange();
    virtual ~Exchange() = default;

    // Market Data API
    virtual json fetchMarkets(const json& params = json::object());
    virtual json fetchTicker(const String& symbol, const json& params = json::object());
    virtual json fetchTickers(const std::vector<String>& symbols = {}, const json& params = json::object());
    virtual json fetchOrderBook(const String& symbol, int limit = 0, const json& params = json::object());
    virtual json fetchTrades(const String& symbol, int since = 0, int limit = 0, const json& params = json::object());
    virtual json fetchOHLCV(const String& symbol, const String& timeframe = "1m",
                          int since = 0, int limit = 0, const json& params = json::object());

    // Trading API
    virtual json fetchBalance(const json& params = json::object());
    virtual json createOrder(const String& symbol, const String& type, const String& side,
                           double amount, double price = 0, const json& params = json::object());
    virtual json cancelOrder(const String& id, const String& symbol = "", const json& params = json::object());
    virtual json fetchOrder(const String& id, const String& symbol = "", const json& params = json::object());
    virtual json fetchOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    virtual json fetchOpenOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    virtual json fetchClosedOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());

protected:
    // Common utilities
    virtual String sign(const String& path, const String& api = "public",
                      const String& method = "GET", const json& params = json::object(),
                      const std::map<String, String>& headers = {}, const json& body = nullptr);
    
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
    
    // Common properties
    String id;
    String name;
    String version;
    bool certified;
    bool pro;
    String baseUrl;
    std::map<String, String> urls;
    std::map<String, std::map<String, std::vector<String>>> api;
    std::map<String, Market> markets;
    std::map<String, Market> markets_by_id;
    std::map<String, Currency> currencies;
    String apiKey;
    String secret;
    
    // Rate limiting
    int rateLimit;
    long long lastRestRequestTimestamp;
    std::map<String, int> rateLimitTokens;
};

} // namespace ccxt
