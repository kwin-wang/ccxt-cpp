#include "ccxt/exchanges/probit.h"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace ccxt {

const std::string probit::defaultBaseURL = "https://api.probit.com";
const std::string probit::defaultVersion = "v1";
const int probit::defaultRateLimit = 50; // 20 requests per second

probit::probit(const Config& config) : Exchange(config) {
    init();
}

void probit::init() {
    
    
    id = "probit";
    name = "ProBit";
    countries = {"US", "KR"};
    version = defaultVersion;
    rateLimit = defaultRateLimit;
    certified = false;
    pro = false;
    
    has = {
        {"CORS", true},
        {"spot", true},
        {"margin", false},
        {"swap", false},
        {"future", false},
        {"option", false},
        {"cancelAllOrders", true},
        {"cancelOrder", true},
        {"createOrder", true},
        {"fetchBalance", true},
        {"fetchClosedOrders", true},
        {"fetchCurrencies", true},
        {"fetchDepositAddress", true},
        {"fetchDeposits", true},
        {"fetchMarkets", true},
        {"fetchMyTrades", true},
        {"fetchOHLCV", true},
        {"fetchOpenOrders", true},
        {"fetchOrder", true},
        {"fetchOrderBook", true},
        {"fetchOrders", true},
        {"fetchTicker", true},
        {"fetchTickers", true},
        {"fetchTrades", true},
        {"fetchWithdrawals", true}
    };

    timeframes = {
        {"1m", "1m"},
        {"3m", "3m"},
        {"5m", "5m"},
        {"15m", "15m"},
        {"30m", "30m"},
        {"1h", "1h"},
        {"4h", "4h"},
        {"12h", "12h"},
        {"1d", "1d"},
        {"1w", "1w"},
        {"1M", "1M"}
    };

    hostname = "api.probit.com";
    urls = {
        {"logo", "https://user-images.githubusercontent.com/51840849/79268032-c4379480-7ea2-11ea-80b3-dd96bb29fd0d.jpg"},
        {"api", {
            {"public", "https://api.probit.com"},
            {"private", "https://api.probit.com"}
        }},
        {"www", "https://www.probit.com"},
        {"doc", {
            "https://docs-en.probit.com",
            "https://docs-ko.probit.com"
        }},
        {"fees", "https://support.probit.com/hc/en-us/articles/360020968611-Trading-Fees"}
    };

    api = {
        {"public", {
            {"get", {
                "market",
                "currency",
                "ticker",
                "order_book",
                "trade",
                "candle"
            }}
        }},
        {"private", {
            {"get", {
                "balance",
                "order",
                "order_history",
                "trade_history",
                "deposit_address",
                "deposit_history",
                "withdrawal_history"
            }},
            {"post", {
                "new_order",
                "cancel_order",
                "cancel_all_orders"
            }}
        }}
    };

    fees = {
        {"trading", {
            {"tierBased", false},
            {"percentage", true},
            {"taker", 0.002},
            {"maker", 0.002}
        }}
    };

    requiredCredentials = {
        {"apiKey", true},
        {"secret", true}
    };

    precisionMode = DECIMAL_PLACES;
}

Json probit::describeImpl() const {
    return ExchangeImpl::describeImpl();
}

Json probit::fetchMarketsImpl() const {
    auto response = request("/market", "public", "GET");
    auto markets = Json::array();
    
    for (const auto& market : response["data"]) {
        auto id = safeString(market, "id");
        auto baseId = safeString(market, "base_currency_id");
        auto quoteId = safeString(market, "quote_currency_id");
        auto base = safeCurrencyCode(baseId);
        auto quote = safeCurrencyCode(quoteId);
        auto symbol = base + "/" + quote;
        
        markets.push_back({
            {"id", id},
            {"symbol", symbol},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", true},
            {"type", "spot"},
            {"spot", true},
            {"precision", {
                {"amount", safeInteger(market, "amount_precision")},
                {"price", safeInteger(market, "price_precision")}
            }},
            {"limits", {
                {"amount", {
                    {"min", safeNumber(market, "min_amount")},
                    {"max", safeNumber(market, "max_amount")}
                }},
                {"price", {
                    {"min", safeNumber(market, "min_price")},
                    {"max", safeNumber(market, "max_price")}
                }},
                {"cost", {
                    {"min", safeNumber(market, "min_value")},
                    {"max", safeNumber(market, "max_value")}
                }}
            }},
            {"info", market}
        });
    }
    
    return markets;
}

