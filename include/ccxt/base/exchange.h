#pragma once

#include <string>
#include <map>
#include <vector>
#include <optional>
#include <future>
#include <nlohmann/json.hpp>
#include <boost/coroutine2/coroutine.hpp>
#include "ccxt/base/exchange_base.h"

namespace ccxt {
class Exchange : public ExchangeBase {
public:
    Exchange(boost::asio::io_context& context, const Config& config = Config());
    virtual ~Exchange() = default;

    // Common methods
    virtual void init();
    virtual void describe() const;
    virtual AsyncPullType performHttpRequest(const std::string& host, const std::string& target, const std::string& method);
    // Usually methods
    virtual std::string implodeParams(const std::string& path, const json& params);
    virtual json omit(const json& params, const std::vector<std::string>& keys);
    virtual std::vector<std::string> extractParams(const std::string& path);
    virtual std::string urlencode(const json& params);
    virtual std::string encode(const std::string& string);
    virtual std::string hmac(const std::string& message, const std::string& secret,
                     const std::string& algorithm, const std::string& digest);
    virtual std::string costToPrecision(const std::string& symbol, double cost);
    virtual std::string priceToPrecision(const std::string& symbol, double price);
    virtual std::string amountToPrecision(const std::string& symbol, double amount);
    virtual std::string currencyToPrecision(const std::string& currency, double fee);
    virtual std::string feeToPrecision(const std::string& symbol, double fee);
    virtual long long parse8601(const std::string& datetime);
    virtual long long milliseconds() const;
    virtual std::string uuid();
    virtual std::string iso8601(long long timestamp) const;
    virtual Market market(const std::string& symbol);
    virtual std::string marketId(const std::string& symbol);

