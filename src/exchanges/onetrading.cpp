#include "ccxt/exchanges/onetrading.h"
#include "ccxt/base/json_helper.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <openssl/hmac.h>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace ccxt {

onetrading::onetrading(const Config& config) : exchange(config) {
    this->describe({
        {"id", "onetrading"},
        {"name", "OneTrading"},
        {"countries", json::array({"EU"})},
        {"version", "v1"},
        {"rateLimit", 50},
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
            {"fetchFundingHistory", true},
            {"fetchPositions", true},
            {"setLeverage", true},
            {"setMarginMode", true}
        }},
        {"urls", {
            {"logo", "https://user-images.githubusercontent.com/1294454/152485636-38b19e4a-bece-4dec-979a-5982859ffc04.jpg"},
            {"api", {
                {"public", "https://api.onetrading.com"},
                {"private", "https://api.onetrading.com"}
            }},
            {"www", "https://onetrading.com"},
            {"doc", {
                "https://docs.onetrading.com"
            }},
            {"fees", "https://onetrading.com/fees"}
        }},
        {"api", {
            {"public", {
                {"get", {
                    "markets",
                    "currencies",
                    "ticker/{symbol}",
                    "orderbook/{symbol}",
                    "trades/{symbol}",
                    "candles/{symbol}",
                    "fees/trading"
                }}
            }},
            {"private", {
                {"get", {
                    "accounts",
                    "orders",
                    "orders/{id}",
                    "trades",
                    "deposits",
                    "withdrawals",
                    "deposit-addresses/{currency}",
                    "positions",
                    "funding-history"
                }},
                {"post", {
                    "orders",
                    "withdrawals",
                    "leverage",
                    "margin-mode"
                }},
                {"put", {
                    "orders/{id}"
                }},
                {"delete", {
                    "orders/{id}",
                    "orders"
                }}
            }}
        }},
        {"fees", {
            {"trading", {
                {"maker", 0.001},
                {"taker", 0.002}
            }}
        }},
        {"timeframes", {
            {"1m", "1min"},
            {"5m", "5min"},
            {"15m", "15min"},
            {"30m", "30min"},
            {"1h", "1hour"},
            {"4h", "4hour"},
            {"1d", "1day"},
            {"1w", "1week"},
            {"1M", "1month"}
        }}
    });
}

json onetrading::fetch_markets(const json& params) {
    auto response = this->publicGetMarkets(params);
    return this->parse_markets(response);
}

json onetrading::fetch_currencies(const json& params) {
    auto response = this->publicGetCurrencies(params);
    return this->parse_currencies(response);
}

json onetrading::fetch_ticker(const std::string& symbol, const json& params) {
    this->load_markets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]}
    };
    auto response = this->publicGetTickerSymbol(this->extend(request, params));
    return this->parse_ticker(response, market);
}

json onetrading::fetch_order_book(const std::string& symbol, int limit, const json& params) {
    this->load_markets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]}
    };
    if (limit)
        request["limit"] = limit;
    auto response = this->publicGetOrderbookSymbol(this->extend(request, params));
    return this->parse_order_book(response, symbol);
}

json onetrading::fetch_trades(const std::string& symbol, int since, int limit, const json& params) {
    this->load_markets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]}
    };
    if (since)
        request["since"] = since;
    if (limit)
        request["limit"] = limit;
    auto response = this->publicGetTradesSymbol(this->extend(request, params));
    return this->parse_trades(response, market, since, limit);
}

json onetrading::create_order(const std::string& symbol, const std::string& type,
                            const std::string& side, double amount, double price, const json& params) {
    this->load_markets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]},
        {"type", type},
        {"side", side},
        {"amount", this->amount_to_precision(symbol, amount)}
    };
    if (type == "limit")
        request["price"] = this->price_to_precision(symbol, price);
    auto response = this->privatePostOrders(this->extend(request, params));
    return this->parse_order(response, market);
}

json onetrading::cancel_order(const std::string& id, const std::string& symbol, const json& params) {
    this->load_markets();
    auto request = {{"id", id}};
    return this->privateDeleteOrdersId(this->extend(request, params));
}

json onetrading::fetch_balance(const json& params) {
    this->load_markets();
    auto response = this->privateGetAccounts(params);
    return this->parse_balance(response);
}

