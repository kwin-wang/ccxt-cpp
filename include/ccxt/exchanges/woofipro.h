#ifndef CCXT_WOOFIPRO_H
#define CCXT_WOOFIPRO_H

#include "ccxt/base/exchange.h"
#include <future>

namespace ccxt {

class WooFiPro : public Exchange {
public:
    WooFiPro();
    ~WooFiPro() = default;

    // Market Data Methods - Sync
    json fetchMarkets(const json& params = json::object()) override;
    json fetchCurrencies(const json& params = json::object()) override;
    json fetchTime(const json& params = json::object()) override;
    json fetchTicker(const String& symbol, const json& params = json::object()) override;
    json fetchTickers(const std::vector<String>& symbols = {}, const json& params = json::object()) override;
    json fetchOrderBook(const String& symbol, int limit = 0, const json& params = json::object()) override;
    json fetchTrades(const String& symbol, int since = 0, int limit = 0,
                    const json& params = json::object()) override;
    json fetchOHLCV(const String& symbol, const String& timeframe = "1m",
                   int since = 0, int limit = 0, const json& params = json::object()) override;
    json fetchFundingRate(const String& symbol, const json& params = json::object()) override;
    json fetchFundingRates(const std::vector<String>& symbols = {}, const json& params = json::object()) override;
    json fetchFundingRateHistory(const String& symbol = "", int since = 0, int limit = 0,
                               const json& params = json::object()) override;

    // Trading Methods - Sync
    json createOrder(const String& symbol, const String& type, const String& side,
                    double amount, double price = 0, const json& params = json::object()) override;
    json createMarketBuyOrderWithCost(const String& symbol, double cost,
                                    const json& params = json::object()) override;
    json createMarketSellOrderWithCost(const String& symbol, double cost,
                                     const json& params = json::object()) override;
    json cancelOrder(const String& id, const String& symbol = "",
                    const json& params = json::object()) override;
    json cancelAllOrders(const String& symbol = "", const json& params = json::object()) override;
    json fetchOrder(const String& id, const String& symbol = "",
                   const json& params = json::object()) override;
    json fetchOrders(const String& symbol = "", int since = 0, int limit = 0,
                    const json& params = json::object()) override;
    json fetchOpenOrders(const String& symbol = "", int since = 0, int limit = 0,
                        const json& params = json::object()) override;
    json fetchClosedOrders(const String& symbol = "", int since = 0, int limit = 0,
                          const json& params = json::object()) override;
    json fetchMyTrades(const String& symbol = "", int since = 0, int limit = 0,
                      const json& params = json::object()) override;

    // Account Methods - Sync
    json fetchAccounts(const json& params = json::object()) override;
    json fetchBalance(const json& params = json::object()) override;
    json fetchLedger(const String& code = "", int since = 0, int limit = 0,
                    const json& params = json::object()) override;
    json fetchDepositAddress(const String& code, const json& params = json::object()) override;
    json fetchDeposits(const String& code = "", int since = 0, int limit = 0,
                      const json& params = json::object()) override;
    json fetchWithdrawals(const String& code = "", int since = 0, int limit = 0,
                         const json& params = json::object()) override;

    // Margin Trading Methods - Sync
    json addMargin(const String& symbol, double amount, const json& params = json::object()) override;
    json fetchLeverage(const String& symbol, const json& params = json::object()) override;
    json setLeverage(int leverage, const String& symbol = "",
                    const json& params = json::object()) override;

