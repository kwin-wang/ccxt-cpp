#include "ccxt/exchanges/hitbtc3.h"
#include "ccxt/base/json_helper.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <openssl/hmac.h>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace ccxt {

hitbtc3::hitbtc3(const Config& config) : exchange(config) {
    this->describe({
        {"id", "hitbtc3"},
        {"name", "HitBTC"},
        {"countries", json::array({"HK"})},
        {"version", "3"},
        {"rateLimit", 100},
        {"has", {
            {"fetchMarkets", true},
            {"fetchCurrencies", true},
            {"fetchTicker", true},
            {"fetchOrderBook", true},
            {"fetchTrades", true},
            {"fetchOHLCV", true},
            {"fetchBalance", true},
            {"createOrder", true},
            {"cancelOrder", true},
            {"cancelAllOrders", true},
            {"fetchOpenOrders", true},
            {"fetchClosedOrders", true},
            {"fetchMyTrades", true},
            {"fetchOrder", true},
            {"editOrder", true},
            {"fetchDepositAddress", true},
            {"fetchDeposits", true},
            {"fetchWithdrawals", true},
            {"withdraw", true},
            {"fetchPositions", true},
            {"setLeverage", true},
            {"setMarginMode", true},
            {"fetchFundingRate", true},
            {"fetchFundingRateHistory", true},
            {"fetchFundingHistory", true},
            {"fetchLeverageTiers", true},
            {"fetchIndexOHLCV", true},
            {"fetchMarkOHLCV", true}
        }},
        {"urls", {
            {"logo", "https://user-images.githubusercontent.com/1294454/92434237-10b31800-f1a9-11ea-8d27-6e3e6eff7061.jpg"},
            {"api", {
                {"public", "https://api.hitbtc.com/api/3"},
                {"private", "https://api.hitbtc.com/api/3"}
            }},
            {"www", "https://hitbtc.com"},
            {"doc", {
                "https://api.hitbtc.com",
                "https://github.com/hitbtc-com/hitbtc-api"
            }},
            {"fees", "https://hitbtc.com/fees-and-limits"}
        }},
        {"api", {
            {"public", {
                {"get", {
                    "public/currency",
                    "public/symbol",
                    "public/ticker",
                    "public/ticker/{symbol}",
                    "public/orderbook/{symbol}",
                    "public/trades/{symbol}",
                    "public/candles/{symbol}",
                    "public/fee/symbol/{symbol}",
                    "public/futures/info",
                    "public/futures/history/funding",
                    "public/futures/candles/index/{symbol}",
                    "public/futures/candles/mark_price/{symbol}"
                }}
            }},
            {"private", {
                {"get", {
                    "spot/balance",
                    "spot/order",
                    "spot/order/{client_order_id}",
                    "spot/order/active",
                    "spot/order/traded",
                    "spot/trade",
                    "wallet/balance",
                    "wallet/address",
                    "wallet/transactions",
                    "wallet/transaction/{id}",
                    "futures/balance",
                    "futures/position",
                    "futures/position/{symbol}"
                }},
                {"post", {
                    "spot/order",
                    "futures/order",
                    "wallet/address/new",
                    "wallet/withdraw",
                    "futures/leverage",
                    "futures/margin-mode"
                }},
                {"patch", {
                    "spot/order/{client_order_id}",
                    "futures/order/{client_order_id}"
                }},
                {"delete", {
                    "spot/order",
                    "spot/order/{client_order_id}",
                    "futures/order",
                    "futures/order/{client_order_id}"
                }}
            }}
        }},
        {"fees", {
            {"trading", {
                {"maker", 0.001},  // 0.1%
                {"taker", 0.002}   // 0.2%
            }}
        }},
        {"timeframes", {
            {"1m", "M1"},
            {"3m", "M3"},
            {"5m", "M5"},
            {"15m", "M15"},
            {"30m", "M30"},
            {"1h", "H1"},
            {"4h", "H4"},
            {"1d", "D1"},
            {"1w", "D7"},
            {"1M", "1M"}
        }}
    });
}

json hitbtc3::fetch_markets(const json& params) {
    auto response = this->publicGetPublicSymbol(params);
    return this->parse_markets(response);
}

json hitbtc3::fetch_currencies(const json& params) {
    auto response = this->publicGetPublicCurrency(params);
    return this->parse_currencies(response);
}