json onetrading::fetch_open_orders(const std::string& symbol, int since, int limit, const json& params) {
    this->load_markets();
    auto request = json::object();
    auto market = nullptr;
    if (!symbol.empty()) {
        market = this->market(symbol);
        request["symbol"] = market["id"];
    }
    if (since)
        request["since"] = since;
    if (limit)
        request["limit"] = limit;
    auto response = this->privateGetOrders(this->extend(request, params));
    return this->parse_orders(response, market, since, limit);
}

json onetrading::fetch_my_trades(const std::string& symbol, int since, int limit, const json& params) {
    this->load_markets();
    auto request = json::object();
    auto market = nullptr;
    if (!symbol.empty()) {
        market = this->market(symbol);
        request["symbol"] = market["id"];
    }
    if (since)
        request["since"] = since;
    if (limit)
        request["limit"] = limit;
    auto response = this->privateGetTrades(this->extend(request, params));
    return this->parse_trades(response, market, since, limit);
}

json onetrading::fetch_positions(const std::string& symbol, const json& params) {
    this->load_markets();
    auto request = json::object();
    if (!symbol.empty()) {
        auto market = this->market(symbol);
        request["symbol"] = market["id"];
    }
    auto response = this->privateGetPositions(this->extend(request, params));
    return this->parse_positions(response);
}

json onetrading::set_leverage(const std::string& symbol, int leverage, const json& params) {
    this->load_markets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]},
        {"leverage", leverage}
    };
    return this->privatePostLeverage(this->extend(request, params));
}

json onetrading::set_margin_mode(const std::string& symbol, const std::string& marginMode, const json& params) {
    this->load_markets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]},
        {"marginMode", marginMode}
    };
    return this->privatePostMarginMode(this->extend(request, params));
}

// Async Market Data
json onetrading::fetch_markets_async(const json& params) {
    return std::async(std::launch::async, [this, params]() {
        return this->fetch_markets(params);
    });
}

json onetrading::fetch_currencies_async(const json& params) {
    return std::async(std::launch::async, [this, params]() {
        return this->fetch_currencies(params);
    });
}

json onetrading::fetch_ticker_async(const std::string& symbol, const json& params) {
    return std::async(std::launch::async, [this, symbol, params]() {
        return this->fetch_ticker(symbol, params);
    });
}

json onetrading::fetch_order_book_async(const std::string& symbol, int limit, const json& params) {
    return std::async(std::launch::async, [this, symbol, limit, params]() {
        return this->fetch_order_book(symbol, limit, params);
    });
}

json onetrading::fetch_trades_async(const std::string& symbol, int since, int limit, const json& params) {
    return std::async(std::launch::async, [this, symbol, since, limit, params]() {
        return this->fetch_trades(symbol, since, limit, params);
    });
}

json onetrading::fetch_ohlcv_async(const std::string& symbol, const std::string& timeframe,
                                  int since, int limit, const json& params) {
    return std::async(std::launch::async, [this, symbol, timeframe, since, limit, params]() {
        return this->fetch_ohlcv(symbol, timeframe, since, limit, params);
    });
}

json onetrading::fetch_trading_fees_async(const std::string& symbol, const json& params) {
    return std::async(std::launch::async, [this, symbol, params]() {
        return this->fetch_trading_fees(symbol, params);
    });
}

// Async Trading
json onetrading::create_order_async(const std::string& symbol, const std::string& type,
                                  const std::string& side, double amount,
                                  double price, const json& params) {
    return std::async(std::launch::async, [this, symbol, type, side, amount, price, params]() {
        return this->create_order(symbol, type, side, amount, price, params);
    });
}

json onetrading::cancel_order_async(const std::string& id, const std::string& symbol, const json& params) {
    return std::async(std::launch::async, [this, id, symbol, params]() {
        return this->cancel_order(id, symbol, params);
    });
}

json onetrading::cancel_all_orders_async(const std::string& symbol, const json& params) {
    return std::async(std::launch::async, [this, symbol, params]() {
        return this->cancel_all_orders(symbol, params);
    });
}

json onetrading::edit_order_async(const std::string& id, const std::string& symbol,
                                const std::string& type, const std::string& side,
                                double amount, double price, const json& params) {
    return std::async(std::launch::async, [this, id, symbol, type, side, amount, price, params]() {
        return this->edit_order(id, symbol, type, side, amount, price, params);
    });
}

// Async Account
json onetrading::fetch_balance_async(const json& params) {
    return std::async(std::launch::async, [this, params]() {
        return this->fetch_balance(params);
    });
}