    // Market Data Methods - Async
    std::future<json> fetchMarketsAsync(const json& params = json::object());
    std::future<json> fetchCurrenciesAsync(const json& params = json::object());
    std::future<json> fetchTimeAsync(const json& params = json::object());
    std::future<json> fetchTickerAsync(const String& symbol, const json& params = json::object());
    std::future<json> fetchTickersAsync(const std::vector<String>& symbols = {},
                                      const json& params = json::object());
    std::future<json> fetchOrderBookAsync(const String& symbol, int limit = 0,
                                        const json& params = json::object());
    std::future<json> fetchTradesAsync(const String& symbol, int since = 0, int limit = 0,
                                     const json& params = json::object());
    std::future<json> fetchOHLCVAsync(const String& symbol, const String& timeframe = "1m",
                                    int since = 0, int limit = 0, const json& params = json::object());
    std::future<json> fetchFundingRateAsync(const String& symbol, const json& params = json::object());
    std::future<json> fetchFundingRatesAsync(const std::vector<String>& symbols = {},
                                           const json& params = json::object());
    std::future<json> fetchFundingRateHistoryAsync(const String& symbol = "", int since = 0,
                                                 int limit = 0, const json& params = json::object());

    // Trading Methods - Async
    std::future<json> createOrderAsync(const String& symbol, const String& type,
                                     const String& side, double amount, double price = 0,
                                     const json& params = json::object());
    std::future<json> createMarketBuyOrderWithCostAsync(const String& symbol, double cost,
                                                      const json& params = json::object());
    std::future<json> createMarketSellOrderWithCostAsync(const String& symbol, double cost,
                                                       const json& params = json::object());
    std::future<json> cancelOrderAsync(const String& id, const String& symbol = "",
                                     const json& params = json::object());
    std::future<json> cancelAllOrdersAsync(const String& symbol = "",
                                         const json& params = json::object());
    std::future<json> fetchOrderAsync(const String& id, const String& symbol = "",
                                    const json& params = json::object());
    std::future<json> fetchOrdersAsync(const String& symbol = "", int since = 0, int limit = 0,
                                     const json& params = json::object());
    std::future<json> fetchOpenOrdersAsync(const String& symbol = "", int since = 0, int limit = 0,
                                         const json& params = json::object());
    std::future<json> fetchClosedOrdersAsync(const String& symbol = "", int since = 0, int limit = 0,
                                           const json& params = json::object());
    std::future<json> fetchMyTradesAsync(const String& symbol = "", int since = 0, int limit = 0,
                                        const json& params = json::object());

    // Account Methods - Async
    std::future<json> fetchAccountsAsync(const json& params = json::object());
    std::future<json> fetchBalanceAsync(const json& params = json::object());
    std::future<json> fetchLedgerAsync(const String& code = "", int since = 0, int limit = 0,
                                     const json& params = json::object());
    std::future<json> fetchDepositAddressAsync(const String& code,
                                             const json& params = json::object());
    std::future<json> fetchDepositsAsync(const String& code = "", int since = 0, int limit = 0,
                                       const json& params = json::object());
    std::future<json> fetchWithdrawalsAsync(const String& code = "", int since = 0, int limit = 0,
                                          const json& params = json::object());

    // Margin Trading Methods - Async
    std::future<json> addMarginAsync(const String& symbol, double amount,
                                   const json& params = json::object());
    std::future<json> fetchLeverageAsync(const String& symbol,
                                       const json& params = json::object());
    std::future<json> setLeverageAsync(int leverage, const String& symbol = "",
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
    json parseDepositAddress(const json& depositAddress, const String& currency = "");
    json parseTransaction(const json& transaction, const String& currency = "");
    String parseOrderStatus(const String& status);
    String parseTimeInForce(const String& timeInForce);
    String sign(const String& path, const String& api = "public",
               const String& method = "GET", const json& params = json::object(),
               const json& headers = nullptr, const String& body = "") override;

private:
    // Async helper methods
    template<typename Func, typename... Args>
    std::future<json> async(Func&& func, Args&&... args) {
        return std::async(std::launch::async,
                         std::forward<Func>(func),
                         this,
                         std::forward<Args>(args)...);
    }

    // API endpoint versions
    String publicApiVersion;
    String privateApiVersion;
    String v1;
    String v2;
    String hostname;
};

} // namespace ccxt

#endif // CCXT_WOOFIPRO_H
