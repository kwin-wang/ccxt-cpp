#include "ccxt/exchanges/coinbaseinternational.h"
#include "ccxt/base/json_helper.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <openssl/hmac.h>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace ccxt {

coinbaseinternational::coinbaseinternational(const Config& config) : exchange(config) {
    this->describe({
        {"id", "coinbaseinternational"},
        {"name", "Coinbase International"},
        {"countries", json::array({"US"})},
        {"version", "v3"},
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
            {"withdraw", true}
        }},
        {"urls", {
            {"logo", "https://user-images.githubusercontent.com/1294454/108623144-67a3ef00-744e-11eb-8140-75c6b851e945.jpg"},
            {"api", {
                {"public", "https://api.coinbase.com"},
                {"private", "https://api.coinbase.com"}
            }},
            {"www", "https://www.coinbase.com"},
            {"doc", {
                "https://docs.cloud.coinbase.com/exchange/reference"
            }},
            {"fees", "https://www.coinbase.com/legal/fees"}
        }},
        {"api", {
            {"public", {
                {"get", {
                    "products",
                    "products/{id}/book",
                    "products/{id}/ticker",
                    "products/{id}/trades",
                    "products/{id}/candles",
                    "currencies",
                    "time"
                }}
            }},
            {"private", {
                {"get", {
                    "accounts",
                    "accounts/{id}",
                    "orders",
                    "orders/{id}",
                    "fills",
                    "payment-methods",
                    "coinbase-accounts",
                    "fees"
                }},
                {"post", {
                    "orders",
                    "deposits/coinbase-account",
                    "deposits/payment-method",
                    "withdrawals/coinbase-account",
                    "withdrawals/crypto",
                    "withdrawals/payment-method"
                }},
                {"delete", {
                    "orders",
                    "orders/{id}"
                }}
            }}
        }},
        {"fees", {
            {"trading", {
                {"maker", 0.004},
                {"taker", 0.006}
            }}
        }},
        {"requiredCredentials", {
            {"apiKey", true},
            {"secret", true},
            {"password", true}
        }},
        {"timeframes", {
            {"1m", "60"},
            {"5m", "300"},
            {"15m", "900"},
            {"1h", "3600"},
            {"6h", "21600"},
            {"1d", "86400"}
        }}
    });

    this->options = {
        {"fetchOrderBookLimit", 100}
    };
}

json coinbaseinternational::fetch_markets(const json& params) {
    auto response = this->publicGetProducts(params);
    return this->parse_markets(response);
}

json coinbaseinternational::fetch_currencies(const json& params) {
    auto response = this->publicGetCurrencies(params);
    return this->parse_currencies(response);
}

json coinbaseinternational::fetch_ticker(const std::string& symbol, const json& params) {
    this->load_markets();
    auto market = this->market(symbol);
    auto request = {
        {"id", market["id"]}
    };
    auto response = this->publicGetProductsIdTicker(this->extend(request, params));
    return this->parse_ticker(response, market);
}

json coinbaseinternational::fetch_order_book(const std::string& symbol, int limit, const json& params) {
    this->load_markets();
    auto market = this->market(symbol);
    auto request = {
        {"id", market["id"]},
        {"level", limit ? 2 : 1}
    };
    auto response = this->publicGetProductsIdBook(this->extend(request, params));
    return this->parse_order_book(response, symbol);
}

json coinbaseinternational::fetch_trades(const std::string& symbol, int since, int limit, const json& params) {
    this->load_markets();
    auto market = this->market(symbol);
    auto request = {
        {"id", market["id"]}
    };
    if (limit)
        request["limit"] = limit;
    auto response = this->publicGetProductsIdTrades(this->extend(request, params));
    return this->parse_trades(response, market, since, limit);
}

json coinbaseinternational::fetch_ohlcv(const std::string& symbol, const std::string& timeframe, int since, int limit, const json& params) {
    this->load_markets();
    auto market = this->market(symbol);
    auto request = {
        {"id", market["id"]},
        {"granularity", this->timeframes[timeframe]}
    };
    if (since)
        request["start"] = this->iso8601(since);
    if (limit)
        request["end"] = this->iso8601(this->sum(since, limit * this->parse_timeframe(timeframe) * 1000));
    auto response = this->publicGetProductsIdCandles(this->extend(request, params));
    return this->parse_ohlcvs(response, market, timeframe, since, limit);
}

