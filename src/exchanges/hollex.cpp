#include "ccxt/exchanges/hollex.h"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>
#include <future>

namespace ccxt {

Hollex::Hollex() {
    this->id = "hollex";
    this->name = "Hollex";
    this->countries = {"CN"};  // China
    this->version = "v1";
    this->rateLimit = 100;  // 10 requests per second
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
        {"fetchTrades", true},
        {"fetchWithdrawals", true},
        {"withdraw", true}
    };

    this->timeframes = {
        {"1m", "1min"},
        {"5m", "5min"},
        {"15m", "15min"},
        {"30m", "30min"},
        {"1h", "1hour"},
        {"4h", "4hour"},
        {"1d", "1day"},
        {"1w", "1week"},
        {"1M", "1month"}
    };

    this->urls = {
        {"logo", ""},
        {"api", {
            {"public", "https://api.hollex.ai/api/v1/public"},
            {"private", "https://api.hollex.ai/api/v1/private"}
        }},
        {"www", "https://www.hollex.ai"},
        {"doc", {"https://doc.hollex.ai"}},
        {"fees", "https://www.hollex.ai/fees"}
    };

    this->api = {
        {"public", {
            {"GET", {
                "markets",
                "ticker/{symbol}",
                "depth/{symbol}",
                "trades/{symbol}",
                "kline/{symbol}"
            }}
        }},
        {"private", {
            {"GET", {
                "account/balances",
                "orders/{symbol}",
                "order/{orderId}",
                "trades/{symbol}",
                "deposit/address/{currency}",
                "deposits",
                "withdrawals"
            }},
            {"POST", {
                "order",
                "withdraw"
            }},
            {"DELETE", {
                "order/{orderId}",
                "orders/{symbol}"
            }}
        }}
    };

    this->fees = {
        {"trading", {
            {"maker", 0.002},  // 0.2%
            {"taker", 0.002}   // 0.2%
        }},
        {"funding", {
            {"withdraw", {}},
            {"deposit", {}}
        }}
    };
}

nlohmann::json Hollex::fetch_markets() {
    auto response = this->publicGetMarkets();
    return this->parse_markets(response);
}

nlohmann::json Hollex::fetch_ticker(const std::string& symbol) {
    this->load_markets();
    auto market = this->market(symbol);
    auto request = {{"symbol", market["id"]}};
    auto response = this->publicGetTickerSymbol(request);
    return this->parse_ticker(response, market);
}

nlohmann::json Hollex::fetch_order_book(const std::string& symbol, int limit) {
    this->load_markets();
    auto request = {
        {"symbol", this->market_id(symbol)},
        {"limit", limit ? limit : 100}
    };
    auto response = this->publicGetDepthSymbol(request);
    return this->parse_order_book(response, symbol);
}

nlohmann::json Hollex::create_order(const std::string& symbol, const std::string& type,
                                  const std::string& side, double amount, double price) {
    this->load_markets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]},
        {"side", side},
        {"type", type},
        {"amount", this->amount_to_precision(symbol, amount)}
    };
    if (type == "limit") {
        request["price"] = this->price_to_precision(symbol, price);
    }
    auto response = this->privatePostOrder(request);
    return this->parse_order(response);
}

nlohmann::json Hollex::cancel_order(const std::string& id, const std::string& symbol) {
    this->load_markets();
    auto request = {{"orderId", id}};
    if (!symbol.empty()) {
        request["symbol"] = this->market_id(symbol);
    }
    return this->privateDeleteOrderOrderId(request);
}

nlohmann::json Hollex::fetch_balance() {
    this->load_markets();
    auto response = this->privateGetAccountBalances();
    return this->parse_balance(response);
}

std::string Hollex::sign(const std::string& path, const std::string& api,
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
        auto nonce = std::to_string(this->nonce());
        auto auth = this->config_.apiKey + ":" + nonce;
        
        std::string payload;
        if (method == "GET") {
            if (!query.empty()) {
                url += "?" + this->urlencode(query);
            }
            payload = url;
        } else {
            if (!query.empty()) {
                payload = this->json(query);
            }
        }

        auto signature = this->hmac(payload, this->config_.secret, "sha256", "hex");
        
        auto headers_map = headers;
        headers_map["API-KEY"] = this->config_.apiKey;
        headers_map["API-TIMESTAMP"] = nonce;
        headers_map["API-SIGN"] = signature;
        
        if (method != "GET") {
            headers_map["Content-Type"] = "application/json";
        }
    }

    return url;
}

