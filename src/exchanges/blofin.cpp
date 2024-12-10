#include "ccxt/exchanges/blofin.h"
#include "ccxt/base/json_helper.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <openssl/hmac.h>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace ccxt {

blofin::blofin(const Config& config) : exchange(config) {
    this->describe({
        {"id", "blofin"},
        {"name", "Blofin"},
        {"countries", json::array({"SG"})},  // Singapore
        {"version", "v1"},
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
            {"fetchLeverageTiers", true}
        }},
        {"urls", {
            {"logo", "https://user-images.githubusercontent.com/1294454/187151936-0c543991-3a1c-4c93-b4cb-33c4f8e3d567.jpg"},
            {"api", {
                {"public", "https://api.blofin.com"},
                {"private", "https://api.blofin.com"}
            }},
            {"www", "https://blofin.com"},
            {"doc", {
                "https://docs.blofin.com/api"
            }},
            {"fees", "https://blofin.com/fees"}
        }},
        {"api", {
            {"public", {
                {"get", {
                    "api/v1/public/instruments",
                    "api/v1/public/currencies",
                    "api/v1/public/ticker",
                    "api/v1/public/depth",
                    "api/v1/public/trades",
                    "api/v1/public/candles",
                    "api/v1/public/funding-rate",
                    "api/v1/public/funding-rate-history",
                    "api/v1/public/leverage-tiers"
                }}
            }},
            {"private", {
                {"get", {
                    "api/v1/account/balance",
                    "api/v1/account/positions",
                    "api/v1/account/leverage",
                    "api/v1/trade/orders",
                    "api/v1/trade/orders/{orderId}",
                    "api/v1/trade/fills",
                    "api/v1/asset/deposit-address",
                    "api/v1/asset/deposits",
                    "api/v1/asset/withdrawals",
                    "api/v1/account/funding-history"
                }},
                {"post", {
                    "api/v1/trade/order",
                    "api/v1/trade/batch-orders",
                    "api/v1/account/set-leverage",
                    "api/v1/account/set-margin-mode",
                    "api/v1/asset/withdrawal"
                }},
                {"delete", {
                    "api/v1/trade/order/{orderId}",
                    "api/v1/trade/orders"
                }},
                {"put", {
                    "api/v1/trade/order/{orderId}"
                }}
            }}
        }},
        {"fees", {
            {"trading", {
                {"maker", 0.0002},  // 0.02%
                {"taker", 0.0005}   // 0.05%
            }}
        }},
        {"timeframes", {
            {"1m", "1min"},
            {"3m", "3min"},
            {"5m", "5min"},
            {"15m", "15min"},
            {"30m", "30min"},
            {"1h", "1hour"},
            {"2h", "2hour"},
            {"4h", "4hour"},
            {"6h", "6hour"},
            {"12h", "12hour"},
            {"1d", "1day"},
            {"1w", "1week"},
            {"1M", "1month"}
        }}
    });
}

json blofin::fetch_markets(const json& params) {
    auto response = this->publicGetApiV1PublicInstruments(params);
    return this->parse_markets(response);
}

json blofin::fetch_currencies(const json& params) {
    auto response = this->publicGetApiV1PublicCurrencies(params);
    return this->parse_currencies(response);
}

json blofin::fetch_ticker(const std::string& symbol, const json& params) {
    this->load_markets();
    auto market = this->market(symbol);
    auto request = {
        {"instId", market["id"]}
    };
    auto response = this->publicGetApiV1PublicTicker(this->extend(request, params));
    return this->parse_ticker(response, market);
}

json blofin::fetch_order_book(const std::string& symbol, int limit, const json& params) {
    this->load_markets();
    auto market = this->market(symbol);
    auto request = {
        {"instId", market["id"]}
    };
    if (limit)
        request["sz"] = limit;
    auto response = this->publicGetApiV1PublicDepth(this->extend(request, params));
    return this->parse_order_book(response, symbol);
}

json blofin::create_order(const std::string& symbol, const std::string& type,
                         const std::string& side, double amount, double price, const json& params) {
    this->load_markets();
    auto market = this->market(symbol);
    auto request = {
        {"instId", market["id"]},
        {"tdMode", "cross"},  // cross/isolated
        {"side", side},
        {"ordType", type},
        {"sz", this->amount_to_precision(symbol, amount)}
    };
    if (type == "limit")
        request["px"] = this->price_to_precision(symbol, price);
    auto response = this->privatePostApiV1TradeOrder(this->extend(request, params));
    return this->parse_order(response, market);
}

json blofin::cancel_order(const std::string& id, const std::string& symbol, const json& params) {
    if (symbol.empty())
        throw std::runtime_error("symbol is required for cancel_order");
    this->load_markets();
    auto market = this->market(symbol);
    auto request = {
        {"instId", market["id"]},
        {"ordId", id}
    };
    auto response = this->privateDeleteApiV1TradeOrderOrderId(this->extend(request, params));
    return this->parse_order(response, market);
}

json blofin::fetch_balance(const json& params) {
    this->load_markets();
    auto response = this->privateGetApiV1AccountBalance(params);
    return this->parse_balance(response);
}

json blofin::fetch_positions(const std::string& symbol, const json& params) {
    this->load_markets();
    auto request = json::object();
    if (!symbol.empty()) {
        auto market = this->market(symbol);
        request["instId"] = market["id"];
    }
    auto response = this->privateGetApiV1AccountPositions(this->extend(request, params));
    return this->parse_positions(response);
}

json blofin::set_leverage(const std::string& symbol, int leverage, const json& params) {
    this->load_markets();
    auto market = this->market(symbol);
    auto request = {
        {"instId", market["id"]},
        {"lever", leverage}
    };
    return this->privatePostApiV1AccountSetLeverage(this->extend(request, params));
}

json blofin::fetch_funding_rate(const std::string& symbol, const json& params) {
    this->load_markets();
    auto market = this->market(symbol);
    auto request = {
        {"instId", market["id"]}
    };
    auto response = this->publicGetApiV1PublicFundingRate(this->extend(request, params));
    return this->parse_funding_rate(response);
}

void blofin::sign(Request& request, const std::string& path,
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

        auto signature = this->hmac(auth, this->decode(this->secret), "sha256", "base64");
        
        request.headers["BF-ACCESS-KEY"] = this->apiKey;
        request.headers["BF-ACCESS-SIGN"] = signature;
        request.headers["BF-ACCESS-TIMESTAMP"] = timestamp;
        request.headers["Content-Type"] = "application/json";
    }

    request.url = url;
    request.method = method;
    request.body = body.empty() ? "" : body;
    request.headers = this->extend(headers, request.headers);
}

std::string blofin::get_signature(const std::string& timestamp, const std::string& method,
                                const std::string& path, const std::string& body) {
    auto what = timestamp + method + path + body;
    return this->hmac(what, this->decode(this->secret), "sha256", "base64");
}

} // namespace ccxt
