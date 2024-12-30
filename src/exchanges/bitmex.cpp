#include "bitmex.h"
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

BitMEX::BitMEX(const Config& config) : Exchange(config) {
    this->describe({
        {"id", "bitmex"},
        {"name", "BitMEX"},
        {"countries", Json::array({"SC"})},  // Seychelles
        {"version", "v1"},
        {"userAgent", "ccxt-cpp"},
        {"rateLimit", 2000},
        {"pro", true},
        {"has", {
            {"CORS", false},
            {"spot", true},
            {"margin", true},
            {"swap", true},
            {"future", true},
            {"option", false},
            {"cancelAllOrders", true},
            {"cancelOrder", true},
            {"createOrder", true},
            {"editOrder", true},
            {"fetchBalance", true},
            {"fetchClosedOrders", true},
            {"fetchCurrencies", true},
            {"fetchDepositAddress", true},
            {"fetchFundingRate", true},
            {"fetchFundingRates", true},
            {"fetchIndexOHLCV", true},
            {"fetchLeverage", true},
            {"fetchMarkets", true},
            {"fetchMarkOHLCV", true},
            {"fetchMyTrades", true},
            {"fetchOHLCV", true},
            {"fetchOpenOrders", true},
            {"fetchOrder", true},
            {"fetchOrderBook", true},
            {"fetchOrders", true},
            {"fetchPosition", true},
            {"fetchPositions", true},
            {"fetchPremiumIndexOHLCV", true},
            {"fetchTicker", true},
            {"fetchTickers", true},
            {"fetchTrades", true},
            {"setLeverage", true},
            {"setMarginMode", true}
        }},
        {"timeframes", {
            {"1m", "1m"},
            {"5m", "5m"},
            {"1h", "1h"},
            {"1d", "1d"}
        }},
        {"urls", {
            {"test", {
                {"rest", "https://testnet.bitmex.com"}
            }},
            {"api", {
                {"rest", "https://www.bitmex.com"}
            }},
            {"www", "https://www.bitmex.com"},
            {"doc", {
                "https://www.bitmex.com/app/apiOverview",
                "https://github.com/BitMEX/api-connectors/tree/master/official-http"
            }}
        }},
        {"api", {
            {"public", {
                {"get", {
                    {"announcement", 1},
                    {"announcement/urgent", 1},
                    {"funding", 1},
                    {"instrument", 1},
                    {"instrument/active", 1},
                    {"instrument/activeAndIndices", 1},
                    {"instrument/activeIntervals", 1},
                    {"instrument/compositeIndex", 1},
                    {"instrument/indices", 1},
                    {"insurance", 1},
                    {"leaderboard", 1},
                    {"liquidation", 1},
                    {"orderBook/L2", 1},
                    {"quote", 1},
                    {"quote/bucketed", 1},
                    {"schema", 1},
                    {"schema/websocketHelp", 1},
                    {"settlement", 1},
                    {"stats", 1},
                    {"stats/history", 1},
                    {"trade", 1},
                    {"trade/bucketed", 1}
                }}
            }},
            {"private", {
                {"get", {
                    {"apiKey", 1},
                    {"chat", 1},
                    {"chat/channels", 1},
                    {"chat/connected", 1},
                    {"execution", 1},
                    {"execution/tradeHistory", 1},
                    {"notification", 1},
                    {"order", 1},
                    {"position", 1},
                    {"user", 1},
                    {"user/affiliateStatus", 1},
                    {"user/checkReferralCode", 1},
                    {"user/commission", 1},
                    {"user/depositAddress", 1},
                    {"user/executionHistory", 1},
                    {"user/margin", 1},
                    {"user/minWithdrawalFee", 1},
                    {"user/wallet", 1},
                    {"user/walletHistory", 1},
                    {"user/walletSummary", 1}
                }},
                {"post", {
                    {"apiKey", 1},
                    {"apiKey/disable", 1},
                    {"apiKey/enable", 1},
                    {"chat", 1},
                    {"order", 1},
                    {"order/bulk", 1},
                    {"order/cancelAllAfter", 1},
                    {"order/closePosition", 1},
                    {"position/isolate", 1},
                    {"position/leverage", 1},
                    {"position/riskLimit", 1},
                    {"position/transferMargin", 1},
                    {"user/cancelWithdrawal", 1},
                    {"user/confirmEmail", 1},
                    {"user/confirmEnableTFA", 1},
                    {"user/confirmWithdrawal", 1},
                    {"user/disableTFA", 1},
                    {"user/logout", 1},
                    {"user/logoutAll", 1},
                    {"user/preferences", 1},
                    {"user/requestEnableTFA", 1},
                    {"user/requestWithdrawal", 1}
                }},
                {"put", {
                    {"order", 1},
                    {"order/bulk", 1},
                    {"user", 1}
                }},
                {"delete", {
                    {"apiKey", 1},
                    {"order", 1},
                    {"order/all", 1}
                }}
            }}
        }}
    });
}

// Market Data Implementation
Json BitMEX::fetchMarketsImpl() const {
    return this->fetch("/instrument/active");
}

