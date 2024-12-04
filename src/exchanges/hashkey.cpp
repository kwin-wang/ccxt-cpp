#include "ccxt/exchanges/hashkey.h"
#include <ctime>
#include <sstream>
#include <iomanip>

namespace ccxt {

Hashkey::Hashkey() {
    this->id = "hashkey";
    this->name = "Hashkey";
    this->countries = {"HK"}; // Hong Kong
    this->version = "v1";
    this->rateLimit = 100;
    this->has = {
        {"cancelAllOrders", true},
        {"cancelOrder", true},
        {"createOrder", true},
        {"fetchBalance", true},
        {"fetchClosedOrders", true},
        {"fetchMarkets", true},
        {"fetchMyTrades", true},
        {"fetchOHLCV", true},
        {"fetchOpenOrders", true},
        {"fetchOrder", true},
        {"fetchOrderBook", true},
        {"fetchOrders", true},
        {"fetchTicker", true},
        {"fetchTickers", true},
        {"fetchTrades", true}
    };

    this->timeframes = {
        {"1m", "1min"},
        {"5m", "5min"},
        {"15m", "15min"},
        {"30m", "30min"},
        {"1h", "1hour"},
        {"4h", "4hour"},
        {"12h", "12hour"},
        {"1d", "1day"},
        {"1w", "1week"},
        {"1M", "1month"}
    };

    this->urls = {
        {"logo", "https://hashkey.com/assets/images/logo.svg"},
        {"api", {
            {"public", "https://api.hashkey.com"},
            {"private", "https://api.hashkey.com"}
        }},
        {"www", "https://hashkey.com"},
        {"doc", {
            "https://hashkey.com/api-docs"
        }},
        {"fees", "https://hashkey.com/fees"}
    };

    this->api = {
        {"public", {
            {"GET", {
                "api/v1/market/symbols",
                "api/v1/market/ticker",
                "api/v1/market/ticker/{symbol}",
                "api/v1/market/depth",
                "api/v1/market/trades",
                "api/v1/market/klines"
            }}
        }},
        {"private", {
            {"GET", {
                "api/v1/account/balance",
                "api/v1/order/orders",
                "api/v1/order/openOrders",
                "api/v1/order/order",
                "api/v1/order/trades",
                "api/v1/order/fills"
            }},
            {"POST", {
                "api/v1/order/orders/place",
                "api/v1/order/orders/cancel",
                "api/v1/order/orders/cancelAll"
            }}
        }}
    };
}

nlohmann::json Hashkey::fetch_markets() {
    auto response = this->fetch("api/v1/market/symbols", "public");
    return this->parse_markets(response["data"]);
}

nlohmann::json Hashkey::fetch_ticker(const std::string& symbol) {
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"].get<std::string>()}
    };
    auto response = this->fetch("api/v1/market/ticker/" + market["id"].get<std::string>(), "public", "GET", request);
    return this->parse_ticker(response["data"], market);
}

nlohmann::json Hashkey::fetch_order_book(const std::string& symbol, int limit) {
    auto market = this->market(symbol);
    nlohmann::json request = {
        {"symbol", market["id"].get<std::string>()}
    };
    if (limit) {
        request["limit"] = limit;
    }
    auto response = this->fetch("api/v1/market/depth", "public", "GET", request);
    return this->parse_order_book(response["data"], symbol);
}

nlohmann::json Hashkey::create_order(const std::string& symbol, const std::string& type,
                                   const std::string& side, double amount, double price) {
    this->check_required_credentials();
    auto market = this->market(symbol);
    
    nlohmann::json request = {
        {"symbol", market["id"].get<std::string>()},
        {"side", side.substr(0, 1).upper() + side.substr(1)},
        {"type", type.upper()},
        {"volume", this->amount_to_precision(symbol, amount)}
    };

    if (type != "MARKET") {
        request["price"] = this->price_to_precision(symbol, price);
    }

    auto response = this->fetch("api/v1/order/orders/place", "private", "POST", request);
    return this->parse_order(response["data"]);
}

nlohmann::json Hashkey::cancel_order(const std::string& id, const std::string& symbol) {
    this->check_required_credentials();
    auto request = {
        {"orderId", id}
    };
    return this->fetch("api/v1/order/orders/cancel", "private", "POST", request);
}

nlohmann::json Hashkey::fetch_balance() {
    this->check_required_credentials();
    auto response = this->fetch("api/v1/account/balance", "private");
    return this->parse_balance(response["data"]);
}

std::string Hashkey::sign(const std::string& path, const std::string& api,
                         const std::string& method, const nlohmann::json& params,
                         const std::map<std::string, std::string>& headers) {
    auto url = this->urls["api"][api].get<std::string>() + "/" + path;
    auto timestamp = std::to_string(this->milliseconds());

    if (api == "public") {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    } else {
        this->check_required_credentials();
        
        auto query = this->extend({
            {"timestamp", timestamp}
        }, params);

        auto payload = method + "\n" +
                      url + "\n" +
                      this->urlencode(this->keysort(query));

        auto signature = this->hmac(payload, this->secret, "sha256");
        
        auto new_headers = headers;
        new_headers["HSK-API-KEY"] = this->apiKey;
        new_headers["HSK-API-SIGNATURE"] = signature;
        new_headers["HSK-API-TIMESTAMP"] = timestamp;

        if (method == "POST") {
            new_headers["Content-Type"] = "application/json";
        }

        if (!params.empty()) {
            if (method == "GET") {
                url += "?" + this->urlencode(params);
            } else {
                new_headers["Content-Type"] = "application/json";
            }
        }
    }

    return url;
}

std::string Hashkey::get_order_status(const std::string& status) {
    static const std::map<std::string, std::string> statuses = {
        {"NEW", "open"},
        {"PARTIALLY_FILLED", "open"},
        {"FILLED", "closed"},
        {"CANCELED", "canceled"},
        {"REJECTED", "rejected"},
        {"EXPIRED", "expired"}
    };
    
    auto it = statuses.find(status);
    return (it != statuses.end()) ? it->second : status;
}

} // namespace ccxt
