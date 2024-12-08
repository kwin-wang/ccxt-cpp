#include "ccxt/exchanges/async/bitfinex2_async.h"

namespace ccxt {

Bitfinex2Async::Bitfinex2Async(const boost::asio::io_context& context, const Config& config)
    : ExchangeAsync(context)
    , bitfinex2(config) {}

boost::future<Json> Bitfinex2Async::fetchAsync(const std::string& path, const std::string& api,
                                             const std::string& method, const Json& params,
                                             const std::map<std::string, std::string>& headers) const {
    return ExchangeAsync::fetchAsync(path, api, method, params, headers);
}

boost::future<Json> Bitfinex2Async::fetchMarketsAsync() const {
    return fetchAsync("/v2/conf/pub:info:pair", "public", "GET");
}

boost::future<Json> Bitfinex2Async::fetchCurrenciesAsync() const {
    return fetchAsync("/v2/conf/pub:map:currency:label", "public", "GET");
}

boost::future<Json> Bitfinex2Async::fetchTickerAsync(const std::string& symbol) const {
    std::string market_id = this->market_id(symbol);
    return fetchAsync("/v2/ticker/t" + market_id, "public", "GET");
}

boost::future<Json> Bitfinex2Async::fetchTickersAsync(const std::vector<std::string>& symbols) const {
    return fetchAsync("/v2/tickers", "public", "GET");
}

boost::future<Json> Bitfinex2Async::fetchOrderBookAsync(const std::string& symbol,
                                                      const std::optional<int>& limit) const {
    std::string market_id = this->market_id(symbol);
    Json params = Json::object();
    if (limit) {
        params["limit_bids"] = *limit;
        params["limit_asks"] = *limit;
    }
    return fetchAsync("/v2/book/t" + market_id + "/P0", "public", "GET", params);
}

boost::future<Json> Bitfinex2Async::fetchOHLCVAsync(const std::string& symbol,
                                                   const std::string& timeframe,
                                                   const std::optional<long long>& since,
                                                   const std::optional<int>& limit) const {
    std::string market_id = this->market_id(symbol);
    Json params = Json::object();
    if (since) {
        params["start"] = *since;
    }
    if (limit) {
        params["limit"] = *limit;
    }
    return fetchAsync("/v2/candles/trade:" + timeframe + ":t" + market_id + "/hist",
                     "public", "GET", params);
}

boost::future<Json> Bitfinex2Async::fetchTradesAsync(const std::string& symbol,
                                                    const std::optional<long long>& since,
                                                    const std::optional<int>& limit) const {
    std::string market_id = this->market_id(symbol);
    Json params = Json::object();
    if (since) {
        params["start"] = *since;
    }
    if (limit) {
        params["limit"] = *limit;
    }
    return fetchAsync("/v2/trades/t" + market_id + "/hist", "public", "GET", params);
}

boost::future<Json> Bitfinex2Async::createOrderAsync(const std::string& symbol,
                                                   const std::string& type,
                                                   const std::string& side,
                                                   double amount,
                                                   const std::optional<double>& price) {
    std::string market_id = this->market_id(symbol);
    Json params = {
        {"symbol", "t" + market_id},
        {"type", type},
        {"side", side},
        {"amount", std::to_string(amount)}
    };
    if (price) {
        params["price"] = std::to_string(*price);
    }
    return fetchAsync("/v2/auth/w/order/submit", "private", "POST", params);
}

boost::future<Json> Bitfinex2Async::editOrderAsync(const std::string& id,
                                                 const std::string& symbol,
                                                 const std::string& type,
                                                 const std::string& side,
                                                 const std::optional<double>& amount,
                                                 const std::optional<double>& price) {
    Json params = {
        {"id", id},
        {"type", type},
        {"side", side}
    };
    if (amount) {
        params["amount"] = std::to_string(*amount);
    }
    if (price) {
        params["price"] = std::to_string(*price);
    }
    return fetchAsync("/v2/auth/w/order/update", "private", "POST", params);
}

boost::future<Json> Bitfinex2Async::cancelOrderAsync(const std::string& id,
                                                   const std::string& symbol) {
    return fetchAsync("/v2/auth/w/order/cancel", "private", "POST", {{"id", id}});
}

boost::future<Json> Bitfinex2Async::cancelOrdersAsync(const std::vector<std::string>& ids,
                                                    const std::string& symbol) {
    return fetchAsync("/v2/auth/w/order/cancel/multi", "private", "POST", {{"id", ids}});
}

boost::future<Json> Bitfinex2Async::cancelAllOrdersAsync(const std::string& symbol) {
    Json params = Json::object();
    if (!symbol.empty()) {
        params["symbol"] = "t" + this->market_id(symbol);
    }
    return fetchAsync("/v2/auth/w/order/cancel/all", "private", "POST", params);
}

boost::future<Json> Bitfinex2Async::fetchOrderAsync(const std::string& id,
                                                  const std::string& symbol) const {
    return fetchAsync("/v2/auth/r/order/" + id, "private", "POST");
}

boost::future<Json> Bitfinex2Async::fetchOpenOrdersAsync(const std::string& symbol,
                                                       const std::optional<long long>& since,
                                                       const std::optional<int>& limit) const {
    Json params = Json::object();
    if (!symbol.empty()) {
        params["symbol"] = "t" + this->market_id(symbol);
    }
    if (since) {
        params["start"] = *since;
    }
    if (limit) {
        params["limit"] = *limit;
    }
    return fetchAsync("/v2/auth/r/orders", "private", "POST", params);
}

boost::future<Json> Bitfinex2Async::fetchClosedOrdersAsync(const std::string& symbol,
                                                         const std::optional<long long>& since,
                                                         const std::optional<int>& limit) const {
    Json params = Json::object();
    if (!symbol.empty()) {
        params["symbol"] = "t" + this->market_id(symbol);
    }
    if (since) {
        params["start"] = *since;
    }
    if (limit) {
        params["limit"] = *limit;
    }
    return fetchAsync("/v2/auth/r/orders/hist", "private", "POST", params);
}

boost::future<Json> Bitfinex2Async::fetchMyTradesAsync(const std::string& symbol,
                                                     const std::optional<long long>& since,
                                                     const std::optional<int>& limit) const {
    Json params = Json::object();
    if (!symbol.empty()) {
        params["symbol"] = "t" + this->market_id(symbol);
    }
    if (since) {
        params["start"] = *since;
    }
    if (limit) {
        params["limit"] = *limit;
    }
    return fetchAsync("/v2/auth/r/trades/hist", "private", "POST", params);
}

boost::future<Json> Bitfinex2Async::fetchBalanceAsync() const {
    return fetchAsync("/v2/auth/r/wallets", "private", "POST");
}

boost::future<Json> Bitfinex2Async::fetchPositionsAsync(const std::string& symbols) const {
    Json params = Json::object();
    if (!symbols.empty()) {
        params["symbols"] = symbols;
    }
    return fetchAsync("/v2/auth/r/positions", "private", "POST", params);
}

boost::future<Json> Bitfinex2Async::fetchLedgerAsync(const std::string& code,
                                                   const std::optional<long long>& since,
                                                   const std::optional<int>& limit) const {
    Json params = Json::object();
    if (!code.empty()) {
        params["currency"] = this->currency_id(code);
    }
    if (since) {
        params["start"] = *since;
    }
    if (limit) {
        params["limit"] = *limit;
    }
    return fetchAsync("/v2/auth/r/ledgers/hist", "private", "POST", params);
}

boost::future<Json> Bitfinex2Async::fetchOrderBookSnapshotAsync(const std::string& symbol,
                                                              const std::optional<int>& limit) const {
    std::string market_id = this->market_id(symbol);
    Json params = Json::object();
    if (limit) {
        params["len"] = *limit;
    }
    return fetchAsync("/v2/book/t" + market_id + "/R0", "public", "GET", params);
}

boost::future<Json> Bitfinex2Async::fetchFundingRatesAsync(const std::vector<std::string>& symbols) const {
    Json params = Json::object();
    if (!symbols.empty()) {
        std::vector<std::string> market_ids;
        for (const auto& symbol : symbols) {
            market_ids.push_back("f" + this->market_id(symbol));
        }
        params["symbols"] = market_ids;
    }
    return fetchAsync("/v2/calc/trade/avg", "public", "POST", params);
}

boost::future<Json> Bitfinex2Async::setLeverageAsync(const std::string& symbol,
                                                   double leverage) {
    std::string market_id = this->market_id(symbol);
    return fetchAsync("/v2/auth/w/margin/set", "private", "POST",
                     {{"symbol", "t" + market_id}, {"leverage", std::to_string(leverage)}});
}

boost::future<Json> Bitfinex2Async::fetchDepositsAsync(const std::string& code,
                                                     const std::optional<long long>& since,
                                                     const std::optional<int>& limit) const {
    Json params = Json::object();
    if (!code.empty()) {
        params["currency"] = this->currency_id(code);
    }
    if (since) {
        params["start"] = *since;
    }
    if (limit) {
        params["limit"] = *limit;
    }
    return fetchAsync("/v2/auth/r/movements/hist", "private", "POST", params);
}

boost::future<Json> Bitfinex2Async::fetchWithdrawalsAsync(const std::string& code,
                                                       const std::optional<long long>& since,
                                                       const std::optional<int>& limit) const {
    return fetchDepositsAsync(code, since, limit);
}

boost::future<Json> Bitfinex2Async::fetchDepositAddressAsync(const std::string& code) const {
    return fetchAsync("/v2/auth/r/deposit/address", "private", "POST",
                     {{"currency", this->currency_id(code)}});
}

boost::future<Json> Bitfinex2Async::transferAsync(const std::string& code,
                                                double amount,
                                                const std::string& fromAccount,
                                                const std::string& toAccount) {
    return fetchAsync("/v2/auth/w/transfer", "private", "POST",
                     {
                         {"currency", this->currency_id(code)},
                         {"amount", std::to_string(amount)},
                         {"from", fromAccount},
                         {"to", toAccount}
                     });
}

} // namespace ccxt
