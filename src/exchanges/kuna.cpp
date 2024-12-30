#include "ccxt/exchanges/kuna.h"
#include <ctime>
#include <sstream>
#include <iomanip>

namespace ccxt {

Kuna::Kuna() {
    this->id = "kuna";
    this->name = "Kuna";
    this->countries = {"UA"}; // Ukraine
    this->rateLimit = 1000;
    this->version = "v3";
    this->has = {
        {"cancelOrder", true},
        {"CORS", false},
        {"createOrder", true},
        {"fetchBalance", true},
        {"fetchClosedOrders", true},
        {"fetchDepositAddress", true},
        {"fetchDeposits", true},
        {"fetchMarkets", true},
        {"fetchMyTrades", true},
        {"fetchOHLCV", true},
        {"fetchOpenOrders", true},
        {"fetchOrder", true},
        {"fetchOrderBook", true},
        {"fetchTicker", true},
        {"fetchTickers", true},
        {"fetchTime", true},
        {"fetchTrades", true},
        {"fetchWithdrawals", true}
    };

    this->timeframes = {
        {"1m", "1"},
        {"3m", "3"},
        {"5m", "5"},
        {"15m", "15"},
        {"30m", "30"},
        {"1h", "60"},
        {"2h", "120"},
        {"4h", "240"},
        {"6h", "360"},
        {"12h", "720"},
        {"1d", "1440"},
        {"3d", "4320"},
        {"1w", "10080"}
    };

    this->urls = {
        {"logo", "https://user-images.githubusercontent.com/51840849/87153927-f0578b80-c2c0-11ea-84b6-74612568e9e1.jpg"},
        {"api", {
            {"public", "https://api.kuna.io"},
            {"private", "https://api.kuna.io"}
        }},
        {"www", "https://kuna.io"},
        {"doc", {
            "https://kuna.io/documents/api",
            "https://github.com/kunadex/api-docs"
        }},
        {"fees", "https://kuna.io/documents/api"}
    };

    this->api = {
        {"public", {
            {"GET", {
                "timestamp",
                "currencies",
                "markets",
                "tickers",
                "order_book",
                "trades",
                "candles"
            }}
        }},
        {"private", {
            {"GET", {
                "accounts/balance",
                "orders",
                "orders/history",
                "trades/history",
                "deposit_address",
                "deposit_history",
                "withdrawal_history"
            }},
            {"POST", {
                "orders/new",
                "orders/cancel"
            }}
        }}
    };
}

nlohmann::json Kuna::fetch_markets() {
    auto response = this->fetch("markets", "public");
    return this->parse_markets(response);
}

nlohmann::json Kuna::fetch_ticker(const std::string& symbol) {
    auto market = this->market(symbol);
    auto request = {
        {"market", market["id"].get<std::string>()}
    };
    auto response = this->fetch("tickers", "public", "GET", request);
    return this->parse_ticker(response, market);
}

nlohmann::json Kuna::fetch_order_book(const std::string& symbol, int limit) {
    auto market = this->market(symbol);
    nlohmann::json request = {
        {"market", market["id"].get<std::string>()}
    };
    if (limit) {
        request["limit"] = limit;
    }
    auto response = this->fetch("order_book", "public", "GET", request);
    return this->parse_order_book(response, symbol);
}

nlohmann::json Kuna::create_order(const std::string& symbol, const std::string& type,
                                 const std::string& side, double amount, double price) {
    this->check_required_credentials();
    auto market = this->market(symbol);
    
    nlohmann::json request = {
        {"market", market["id"].get<std::string>()},
        {"side", side},
        {"volume", this->amount_to_precision(symbol, amount)},
        {"ord_type", type}
    };

    if (type == "limit") {
        request["price"] = this->price_to_precision(symbol, price);
    }

    auto response = this->fetch("orders/new", "private", "POST", request);
    return this->parse_order(response);
}

nlohmann::json Kuna::cancel_order(const std::string& id, const std::string& symbol) {
    this->check_required_credentials();
    auto request = {
        {"order_id", std::stoi(id)}
    };
    return this->fetch("orders/cancel", "private", "POST", request);
}

nlohmann::json Kuna::fetch_balance() {
    this->check_required_credentials();
    auto response = this->fetch("accounts/balance", "private");
    return this->parse_balance(response);
}

nlohmann::json Kuna::fetch_deposit_address(const std::string& code, const nlohmann::json& params) {
    this->check_required_credentials();
    auto currency = this->currency(code);
    auto request = this->extend({
        {"currency", currency["id"].get<std::string>()}
    }, params);
    auto response = this->fetch("deposit_address", "private", "GET", request);
    return this->parse_deposit_address(response);
}

std::string Kuna::sign(const std::string& path, const std::string& api,
                      const std::string& method, const nlohmann::json& params,
                      const std::map<std::string, std::string>& headers) {
    auto url = this->urls["api"][api].get<std::string>() + "/" + this->version + "/" + path;
    auto query = this->omit(params, this->extract_params(path));

    if (api == "public") {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->check_required_credentials();
        auto nonce = std::to_string(this->nonce());
        auto body = "";
        
        if (method == "GET") {
            if (!query.empty()) {
                url += "?" + this->urlencode(query);
            }
        } else {
            if (!query.empty()) {
                body = this->json(query);
            }
        }

        auto auth = nonce + method + "/v3/" + path + body;
        auto signature = this->hmac(auth, this->config_.secret, "sha384", "hex");
        
        auto new_headers = headers;
        new_headers["kun-nonce"] = nonce;
        new_headers["kun-apikey"] = this->config_.apiKey;
        new_headers["kun-signature"] = signature;

        if (method != "GET") {
            new_headers["Content-Type"] = "application/json";
        }
    }

    return url;
}

std::string Kuna::get_market_id(const std::string& symbol) {
    if (this->markets.find(symbol) != this->markets.end()) {
        return this->markets[symbol]["id"].get<std::string>();
    }
    throw std::runtime_error("Market " + symbol + " not found");
}

} // namespace ccxt
