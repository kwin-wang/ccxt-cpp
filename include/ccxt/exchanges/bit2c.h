#pragma once

#include "ccxt/base/exchange.h"

namespace ccxt {

class Bit2c : public Exchange {
public:
    Bit2c();
    ~Bit2c() override = default;

    // Market Data API
    nlohmann::json fetchMarkets(const nlohmann::json& params = nlohmann::json::object()) override;
    nlohmann::json fetchTicker(const std::string& symbol, const nlohmann::json& params = nlohmann::json::object()) override;
    nlohmann::json fetchTickers(const std::vector<std::string>& symbols = {}, const nlohmann::json& params = nlohmann::json::object()) override;
    nlohmann::json fetchOrderBook(const std::string& symbol, int limit = 0, const nlohmann::json& params = nlohmann::json::object()) override;
    nlohmann::json fetchTrades(const std::string& symbol, int since = 0, int limit = 0, const nlohmann::json& params = nlohmann::json::object()) override;
    nlohmann::json fetchTradingFees(const nlohmann::json& params = nlohmann::json::object()) override;

    // Async Market Data API
    boost::future<nlohmann::json> fetchMarketsAsync(const nlohmann::json& params = nlohmann::json::object());
    boost::future<nlohmann::json> fetchTickerAsync(const std::string& symbol, const nlohmann::json& params = nlohmann::json::object());
    boost::future<nlohmann::json> fetchTickersAsync(const std::vector<std::string>& symbols = {}, const nlohmann::json& params = nlohmann::json::object());
    boost::future<nlohmann::json> fetchOrderBookAsync(const std::string& symbol, int limit = 0, const nlohmann::json& params = nlohmann::json::object());
    boost::future<nlohmann::json> fetchTradesAsync(const std::string& symbol, int since = 0, int limit = 0, const nlohmann::json& params = nlohmann::json::object());
    boost::future<nlohmann::json> fetchTradingFeesAsync(const nlohmann::json& params = nlohmann::json::object());

    // Trading API
    nlohmann::json fetchBalance(const nlohmann::json& params = nlohmann::json::object()) override;
    nlohmann::json createOrder(const std::string& symbol, const std::string& type, const std::string& side,
                              double amount, double price = 0, const nlohmann::json& params = nlohmann::json::object()) override;
    nlohmann::json cancelOrder(const std::string& id, const std::string& symbol = "", const nlohmann::json& params = nlohmann::json::object()) override;
    nlohmann::json fetchOrder(const std::string& id, const std::string& symbol = "", const nlohmann::json& params = nlohmann::json::object()) override;
    nlohmann::json fetchOpenOrders(const std::string& symbol = "", int since = 0, int limit = 0, const nlohmann::json& params = nlohmann::json::object()) override;
    nlohmann::json fetchMyTrades(const std::string& symbol = "", int since = 0, int limit = 0, const nlohmann::json& params = nlohmann::json::object()) override;

    // Async Trading API
    boost::future<nlohmann::json> fetchBalanceAsync(const nlohmann::json& params = nlohmann::json::object());
    boost::future<nlohmann::json> createOrderAsync(const std::string& symbol, const std::string& type, const std::string& side,
                                                  double amount, double price = 0, const nlohmann::json& params = nlohmann::json::object());
    boost::future<nlohmann::json> cancelOrderAsync(const std::string& id, const std::string& symbol = "", const nlohmann::json& params = nlohmann::json::object());
    boost::future<nlohmann::json> fetchOrderAsync(const std::string& id, const std::string& symbol = "", const nlohmann::json& params = nlohmann::json::object());
    boost::future<nlohmann::json> fetchOpenOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const nlohmann::json& params = nlohmann::json::object());
    boost::future<nlohmann::json> fetchMyTradesAsync(const std::string& symbol = "", int since = 0, int limit = 0, const nlohmann::json& params = nlohmann::json::object());

    // Account API
    nlohmann::json fetchDepositAddress(const std::string& code, const nlohmann::json& params = nlohmann::json::object()) override;

    // Async Account API
    boost::future<nlohmann::json> fetchDepositAddressAsync(const std::string& code, const nlohmann::json& params = nlohmann::json::object());

protected:
    std::string sign(const std::string& path, const std::string& api = "public",
                    const std::string& method = "GET", const nlohmann::json& params = nlohmann::json::object(),
                    const std::map<std::string, std::string>& headers = {}, const nlohmann::json* body = nullptr) override;

private:
    void initializeApiEndpoints();
    std::string getBit2cSymbol(const std::string& symbol);
    std::string getCommonSymbol(const std::string& bit2cSymbol);
    nlohmann::json parseTicker(const nlohmann::json& ticker, const Market& market = Market());
    nlohmann::json parseTrade(const nlohmann::json& trade, const Market& market = Market());
    nlohmann::json parseOrder(const nlohmann::json& order, const Market& market = Market());
    nlohmann::json parseBalance(const nlohmann::json& response);
    nlohmann::json parseFee(const nlohmann::json& fee, const Market& market = Market());
    nlohmann::json parseDepositAddress(const nlohmann::json& depositAddress, const std::string& currency = "");
    std::string createSignature(const std::string& timestamp, const std::string& method,
                              const std::string& path, const std::string& queryString);
};

} // namespace ccxt
