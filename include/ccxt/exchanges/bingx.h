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
    json fetchTicker(const String& symbol, const json& params = json::object()) override;
    json fetchTickers(const std::vector<String>& symbols = {}, const json& params = json::object()) override;
    json fetchOrderBook(const String& symbol, int limit = 0, const json& params = json::object()) override;
    json fetchTrades(const String& symbol, int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchOHLCV(const String& symbol, const String& timeframe = "1m",
                    int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchTime(const json& params = json::object()) override;
    json fetchTradingFee(const String& symbol, const json& params = json::object()) override;

    // Synchronous Trading API
    json fetchBalance(const json& params = json::object()) override;
    json createOrder(const String& symbol, const String& type, const String& side,
                    double amount, double price = 0, const json& params = json::object()) override;
    json cancelOrder(const String& id, const String& symbol = "", const json& params = json::object()) override;
    json fetchOrder(const String& id, const String& symbol = "", const json& params = json::object()) override;
    json fetchOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchOpenOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchClosedOrders(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchMyTrades(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object()) override;

    // Synchronous Account API
    json fetchDeposits(const String& code = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchWithdrawals(const String& code = "", int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchDepositAddress(const String& code, const json& params = json::object()) override;
    json withdraw(const String& code, double amount, const String& address, const String& tag = "", const json& params = json::object()) override;
    json transfer(const String& code, double amount, const String& fromAccount, const String& toAccount, const json& params = json::object()) override;
    json fetchTransfers(const String& code = "", int since = 0, int limit = 0, const json& params = json::object()) override;

    // Asynchronous Market Data API
    boost::future<json> fetchMarketsAsync(const json& params = json::object());
    boost::future<json> fetchTickerAsync(const String& symbol, const json& params = json::object());
    boost::future<json> fetchTickersAsync(const std::vector<String>& symbols = {}, const json& params = json::object());
    boost::future<json> fetchOrderBookAsync(const String& symbol, int limit = 0, const json& params = json::object());
    boost::future<json> fetchTradesAsync(const String& symbol, int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchOHLCVAsync(const String& symbol, const String& timeframe = "1m",
                                      int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchTimeAsync(const json& params = json::object());
    boost::future<json> fetchTradingFeeAsync(const String& symbol, const json& params = json::object());

    // Asynchronous Trading API
    boost::future<json> fetchBalanceAsync(const json& params = json::object());
    boost::future<json> createOrderAsync(const String& symbol, const String& type, const String& side,
                                      double amount, double price = 0, const json& params = json::object());
    boost::future<json> cancelOrderAsync(const String& id, const String& symbol = "", const json& params = json::object());
    boost::future<json> fetchOrderAsync(const String& id, const String& symbol = "", const json& params = json::object());
    boost::future<json> fetchOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchOpenOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchClosedOrdersAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchMyTradesAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());

    // Asynchronous Account API
    boost::future<json> fetchDepositsAsync(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchWithdrawalsAsync(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> fetchDepositAddressAsync(const String& code, const json& params = json::object());
    boost::future<json> withdrawAsync(const String& code, double amount, const String& address, const String& tag = "", const json& params = json::object());
    boost::future<json> transferAsync(const String& code, double amount, const String& fromAccount, const String& toAccount, const json& params = json::object());
    boost::future<json> fetchTransfersAsync(const String& code = "", int since = 0, int limit = 0, const json& params = json::object());

    // Perpetual Swap API (both sync and async)
    json fetchPerpetualMarkets(const json& params = json::object());
    json fetchPerpetualBalance(const json& params = json::object());
    json createPerpetualOrder(const String& symbol, const String& type, const String& side,
                           double amount, double price = 0, const json& params = json::object());
    json cancelPerpetualOrder(const String& id, const String& symbol = "", const json& params = json::object());
    json fetchPerpetualPosition(const String& symbol = "", const json& params = json::object());
    json fetchPerpetualPositions(const std::vector<String>& symbols = {}, const json& params = json::object());
    json fetchPerpetualFundingRate(const String& symbol, const json& params = json::object());
    json fetchPerpetualFundingHistory(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    json setLeverage(int leverage, const String& symbol = "", const json& params = json::object());
    json setMarginMode(const String& marginMode, const String& symbol = "", const json& params = json::object());
    json setPositionMode(const String& hedged, const String& symbol = "", const json& params = json::object());

    boost::future<json> fetchPerpetualMarketsAsync(const json& params = json::object());
    boost::future<json> fetchPerpetualBalanceAsync(const json& params = json::object());
    boost::future<json> createPerpetualOrderAsync(const String& symbol, const String& type, const String& side,
                                               double amount, double price = 0, const json& params = json::object());
    boost::future<json> cancelPerpetualOrderAsync(const String& id, const String& symbol = "", const json& params = json::object());
    boost::future<json> fetchPerpetualPositionAsync(const String& symbol = "", const json& params = json::object());
    boost::future<json> fetchPerpetualPositionsAsync(const std::vector<String>& symbols = {}, const json& params = json::object());
    boost::future<json> fetchPerpetualFundingRateAsync(const String& symbol, const json& params = json::object());
    boost::future<json> fetchPerpetualFundingHistoryAsync(const String& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    boost::future<json> setLeverageAsync(int leverage, const String& symbol = "", const json& params = json::object());
    boost::future<json> setMarginModeAsync(const String& marginMode, const String& symbol = "", const json& params = json::object());
    boost::future<json> setPositionModeAsync(const String& hedged, const String& symbol = "", const json& params = json::object());

    // Copy Trading API (both sync and async)
    json fetchCopyTradingPositions(const json& params = json::object());
    json createCopyTradingOrder(const String& symbol, const String& type, const String& side,
                             double amount, double price = 0, const json& params = json::object());
    json cancelCopyTradingOrder(const String& id, const String& symbol = "", const json& params = json::object());
    json fetchCopyTradingBalance(const json& params = json::object());

    boost::future<json> fetchCopyTradingPositionsAsync(const json& params = json::object());
    boost::future<json> createCopyTradingOrderAsync(const String& symbol, const String& type, const String& side,
                                                 double amount, double price = 0, const json& params = json::object());
    boost::future<json> cancelCopyTradingOrderAsync(const String& id, const String& symbol = "", const json& params = json::object());
    boost::future<json> fetchCopyTradingBalanceAsync(const json& params = json::object());

protected:
    String sign(const String& path, const String& api = "public",
               const String& method = "GET", const json& params = json::object(),
               const std::map<String, String>& headers = {}, const json& body = nullptr) override;

private:
    void initializeApiEndpoints();
    String getBingXSymbol(const String& symbol);
    String getCommonSymbol(const String& bingXSymbol);
    json parseOrder(const json& order, const Market& market = Market());
    json parseTrade(const json& trade, const Market& market = Market());
    json parseOrderStatus(const String& status);
    json parseTicker(const json& ticker, const Market& market = Market());
    json parseOHLCV(const json& ohlcv, const Market& market = Market());
    json parseBalance(const json& response);
    json parseFee(const json& fee, const Market& market = Market());
    json parsePosition(const json& position, const Market& market = Market());
    json parseFundingRate(const json& fundingRate, const Market& market = Market());
    json parseDepositAddress(const json& depositAddress, const String& currency = "");
    json parseTransaction(const json& transaction, const String& currency = "");
    String createSignature(const String& timestamp, const String& method,
                         const String& path, const String& queryString);

    // Asynchronous helpers
    boost::future<json> makeAsyncRequest(const String& path, const String& api,
                                      const String& method, const json& params = json::object(),
                                      const std::map<String, String>& headers = {}, const json& body = nullptr);

    std::map<String, String> timeframes;
    std::map<String, String> options;
    std::map<int, String> errorCodes;
    boost::asio::io_context io_context;
};

} // namespace ccxt
