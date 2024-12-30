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
    json response = this->publicGetInstruments(params);
    return this->parse_markets(response["data"]);
}

json blofin::fetch_currencies(const json& params) {
    json response = this->publicGetCurrencies(params);
    return this->parse_currencies(response["data"]);
}

json blofin::fetch_ticker(const std::string& symbol, const json& params) {
    this->load_markets();
    json market = this->market(symbol);
    json request = {
        {"instId", market["id"]}
    };
    json response = this->publicGetTicker(this->extend(request, params));
    return this->parse_ticker(response["data"], market);
}

json blofin::fetch_order_book(const std::string& symbol, int limit, const json& params) {
    this->load_markets();
    json market = this->market(symbol);
    json request = {
        {"instId", market["id"]}
    };
    if (limit > 0) {
        request["sz"] = limit;
    }
    json response = this->publicGetDepth(this->extend(request, params));
    json orderbook = this->parse_order_book(response["data"], symbol);
    orderbook["nonce"] = this->safe_integer(response["data"], "ts");
    return orderbook;
}

json blofin::fetch_trades(const std::string& symbol, int since, int limit, const json& params) {
    this->load_markets();
    json market = this->market(symbol);
    json request = {
        {"instId", market["id"]}
    };
    if (limit > 0) {
        request["limit"] = limit;
    }
    json response = this->publicGetTrades(this->extend(request, params));
    return this->parse_trades(response["data"], market, since, limit);
}