// Async implementations
std::future<nlohmann::json> Hollex::fetch_markets_async() {
    return std::async(std::launch::async, [this]() {
        return this->fetch_markets();
    });
}

std::future<nlohmann::json> Hollex::fetch_ticker_async(const std::string& symbol) {
    return std::async(std::launch::async, [this, symbol]() {
        return this->fetch_ticker(symbol);
    });
}

std::future<nlohmann::json> Hollex::fetch_order_book_async(const std::string& symbol, int limit) {
    return std::async(std::launch::async, [this, symbol, limit]() {
        return this->fetch_order_book(symbol, limit);
    });
}

std::future<nlohmann::json> Hollex::fetch_trades_async(const std::string& symbol, int since, int limit) {
    return std::async(std::launch::async, [this, symbol, since, limit]() {
        return this->fetch_trades(symbol, since, limit);
    });
}

std::future<nlohmann::json> Hollex::fetch_ohlcv_async(const std::string& symbol, const std::string& timeframe,
                                                    int since, int limit) {
    return std::async(std::launch::async, [this, symbol, timeframe, since, limit]() {
        return this->fetch_ohlcv(symbol, timeframe, since, limit);
    });
}

std::future<nlohmann::json> Hollex::create_order_async(const std::string& symbol, const std::string& type,
                                                     const std::string& side, double amount, double price) {
    return std::async(std::launch::async, [this, symbol, type, side, amount, price]() {
        return this->create_order(symbol, type, side, amount, price);
    });
}

std::future<nlohmann::json> Hollex::cancel_order_async(const std::string& id, const std::string& symbol) {
    return std::async(std::launch::async, [this, id, symbol]() {
        return this->cancel_order(id, symbol);
    });
}

std::future<nlohmann::json> Hollex::fetch_order_async(const std::string& id, const std::string& symbol) {
    return std::async(std::launch::async, [this, id, symbol]() {
        return this->fetch_order(id, symbol);
    });
}

std::future<nlohmann::json> Hollex::fetch_orders_async(const std::string& symbol, int since, int limit) {
    return std::async(std::launch::async, [this, symbol, since, limit]() {
        return this->fetch_orders(symbol, since, limit);
    });
}

std::future<nlohmann::json> Hollex::fetch_open_orders_async(const std::string& symbol, int since, int limit) {
    return std::async(std::launch::async, [this, symbol, since, limit]() {
        return this->fetch_open_orders(symbol, since, limit);
    });
}

std::future<nlohmann::json> Hollex::fetch_closed_orders_async(const std::string& symbol, int since, int limit) {
    return std::async(std::launch::async, [this, symbol, since, limit]() {
        return this->fetch_closed_orders(symbol, since, limit);
    });
}

std::future<nlohmann::json> Hollex::fetch_my_trades_async(const std::string& symbol, int since, int limit) {
    return std::async(std::launch::async, [this, symbol, since, limit]() {
        return this->fetch_my_trades(symbol, since, limit);
    });
}

std::future<nlohmann::json> Hollex::fetch_balance_async() {
    return std::async(std::launch::async, [this]() {
        return this->fetch_balance();
    });
}

std::future<nlohmann::json> Hollex::fetch_deposit_address_async(const std::string& code) {
    return std::async(std::launch::async, [this, code]() {
        return this->fetch_deposit_address(code);
    });
}

std::future<nlohmann::json> Hollex::fetch_deposits_async(const std::string& code, int since, int limit) {
    return std::async(std::launch::async, [this, code, since, limit]() {
        return this->fetch_deposits(code, since, limit);
    });
}

std::future<nlohmann::json> Hollex::fetch_withdrawals_async(const std::string& code, int since, int limit) {
    return std::async(std::launch::async, [this, code, since, limit]() {
        return this->fetch_withdrawals(code, since, limit);
    });
}

std::future<nlohmann::json> Hollex::withdraw_async(const std::string& code, double amount, const std::string& address,
                                                const std::string& tag, const nlohmann::json& params) {
    return std::async(std::launch::async, [this, code, amount, address, tag, params]() {
        return this->withdraw(code, amount, address, tag, params);
    });
}

} // namespace ccxt
