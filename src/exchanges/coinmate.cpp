#include "ccxt/exchanges/coinmate.h"
#include <ctime>
#include <sstream>
#include <iomanip>

namespace ccxt {

Coinmate::Coinmate() {
    this->id = "coinmate";
    this->name = "CoinMate";
    this->version = "v1";
    this->has = {
        {"fetchMarkets", true},
        {"fetchTicker", true},
        {"fetchOrderBook", true},
        {"fetchTrades", true},
        {"fetchBalance", true},
        {"createOrder", true},
        {"cancelOrder", true},
        {"fetchOrder", true},
        {"fetchOrders", true},
        {"fetchOpenOrders", true},
        {"fetchMyTrades", true}
    };

    this->urls = {
        {"logo", "https://user-images.githubusercontent.com/51840849/87460806-1c9f3f00-c616-11ea-8c46-a77018a8f3f4.jpg"},
        {"api", {
            {"public", "https://coinmate.io/api"},
            {"private", "https://coinmate.io/api"}
        }},
        {"www", "https://coinmate.io"},
        {"doc", {
            "https://coinmate.docs.apiary.io",
            "https://coinmate.io/developers"
        }},
        {"fees", "https://coinmate.io/fees"}
    };

    this->api = {
        {"public", {
            {"GET", {
                "ticker",
                "trades",
                "orderBook",
                "pairs"
            }}
        }},
        {"private", {
            {"POST", {
                "balances",
                "transactionHistory",
                "openOrders",
                "order",
                "cancelOrder",
                "orderHistory"
            }}
        }}
    };

    this->markets = {
        {"BTC/EUR", {{"id", "BTC_EUR"}, {"symbol", "BTC/EUR"}, {"base", "BTC"}, {"quote", "EUR"}}},
        {"BTC/CZK", {{"id", "BTC_CZK"}, {"symbol", "BTC/CZK"}, {"base", "BTC"}, {"quote", "CZK"}}},
        {"LTC/BTC", {{"id", "LTC_BTC"}, {"symbol", "LTC/BTC"}, {"base", "LTC"}, {"quote", "BTC"}}},
        {"ETH/EUR", {{"id", "ETH_EUR"}, {"symbol", "ETH/EUR"}, {"base", "ETH"}, {"quote", "EUR"}}}
    };
}

nlohmann::json Coinmate::fetch_markets() {
    auto response = this->fetch("pairs", "public");
    return this->parse_markets(response["data"]);
}

nlohmann::json Coinmate::fetch_ticker(const std::string& symbol) {
    auto market = this->market(symbol);
    auto request = {
        {"currencyPair", market["id"].get<std::string>()}
    };
    auto response = this->fetch("ticker", "public", "GET", request);
    return this->parse_ticker(response["data"], market);
}

nlohmann::json Coinmate::fetch_order_book(const std::string& symbol, int limit) {
    auto market = this->market(symbol);
    auto request = {
        {"currencyPair", market["id"].get<std::string>()},
        {"groupByPriceLimit", "False"}
    };
    auto response = this->fetch("orderBook", "public", "GET", request);
    return this->parse_order_book(response["data"], market);
}

std::string Coinmate::sign(const std::string& path, const std::string& api,
                          const std::string& method, const nlohmann::json& params,
                          const std::map<std::string, std::string>& headers) {
    auto url = this->urls["api"][api].get<std::string>() + "/" + path;

    if (api == "public") {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    } else {
        this->check_required_credentials();
        auto nonce = std::to_string(this->nonce());
        auto auth = nonce + this->clientId + this->apiKey;
        auto signature = this->hmac(auth, this->secret, "sha256");
        
        auto request = params;
        request["clientId"] = this->clientId;
        request["nonce"] = nonce;
        request["signature"] = signature;
        
        auto body = this->urlencode(request);
        auto new_headers = headers;
        new_headers["Content-Type"] = "application/x-www-form-urlencoded";
        
        if (method == "POST") {
            new_headers["Content-Length"] = std::to_string(body.length());
        }
    }

    return url;
}

std::string Coinmate::get_currency_id(const std::string& code) {
    if (this->currencies.find(code) != this->currencies.end()) {
        return this->currencies[code]["id"].get<std::string>();
    }
    return code;
}

} // namespace ccxt
