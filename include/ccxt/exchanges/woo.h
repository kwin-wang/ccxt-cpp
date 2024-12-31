#ifndef CCXT_WOO_H
#define CCXT_WOO_H

#include "ccxt/base/exchange.h"
#include <future>

namespace ccxt {

class Woo : public Exchange {
public:
    Woo();
    ~Woo() = default;

    // Market Data Methods - Sync
    json fetchMarkets(const json& params = json::object()) override;
    json fetchCurrencies(const json& params = json::object()) override;
    json fetchTime(const json& params = json::object()) override;
    json fetchTicker(const std::string& symbol, const json& params = json::object()) override;
    json fetchTickers(const std::vector<std::string>& symbols = {}, const json& params = json::object()) override;
    json fetchOrderBook(const std::string& symbol, int limit = 0, const json& params = json::object()) override;
    json fetchTrades(const std::string& symbol, int since = 0, int limit = 0,
                    const json& params = json::object()) override;
    json fetchOHLCV(const std::string& symbol, const std::string& timeframe = "1m",
                    int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchFundingRate(const std::string& symbol, const json& params = json::object()) override;
    json fetchFundingRates(const std::vector<std::string>& symbols = {}, const json& params = json::object()) override;
    json fetchFundingRateHistory(const std::string& symbol = "", int since = 0, int limit = 0,
                               const json& params = json::object()) override;

    // Trading Methods - Sync
    json createOrder(const std::string& symbol, const std::string& type, const std::string& side,
                    double amount, double price = 0, const json& params = json::object()) override;
    json createMarketBuyOrderWithCost(const std::string& symbol, double cost,
                                    const json& params = json::object()) override;
    json createMarketSellOrderWithCost(const std::string& symbol, double cost,
                                     const json& params = json::object()) override;
    json createTakeProfitOrder(const std::string& symbol, const std::string& type, const std::string& side,
                             double amount, double price = 0, const json& params = json::object());
    json createStopLossOrder(const std::string& symbol, const std::string& type, const std::string& side,
                           double amount, double price = 0, const json& params = json::object());
    json createTrailingAmountOrder(const std::string& symbol, const std::string& type, const std::string& side,
                                 double amount, double trailingAmount, const json& params = json::object());
    json createTrailingPercentOrder(const std::string& symbol, const std::string& type, const std::string& side,
                                  double amount, double trailingPercent, const json& params = json::object());
    json cancelOrder(const std::string& id, const std::string& symbol = "",
                    const json& params = json::object()) override;
    json cancelAllOrders(const std::string& symbol = "", const json& params = json::object()) override;
    json fetchOrder(const std::string& id, const std::string& symbol = "",
                   const json& params = json::object()) override;
    json fetchOrders(const std::string& symbol = "", int since = 0, int limit = 0,
                    const json& params = json::object()) override;
    json fetchOpenOrders(const std::string& symbol = "", int since = 0, int limit = 0,
                        const json& params = json::object()) override;
    json fetchClosedOrders(const std::string& symbol = "", int since = 0, int limit = 0,
                          const json& params = json::object()) override;
    json fetchMyTrades(const std::string& symbol = "", int since = 0, int limit = 0,
                      const json& params = json::object()) override;

    // Account Methods - Sync
    json fetchAccounts(const json& params = json::object()) override;
    json fetchBalance(const json& params = json::object()) override;
    json fetchLedger(const std::string& code = "", int since = 0, int limit = 0,
                    const json& params = json::object()) override;
    json fetchDepositAddress(const std::string& code, const json& params = json::object()) override;
    json fetchDeposits(const std::string& code = "", int since = 0, int limit = 0,
                      const json& params = json::object()) override;
    json fetchWithdrawals(const std::string& code = "", int since = 0, int limit = 0,
                         const json& params = json::object()) override;

    // Margin Trading Methods - Sync
    json addMargin(const std::string& symbol, double amount, const json& params = json::object()) override;
    json fetchLeverage(const std::string& symbol, const json& params = json::object()) override;
    json setLeverage(int leverage, const std::string& symbol = "",
                    const json& params = json::object()) override;

    // Convert Methods - Sync
    json fetchConvertCurrencies(const json& params = json::object());
    json fetchConvertQuote(const std::string& fromCurrency, const std::string& toCurrency,
                         double amount, const json& params = json::object());
    json createConvertTrade(const std::string& fromCurrency, const std::string& toCurrency,
                          double amount, const json& params = json::object());
    json fetchConvertTrade(const std::string& id, const json& params = json::object());
    json fetchConvertTradeHistory(const std::string& fromCurrency = "", const std::string& toCurrency = "",
                                int since = 0, int limit = 0, const json& params = json::object());

    // Market Data Methods - Async
    AsyncPullType fetchMarketsAsync(const json& params = json::object());
    AsyncPullType fetchCurrenciesAsync(const json& params = json::object());
    AsyncPullType fetchTimeAsync(const json& params = json::object());
    AsyncPullType fetchTickerAsync(const std::string& symbol, const json& params = json::object());
    AsyncPullType fetchTickersAsync(const std::vector<std::string>& symbols = {},
                                      const json& params = json::object());
    AsyncPullType fetchOrderBookAsync(const std::string& symbol, int limit = 0,
                                        const json& params = json::object());
    AsyncPullType fetchTradesAsync(const std::string& symbol, int since = 0, int limit = 0,
                                     const json& params = json::object());
    AsyncPullType fetchOHLCVAsync(const std::string& symbol, const std::string& timeframe = "1m",
                                    int since = 0, int limit = 0, const json& params = json::object());
    AsyncPullType fetchFundingRateAsync(const std::string& symbol, const json& params = json::object());
    AsyncPullType fetchFundingRatesAsync(const std::vector<std::string>& symbols = {},
                                           const json& params = json::object());
    AsyncPullType fetchFundingRateHistoryAsync(const std::string& symbol = "", int since = 0,
                                                 int limit = 0, const json& params = json::object());

    // Trading Methods - Async
    AsyncPullType createOrderAsync(const std::string& symbol, const std::string& type, const std::string& side,
                                     double amount, double price = 0, const json& params = json::object());
    AsyncPullType createMarketBuyOrderWithCostAsync(const std::string& symbol, double cost,
                                                      const json& params = json::object());
    AsyncPullType createMarketSellOrderWithCostAsync(const std::string& symbol, double cost,
                                                       const json& params = json::object());
    AsyncPullType createTakeProfitOrderAsync(const std::string& symbol, const std::string& type,
                                               const std::string& side, double amount, double price = 0,
                                               const json& params = json::object());
    AsyncPullType createStopLossOrderAsync(const std::string& symbol, const std::string& type,
                                             const std::string& side, double amount, double price = 0,
                                             const json& params = json::object());
    AsyncPullType createTrailingAmountOrderAsync(const std::string& symbol, const std::string& type,
                                                   const std::string& side, double amount,
                                                   double trailingAmount,
                                                   const json& params = json::object());
    AsyncPullType createTrailingPercentOrderAsync(const std::string& symbol, const std::string& type,
                                                    const std::string& side, double amount,
                                                    double trailingPercent,
                                                    const json& params = json::object());
    AsyncPullType cancelOrderAsync(const std::string& id, const std::string& symbol = "",
                                     const json& params = json::object());
    AsyncPullType cancelAllOrdersAsync(const std::string& symbol = "",
                                         const json& params = json::object());
    AsyncPullType fetchOrderAsync(const std::string& id, const std::string& symbol = "",
                                    const json& params = json::object());
    AsyncPullType fetchOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0,
                                     const json& params = json::object());
    AsyncPullType fetchOpenOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0,
                                         const json& params = json::object());
    AsyncPullType fetchClosedOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0,
                                           const json& params = json::object());
    AsyncPullType fetchMyTradesAsync(const std::string& symbol = "", int since = 0, int limit = 0,
                                        const json& params = json::object());

    // Account Methods - Async
    AsyncPullType fetchAccountsAsync(const json& params = json::object());
    AsyncPullType fetchBalanceAsync(const json& params = json::object());
    AsyncPullType fetchLedgerAsync(const std::string& code = "", int since = 0, int limit = 0,
                                     const json& params = json::object());
    AsyncPullType fetchDepositAddressAsync(const std::string& code,
                                             const json& params = json::object());
    AsyncPullType fetchDepositsAsync(const std::string& code = "", int since = 0, int limit = 0,
                                       const json& params = json::object());
    AsyncPullType fetchWithdrawalsAsync(const std::string& code = "", int since = 0, int limit = 0,
                                          const json& params = json::object());

    // Margin Trading Methods - Async
    AsyncPullType addMarginAsync(const std::string& symbol, double amount,
                                   const json& params = json::object());
    AsyncPullType fetchLeverageAsync(const std::string& symbol,
                                       const json& params = json::object());
    AsyncPullType setLeverageAsync(int leverage, const std::string& symbol = "",
                                     const json& params = json::object());

    // Convert Methods - Async
    AsyncPullType fetchConvertCurrenciesAsync(const json& params = json::object());
    AsyncPullType fetchConvertQuoteAsync(const std::string& fromCurrency,
                                           const std::string& toCurrency,
                                           double amount,
                                           const json& params = json::object());
    AsyncPullType createConvertTradeAsync(const std::string& fromCurrency,
                                            const std::string& toCurrency,
                                            double amount,
                                            const json& params = json::object());
    AsyncPullType fetchConvertTradeAsync(const std::string& id,
                                           const json& params = json::object());
    AsyncPullType fetchConvertTradeHistoryAsync(const std::string& fromCurrency = "",
                                                  const std::string& toCurrency = "",
                                                  int since = 0, int limit = 0,
                                                  const json& params = json::object());

