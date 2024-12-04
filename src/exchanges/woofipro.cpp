#include "ccxt/exchanges/woofipro.h"
#include <ctime>
#include <sstream>
#include <iomanip>

namespace ccxt {

WooFiPro::WooFiPro() {
    this->id = "woofipro";
    this->name = "WOO X Pro";
    this->countries = {"KY"}; // Cayman Islands
    this->version = "v1";
    this->rateLimit = 50;
    this->has = {
        {"cancelAllOrders", true},
        {"cancelOrder", true},
        {"createOrder", true},
        {"fetchBalance", true},
        {"fetchClosedOrders", true},
        {"fetchFundingRate", true},
        {"fetchFundingRates", true},
        {"fetchMarkets", true},
        {"fetchMyTrades", true},
        {"fetchOpenOrders", true},
        {"fetchOrder", true},
        {"fetchOrderBook", true},
        {"fetchOrders", true},
        {"fetchPositions", true},
        {"fetchTicker", true},
        {"fetchTickers", true},
        {"fetchTrades", true}
    };

    this->urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/150730761-1a00e5e0-d28c-480f-9e65-089ce3e6ef3b.jpg"},
        {"api", {
            {"public", "https://api.woo.org"},
            {"private", "https://api.woo.org"}
        }},
        {"www", "https://pro.woo.org"},
        {"doc", {
            "https://docs.woo.org/"
        }},
        {"fees", "https://support.woo.org/hc/en-001/articles/4404611795353"}
    };

    this->api = {
        {"public", {
            {"GET", {
                "v1/public/info/{symbol}",
                "v1/public/info/price",
                "v1/public/info/kline",
                "v1/public/info/depth",
                "v1/public/info/trades",
                "v1/public/funding_rates",
                "v1/public/funding_rate/{symbol}"
            }}
        }},
        {"private", {
            {"GET", {
                "v1/positions",
                "v1/orders/{orderId}",
                "v1/orders/pending",
                "v1/orders/completed",
                "v1/trades"
            }},
            {"POST", {
                "v1/orders",
                "v1/orders/cancel"
            }},
            {"DELETE", {
                "v1/orders/pending"
            }}
        }}
    };
}

nlohmann::json WooFiPro::fetch_markets() {
    auto response = this->fetch("v1/public/info/symbols", "public");
    return this->parse_markets(response["data"]);
}

nlohmann::json WooFiPro::fetch_ticker(const std::string& symbol) {
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"].get<std::string>()}
    };
    auto response = this->fetch("v1/public/info/ticker/" + market["id"].get<std::string>(), "public", "GET", request);
    return this->parse_ticker(response["data"], market);
}

nlohmann::json WooFiPro::fetch_order_book(const std::string& symbol, int limit) {
    auto market = this->market(symbol);
    nlohmann::json request = {
        {"symbol", market["id"].get<std::string>()}
    };
    if (limit) {
        request["max_level"] = limit;
    }
    auto response = this->fetch("v1/public/info/depth", "public", "GET", request);
    return this->parse_order_book(response["data"], symbol);
}

nlohmann::json WooFiPro::fetch_funding_rate(const std::string& symbol) {
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"].get<std::string>()}
    };
    auto response = this->fetch("v1/public/funding_rate/" + market["id"].get<std::string>(), "public", "GET", request);
    return this->parse_funding_rate(response["data"], market);
}

nlohmann::json WooFiPro::create_order(const std::string& symbol, const std::string& type,
                                     const std::string& side, double amount, double price) {
    this->check_required_credentials();
    auto market = this->market(symbol);
    
    nlohmann::json request = {
        {"symbol", market["id"].get<std::string>()},
        {"side", side.upper()},
        {"order_type", type.upper()},
        {"quantity", this->amount_to_precision(symbol, amount)}
    };

    if (type != "MARKET") {
        request["price"] = this->price_to_precision(symbol, price);
    }

    auto response = this->fetch("v1/orders", "private", "POST", request);
    return this->parse_order(response["data"]);
}

nlohmann::json WooFiPro::cancel_order(const std::string& id, const std::string& symbol) {
    this->check_required_credentials();
    auto request = {
        {"order_id", id}
    };
    if (!symbol.empty()) {
        auto market = this->market(symbol);
        request["symbol"] = market["id"].get<std::string>();
    }
    return this->fetch("v1/orders/cancel", "private", "POST", request);
}

nlohmann::json WooFiPro::fetch_positions(const std::vector<std::string>& symbols) {
    this->check_required_credentials();
    nlohmann::json request;
    if (!symbols.empty()) {
        request["symbols"] = symbols;
    }
    auto response = this->fetch("v1/positions", "private", "GET", request);
    return this->parse_positions(response["data"]);
}

std::string WooFiPro::sign(const std::string& path, const std::string& api,
                          const std::string& method, const nlohmann::json& params,
                          const std::map<std::string, std::string>& headers) {
    auto url = this->urls["api"][api].get<std::string>() + "/" + this->implode_params(path, params);
    auto query = this->omit(params, this->extract_params(path));
    
    if (api == "public") {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->check_required_credentials();
        auto timestamp = std::to_string(this->milliseconds());
        auto content = timestamp + method + "/api/" + path;

        if (!query.empty()) {
            if (method == "GET") {
                url += "?" + this->urlencode(query);
                content += "?" + this->urlencode(query);
            } else {
                content += this->json(query);
            }
        }

        auto signature = this->hmac(content, this->secret, "sha256");
        auto new_headers = headers;
        new_headers["x-api-key"] = this->apiKey;
        new_headers["x-api-signature"] = signature;
        new_headers["x-api-timestamp"] = timestamp;

        if (method != "GET") {
            new_headers["Content-Type"] = "application/json";
        }
    }

    return url;
}

} // namespace ccxt