    // Synchronous REST API methods
    virtual json fetchMarkets(const json& params = json::object());
    virtual json fetchTicker(const std::string& symbol, const json& params = json::object());
    virtual json fetchTickers(const std::vector<std::string>& symbols = {}, const json& params = json::object());
    virtual json fetchOrderBook(const std::string& symbol, int limit = 0, const json& params = json::object());
    virtual json fetchTrades(const std::string& symbol, int since = 0, int limit = 0, const json& params = json::object());
    virtual json fetchOHLCV(const std::string& symbol, const std::string& timeframe = "1m",
                          int since = 0, int limit = 0, const json& params = json::object());
    virtual json fetchBalance(const json& params = json::object());
    virtual json createOrder(const std::string& symbol, const std::string& type, const std::string& side,
                           double amount, double price = 0, const json& params = json::object());
    virtual json cancelOrder(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    virtual json fetchOrder(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    virtual json fetchOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    virtual json fetchOpenOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    virtual json fetchClosedOrders(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    virtual void loadMarkets(bool reload = false);
    std::string symbol(const std::string& marketId);

    // Asynchronous REST API methods
    virtual AsyncPullType fetchMarketsAsync(const json& params = json::object());
    virtual AsyncPullType fetchTickerAsync(const std::string& symbol, const json& params = json::object());
    virtual AsyncPullType fetchTickersAsync(const std::vector<std::string>& symbols = {}, const json& params = json::object());
    virtual AsyncPullType fetchOrderBookAsync(const std::string& symbol, int limit = 0, const json& params = json::object());
    virtual AsyncPullType fetchTradesAsync(const std::string& symbol, int since = 0, int limit = 0, const json& params = json::object());
    virtual AsyncPullType fetchOHLCVAsync(const std::string& symbol, const std::string& timeframe = "1m",
                                           int since = 0, int limit = 0, const json& params = json::object());
    virtual AsyncPullType fetchBalanceAsync(const json& params = json::object());
    virtual AsyncPullType createOrderAsync(const std::string& symbol, const std::string& type, const std::string& side,
                                           double amount, double price = 0, const json& params = json::object());
    virtual AsyncPullType cancelOrderAsync(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    virtual AsyncPullType fetchOrderAsync(const std::string& id, const std::string& symbol = "", const json& params = json::object());
    virtual AsyncPullType fetchOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    virtual AsyncPullType fetchOpenOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());
    virtual AsyncPullType fetchClosedOrdersAsync(const std::string& symbol = "", int since = 0, int limit = 0, const json& params = json::object());

protected:
    // Synchronous HTTP methods
    virtual json fetch(const std::string& url,
                      const std::string& method = "GET",
                      const std::map<std::string, std::string>& headers = {},
                      const std::string& body = "");

    // Asynchronous HTTP methods
    virtual AsyncPullType fetchAsync(const std::string& url,
                                     const std::string& method = "GET",
                                     const std::map<std::string, std::string>& headers = {},
                                     const std::string& body = "");

    // Utility methods
    virtual std::string sign(const std::string& path, const std::string& api = "public",
                     const std::string& method = "GET",
                     const std::map<std::string, std::string>& params = {},
                     const std::map<std::string, std::string>& headers = {});

    // Safe type conversion helpers
    std::string safeString(const json& obj, const std::string& key, const std::string& defaultValue = "") const;
    double safeNumber(const json& obj, const std::string& key, double defaultValue = 0.0) const;
    long long safeInteger(const json& obj, const std::string& key, long long defaultValue = 0) const;
    bool safeBoolean(const json& obj, const std::string& key, bool defaultValue = false) const;

    // Parsing methods
    virtual json parseMarket(const json& market) const = 0;
    virtual json parseTicker(const json& ticker, const Market& market = Market()) const = 0;
    virtual json parseOrderBook(const json& orderbook, const std::string& symbol = "", const Market& market = Market()) const = 0;
    virtual json parseOHLCV(const json& ohlcv, const Market& market = Market(), const std::string& timeframe = "1m") const = 0;
    virtual json parseOrder(const json& order, const Market& market = Market()) const = 0;
    virtual json parseTrade(const json& trade, const Market& market = Market()) const = 0;
    virtual json parseBalance(const json& balance) const = 0;
    virtual json parseFee(const json& fee, const Market& market = Market()) const = 0;
    virtual json parsePosition(const json& position, const Market& market = Market()) const = 0;
    virtual json parseFundingRate(const json& fundingRate, const Market& market = Market()) const = 0;
    virtual json parseTransaction(const json& transaction, const std::string& currency = "") const = 0;
    virtual json parseDepositAddress(const json& depositAddress, const std::string& currency = "") const = 0;
    virtual json parseWithdrawal(const json& withdrawal, const std::string& currency = "") const = 0;
    virtual json parseDeposit(const json& deposit, const std::string& currency = "") const = 0;

    // Protected virtual functions that derived classes must implement
    virtual json fetchMarketsImpl() const = 0;
    virtual json fetchTickerImpl(const std::string& symbol) const = 0;
    virtual json fetchTickersImpl(const std::vector<std::string>& symbols = {}) const = 0;
    virtual json fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit = std::nullopt) const = 0;
    virtual json fetchTradesImpl(const std::string& symbol, const std::optional<long long>& since = std::nullopt,
                              const std::optional<int>& limit = std::nullopt) const = 0;
    virtual json fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe = "1m",
                             const std::optional<long long>& since = std::nullopt,
                             const std::optional<int>& limit = std::nullopt) const = 0;
    virtual json fetchTimeImpl() const = 0;
    virtual json fetchCurrenciesImpl() const = 0;
    virtual json fetchTradingFeesImpl() const = 0;
    virtual json fetchBalanceImpl() const = 0;
    virtual json fetchDepositAddressImpl(const std::string& code, const json& params = json::object()) const = 0;
    virtual json fetchDepositsImpl(const std::string& code = "", const std::optional<long long>& since = std::nullopt,
                                const std::optional<int>& limit = std::nullopt) const = 0;
    virtual json fetchWithdrawalsImpl(const std::string& code = "", const std::optional<long long>& since = std::nullopt,
                                   const std::optional<int>& limit = std::nullopt) const = 0;
    virtual json fetchDepositsWithdrawalsImpl(const std::string& code = "", const std::optional<long long>& since = std::nullopt,
                                           const std::optional<int>& limit = std::nullopt) const = 0;
    virtual json fetchDepositWithdrawFeesImpl() const = 0;
    virtual json fetchFundingRatesImpl(const std::vector<std::string>& symbols = {}) const = 0;
    virtual json fetchFundingRateHistoryImpl(const std::string& symbol, const std::optional<long long>& since = std::nullopt,
                                          const std::optional<int>& limit = std::nullopt) const = 0;
    virtual json fetchLeverageImpl(const std::string& symbol) const = 0;
    virtual json fetchMarginModesImpl(const std::vector<std::string>& symbols = {}) const = 0;
    virtual json fetchPositionsImpl(const std::vector<std::string>& symbols = {}) const = 0;
    virtual json fetchBorrowRatesImpl() const = 0;
    virtual json fetchBorrowRateHistoryImpl(const std::string& code, const std::optional<long long>& since = std::nullopt,
                                         const std::optional<int>& limit = std::nullopt) const = 0;
    virtual json fetchBorrowInterestImpl(const std::string& code = "", const std::optional<long long>& since = std::nullopt,
                                      const std::optional<int>& limit = std::nullopt) const = 0;
    virtual json fetchMyTradesImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt,
                                const std::optional<int>& limit = std::nullopt) const = 0;
    virtual json fetchOrdersImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt,
                              const std::optional<int>& limit = std::nullopt) const = 0;
    virtual json fetchOpenOrdersImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt,
                                  const std::optional<int>& limit = std::nullopt) const = 0;
    virtual json fetchClosedOrdersImpl(const std::string& symbol = "", const std::optional<long long>& since = std::nullopt,
                                    const std::optional<int>& limit = std::nullopt) const = 0;
    virtual json fetchOrderImpl(const std::string& id, const std::string& symbol = "") const = 0;

    // Trading API
    virtual json createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side,
                              double amount, const std::optional<double>& price = std::nullopt) = 0;
    virtual json cancelOrderImpl(const std::string& id, const std::string& symbol = "") = 0;
    virtual json cancelAllOrdersImpl(const std::string& symbol = "") = 0;
    virtual json editOrderImpl(const std::string& id, const std::string& symbol, const std::string& type,
                            const std::string& side, const std::optional<double>& amount = std::nullopt,
                            const std::optional<double>& price = std::nullopt) = 0;
    virtual json setLeverageImpl(int leverage, const std::string& symbol = "") = 0;
    virtual json setMarginModeImpl(const std::string& marginMode, const std::string& symbol = "") = 0;
    virtual json addMarginImpl(const std::string& symbol, double amount) = 0;
    virtual json reduceMarginImpl(const std::string& symbol, double amount) = 0;
    virtual json borrowCrossMarginImpl(const std::string& code, double amount, const std::string& symbol = "") = 0;
    virtual json borrowIsolatedMarginImpl(const std::string& symbol, const std::string& code, double amount) = 0;
    virtual json repayCrossMarginImpl(const std::string& code, double amount, const std::string& symbol = "") = 0;
    virtual json repayIsolatedMarginImpl(const std::string& symbol, const std::string& code, double amount) = 0;
    virtual json transferImpl(const std::string& code, double amount, const std::string& fromAccount,
                           const std::string& toAccount) = 0;

    // Helper methods
    virtual std::string sign(const std::string& path, const std::string& api = "public",
                          const std::string& method = "GET", const json& params = json::object(),
                          const std::map<std::string, std::string>& headers = {},
                          const json& body = nullptr) const = 0;
};

} // namespace ccxt