json onetrading::fetch_open_orders_async(const std::string& symbol, int since, int limit, const json& params) {
    return std::async(std::launch::async, [this, symbol, since, limit, params]() {
        return this->fetch_open_orders(symbol, since, limit, params);
    });
}

json onetrading::fetch_closed_orders_async(const std::string& symbol, int since, int limit, const json& params) {
    return std::async(std::launch::async, [this, symbol, since, limit, params]() {
        return this->fetch_closed_orders(symbol, since, limit, params);
    });
}

json onetrading::fetch_my_trades_async(const std::string& symbol, int since, int limit, const json& params) {
    return std::async(std::launch::async, [this, symbol, since, limit, params]() {
        return this->fetch_my_trades(symbol, since, limit, params);
    });
}

json onetrading::fetch_order_async(const std::string& id, const std::string& symbol, const json& params) {
    return std::async(std::launch::async, [this, id, symbol, params]() {
        return this->fetch_order(id, symbol, params);
    });
}

json onetrading::fetch_deposit_address_async(const std::string& code, const json& params) {
    return std::async(std::launch::async, [this, code, params]() {
        return this->fetch_deposit_address(code, params);
    });
}

json onetrading::fetch_deposits_async(const std::string& code, int since, int limit, const json& params) {
    return std::async(std::launch::async, [this, code, since, limit, params]() {
        return this->fetch_deposits(code, since, limit, params);
    });
}

json onetrading::fetch_withdrawals_async(const std::string& code, int since, int limit, const json& params) {
    return std::async(std::launch::async, [this, code, since, limit, params]() {
        return this->fetch_withdrawals(code, since, limit, params);
    });
}

json onetrading::withdraw_async(const std::string& code, double amount,
                              const std::string& address, const std::string& tag,
                              const json& params) {
    return std::async(std::launch::async, [this, code, amount, address, tag, params]() {
        return this->withdraw(code, amount, address, tag, params);
    });
}

// Async OneTrading specific methods
json onetrading::fetch_funding_history_async(const std::string& symbol, int since, int limit, const json& params) {
    return std::async(std::launch::async, [this, symbol, since, limit, params]() {
        return this->fetch_funding_history(symbol, since, limit, params);
    });
}

json onetrading::fetch_positions_async(const std::string& symbol, const json& params) {
    return std::async(std::launch::async, [this, symbol, params]() {
        return this->fetch_positions(symbol, params);
    });
}

json onetrading::set_leverage_async(const std::string& symbol, int leverage, const json& params) {
    return std::async(std::launch::async, [this, symbol, leverage, params]() {
        return this->set_leverage(symbol, leverage, params);
    });
}

json onetrading::set_margin_mode_async(const std::string& symbol, const std::string& marginMode, const json& params) {
    return std::async(std::launch::async, [this, symbol, marginMode, params]() {
        return this->set_margin_mode(symbol, marginMode, params);
    });
}

void onetrading::sign(Request& request, const std::string& path,
                     const std::string& api, const std::string& method,
                     const json& params, const json& headers, const json& body) {
    auto endpoint = "/" + this->version + "/" + this->implode_params(path, params);
    auto query = this->omit(params, this->extract_params(path));

    if (api == "public") {
        if (!query.empty())
            endpoint += "?" + this->urlencode(query);
    } else {
        this->check_required_credentials();
        
        auto timestamp = std::to_string(this->nonce());
        auto auth = timestamp + method + endpoint;
        
        if (method == "GET") {
            if (!query.empty())
                endpoint += "?" + this->urlencode(query);
        } else {
            if (!query.empty()) {
                body = query;
                auth += this->json(body);
            }
        }

        auto signature = this->hmac(auth, this->decode(this->config_.secret), "sha256", "hex");
        
        request.headers["ONE-ACCESS-KEY"] = this->config_.apiKey;
        request.headers["ONE-ACCESS-SIGN"] = signature;
        request.headers["ONE-ACCESS-TIMESTAMP"] = timestamp;
        request.headers["Content-Type"] = "application/json";
    }

    request.url = this->urls["api"][api] + endpoint;
    request.method = method;
    request.body = body.empty() ? "" : this->json(body);
    request.headers = this->extend(headers, request.headers);
}

std::string onetrading::get_signature(const std::string& timestamp, const std::string& method,
                                    const std::string& path, const std::string& body) {
    auto what = timestamp + method + path + body;
    return this->hmac(what, this->decode(this->config_.secret), "sha256", "hex");
}

} // namespace ccxt
