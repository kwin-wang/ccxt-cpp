#pragma once

#include "ccxt/base/exchange.h"

namespace ccxt {

class Coincheck : public Exchange {
public:
    Coincheck();
    ~Coincheck() override = default;

    // Market Data API
    json fetchMarkets(const json& params = json::object()) override;
    json fetchTicker(const std::string& symbol, const json& params = json::object()) override;
    json fetchTickers(const std::vector<std::string>& symbols = {}, const json& params = json::object()) override;
    json fetchOrderBook(const std::string& symbol, int limit = 0, const json& params = json::object()) override;
    json fetchTrades(const std::string& symbol, int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchOHLCV(const std::string& symbol, const std::string& timeframe = "1m",
                    int since = 0, int limit = 0, const json& params = json::object()) override;

    // Async Market Data API
    boost::future<json> fetchMarketsAsync(const json& params = json::object());
    boost::future<json> fetchTickerAsync(const std::string& symbol, const json& params = json::object());
    boost::future<json> fetchTickersAsync(const std::vector<std::string>& symbols = {}, const json& params = json::object());
    boost::future<json> fetchOrderBookAsync(const std::string& symbol, int limit = 0, const json& params = json::object());
    boost::future<json> fetchTradesAsync(const std::string& symbol, int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchOHLCVAsync(const std::string& symbol, const std::string& timeframe = "1m",
                                      int since = 0, int limit = 0, const json& params = json::object());

    // Trading API
    json fetchBalance(const json& params = json::object()) override;
    json createOrder(const std::string& symbol, const std::string& type, const std::string& side,
                    double amount, double price = 0, const json& params = json::object()) override;
    json cancelOrder(const std::string& id, const std::string& symbol = "", const json& params = json::object()) override;
    json fetchOrder(const std::string& id, const std::string& symbol = "", const json& params = json::object()) override;
    json fetchOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchOpenOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchClosedOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;

    // Async Trading API
    boost::future<json> fetchBalanceAsync(const json& params = json::object());
    boost::future<json> createOrderAsync(const std::string& symbol, const std::string& type, const std::string& side,
                                       double amount, double price = 0, const json& params = json::object());
    boost::future<json> cancelOrderAsync(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    boost::future<json> fetchOrderAsync(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    boost::future<json> fetchOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchOpenOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchClosedOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());

    // Account API
    json fetchMyTrades(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchDeposits(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchWithdrawals(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    json fetchDepositAddress(const std::string& code, const json& params = json::object());
    json withdraw(const std::string& code, double amount, const std::string& address, const std::string& tag = "", const json& params = json::object());

    // Async Account API
    boost::future<json> fetchMyTradesAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchDepositsAsync(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchWithdrawalsAsync(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchDepositAddressAsync(const std::string& code, const json& params = json::object());
    boost::future<json> withdrawAsync(const std::string& code, double amount, const std::string& address, const std::string& tag = "", const json& params = json::object());

    // Leverage Trading API
    json fetchLeverageBalance(const json& params = json::object());
    json createLeverageOrder(const std::string& symbol, const std::string& type, const std::string& side,
                           double amount, double price = 0, const json& params = json::object());
    json cancelLeverageOrder(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    json fetchLeveragePositions(const json& params = json::object());
    json fetchLeverageOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());

    // Async Leverage Trading API
    boost::future<json> fetchLeverageBalanceAsync(const json& params = json::object());
    boost::future<json> createLeverageOrderAsync(const std::string& symbol, const std::string& type, const std::string& side,
                                               double amount, double price = 0, const json& params = json::object());
    boost::future<json> cancelLeverageOrderAsync(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    boost::future<json> fetchLeveragePositionsAsync(const json& params = json::object());
    boost::future<json> fetchLeverageOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());

protected:
    std::string sign(const std::string& path, const std::string& api = "public",
               const std::string& method = "GET", const json& params = json::object(),
               const std::map<std::string, std::string>& headers = {}, const json& body = nullptr) override;

private:
    void initializeApiEndpoints();
    std::string getCoincheckSymbol(const std::string& symbol);
    std::string getCommonSymbol(const std::string& coincheckSymbol);
    json parseOrder(const json& order, const Market& market = Market());
    json parseTrade(const json& trade, const Market& market = Market());
    json parseOrderStatus(const std::string& status);
    json parseTicker(const json& ticker, const Market& market = Market());
    json parseOHLCV(const json& ohlcv, const Market& market = Market());
    json parseBalance(const json& response);
    json parseFee(const json& fee, const Market& market = Market());
    json parsePosition(const json& position, const Market& market = Market());
    json parseDepositAddress(const json& depositAddress, const std::string& currency = "");
    json parseTransaction(const json& transaction, const std::string& currency = "");
    std::string createSignature(const std::string& timestamp, const std::string& method,
                         const std::string& path, const std::string& body = "");
    std::string createNonce();

    std::map<std::string, std::string> timeframes;
    std::map<std::string, std::string> options;
    std::map<int, std::string> errorCodes;
};

} // namespace ccxt