Json BitMEX::fetchTickerImpl(const std::string& symbol) const {
    auto market = this->market(symbol);
    auto request = Json::object({{"symbol", market["id"]}});
    return this->fetch("/instrument?" + this->urlencode(request));
}

Json BitMEX::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({{"symbol", market["id"]}});
    if (limit) {
        request["depth"] = *limit;
    }
    return this->fetch("/orderBook/L2?" + this->urlencode(request));
}

Json BitMEX::fetchTradesImpl(const std::string& symbol, const std::optional<long long>& since,
                          const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({{"symbol", market["id"]}});
    if (since) {
        request["startTime"] = this->iso8601(*since);
    }
    if (limit) {
        request["count"] = *limit;
    }
    return this->fetch("/trade?" + this->urlencode(request));
}

Json BitMEX::fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe,
                         const std::optional<long long>& since,
                         const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"symbol", market["id"]},
        {"binSize", timeframe},
        {"partial", true}
    });
    if (since) {
        request["startTime"] = this->iso8601(*since);
    }
    if (limit) {
        request["count"] = *limit;
    }
    return this->fetch("/trade/bucketed?" + this->urlencode(request));
}

// Trading Implementation
Json BitMEX::createOrderImpl(const std::string& symbol, const std::string& type,
                         const std::string& side, double amount,
                         const std::optional<double>& price) {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"symbol", market["id"]},
        {"side", side.upper()},
        {"orderQty", amount},
        {"ordType", type.upper()}
    });
    if (price) {
        request["price"] = *price;
    }
    return this->fetch("/order", "private", "POST", request);
}

Json BitMEX::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    auto request = Json::object({{"orderID", id}});
    if (!symbol.empty()) {
        auto market = this->market(symbol);
        request["symbol"] = market["id"];
    }
    return this->fetch("/order", "private", "DELETE", request);
}

Json BitMEX::fetchOrderImpl(const std::string& id, const std::string& symbol) const {
    auto request = Json::object({{"orderID", id}});
    if (!symbol.empty()) {
        auto market = this->market(symbol);
        request["symbol"] = market["id"];
    }
    return this->fetch("/order?" + this->urlencode(request), "private");
}

Json BitMEX::fetchOrdersImpl(const std::string& symbol, const std::optional<long long>& since,
                         const std::optional<int>& limit) const {
    auto request = Json::object();
    if (!symbol.empty()) {
        auto market = this->market(symbol);
        request["symbol"] = market["id"];
    }
    if (since) {
        request["startTime"] = this->iso8601(*since);
    }
    if (limit) {
        request["count"] = *limit;
    }
    return this->fetch("/order?" + this->urlencode(request), "private");
}

Json BitMEX::fetchOpenOrdersImpl(const std::string& symbol, const std::optional<long long>& since,
                              const std::optional<int>& limit) const {
    auto request = Json::object({{"filter", {{"open", true}}}});
    if (!symbol.empty()) {
        auto market = this->market(symbol);
        request["symbol"] = market["id"];
    }
    if (since) {
        request["startTime"] = this->iso8601(*since);
    }
    if (limit) {
        request["count"] = *limit;
    }
    return this->fetch("/order?" + this->urlencode(request), "private");
}

Json BitMEX::fetchClosedOrdersImpl(const std::string& symbol, const std::optional<long long>& since,
                                const std::optional<int>& limit) const {
    auto request = Json::object({{"filter", {{"open", false}}}});
    if (!symbol.empty()) {
        auto market = this->market(symbol);
        request["symbol"] = market["id"];
    }
    if (since) {
        request["startTime"] = this->iso8601(*since);
    }
    if (limit) {
        request["count"] = *limit;
    }
    return this->fetch("/order?" + this->urlencode(request), "private");
}

// Account Implementation
Json BitMEX::fetchBalanceImpl() const {
    return this->fetch("/user/margin", "private");
}

Json BitMEX::fetchPositionsImpl(const std::string& symbol) const {
    auto request = Json::object();
    if (!symbol.empty()) {
        auto market = this->market(symbol);
        request["symbol"] = market["id"];
    }
    return this->fetch("/position?" + this->urlencode(request), "private");
}

// Async Implementation
boost::future<Json> BitMEX::fetchAsync(const std::string& path, const std::string& api,
                                   const std::string& method, const Json& params,
                                   const std::map<std::string, std::string>& headers) const {
    return Exchange::fetchAsync(path, api, method, params, headers);
}

boost::future<Json> BitMEX::fetchMarketsAsync() const {
    return this->fetchAsync("/instrument/active");
}

boost::future<Json> BitMEX::fetchTickerAsync(const std::string& symbol) const {
    auto market = this->market(symbol);
    auto request = Json::object({{"symbol", market["id"]}});
    return this->fetchAsync("/instrument?" + this->urlencode(request));
}

boost::future<Json> BitMEX::fetchOrderBookAsync(const std::string& symbol,
                                            const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({{"symbol", market["id"]}});
    if (limit) {
        request["depth"] = *limit;
    }
    return this->fetchAsync("/orderBook/L2?" + this->urlencode(request));
}

