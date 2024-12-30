#include "ccxt/exchanges/hitbtc2.h"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>
#include <future>

namespace ccxt {

HitBTC2::HitBTC2() {
    this->id = "hitbtc2";
    this->name = "HitBTC";
    this->countries = {"HK"};  // Hong Kong
    this->version = "2";
    this->rateLimit = 1500;
    this->has = {
        {"cancelAllOrders", true},
        {"cancelOrder", true},
        {"createOrder", true},
        {"fetchBalance", true},
        {"fetchClosedOrders", true},
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
        {"fetchWithdrawals", true},
        {"withdraw", true}
    };

    this->timeframes = {
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
    };

    this->urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/27766555-8eaec20e-5edc-11e7-9c5b-6dc69fc42f5e.jpg"},
        {"api", {
            {"public", "https://api.hitbtc.com/api/2/public"},
            {"private", "https://api.hitbtc.com/api/2"}
        }},
        {"www", "https://hitbtc.com"},
        {"doc", {
            "https://api.hitbtc.com",
            "https://github.com/hitbtc-com/hitbtc-api"
        }},
        {"fees", "https://hitbtc.com/fees-and-limits"}
    };

    this->api = {
        {"public", {
            {"GET", {
                "symbol",
                "symbol/{symbol}",
                "currency",
                "currency/{currency}",
                "ticker",
                "ticker/{symbol}",
                "trades/{symbol}",
                "orderbook/{symbol}",
                "candles/{symbol}"
            }}
        }},
        {"private", {
            {"GET", {
                "order",
                "order/{clientOrderId}",
                "trading/balance",
                "trading/fee/{symbol}",
                "history/trades",
                "history/order",
                "history/order/{orderId}/trades",
                "account/balance",
                "account/transactions",
                "account/transactions/{id}",
                "account/crypto/address/{currency}"
            }},
            {"POST", {
                "order",
                "account/crypto/withdraw",
                "account/crypto/address/{currency}"
            }},
            {"PUT", {
                "order/{clientOrderId}"
            }},
            {"DELETE", {
                "order",
                "order/{clientOrderId}"
            }}
        }}
    };

    this->fees = {
        {"trading", {
            {"tierBased", true},
            {"percentage", true},
            {"maker", 0.1 / 100},
            {"taker", 0.2 / 100},
            {"tiers", {
                {"maker", {
                    {0, 0.1 / 100},
                    {10, 0.08 / 100},
                    {100, 0.06 / 100},
                    {500, 0.04 / 100},
                    {1000, 0.02 / 100},
                    {5000, 0}
                }},
                {"taker", {
                    {0, 0.2 / 100},
                    {10, 0.18 / 100},
                    {100, 0.16 / 100},
                    {500, 0.14 / 100},
                    {1000, 0.12 / 100},
                    {5000, 0.1 / 100}
                }}
            }}
        }},
        {"funding", {
            {"withdraw", {}},
            {"deposit", {}}
        }}
    };
}

nlohmann::json HitBTC2::fetch_markets() {
    auto response = this->publicGetSymbol();
    return this->parse_markets(response);
}

nlohmann::json HitBTC2::fetch_ticker(const std::string& symbol) {
    this->load_markets();
    auto market = this->market(symbol);
    auto request = {{"symbol", market["id"]}};
    auto response = this->publicGetTickerSymbol(request);
    return this->parse_ticker(response, market);
}

nlohmann::json HitBTC2::fetch_order_book(const std::string& symbol, int limit) {
    this->load_markets();
    auto request = {
        {"symbol", this->market_id(symbol)},
        {"limit", limit ? limit : 100}
    };
    auto response = this->publicGetOrderbookSymbol(request);
    return this->parse_order_book(response, symbol);
}

nlohmann::json HitBTC2::create_order(const std::string& symbol, const std::string& type,
                                   const std::string& side, double amount, double price) {
    this->load_markets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]},
        {"side", side},
        {"type", type},
        {"quantity", this->amount_to_precision(symbol, amount)}
    };
    if (type == "limit") {
        request["price"] = this->price_to_precision(symbol, price);
    }
    auto response = this->privatePostOrder(request);
    return this->parse_order(response);
}

nlohmann::json HitBTC2::cancel_order(const std::string& id, const std::string& symbol) {
    this->load_markets();
    auto request = {{"clientOrderId", id}};
    return this->privateDeleteOrderClientOrderId(request);
}

nlohmann::json HitBTC2::fetch_balance() {
    this->load_markets();
    auto response = this->privateGetTradingBalance();
    return this->parse_balance(response);
}