Json probit::fetchCurrenciesImpl() const {
    auto response = request("/currency", "public", "GET");
    auto result = Json::object();
    
    for (const auto& currency : response["data"]) {
        auto id = safeString(currency, "id");
        auto code = safeCurrencyCode(id);
        
        result[code] = {
            {"id", id},
            {"code", code},
            {"name", safeString(currency, "name")},
            {"active", safeValue(currency, "deposit_status") == "active" &&
                      safeValue(currency, "withdrawal_status") == "active"},
            {"deposit", safeValue(currency, "deposit_status") == "active"},
            {"withdraw", safeValue(currency, "withdrawal_status") == "active"},
            {"precision", safeInteger(currency, "precision")},
            {"fee", safeNumber(currency, "withdrawal_fee")},
            {"limits", {
                {"amount", {
                    {"min", safeNumber(currency, "min_amount")},
                    {"max", safeNumber(currency, "max_amount")}
                }},
                {"withdraw", {
                    {"min", safeNumber(currency, "min_withdrawal_amount")},
                    {"max", safeNumber(currency, "max_withdrawal_amount")}
                }}
            }},
            {"info", currency}
        };
    }
    
    return result;
}

Json probit::fetchTickerImpl(const std::string& symbol) const {
    auto market = loadMarket(symbol);
    auto request = Json::object({
        {"market_ids", market["id"]}
    });
    
    auto response = request("/ticker", "public", "GET", request);
    auto ticker = response["data"][0];
    return parseTicker(ticker, market);
}

Json probit::fetchTickersImpl(const std::vector<std::string>& symbols) const {
    auto request = Json::object();
    if (!symbols.empty()) {
        std::vector<std::string> marketIds;
        for (const auto& symbol : symbols) {
            auto market = loadMarket(symbol);
            marketIds.push_back(market["id"].get<std::string>());
        }
        request["market_ids"] = join(marketIds, ",");
    }
    
    auto response = request("/ticker", "public", "GET", request);
    auto tickers = Json::array();
    for (const auto& ticker : response["data"]) {
        auto marketId = safeString(ticker, "market_id");
        auto market = safeValue(markets, marketId);
        if (market.is_null()) continue;
        tickers.push_back(parseTicker(ticker, market));
    }
    
    return filterByArray(tickers, "symbol", symbols);
}

Json probit::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
    auto market = loadMarket(symbol);
    auto request = Json::object({
        {"market_id", market["id"]}
    });
    
    if (limit) {
        request["depth"] = *limit;
    }
    
    auto response = request("/order_book", "public", "GET", request);
    auto orderbook = response["data"];
    auto timestamp = safeInteger(orderbook, "timestamp");
    
    return {
        {"symbol", symbol},
        {"bids", parseBidAsks(orderbook["bids"])},
        {"asks", parseBidAsks(orderbook["asks"])},
        {"timestamp", timestamp},
        {"datetime", iso8601(timestamp)},
        {"nonce", nullptr}
    };
}

Json probit::fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe,
                           const std::optional<long long>& since,
                           const std::optional<int>& limit) const {
    auto market = loadMarket(symbol);
    auto request = Json::object({
        {"market_id", market["id"]},
        {"interval", timeframes[timeframe]}
    });
    
    if (since) {
        request["start_time"] = iso8601(*since);
    }
    if (limit) {
        request["limit"] = *limit;
    }
    
    auto response = request("/candle", "public", "GET", request);
    return parseOHLCVs(response["data"]);
}

Json probit::fetchTradesImpl(const std::string& symbol, const std::optional<int>& limit,
                            const std::optional<long long>& since) const {
    auto market = loadMarket(symbol);
    auto request = Json::object({
        {"market_id", market["id"]}
    });
    
    if (limit) {
        request["limit"] = *limit;
    }
    if (since) {
        request["start_time"] = iso8601(*since);
    }
    
    auto response = request("/trade", "public", "GET", request);
    return parseTrades(response["data"], market);
}

Json probit::parseTicker(const Json& ticker, const Json& market) const {
    auto timestamp = safeInteger(ticker, "time");
    auto symbol = market["symbol"];
    
    return {
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", iso8601(timestamp)},
        {"high", safeNumber(ticker, "high")},
        {"low", safeNumber(ticker, "low")},
        {"bid", safeNumber(ticker, "bid")},
        {"bidVolume", nullptr},
        {"ask", safeNumber(ticker, "ask")},
        {"askVolume", nullptr},
        {"vwap", nullptr},
        {"open", safeNumber(ticker, "open")},
        {"close", safeNumber(ticker, "close")},
        {"last", safeNumber(ticker, "last")},
        {"previousClose", nullptr},
        {"change", nullptr},
        {"percentage", nullptr},
        {"average", nullptr},
        {"baseVolume", safeNumber(ticker, "volume")},
        {"quoteVolume", nullptr},
        {"info", ticker}
    };
}

Json probit::parseOHLCV(const Json& ohlcv) const {
    return {
        safeTimestamp(ohlcv, "time"),
        safeNumber(ohlcv, "open"),
        safeNumber(ohlcv, "high"),
        safeNumber(ohlcv, "low"),
        safeNumber(ohlcv, "close"),
        safeNumber(ohlcv, "volume")
    };
}

} // namespace ccxt