json coinbaseinternational::create_order(const std::string& symbol, const std::string& type,
                                       const std::string& side, double amount, double price, const json& params) {
    this->load_markets();
    auto market = this->market(symbol);
    auto request = {
        {"product_id", market["id"]},
        {"side", side},
        {"type", type},
        {"size", this->amount_to_precision(symbol, amount)}
    };
    if (type == "limit")
        request["price"] = this->price_to_precision(symbol, price);
    auto response = this->privatePostOrders(this->extend(request, params));
    return this->parse_order(response, market);
}

json coinbaseinternational::cancel_order(const std::string& id, const std::string& symbol, const json& params) {
    this->load_markets();
    auto request = {{"id", id}};
    return this->privateDeleteOrdersId(this->extend(request, params));
}

json coinbaseinternational::cancel_all_orders(const std::string& symbol, const json& params) {
    this->load_markets();
    auto request = json::object();
    if (!symbol.empty()) {
        auto market = this->market(symbol);
        request["product_id"] = market["id"];
    }
    return this->privateDeleteOrders(this->extend(request, params));
}

json coinbaseinternational::fetch_order(const std::string& id, const std::string& symbol, const json& params) {
    this->load_markets();
    auto request = {{"id", id}};
    auto response = this->privateGetOrdersId(this->extend(request, params));
    return this->parse_order(response);
}

json coinbaseinternational::fetch_open_orders(const std::string& symbol, int since, int limit, const json& params) {
    this->load_markets();
    auto request = json::object();
    auto market = nullptr;
    if (!symbol.empty()) {
        market = this->market(symbol);
        request["product_id"] = market["id"];
    }
    if (limit)
        request["limit"] = limit;
    auto response = this->privateGetOrders(this->extend(request, params));
    return this->parse_orders(response, market, since, limit);
}

json coinbaseinternational::fetch_my_trades(const std::string& symbol, int since, int limit, const json& params) {
    this->load_markets();
    auto request = json::object();
    if (!symbol.empty()) {
        auto market = this->market(symbol);
        request["product_id"] = market["id"];
    }
    if (limit)
        request["limit"] = limit;
    auto response = this->privateGetFills(this->extend(request, params));
    return this->parse_trades(response, nullptr, since, limit);
}

json coinbaseinternational::fetch_balance(const json& params) {
    this->load_markets();
    auto response = this->privateGetAccounts(params);
    return this->parse_balance(response);
}

void coinbaseinternational::sign(Request& request, const std::string& path,
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

        auto signature = this->hmac(auth, this->decode(this->secret), "sha256", "hex");
        
        request.headers["CB-ACCESS-KEY"] = this->apiKey;
        request.headers["CB-ACCESS-SIGN"] = signature;
        request.headers["CB-ACCESS-TIMESTAMP"] = timestamp;
        request.headers["CB-ACCESS-PASSPHRASE"] = this->password;
        request.headers["Content-Type"] = "application/json";
    }

    request.url = this->urls["api"][api] + endpoint;
    request.method = method;
    request.body = body.empty() ? "" : this->json(body);
    request.headers = this->extend(headers, request.headers);
}

std::string coinbaseinternational::get_signature(const std::string& timestamp, const std::string& method,
                                               const std::string& path, const std::string& body) {
    auto what = timestamp + method + path + body;
    return this->hmac(what, this->decode(this->secret), "sha256", "hex");
}

// Async Market Data Methods
boost::future<json> coinbaseinternational::fetch_markets_async(const json& params) {
    return boost::async(boost::launch::async, [this, params]() {
        return this->fetch_markets(params);
    });
}

boost::future<json> coinbaseinternational::fetch_currencies_async(const json& params) {
    return boost::async(boost::launch::async, [this, params]() {
        return this->fetch_currencies(params);
    });
}

boost::future<json> coinbaseinternational::fetch_ticker_async(const std::string& symbol, const json& params) {
    return boost::async(boost::launch::async, [this, symbol, params]() {
        return this->fetch_ticker(symbol, params);
    });
}

boost::future<json> coinbaseinternational::fetch_order_book_async(const std::string& symbol, int limit, const json& params) {
    return boost::async(boost::launch::async, [this, symbol, limit, params]() {
        return this->fetch_order_book(symbol, limit, params);
    });
}

