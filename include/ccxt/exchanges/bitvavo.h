#pragma once

#include "ccxt/base/exchange.h"

namespace ccxt {

class Bitvavo : public Exchange {
public:
    Bitvavo();
    ~Bitvavo() override = default;

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

    // Additional Features
    json fetchCurrencies(const json& params = json::object());
    json fetchTradingFees(const json& params = json::object());
    json fetchFundingFees(const json& params = json::object());
    json fetchLedger(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());

    // Async Additional Features
    boost::future<json> fetchCurrenciesAsync(const json& params = json::object());
    boost::future<json> fetchTradingFeesAsync(const json& params = json::object());
    boost::future<json> fetchFundingFeesAsync(const json& params = json::object());
    boost::future<json> fetchLedgerAsync(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());

protected:
    std::string sign(const std::string& path, const std::string& api = "public",
               const std::string& method = "GET", const json& params = json::object(),
               const std::map<std::string, std::string>& headers = {}, const json& body = nullptr) override;

private:
    void initializeApiEndpoints();
    void initializeTimeframes();
    void initializeMarketTypes();
    void initializeOptions();
    void initializeErrorCodes();
    void initializeFees();

    std::string getBitvavoSymbol(const std::string& symbol);
    std::string getCommonSymbol(const std::string& bitvavoSymbol);
    std::string parseOrderStatus(const std::string& status);
    std::string parseOrderType(const std::string& type);
    std::string parseOrderSide(const std::string& side);
    std::string parseTimeInForce(const std::string& timeInForce);

    json parseOrder(const json& order, const Market& market = Market());
    json parseTrade(const json& trade, const Market& market = Market());
    json parseTicker(const json& ticker, const Market& market = Market());
    json parseOHLCV(const json& ohlcv, const Market& market = Market(), const std::string& timeframe = "1m");
    json parseBalance(const json& response);
    json parseFee(const json& fee, const Market& market = Market());
    json parseTransaction(const json& transaction, const std::string& currency = "");
    json parseDepositAddress(const json& depositAddress, const std::string& currency = "");

    std::string createSignature(const std::string& timestamp, const std::string& method,
                         const std::string& path, const std::string& body = "");
    std::string getNonce();

    std::map<std::string, std::string> timeframes;
    std::map<std::string, std::string> marketTypes;
    std::map<std::string, std::string> options;
    std::map<int, std::string> errorCodes;
    std::map<std::string, json> fees;
    bool hasPublicAPI;
    bool hasPrivateAPI;
    bool hasFiatAPI;
    bool hasMarginAPI;
    bool hasFuturesAPI;
};

} // namespace ccxt
