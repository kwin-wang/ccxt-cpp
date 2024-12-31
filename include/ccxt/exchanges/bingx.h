#pragma once

#include "ccxt/base/exchange.h"
#include <boost/asio.hpp>
#include <boost/future.hpp>

namespace ccxt {

class BingX : public Exchange {
public:
    BingX();
    ~BingX() override = default;

    // Synchronous Market Data API
    json fetchMarkets(const json& params = json::object()) override;
    json fetchTicker(const std::string& symbol, const json& params = json::object()) override;
    json fetchTickers(const std::vector<std::string>& symbols = {}, const json& params = json::object()) override;
    json fetchOrderBook(const std::string& symbol, int limit = 0, const json& params = json::object()) override;
    json fetchTrades(const std::string& symbol, int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchOHLCV(const std::string& symbol, const std::string& timeframe = "1m",
                    int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchTime(const json& params = json::object()) override;
    json fetchTradingFee(const std::string& symbol, const json& params = json::object()) override;

    // Synchronous Trading API
    json fetchBalance(const json& params = json::object()) override;
    json createOrder(const std::string& symbol, const std::string& type, const std::string& side,
                    double amount, double price = 0, const json& params = json::object()) override;
    json cancelOrder(const std::string& id, const std::string& symbol = "", const json& params = json::object()) override;
    json fetchOrder(const std::string& id, const std::string& symbol = "", const json& params = json::object()) override;
    json fetchOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchOpenOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchClosedOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchMyTrades(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;

    // Synchronous Account API
    json fetchDeposits(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchWithdrawals(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchDepositAddress(const std::string& code, const json& params = json::object()) override;
    json withdraw(const std::string& code, double amount, const std::string& address, const std::string& tag = "", const json& params = json::object()) override;
    json transfer(const std::string& code, double amount, const std::string& fromAccount, const std::string& toAccount, const json& params = json::object()) override;
    json fetchTransfers(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object()) override;

    // Asynchronous Market Data API
    boost::future<json> fetchMarketsAsync(const json& params = json::object());
    boost::future<json> fetchTickerAsync(const std::string& symbol, const json& params = json::object());
    boost::future<json> fetchTickersAsync(const std::vector<std::string>& symbols = {}, const json& params = json::object());
    boost::future<json> fetchOrderBookAsync(const std::string& symbol, int limit = 0, const json& params = json::object());
    boost::future<json> fetchTradesAsync(const std::string& symbol, int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchOHLCVAsync(const std::string& symbol, const std::string& timeframe = "1m",
                                      int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchTimeAsync(const json& params = json::object());
    boost::future<json> fetchTradingFeeAsync(const std::string& symbol, const json& params = json::object());

    // Asynchronous Trading API
    boost::future<json> fetchBalanceAsync(const json& params = json::object());
    boost::future<json> createOrderAsync(const std::string& symbol, const std::string& type, const std::string& side,
                                      double amount, double price = 0, const json& params = json::object());
    boost::future<json> cancelOrderAsync(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    boost::future<json> fetchOrderAsync(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    boost::future<json> fetchOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchOpenOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchClosedOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchMyTradesAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());

    // Asynchronous Account API
    boost::future<json> fetchDepositsAsync(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchWithdrawalsAsync(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchDepositAddressAsync(const std::string& code, const json& params = json::object());
    boost::future<json> withdrawAsync(const std::string& code, double amount, const std::string& address, const std::string& tag = "", const json& params = json::object());
    boost::future<json> transferAsync(const std::string& code, double amount, const std::string& fromAccount, const std::string& toAccount, const json& params = json::object());
    boost::future<json> fetchTransfersAsync(const std::string& code = "", int since = 0, int limit = 0, const json& params = json::object());

    // Perpetual Swap API (both sync and async)
    json fetchPerpetualMarkets(const json& params = json::object());
    json fetchPerpetualBalance(const json& params = json::object());
    json createPerpetualOrder(const std::string& symbol, const std::string& type, const std::string& side,
                           double amount, double price = 0, const json& params = json::object());
    json cancelPerpetualOrder(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    json fetchPerpetualPosition(const std::string& symbol = "", const json& params = json::object());
    json fetchPerpetualPositions(const std::vector<std::string>& symbols = {}, const json& params = json::object());
    json fetchPerpetualFundingRate(const std::string& symbol, const json& params = json::object());
    json fetchPerpetualFundingHistory(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json setLeverage(int leverage, const std::string& symbol = "", const json& params = json::object());
    json setMarginMode(const std::string& marginMode, const std::string& symbol = "", const json& params = json::object());
    json setPositionMode(const std::string& hedged, const std::string& symbol = "", const json& params = json::object());

    boost::future<json> fetchPerpetualMarketsAsync(const json& params = json::object());
    boost::future<json> fetchPerpetualBalanceAsync(const json& params = json::object());
    boost::future<json> createPerpetualOrderAsync(const std::string& symbol, const std::string& type, const std::string& side,
                                               double amount, double price = 0, const json& params = json::object());
    boost::future<json> cancelPerpetualOrderAsync(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    boost::future<json> fetchPerpetualPositionAsync(const std::string& symbol = "", const json& params = json::object());
    boost::future<json> fetchPerpetualPositionsAsync(const std::vector<std::string>& symbols = {}, const json& params = json::object());
    boost::future<json> fetchPerpetualFundingRateAsync(const std::string& symbol, const json& params = json::object());
    boost::future<json> fetchPerpetualFundingHistoryAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> setLeverageAsync(int leverage, const std::string& symbol = "", const json& params = json::object());
    boost::future<json> setMarginModeAsync(const std::string& marginMode, const std::string& symbol = "", const json& params = json::object());
    boost::future<json> setPositionModeAsync(const std::string& hedged, const std::string& symbol = "", const json& params = json::object());

    // Copy Trading API (both sync and async)
    json fetchCopyTradingPositions(const json& params = json::object());
    json createCopyTradingOrder(const std::string& symbol, const std::string& type, const std::string& side,
                             double amount, double price = 0, const json& params = json::object());
    json cancelCopyTradingOrder(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    json fetchCopyTradingBalance(const json& params = json::object());

    boost::future<json> fetchCopyTradingPositionsAsync(const json& params = json::object());
    boost::future<json> createCopyTradingOrderAsync(const std::string& symbol, const std::string& type, const std::string& side,
                                                 double amount, double price = 0, const json& params = json::object());
    boost::future<json> cancelCopyTradingOrderAsync(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    boost::future<json> fetchCopyTradingBalanceAsync(const json& params = json::object());

protected:
    std::string sign(const std::string& path, const std::string& api = "public",
               const std::string& method = "GET", const json& params = json::object(),
               const std::map<std::string, std::string>& headers = {}, const json& body = nullptr) override;

private:
    void initializeApiEndpoints();
    std::string getBingXSymbol(const std::string& symbol);
    std::string getCommonSymbol(const std::string& bingXSymbol);
    json parseOrder(const json& order, const Market& market = Market());
    json parseTrade(const json& trade, const Market& market = Market());
    json parseOrderStatus(const std::string& status);
    json parseTicker(const json& ticker, const Market& market = Market());
    json parseOHLCV(const json& ohlcv, const Market& market = Market());
    json parseBalance(const json& response);
    json parseFee(const json& fee, const Market& market = Market());
    json parsePosition(const json& position, const Market& market = Market());
    json parseFundingRate(const json& fundingRate, const Market& market = Market());
    json parseDepositAddress(const json& depositAddress, const std::string& currency = "");
    json parseTransaction(const json& transaction, const std::string& currency = "");
    std::string createSignature(const std::string& timestamp, const std::string& method,
                         const std::string& path, const std::string& querystd::string);

    // Asynchronous helpers
    boost::future<json> makeAsyncRequest(const std::string& path, const std::string& api,
                                      const std::string& method, const json& params = json::object(),
                                      const std::map<std::string, std::string>& headers = {}, const json& body = nullptr);

    std::map<std::string, std::string> timeframes;
    std::map<std::string, std::string> options;
    std::map<int, std::string> errorCodes;
    boost::asio::io_context io_context;
};

} // namespace ccxt