boost::future<Json> BitMEX::fetchTradesAsync(const std::string& symbol,
                                         const std::optional<long long>& since,
                                         const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({{"symbol", market["id"]}});
    if (since) {
        request["startTime"] = this->iso8601(*since);
    }
    if (limit) {
        request["count"] = *limit;
    }
    return this->fetchAsync("/trade?" + this->urlencode(request));
}

boost::future<Json> BitMEX::fetchOHLCVAsync(const std::string& symbol,
                                        const std::string& timeframe,
                                        const std::optional<long long>& since,
                                        const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"symbol", market["id"]},
        {"binSize", timeframe},
        {"partial", true}
    });
    if (since) {
        request["startTime"] = this->iso8601(*since);
    }
    if (limit) {
        request["count"] = *limit;
    }
    return this->fetchAsync("/trade/bucketed?" + this->urlencode(request));
}

boost::future<Json> BitMEX::createOrderAsync(const std::string& symbol,
                                        const std::string& type,
                                        const std::string& side,
                                        double amount,
                                        const std::optional<double>& price) {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"symbol", market["id"]},
        {"side", side.upper()},
        {"orderQty", amount},
        {"ordType", type.upper()}
    });
    if (price) {
        request["price"] = *price;
    }
    return this->fetchAsync("/order", "private", "POST", request);
}

boost::future<Json> BitMEX::cancelOrderAsync(const std::string& id,
                                        const std::string& symbol) {
    auto request = Json::object({{"orderID", id}});
    if (!symbol.empty()) {
        auto market = this->market(symbol);
        request["symbol"] = market["id"];
    }
    return this->fetchAsync("/order", "private", "DELETE", request);
}

boost::future<Json> BitMEX::fetchOrderAsync(const std::string& id,
                                       const std::string& symbol) const {
    auto request = Json::object({{"orderID", id}});
    if (!symbol.empty()) {
        auto market = this->market(symbol);
        request["symbol"] = market["id"];
    }
    return this->fetchAsync("/order?" + this->urlencode(request), "private");
}

boost::future<Json> BitMEX::fetchOrdersAsync(const std::string& symbol,
                                        const std::optional<long long>& since,
                                        const std::optional<int>& limit) const {
    auto request = Json::object();
    if (!symbol.empty()) {
        auto market = this->market(symbol);
        request["symbol"] = market["id"];
    }
    if (since) {
        request["startTime"] = this->iso8601(*since);
    }
    if (limit) {
        request["count"] = *limit;
    }
    return this->fetchAsync("/order?" + this->urlencode(request), "private");
}

boost::future<Json> BitMEX::fetchOpenOrdersAsync(const std::string& symbol,
                                             const std::optional<long long>& since,
                                             const std::optional<int>& limit) const {
    auto request = Json::object({{"filter", {{"open", true}}}});
    if (!symbol.empty()) {
        auto market = this->market(symbol);
        request["symbol"] = market["id"];
    }
    if (since) {
        request["startTime"] = this->iso8601(*since);
    }
    if (limit) {
        request["count"] = *limit;
    }
    return this->fetchAsync("/order?" + this->urlencode(request), "private");
}

boost::future<Json> BitMEX::fetchClosedOrdersAsync(const std::string& symbol,
                                               const std::optional<long long>& since,
                                               const std::optional<int>& limit) const {
    auto request = Json::object({{"filter", {{"open", false}}}});
    if (!symbol.empty()) {
        auto market = this->market(symbol);
        request["symbol"] = market["id"];
    }
    if (since) {
        request["startTime"] = this->iso8601(*since);
    }
    if (limit) {
        request["count"] = *limit;
    }
    return this->fetchAsync("/order?" + this->urlencode(request), "private");
}

boost::future<Json> BitMEX::fetchBalanceAsync() const {
    return this->fetchAsync("/user/margin", "private");
}

boost::future<Json> BitMEX::fetchPositionsAsync(const std::string& symbol) const {
    auto request = Json::object();
    if (!symbol.empty()) {
        auto market = this->market(symbol);
        request["symbol"] = market["id"];
    }
    return this->fetchAsync("/position?" + this->urlencode(request), "private");
}

// Helper methods
std::string BitMEX::sign(const std::string& path, const std::string& api,
                      const std::string& method, const Json& params,
                      std::map<std::string, std::string>& headers,
                      const Json& body) {
    auto url = this->urls["api"][api] + path;
    auto expires = std::to_string(this->milliseconds() + 60000);  // 1 minute in the future
    
    if (api == "private") {
        headers["api-expires"] = expires;
        headers["api-key"] = this->config_.apiKey;
        
        std::string data;
        if (method == "GET") {
            if (!params.empty()) {
                url += "?" + this->urlencode(params);
            }
        } else {
            data = body.dump();
            headers["Content-Type"] = "application/json";
        }
        
        auto auth = method + url.substr(url.find("/api")) + data;
        auto signature = this->hmac(auth, this->config_.secret, "sha256", "hex");
        headers["api-signature"] = signature;
    }
    
    return url;
}

} // namespace ccxt
