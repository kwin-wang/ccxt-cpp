#include "bitmart.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace ccxt {

Bitmart::Bitmart(const Config& config) : Exchange(config) {
    this->describe({
        {"id", "bitmart"},
        {"name", "BitMart"},
        {"countries", Json::array({"US", "CN", "HK", "KR"})},
        {"rateLimit", 33.34},  // 150 per 5 seconds = 30 per second
        {"version", "v2"},
        {"certified", true},
        {"pro", true},
        {"has", {
            {"CORS", nullptr},
            {"spot", true},
            {"margin", true},
            {"swap", true},
            {"future", false},
            {"option", false},
            {"borrowCrossMargin", false},
            {"borrowIsolatedMargin", true},
            {"cancelAllOrders", true},
            {"cancelOrder", true},
            {"cancelOrders", true},
            {"createOrder", true},
            {"createOrders", true},
            {"fetchBalance", true},
            {"fetchBorrowInterest", true},
            {"fetchCurrencies", true},
            {"fetchDeposit", true},
            {"fetchDepositAddress", true},
            {"fetchDeposits", true},
            {"fetchFundingRate", true},
            {"fetchIsolatedBorrowRate", true},
            {"fetchIsolatedBorrowRates", true},
            {"fetchMarkets", true},
            {"fetchMyTrades", true},
            {"fetchOHLCV", true},
            {"fetchOpenOrders", true},
            {"fetchOrder", true},
            {"fetchOrderBook", true},
            {"fetchTicker", true},
            {"fetchTickers", true},
            {"fetchTime", true},
            {"fetchTrades", true},
            {"fetchWithdrawals", true},
            {"withdraw", true}
        }},
        {"timeframes", {
            {"1m", "1"},
            {"3m", "3"},
            {"5m", "5"},
            {"15m", "15"},
            {"30m", "30"},
            {"1h", "60"},
            {"2h", "120"},
            {"4h", "240"},
            {"6h", "360"},
            {"12h", "720"},
            {"1d", "1D"},
            {"1w", "1W"}
        }},
        {"hostname", "bitmart.com"},
        {"urls", {
            {"logo", "https://github.com/user-attachments/assets/0623e9c4-f50e-48c9-82bd-65c3908c3a14"},
            {"api", {
                {"spot", "https://api-cloud.bitmart.com"},
                {"swap", "https://api-cloud-v2.bitmart.com"}
            }},
            {"www", "https://www.bitmart.com/"},
            {"doc", "https://developer-pro.bitmart.com/"}
        }},
        {"api", {
            {"public", {
                {"get", {
                    {"system/time", 3},
                    {"system/service", 3},
                    {"spot/v1/currencies", 7.5},
                    {"spot/v1/symbols", 7.5},
                    {"spot/v1/symbols/details", 5},
                    {"spot/quotation/v3/tickers", 6},
                    {"spot/quotation/v3/ticker", 4},
                    {"spot/quotation/v3/lite-klines", 5},
                    {"spot/quotation/v3/klines", 7},
                    {"spot/quotation/v3/books", 4},
                    {"spot/quotation/v3/trades", 4}
                }}
            }},
            {"private", {
                {"get", {
                    {"spot/v1/wallet", 5},
                    {"spot/v2/orders", 5},
                    {"spot/v1/trades", 5}
                }},
                {"post", {
                    {"spot/v1/submit_order", 1},
                    {"spot/v2/cancel_order", 1},
                    {"spot/v1/batch_orders", 1}
                }}
            }}
        }}
    });
}

// Market Data Implementation
Json Bitmart::fetchMarketsImpl() const {
    return this->fetch("/spot/v1/symbols");
}

Json Bitmart::fetchTickerImpl(const std::string& symbol) const {
    auto market = this->market(symbol);
    return this->fetch("/spot/quotation/v3/ticker?symbol=" + market["id"].get<std::string>());
}

Json Bitmart::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({{"symbol", market["id"]}});
    if (limit) {
        request["limit"] = *limit;
    }
    return this->fetch("/spot/quotation/v3/books?" + this->urlencode(request));
}

Json Bitmart::fetchTradesImpl(const std::string& symbol, const std::optional<long long>& since,
                           const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({{"symbol", market["id"]}});
    if (limit) {
        request["limit"] = *limit;
    }
    return this->fetch("/spot/quotation/v3/trades?" + this->urlencode(request));
}