boost::future<json> coinbaseinternational::fetch_trades_async(const std::string& symbol, int since, int limit, const json& params) {
    return boost::async(boost::launch::async, [this, symbol, since, limit, params]() {
        return this->fetch_trades(symbol, since, limit, params);
    });
}

boost::future<json> coinbaseinternational::fetch_ohlcv_async(const std::string& symbol, const std::string& timeframe, int since, int limit, const json& params) {
    return boost::async(boost::launch::async, [this, symbol, timeframe, since, limit, params]() {
        return this->fetch_ohlcv(symbol, timeframe, since, limit, params);
    });
}

boost::future<json> coinbaseinternational::fetch_trading_fees_async(const std::string& symbol, const json& params) {
    return boost::async(boost::launch::async, [this, symbol, params]() {
        return this->fetch_trading_fees(symbol, params);
    });
}

// Async Trading Methods
boost::future<json> coinbaseinternational::create_order_async(const std::string& symbol, const std::string& type, const std::string& side,
                                                            double amount, double price, const json& params) {
    return boost::async(boost::launch::async, [this, symbol, type, side, amount, price, params]() {
        return this->create_order(symbol, type, side, amount, price, params);
    });
}

boost::future<json> coinbaseinternational::cancel_order_async(const std::string& id, const std::string& symbol, const json& params) {
    return boost::async(boost::launch::async, [this, id, symbol, params]() {
        return this->cancel_order(id, symbol, params);
    });
}

boost::future<json> coinbaseinternational::cancel_all_orders_async(const std::string& symbol, const json& params) {
    return boost::async(boost::launch::async, [this, symbol, params]() {
        return this->cancel_all_orders(symbol, params);
    });
}

boost::future<json> coinbaseinternational::edit_order_async(const std::string& id, const std::string& symbol, const std::string& type, const std::string& side,
                                                          double amount, double price, const json& params) {
    return boost::async(boost::launch::async, [this, id, symbol, type, side, amount, price, params]() {
        return this->edit_order(id, symbol, type, side, amount, price, params);
    });
}

// Async Account Methods
boost::future<json> coinbaseinternational::fetch_balance_async(const json& params) {
    return boost::async(boost::launch::async, [this, params]() {
        return this->fetch_balance(params);
    });
}

boost::future<json> coinbaseinternational::fetch_open_orders_async(const std::string& symbol, int since, int limit, const json& params) {
    return boost::async(boost::launch::async, [this, symbol, since, limit, params]() {
        return this->fetch_open_orders(symbol, since, limit, params);
    });
}

boost::future<json> coinbaseinternational::fetch_closed_orders_async(const std::string& symbol, int since, int limit, const json& params) {
    return boost::async(boost::launch::async, [this, symbol, since, limit, params]() {
        return this->fetch_closed_orders(symbol, since, limit, params);
    });
}

boost::future<json> coinbaseinternational::fetch_my_trades_async(const std::string& symbol, int since, int limit, const json& params) {
    return boost::async(boost::launch::async, [this, symbol, since, limit, params]() {
        return this->fetch_my_trades(symbol, since, limit, params);
    });
}

boost::future<json> coinbaseinternational::fetch_order_async(const std::string& id, const std::string& symbol, const json& params) {
    return boost::async(boost::launch::async, [this, id, symbol, params]() {
        return this->fetch_order(id, symbol, params);
    });
}

boost::future<json> coinbaseinternational::fetch_deposit_address_async(const std::string& code, const json& params) {
    return boost::async(boost::launch::async, [this, code, params]() {
        return this->fetch_deposit_address(code, params);
    });
}

boost::future<json> coinbaseinternational::fetch_deposits_async(const std::string& code, int since, int limit, const json& params) {
    return boost::async(boost::launch::async, [this, code, since, limit, params]() {
        return this->fetch_deposits(code, since, limit, params);
    });
}

boost::future<json> coinbaseinternational::fetch_withdrawals_async(const std::string& code, int since, int limit, const json& params) {
    return boost::async(boost::launch::async, [this, code, since, limit, params]() {
        return this->fetch_withdrawals(code, since, limit, params);
    });
}

boost::future<json> coinbaseinternational::withdraw_async(const std::string& code, double amount, const std::string& address, const std::string& tag, const json& params) {
    return boost::async(boost::launch::async, [this, code, amount, address, tag, params]() {
        return this->withdraw(code, amount, address, tag, params);
    });
}

} // namespace ccxt