json blofin::fetch_ohlcv(const std::string& symbol, const std::string& timeframe, int since, int limit, const json& params) {
    this->load_markets();
    json market = this->market(symbol);
    json request = {
        {"instId", market["id"]},
        {"bar", this->timeframes[timeframe]}
    };
    if (since > 0) {
        request["after"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    json response = this->publicGetCandles(this->extend(request, params));
    return this->parse_ohlcvs(response["data"], market, timeframe, since, limit);
}

json blofin::create_order(const std::string& symbol, const std::string& type,
                         const std::string& side, double amount, double price, const json& params) {
    this->load_markets();
    json market = this->market(symbol);
    std::string order_type = type.substr(0, 1).upper() + type.substr(1);  // Capitalize first letter
    
    json request = {
        {"instId", market["id"]},
        {"tdMode", "cross"},  // Default to cross margin
        {"side", side.upper()},
        {"ordType", order_type},
        {"sz", this->amount_to_precision(symbol, amount)}
    };
    
    if (type == "limit") {
        if (price <= 0) {
            throw ArgumentsRequired("createOrder() requires a price argument for limit orders");
        }
        request["px"] = this->price_to_precision(symbol, price);
    }
    
    json response = this->privatePostOrder(this->extend(request, params));
    return this->parse_order(response["data"], market);
}

json blofin::cancel_order(const std::string& id, const std::string& symbol, const json& params) {
    if (symbol.empty()) {
        throw ArgumentsRequired("cancelOrder() requires a symbol argument");
    }
    this->load_markets();
    json market = this->market(symbol);
    json request = {
        {"instId", market["id"]},
        {"ordId", id}
    };
    json response = this->privateDeleteOrder(this->extend(request, params));
    return this->parse_order(response["data"], market);
}

json blofin::cancel_all_orders(const std::string& symbol, const json& params) {
    if (symbol.empty()) {
        throw ArgumentsRequired("cancelAllOrders() requires a symbol argument");
    }
    this->load_markets();
    json market = this->market(symbol);
    json request = {
        {"instId", market["id"]}
    };
    json response = this->privateDeleteOrders(this->extend(request, params));
    return response;
}

json blofin::edit_order(const std::string& id, const std::string& symbol, const std::string& type,
                       const std::string& side, double amount, double price, const json& params) {
    this->load_markets();
    json market = this->market(symbol);
    json request = {
        {"instId", market["id"]},
        {"ordId", id}
    };
    
    if (amount > 0) {
        request["newSz"] = this->amount_to_precision(symbol, amount);
    }
    if (price > 0) {
        request["newPx"] = this->price_to_precision(symbol, price);
    }
    
    json response = this->privatePutOrder(this->extend(request, params));
    return this->parse_order(response["data"], market);
}

json blofin::fetch_balance(const json& params) {
    this->load_markets();
    json response = this->privateGetBalance(params);
    return this->parse_balance(response["data"]);
}

json blofin::fetch_open_orders(const std::string& symbol, int since, int limit, const json& params) {
    this->load_markets();
    json request;
    json market;
    if (!symbol.empty()) {
        market = this->market(symbol);
        request["instId"] = market["id"];
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    json response = this->privateGetOrders(this->extend(request, params));
    return this->parse_orders(response["data"], market, since, limit);
}

json blofin::fetch_closed_orders(const std::string& symbol, int since, int limit, const json& params) {
    json request = {
        {"state", "filled"}
    };
    return fetch_open_orders(symbol, since, limit, this->extend(request, params));
}

json blofin::fetch_my_trades(const std::string& symbol, int since, int limit, const json& params) {
    this->load_markets();
    json request;
    json market;
    if (!symbol.empty()) {
        market = this->market(symbol);
        request["instId"] = market["id"];
    }
    if (since > 0) {
        request["start"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    json response = this->privateGetFills(this->extend(request, params));
    return this->parse_trades(response["data"], market, since, limit);
}

json blofin::fetch_order(const std::string& id, const std::string& symbol, const json& params) {
    if (symbol.empty()) {
        throw ArgumentsRequired("fetchOrder() requires a symbol argument");
    }
    this->load_markets();
    json market = this->market(symbol);
    json request = {
        {"instId", market["id"]},
        {"ordId", id}
    };
    json response = this->privateGetOrder(this->extend(request, params));
    return this->parse_order(response["data"], market);
}

json blofin::fetch_deposit_address(const std::string& code, const json& params) {
    this->load_markets();
    json currency = this->currency(code);
    json request = {
        {"ccy", currency["id"]}
    };
    json response = this->privateGetDepositAddress(this->extend(request, params));
    return this->parse_deposit_address(response["data"], currency);
}

json blofin::fetch_deposits(const std::string& code, int since, int limit, const json& params) {
    this->load_markets();
    json request;
    json currency;
    if (!code.empty()) {
        currency = this->currency(code);
        request["ccy"] = currency["id"];
    }
    if (since > 0) {
        request["after"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    json response = this->privateGetDeposits(this->extend(request, params));
    return this->parse_transactions(response["data"], currency, since, limit, {"type", "deposit"});
}

json blofin::fetch_withdrawals(const std::string& code, int since, int limit, const json& params) {
    this->load_markets();
    json request;
    json currency;
    if (!code.empty()) {
        currency = this->currency(code);
        request["ccy"] = currency["id"];
    }
    if (since > 0) {
        request["after"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    json response = this->privateGetWithdrawals(this->extend(request, params));
    return this->parse_transactions(response["data"], currency, since, limit, {"type", "withdrawal"});
}

json blofin::fetch_positions(const std::string& symbol, const json& params) {
    this->load_markets();
    json request;
    json market;
    if (!symbol.empty()) {
        market = this->market(symbol);
        request["instId"] = market["id"];
    }
    json response = this->privateGetPositions(this->extend(request, params));
    return this->parse_positions(response["data"], market);
}

json blofin::fetch_position(const std::string& symbol, const json& params) {
    json response = fetch_positions(symbol, params);
    return this->safe_value(response, 0);
}

json blofin::set_leverage(const std::string& symbol, int leverage, const json& params) {
    this->load_markets();
    if (leverage < 1 || leverage > 125) {
        throw new ExchangeError("Leverage should be between 1 and 125");
    }
    json market = this->market(symbol);
    json request = {
        {"instId", market["id"]},
        {"lever", leverage}
    };
    return this->privatePostSetLeverage(this->extend(request, params));
}

json blofin::set_margin_mode(const std::string& symbol, const std::string& marginMode, const json& params) {
    this->load_markets();
    std::string mode = marginMode.upper();
    if (mode != "CROSS" && mode != "ISOLATED") {
        throw new ExchangeError("Margin mode must be either 'cross' or 'isolated'");
    }
    json market = this->market(symbol);
    json request = {
        {"instId", market["id"]},
        {"marginMode", mode}
    };
    return this->privatePostSetMarginMode(this->extend(request, params));
}

json blofin::fetch_funding_rate(const std::string& symbol, const json& params) {
    this->load_markets();
    json market = this->market(symbol);
    json request = {
        {"instId", market["id"]}
    };
    json response = this->publicGetFundingRate(this->extend(request, params));
    return this->parse_funding_rate(response["data"], market);
}

json blofin::fetch_funding_rate_history(const std::string& symbol, int since, int limit, const json& params) {
    this->load_markets();
    json request;
    json market;
    if (!symbol.empty()) {
        market = this->market(symbol);
        request["instId"] = market["id"];
    }
    if (since > 0) {
        request["start"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    json response = this->publicGetFundingRateHistory(this->extend(request, params));
    return this->parse_funding_rate_history(response["data"], market, since, limit);
}

json blofin::fetch_funding_history(const std::string& symbol, int since, int limit, const json& params) {
    this->load_markets();
    json request;
    json market;
    if (!symbol.empty()) {
        market = this->market(symbol);
        request["instId"] = market["id"];
    }
    if (since > 0) {
        request["start"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    json response = this->privateGetFundingHistory(this->extend(request, params));
    return this->parse_funding_history(response["data"], market, since, limit);
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

        auto signature = this->hmac(auth, this->decode(this->config_.secret), "sha256", "base64");
        
        request.headers["BF-ACCESS-KEY"] = this->config_.apiKey;
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
    return this->hmac(what, this->decode(this->config_.secret), "sha256", "base64");
}

} // namespace ccxt
