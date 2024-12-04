#include "ccxt/exchanges/woo.h"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace ccxt {

Woo::Woo() {
    this->id = "woo";
    this->name = "WOO X";
    this->version = "v1";
    this->has = {
        {"fetchMarkets", true},
        {"fetchTicker", true},
        {"fetchOrderBook", true},
        {"fetchTrades", true},
        {"fetchOHLCV", true},
        {"createOrder", true},
        {"cancelOrder", true},
        {"fetchOrder", true},
        {"fetchOrders", true},
        {"fetchOpenOrders", true},
        {"fetchClosedOrders", true}
    };

    this->urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/150730761-1a00e5e0-d28c-480f-9e65-089ce3e6ef3b.jpg"},
        {"api", {
            {"public", "https://api.woo.org"},
            {"private", "https://api.woo.org"}
        }},
        {"www", "https://woo.org"},
        {"doc", {
            "https://docs.woo.org/",
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
            }},
        }},
        {"private", {
            {"GET", {
                "v1/orders/{orderId}",
                "v1/orders/pending",
                "v1/orders/completed",
            }},
            {"POST", {
                "v1/orders",
            }},
            {"DELETE", {
                "v1/orders/{orderId}",
            }},
        }},
    };

    this->timeframes = {
        {"1m", "1m"},
        {"5m", "5m"},
        {"15m", "15m"},
        {"30m", "30m"},
        {"1h", "1h"},
        {"4h", "4h"},
        {"12h", "12h"},
        {"1d", "1d"},
        {"1w", "1w"},
        {"1M", "1M"}
    };
}

nlohmann::json Woo::fetch_markets() {
    auto response = this->fetch("v1/public/info/symbols");
    return this->parse_markets(response["data"]);
}

nlohmann::json Woo::fetch_ticker(const std::string& symbol) {
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"].get<std::string>()}
    };
    auto response = this->fetch("v1/public/info/ticker", "public", "GET", request);
    return this->parse_ticker(response["data"], market);
}

nlohmann::json Woo::fetch_order_book(const std::string& symbol, int limit) {
    auto market = this->market(symbol);
    nlohmann::json request = {
        {"symbol", market["id"].get<std::string>()}
    };
    if (limit) {
        request["max_level"] = limit;
    }
    auto response = this->fetch("v1/public/info/depth", "public", "GET", request);
    return this->parse_order_book(response["data"], market);
}

std::string Woo::sign(const std::string& path, const std::string& api,
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
        auto auth = timestamp + method + path;

        if (method == "POST") {
            auth += this->json(query);
        } else if (!query.empty()) {
            auth += "?" + this->urlencode(query);
        }

        auto signature = this->hmac(auth, this->secret, "sha256");
        auto new_headers = headers;
        new_headers["x-api-key"] = this->apiKey;
        new_headers["x-api-signature"] = signature;
        new_headers["x-api-timestamp"] = timestamp;

        if (method == "POST") {
            new_headers["Content-Type"] = "application/json";
        }
    }

    return url;
}

} // namespace ccxt