json hitbtc3::fetch_ticker(const std::string& symbol, const json& params) {
    this->load_markets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]}
    };
    auto response = this->publicGetPublicTickerSymbol(this->extend(request, params));
    return this->parse_ticker(response, market);
}

json hitbtc3::fetch_order_book(const std::string& symbol, int limit, const json& params) {
    this->load_markets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]}
    };
    if (limit)
        request["limit"] = limit;
    auto response = this->publicGetPublicOrderbookSymbol(this->extend(request, params));
    return this->parse_order_book(response, symbol);
}

json hitbtc3::create_order(const std::string& symbol, const std::string& type,
                          const std::string& side, double amount, double price, const json& params) {
    this->load_markets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]},
        {"side", side.substr(0, 1).c_str()},
        {"type", type},
        {"quantity", this->amount_to_precision(symbol, amount)}
    };
    if (type == "limit")
        request["price"] = this->price_to_precision(symbol, price);
    
    auto response = this->privatePostSpotOrder(this->extend(request, params));
    return this->parse_order(response, market);
}

json hitbtc3::cancel_order(const std::string& id, const std::string& symbol, const json& params) {
    this->load_markets();
    auto request = {
        {"client_order_id", id}
    };
    return this->privateDeleteSpotOrderClientOrderId(this->extend(request, params));
}

json hitbtc3::fetch_balance(const json& params) {
    this->load_markets();
    auto response = this->privateGetSpotBalance(params);
    return this->parse_balance(response);
}

json hitbtc3::fetch_positions(const std::string& symbol, const json& params) {
    this->load_markets();
    auto request = json::object();
    auto market = nullptr;
    if (!symbol.empty()) {
        market = this->market(symbol);
        request["symbol"] = market["id"];
    }
    auto response = this->privateGetFuturesPosition(this->extend(request, params));
    return this->parse_positions(response, market);
}

json hitbtc3::set_leverage(const std::string& symbol, int leverage, const json& params) {
    this->load_markets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]},
        {"leverage", leverage}
    };
    return this->privatePostFuturesLeverage(this->extend(request, params));
}

void hitbtc3::sign(Request& request, const std::string& path,
                   const std::string& api, const std::string& method,
                   const json& params, const json& headers, const json& body) {
    auto endpoint = "/" + this->implode_params(path, params);
    auto query = this->omit(params, this->extract_params(path));
    auto url = this->urls["api"][api] + endpoint;

    if (api == "public") {
        if (!query.empty())
            url += "?" + this->urlencode(query);
    } else {
        this->check_required_credentials();
        
        auto timestamp = std::to_string(this->nonce());
        auto auth = timestamp + method + endpoint;
        
        if (method == "GET" || method == "DELETE") {
            if (!query.empty()) {
                url += "?" + this->urlencode(query);
                auth += "?" + this->urlencode(query);
            }
        } else {
            if (!query.empty()) {
                body = this->json(query);
                auth += body;
            }
        }

        auto signature = this->hmac(auth, this->decode(this->secret), "sha256", "hex");
        
        request.headers["Authorization"] = "HS256 " + this->apiKey + ":" + signature;
        request.headers["Content-Type"] = "application/json";
    }

    request.url = url;
    request.method = method;
    request.body = body.empty() ? "" : body;
    request.headers = this->extend(headers, request.headers);
}

json hitbtc3::parse_ticker(const json& ticker, const json& market) {
    auto timestamp = this->safe_integer(ticker, "timestamp");
    auto symbol = this->safe_string(market, "symbol");
    
    return {
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"high", this->safe_number(ticker, "high")},
        {"low", this->safe_number(ticker, "low")},
        {"bid", this->safe_number(ticker, "bid")},
        {"ask", this->safe_number(ticker, "ask")},
        {"last", this->safe_number(ticker, "last")},
        {"close", this->safe_number(ticker, "last")},
        {"baseVolume", this->safe_number(ticker, "volume")},
        {"quoteVolume", this->safe_number(ticker, "volume_quote")},
        {"info", ticker}
    };
}

std::string hitbtc3::get_signature(const std::string& timestamp, const std::string& method,
                                 const std::string& path, const std::string& body) {
    auto what = timestamp + method + path + body;
    return this->hmac(what, this->decode(this->secret), "sha256", "hex");
}

} // namespace ccxt