std::string HitBTC2::sign(const std::string& path, const std::string& api,
                        const std::string& method, const nlohmann::json& params,
                        const std::map<std::string, std::string>& headers) {
    auto endpoint = "/" + this->implode_params(path, params);
    auto query = this->omit(params, this->extract_params(path));
    auto url = this->urls["api"][api] + endpoint;

    if (api == "public") {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->check_required_credentials();
        auto auth = this->config_.apiKey + ":" + this->config_.secret;
        auto auth_base64 = this->string_to_base64(auth);
        auto headers_map = headers;
        headers_map["Authorization"] = "Basic " + auth_base64;
        
        if (!query.empty()) {
            if (method == "GET") {
                url += "?" + this->urlencode(query);
            } else {
                headers_map["Content-Type"] = "application/json";
            }
        }
    }

    return url;
}

// Async implementations
std::future<nlohmann::json> HitBTC2::fetch_markets_async() {
    return std::async(std::launch::async, [this]() {
        return this->fetch_markets();
    });
}

std::future<nlohmann::json> HitBTC2::fetch_ticker_async(const std::string& symbol) {
    return std::async(std::launch::async, [this, symbol]() {
        return this->fetch_ticker(symbol);
    });
}

std::future<nlohmann::json> HitBTC2::fetch_tickers_async(const std::vector<std::string>& symbols) {
    return std::async(std::launch::async, [this, symbols]() {
        return this->fetch_tickers(symbols);
    });
}

std::future<nlohmann::json> HitBTC2::fetch_order_book_async(const std::string& symbol, int limit) {
    return std::async(std::launch::async, [this, symbol, limit]() {
        return this->fetch_order_book(symbol, limit);
    });
}

std::future<nlohmann::json> HitBTC2::fetch_trades_async(const std::string& symbol, int since, int limit) {
    return std::async(std::launch::async, [this, symbol, since, limit]() {
        return this->fetch_trades(symbol, since, limit);
    });
}

std::future<nlohmann::json> HitBTC2::fetch_ohlcv_async(const std::string& symbol, const std::string& timeframe,
                                                     int since, int limit) {
    return std::async(std::launch::async, [this, symbol, timeframe, since, limit]() {
        return this->fetch_ohlcv(symbol, timeframe, since, limit);
    });
}

std::future<nlohmann::json> HitBTC2::create_order_async(const std::string& symbol, const std::string& type,
                                                      const std::string& side, double amount, double price) {
    return std::async(std::launch::async, [this, symbol, type, side, amount, price]() {
        return this->create_order(symbol, type, side, amount, price);
    });
}

std::future<nlohmann::json> HitBTC2::cancel_order_async(const std::string& id, const std::string& symbol) {
    return std::async(std::launch::async, [this, id, symbol]() {
        return this->cancel_order(id, symbol);
    });
}

std::future<nlohmann::json> HitBTC2::fetch_order_async(const std::string& id, const std::string& symbol) {
    return std::async(std::launch::async, [this, id, symbol]() {
        return this->fetch_order(id, symbol);
    });
}

std::future<nlohmann::json> HitBTC2::fetch_orders_async(const std::string& symbol, int since, int limit) {
    return std::async(std::launch::async, [this, symbol, since, limit]() {
        return this->fetch_orders(symbol, since, limit);
    });
}

std::future<nlohmann::json> HitBTC2::fetch_open_orders_async(const std::string& symbol, int since, int limit) {
    return std::async(std::launch::async, [this, symbol, since, limit]() {
        return this->fetch_open_orders(symbol, since, limit);
    });
}

std::future<nlohmann::json> HitBTC2::fetch_closed_orders_async(const std::string& symbol, int since, int limit) {
    return std::async(std::launch::async, [this, symbol, since, limit]() {
        return this->fetch_closed_orders(symbol, since, limit);
    });
}

std::future<nlohmann::json> HitBTC2::fetch_my_trades_async(const std::string& symbol, int since, int limit) {
    return std::async(std::launch::async, [this, symbol, since, limit]() {
        return this->fetch_my_trades(symbol, since, limit);
    });
}

std::future<nlohmann::json> HitBTC2::fetch_balance_async() {
    return std::async(std::launch::async, [this]() {
        return this->fetch_balance();
    });
}

std::future<nlohmann::json> HitBTC2::fetch_deposit_address_async(const std::string& code) {
    return std::async(std::launch::async, [this, code]() {
        return this->fetch_deposit_address(code);
    });
}

std::future<nlohmann::json> HitBTC2::fetch_deposits_async(const std::string& code, int since, int limit) {
    return std::async(std::launch::async, [this, code, since, limit]() {
        return this->fetch_deposits(code, since, limit);
    });
}

std::future<nlohmann::json> HitBTC2::fetch_withdrawals_async(const std::string& code, int since, int limit) {
    return std::async(std::launch::async, [this, code, since, limit]() {
        return this->fetch_withdrawals(code, since, limit);
    });
}

std::future<nlohmann::json> HitBTC2::withdraw_async(const std::string& code, double amount, const std::string& address,
                                                 const std::string& tag, const nlohmann::json& params) {
    return std::async(std::launch::async, [this, code, amount, address, tag, params]() {
        return this->withdraw(code, amount, address, tag, params);
    });
}

} // namespace ccxt