Json Bitmart::fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe,
                          const std::optional<long long>& since,
                          const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"symbol", market["id"]},
        {"step", this->timeframes[timeframe]}
    });
    if (since) {
        request["start_time"] = *since;
    }
    if (limit) {
        request["limit"] = *limit;
    }
    return this->fetch("/spot/quotation/v3/klines?" + this->urlencode(request));
}

// Trading Implementation
Json Bitmart::createOrderImpl(const std::string& symbol, const std::string& type,
                          const std::string& side, double amount,
                          const std::optional<double>& price) {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"symbol", market["id"]},
        {"side", side},
        {"type", type},
        {"size", this->amountToPrecision(symbol, amount)}
    });
    if (price) {
        request["price"] = this->priceToPrecision(symbol, *price);
    }
    return this->fetch("/spot/v1/submit_order", "private", "POST", request);
}

Json Bitmart::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"order_id", id},
        {"symbol", market["id"]}
    });
    return this->fetch("/spot/v2/cancel_order", "private", "POST", request);
}

Json Bitmart::fetchOrderImpl(const std::string& id, const std::string& symbol) const {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"order_id", id},
        {"symbol", market["id"]}
    });
    return this->fetch("/spot/v2/orders?" + this->urlencode(request), "private");
}

Json Bitmart::fetchOrdersImpl(const std::string& symbol, const std::optional<long long>& since,
                          const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({{"symbol", market["id"]}});
    if (since) {
        request["start_time"] = *since;
    }
    if (limit) {
        request["limit"] = *limit;
    }
    return this->fetch("/spot/v2/orders?" + this->urlencode(request), "private");
}

Json Bitmart::fetchOpenOrdersImpl(const std::string& symbol, const std::optional<long long>& since,
                               const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"symbol", market["id"]},
        {"status", "active"}
    });
    if (since) {
        request["start_time"] = *since;
    }
    if (limit) {
        request["limit"] = *limit;
    }
    return this->fetch("/spot/v2/orders?" + this->urlencode(request), "private");
}

Json Bitmart::fetchClosedOrdersImpl(const std::string& symbol, const std::optional<long long>& since,
                                const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"symbol", market["id"]},
        {"status", "done"}
    });
    if (since) {
        request["start_time"] = *since;
    }
    if (limit) {
        request["limit"] = *limit;
    }
    return this->fetch("/spot/v2/orders?" + this->urlencode(request), "private");
}

// Account Implementation
Json Bitmart::fetchBalanceImpl() const {
    return this->fetch("/spot/v1/wallet", "private");
}

Json Bitmart::fetchMyTradesImpl(const std::string& symbol, const std::optional<long long>& since,
                             const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({{"symbol", market["id"]}});
    if (since) {
        request["start_time"] = *since;
    }
    if (limit) {
        request["limit"] = *limit;
    }
    return this->fetch("/spot/v1/trades?" + this->urlencode(request), "private");
}

// Async Implementation
boost::future<Json> Bitmart::fetchAsync(const std::string& path, const std::string& api,
                                    const std::string& method, const Json& params,
                                    const std::map<std::string, std::string>& headers) const {
    return Exchange::fetchAsync(path, api, method, params, headers);
}

boost::future<Json> Bitmart::fetchMarketsAsync() const {
    return this->fetchAsync("/spot/v1/symbols");
}

boost::future<Json> Bitmart::fetchTickerAsync(const std::string& symbol) const {
    auto market = this->market(symbol);
    return this->fetchAsync("/spot/quotation/v3/ticker?symbol=" + market["id"].get<std::string>());
}

boost::future<Json> Bitmart::fetchOrderBookAsync(const std::string& symbol,
                                             const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({{"symbol", market["id"]}});
    if (limit) {
        request["limit"] = *limit;
    }
    return this->fetchAsync("/spot/quotation/v3/books?" + this->urlencode(request));
}

boost::future<Json> Bitmart::fetchTradesAsync(const std::string& symbol,
                                          const std::optional<long long>& since,
                                          const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({{"symbol", market["id"]}});
    if (limit) {
        request["limit"] = *limit;
    }
    return this->fetchAsync("/spot/quotation/v3/trades?" + this->urlencode(request));
}