protected:
    // Helper Methods
    void initializeApiEndpoints();
    json parseTicker(const json& ticker, const Market& market);
    json parseOrder(const json& order, const Market& market = Market());
    json parseTrade(const json& trade, const Market& market = Market());
    json parseOHLCV(const json& ohlcv, const Market& market);
    json parseBalance(const json& response);
    json parsePosition(const json& position, const Market& market = Market());
    json parseFundingRate(const json& fundingRate, const Market& market = Market());
    json parseLedgerEntry(const json& item, const Currency& currency = Currency());
    json parseDepositAddress(const json& depositAddress, const std::string& currency = "");
    json parseTransaction(const json& transaction, const std::string& currency = "");
    json parseConvertTrade(const json& trade);
    std::string parseOrderStatus(const std::string& status);
    std::string parseTimeInForce(const std::string& timeInForce);
    std::string sign(const std::string& path, const std::string& api = "public",
               const std::string& method = "GET", const json& params = json::object(),
               const json& headers = nullptr, const std::string& body = "") override;

private:
    // Async helper methods
    template<typename Func, typename... Args>
    AsyncPullType async(Func&& func, Args&&... args) {
        return std::async(std::launch::async,
                         std::forward<Func>(func),
                         this,
                         std::forward<Args>(args)...);
    }

    // API endpoint versions
    std::string publicApiVersion;
    std::string privateApiVersion;
    std::string v1;
    std::string v2;
    std::string hostname;
};

} // namespace ccxt

#endif // CCXT_WOO_H
