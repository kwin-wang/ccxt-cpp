#include "ccxt/exchanges/luno.h"
#include <ctime>
#include <sstream>
#include <iomanip>

namespace ccxt {

Luno::Luno() {
    this->id = "luno";
    this->name = "Luno";
    this->countries = {"GB", "SG", "ZA"}; // UK, Singapore, South Africa
    this->rateLimit = 1000;
    this->version = "1";
    this->has = {
        {"cancelOrder", true},
        {"CORS", false},
        {"createOrder", true},
        {"fetchBalance", true},
        {"fetchClosedOrders", true},
        {"fetchMarkets", true},
        {"fetchMyTrades", true},
        {"fetchOpenOrders", true},
        {"fetchOrder", true},
        {"fetchOrderBook", true},
        {"fetchOrders", true},
        {"fetchTicker", true},
        {"fetchTickers", true},
        {"fetchTrades", true}
    };

    this->urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/27766607-8c1a69d8-5ede-11e7-930c-540b5eb9be24.jpg"},
        {"api", {
            {"public", "https://api.luno.com/api"},
            {"private", "https://api.luno.com/api"}
        }},
        {"www", "https://www.luno.com"},
        {"doc", {
            "https://www.luno.com/en/api",
            "https://npmjs.org/package/bitx",
            "https://github.com/bausmeier/node-bitx"
        }},
        {"fees", "https://www.luno.com/en/countries"}
    };

    this->api = {
        {"public", {
            {"GET", {
                "exchange/1/orderbook",
                "exchange/1/ticker",
                "exchange/1/tickers",
                "exchange/1/trades"
            }}
        }},
        {"private", {
            {"GET", {
                "exchange/1/accounts/{id}/pending",
                "exchange/1/accounts/{id}/transactions",
                "exchange/1/balance",
                "exchange/1/fee_info",
                "exchange/1/funding_address",
                "exchange/1/listorders",
                "exchange/1/listtrades",
                "exchange/1/orders/{id}",
                "exchange/1/quotes/{id}",
                "exchange/1/withdrawals",
                "exchange/1/withdrawals/{id}"
            }},
            {"POST", {
                "exchange/1/accounts",
                "exchange/1/postorder",
                "exchange/1/marketorder",
                "exchange/1/stoporder",
                "exchange/1/funding_address",
                "exchange/1/withdrawals",
                "exchange/1/send",
                "exchange/1/quotes",
                "exchange/1/buy",
                "exchange/1/sell"
            }},
            {"PUT", {
                "exchange/1/quotes/{id}"
            }},
            {"DELETE", {
                "exchange/1/orders/{id}",
                "exchange/1/quotes/{id}"
            }}
        }}
    };

    this->markets = {
        {"BTC/ZAR", {{"id", "XBTZAR"}, {"symbol", "BTC/ZAR"}, {"base", "BTC"}, {"quote", "ZAR"}}},
        {"ETH/ZAR", {{"id", "ETHZAR"}, {"symbol", "ETH/ZAR"}, {"base", "ETH"}, {"quote", "ZAR"}}},
        {"XRP/ZAR", {{"id", "XRPZAR"}, {"symbol", "XRP/ZAR"}, {"base", "XRP"}, {"quote", "ZAR"}}},
        {"BCH/ZAR", {{"id", "BCHZAR"}, {"symbol", "BCH/ZAR"}, {"base", "BCH"}, {"quote", "ZAR"}}}
    };
}

nlohmann::json Luno::fetch_markets() {
    auto response = this->fetch("exchange/1/markets");
    return this->parse_markets(response["markets"]);
}

nlohmann::json Luno::fetch_balance() {
    this->check_required_credentials();
    auto response = this->fetch("exchange/1/balance", "private");
    return this->parse_balance(response);
}

nlohmann::json Luno::fetch_order_book(const std::string& symbol, int limit) {
    auto market = this->market(symbol);
    auto request = {
        {"pair", market["id"].get<std::string>()}
    };
    auto response = this->fetch("exchange/1/orderbook", "public", "GET", request);
    return this->parse_order_book(response, market["symbol"].get<std::string>());
}

nlohmann::json Luno::fetch_ticker(const std::string& symbol) {
    auto market = this->market(symbol);
    auto request = {
        {"pair", market["id"].get<std::string>()}
    };
    auto response = this->fetch("exchange/1/ticker", "public", "GET", request);
    return this->parse_ticker(response, market);
}

nlohmann::json Luno::create_order(const std::string& symbol, const std::string& type,
                                 const std::string& side, double amount, double price) {
    this->check_required_credentials();
    auto market = this->market(symbol);
    auto method = (type == "market") ? "exchange/1/marketorder" : "exchange/1/postorder";
    
    auto request = {
        {"pair", market["id"].get<std::string>()},
        {"volume", this->amount_to_precision(symbol, amount)}
    };

    if (type != "market") {
        request["price"] = this->price_to_precision(symbol, price);
    }

    if (side == "buy") {
        request["type"] = "BID";
    } else {
        request["type"] = "ASK";
    }

    auto response = this->fetch(method, "private", "POST", request);
    return this->parse_order(response);
}

nlohmann::json Luno::cancel_order(const std::string& id, const std::string& symbol) {
    this->check_required_credentials();
    return this->fetch("exchange/1/orders/" + id, "private", "DELETE");
}

std::string Luno::sign(const std::string& path, const std::string& api,
                      const std::string& method, const nlohmann::json& params,
                      const std::map<std::string, std::string>& headers) {
    auto url = this->urls["api"][api].get<std::string>() + "/" + this->version + "/" + this->implode_params(path, params);
    auto query = this->omit(params, this->extract_params(path));

    if (api == "public") {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->check_required_credentials();
        auto auth = this->apiKey + ":" + this->secret;
        auto auth_base64 = this->encode(auth);
        auto new_headers = headers;
        new_headers["Authorization"] = "Basic " + auth_base64;

        if (method == "GET") {
            if (!query.empty()) {
                url += "?" + this->urlencode(query);
            }
        } else {
            new_headers["Content-Type"] = "application/x-www-form-urlencoded";
        }
    }

    return url;
}

std::string Luno::get_currency_id(const std::string& code) {
    if (code == "BTC") return "XBT";
    return code;
}

} // namespace ccxt