boost::future<Json> Bitmart::fetchOHLCVAsync(const std::string& symbol,
                                         const std::string& timeframe,
                                         const std::optional<long long>& since,
                                         const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"symbol", market["id"]},
        {"step", this->timeframes[timeframe]}
    });
    if (since) {
        request["start_time"] = *since;
    }
    if (limit) {
        request["limit"] = *limit;
    }
    return this->fetchAsync("/spot/quotation/v3/klines?" + this->urlencode(request));
}

boost::future<Json> Bitmart::createOrderAsync(const std::string& symbol,
                                         const std::string& type,
                                         const std::string& side,
                                         double amount,
                                         const std::optional<double>& price) {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"symbol", market["id"]},
        {"side", side},
        {"type", type},
        {"size", this->amountToPrecision(symbol, amount)}
    });
    if (price) {
        request["price"] = this->priceToPrecision(symbol, *price);
    }
    return this->fetchAsync("/spot/v1/submit_order", "private", "POST", request);
}

boost::future<Json> Bitmart::cancelOrderAsync(const std::string& id,
                                         const std::string& symbol) {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"order_id", id},
        {"symbol", market["id"]}
    });
    return this->fetchAsync("/spot/v2/cancel_order", "private", "POST", request);
}

boost::future<Json> Bitmart::fetchOrderAsync(const std::string& id,
                                        const std::string& symbol) const {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"order_id", id},
        {"symbol", market["id"]}
    });
    return this->fetchAsync("/spot/v2/orders?" + this->urlencode(request), "private");
}

boost::future<Json> Bitmart::fetchOrdersAsync(const std::string& symbol,
                                         const std::optional<long long>& since,
                                         const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({{"symbol", market["id"]}});
    if (since) {
        request["start_time"] = *since;
    }
    if (limit) {
        request["limit"] = *limit;
    }
    return this->fetchAsync("/spot/v2/orders?" + this->urlencode(request), "private");
}

boost::future<Json> Bitmart::fetchOpenOrdersAsync(const std::string& symbol,
                                             const std::optional<long long>& since,
                                             const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"symbol", market["id"]},
        {"status", "active"}
    });
    if (since) {
        request["start_time"] = *since;
    }
    if (limit) {
        request["limit"] = *limit;
    }
    return this->fetchAsync("/spot/v2/orders?" + this->urlencode(request), "private");
}

boost::future<Json> Bitmart::fetchClosedOrdersAsync(const std::string& symbol,
                                               const std::optional<long long>& since,
                                               const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"symbol", market["id"]},
        {"status", "done"}
    });
    if (since) {
        request["start_time"] = *since;
    }
    if (limit) {
        request["limit"] = *limit;
    }
    return this->fetchAsync("/spot/v2/orders?" + this->urlencode(request), "private");
}

boost::future<Json> Bitmart::fetchBalanceAsync() const {
    return this->fetchAsync("/spot/v1/wallet", "private");
}

boost::future<Json> Bitmart::fetchMyTradesAsync(const std::string& symbol,
                                           const std::optional<long long>& since,
                                           const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({{"symbol", market["id"]}});
    if (since) {
        request["start_time"] = *since;
    }
    if (limit) {
        request["limit"] = *limit;
    }
    return this->fetchAsync("/spot/v1/trades?" + this->urlencode(request), "private");
}

// Helper methods
std::string Bitmart::sign(const std::string& path, const std::string& api,
                       const std::string& method, const Json& params,
                       std::map<std::string, std::string>& headers,
                       const Json& body) {
    auto url = this->urls["api"][api] + path;
    auto timestamp = std::to_string(this->milliseconds());
    
    if (api == "private") {
        headers["X-BM-KEY"] = this->config_.apiKey;
        headers["X-BM-TIMESTAMP"] = timestamp;
        
        std::string payload;
        if (method == "POST") {
            payload = body.dump();
            headers["Content-Type"] = "application/json";
        } else {
            payload = this->urlencode(params);
        }
        
        auto auth = timestamp + "#" + this->config_.apiKey + "#" + payload;
        auto signature = this->hmac(auth, this->config_.secret, "sha256", "hex");
        headers["X-BM-SIGN"] = signature;
    }
    
    return url;
}

} // namespace ccxt
