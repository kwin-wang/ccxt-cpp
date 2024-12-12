#include "ccxt/exchanges/hashkey.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>
#include <openssl/sha.h>

namespace ccxt {

Hashkey::Hashkey() {
    id = "hashkey";
    name = "HashKey Global";
    countries = {"BM"};  // Bermuda
    rateLimit = 100;
    version = "v1";
    certified = true;
    pro = true;

    // Initialize API endpoints
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/83913589-6891857e-a950-11ea-8b2a-81fd1d8f8d73.jpg"},
        {"api", {
            {"public", "https://api.hashkey.com/api/v1"},
            {"private", "https://api.hashkey.com/api/v1"}
        }},
        {"www", "https://www.hashkey.com"},
        {"doc", {
            "https://hashkey.github.io/api-docs",
            "https://hashkey-api.readme.io/docs"
        }},
        {"fees", "https://www.hashkey.com/fees"}
    };

    api = {
        {"public", {
            {"GET", {
                "market/symbols",
                "market/ticker/{symbol}",
                "market/tickers",
                "market/depth/{symbol}",
                "market/trades/{symbol}",
                "market/klines/{symbol}"
            }}
        }},
        {"private", {
            {"GET", {
                "account/balances",
                "account/orders/{symbol}",
                "account/order/{orderId}",
                "account/trades/{symbol}"
            }},
            {"POST", {
                "account/order",
                "account/cancel/{orderId}",
                "account/cancel_all"
            }}
        }}
    };
}

// Synchronous implementations
nlohmann::json Hashkey::fetch_markets() {
    auto response = this->publicGetMarketSymbols();
    return this->parse_markets(response);
}

nlohmann::json Hashkey::fetch_ticker(const std::string& symbol) {
    this->load_markets();
    auto market = this->market(symbol);
    auto request = {{"symbol", market["id"]}};
    auto response = this->publicGetMarketTickerSymbol(this->extend(request, {}));
    return this->parse_ticker(response, market);
}

// ... (other synchronous implementations)

// Async implementations
std::future<nlohmann::json> Hashkey::fetch_markets_async() {
    return std::async(std::launch::async, [this]() {
        return this->fetch_markets();
    });
}

std::future<nlohmann::json> Hashkey::fetch_ticker_async(const std::string& symbol) {
    return std::async(std::launch::async, [this, symbol]() {
        return this->fetch_ticker(symbol);
    });
}

std::future<nlohmann::json> Hashkey::fetch_tickers_async(const std::vector<std::string>& symbols) {
    return std::async(std::launch::async, [this, symbols]() {
        return this->fetch_tickers(symbols);
    });
}

std::future<nlohmann::json> Hashkey::fetch_order_book_async(const std::string& symbol, int limit) {
    return std::async(std::launch::async, [this, symbol, limit]() {
        return this->fetch_order_book(symbol, limit);
    });
}

std::future<nlohmann::json> Hashkey::fetch_trades_async(const std::string& symbol, int limit) {
    return std::async(std::launch::async, [this, symbol, limit]() {
        return this->fetch_trades(symbol, limit);
    });
}

std::future<nlohmann::json> Hashkey::fetch_balance_async() {
    return std::async(std::launch::async, [this]() {
        return this->fetch_balance();
    });
}

std::future<nlohmann::json> Hashkey::fetch_ohlcv_async(const std::string& symbol, const std::string& timeframe,
                                                      long since, int limit) {
    return std::async(std::launch::async, [this, symbol, timeframe, since, limit]() {
        return this->fetch_ohlcv(symbol, timeframe, since, limit);
    });
}

std::future<nlohmann::json> Hashkey::create_order_async(const std::string& symbol, const std::string& type,
                                                       const std::string& side, double amount, double price) {
    return std::async(std::launch::async, [this, symbol, type, side, amount, price]() {
        return this->create_order(symbol, type, side, amount, price);
    });
}

std::future<nlohmann::json> Hashkey::cancel_order_async(const std::string& id, const std::string& symbol) {
    return std::async(std::launch::async, [this, id, symbol]() {
        return this->cancel_order(id, symbol);
    });
}

std::future<nlohmann::json> Hashkey::cancel_all_orders_async(const std::string& symbol) {
    return std::async(std::launch::async, [this, symbol]() {
        return this->cancel_all_orders(symbol);
    });
}

std::future<nlohmann::json> Hashkey::fetch_order_async(const std::string& id, const std::string& symbol) {
    return std::async(std::launch::async, [this, id, symbol]() {
        return this->fetch_order(id, symbol);
    });
}

std::future<nlohmann::json> Hashkey::fetch_orders_async(const std::string& symbol, int limit) {
    return std::async(std::launch::async, [this, symbol, limit]() {
        return this->fetch_orders(symbol, limit);
    });
}

std::future<nlohmann::json> Hashkey::fetch_open_orders_async(const std::string& symbol, int limit) {
    return std::async(std::launch::async, [this, symbol, limit]() {
        return this->fetch_open_orders(symbol, limit);
    });
}

std::future<nlohmann::json> Hashkey::fetch_closed_orders_async(const std::string& symbol, int limit) {
    return std::async(std::launch::async, [this, symbol, limit]() {
        return this->fetch_closed_orders(symbol, limit);
    });
}

std::future<nlohmann::json> Hashkey::fetch_my_trades_async(const std::string& symbol, int since, int limit) {
    return std::async(std::launch::async, [this, symbol, since, limit]() {
        return this->fetch_my_trades(symbol, since, limit);
    });
}

// Helper methods
std::string Hashkey::sign(const std::string& path, const std::string& api,
                         const std::string& method, const nlohmann::json& params,
                         const std::map<std::string, std::string>& headers) {
    auto url = this->urls["api"][api] + "/" + this->implode_params(path, params);
    auto query = this->omit(params, this->extract_params(path));

    if (api == "public") {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->check_required_credentials();
        auto nonce = std::to_string(this->nonce());
        auto auth = nonce + method + url;

        if (method == "POST") {
            auth += this->json(query);
        } else if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }

        auto signature = this->hmac(auth, this->secret, "sha256", "hex");
        headers["HK-API-KEY"] = this->apiKey;
        headers["HK-API-SIGNATURE"] = signature;
        headers["HK-API-TIMESTAMP"] = nonce;
    }

    return url;
}

} // namespace ccxt
