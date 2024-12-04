#include "ccxt/exchanges/bit2c.h"
#include <ctime>
#include <sstream>
#include <iomanip>

namespace ccxt {

Bit2c::Bit2c() {
    this->id = "bit2c";
    this->name = "Bit2C";
    this->countries = {"IL"}; // Israel
    this->rateLimit = 3000;
    this->has = {
        {"cancelOrder", true},
        {"CORS", false},
        {"createOrder", true},
        {"fetchBalance", true},
        {"fetchMyTrades", true},
        {"fetchOpenOrders", true},
        {"fetchOrderBook", true},
        {"fetchTicker", true},
        {"fetchTrades", true}
    };
    
    this->urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/27766119-3593220e-5ece-11e7-8b3a-5a041f6bcc3f.jpg"},
        {"api", {
            {"public", "https://bit2c.co.il"},
            {"private", "https://bit2c.co.il"}
        }},
        {"www", "https://www.bit2c.co.il"},
        {"doc", {
            "https://www.bit2c.co.il/home/api",
            "https://github.com/OferE/bit2c"
        }},
        {"fees", "https://bit2c.co.il/home/fee"}
    };

    this->api = {
        {"public", {
            {"GET", {
                "Exchanges/{pair}/Ticker",
                "Exchanges/{pair}/orderbook",
                "Exchanges/{pair}/trades",
                "Exchanges/pairs"
            }}
        }},
        {"private", {
            {"POST", {
                "Account/Balance",
                "Account/Balance/v2",
                "Merchant/CreateCheckout",
                "Order/AccountHistory",
                "Order/AddCoinFundsRequest",
                "Order/AddFund",
                "Order/AddOrder",
                "Order/AddOrderMarketPriceBuy",
                "Order/AddOrderMarketPriceSell",
                "Order/CancelOrder",
                "Order/MyOrders",
                "Payment/GetMyId",
                "Payment/Send"
            }}
        }}
    };

    this->markets = {
        {"BTC/NIS", {{"id", "BtcNis"}, {"symbol", "BTC/NIS"}, {"base", "BTC"}, {"quote", "NIS"}}},
        {"ETH/NIS", {{"id", "EthNis"}, {"symbol", "ETH/NIS"}, {"base", "ETH"}, {"quote", "NIS"}}},
        {"BCH/NIS", {{"id", "BchNis"}, {"symbol", "BCH/NIS"}, {"base", "BCH"}, {"quote", "NIS"}}},
        {"LTC/NIS", {{"id", "LtcNis"}, {"symbol", "LTC/NIS"}, {"base", "LTC"}, {"quote", "NIS"}}},
        {"ETC/NIS", {{"id", "EtcNis"}, {"symbol", "ETC/NIS"}, {"base", "ETC"}, {"quote", "NIS"}}}
    };
}

nlohmann::json Bit2c::fetch_balance() {
    this->check_required_credentials();
    auto response = this->fetch("Account/Balance", "private", "POST");
    return this->parse_balance(response);
}

nlohmann::json Bit2c::fetch_order_book(const std::string& symbol, int limit) {
    auto market = this->market(symbol);
    auto request = this->extend({
        {"pair", market["id"].get<std::string>()}
    });
    auto response = this->fetch("Exchanges/" + market["id"].get<std::string>() + "/orderbook", "public", "GET", request);
    return this->parse_order_book(response, symbol);
}

nlohmann::json Bit2c::fetch_ticker(const std::string& symbol) {
    auto market = this->market(symbol);
    auto request = this->extend({
        {"pair", market["id"].get<std::string>()}
    });
    auto response = this->fetch("Exchanges/" + market["id"].get<std::string>() + "/Ticker", "public", "GET", request);
    return this->parse_ticker(response, market);
}

nlohmann::json Bit2c::create_order(const std::string& symbol, const std::string& type,
                                  const std::string& side, double amount, double price) {
    this->check_required_credentials();
    auto method = (type == "market") ? 
        (side == "buy" ? "Order/AddOrderMarketPriceBuy" : "Order/AddOrderMarketPriceSell") :
        "Order/AddOrder";

    auto market = this->market(symbol);
    auto request = {
        {"Amount", amount},
        {"Pair", market["id"].get<std::string>()}
    };

    if (type != "market") {
        request["Price"] = price;
        request["Total"] = amount * price;
        request["IsBid"] = (side == "buy");
    }

    auto response = this->fetch(method, "private", "POST", request);
    return this->parse_order(response);
}

nlohmann::json Bit2c::cancel_order(const std::string& id, const std::string& symbol) {
    this->check_required_credentials();
    auto request = {
        {"id", std::stoi(id)}
    };
    return this->fetch("Order/CancelOrder", "private", "POST", request);
}

std::string Bit2c::sign(const std::string& path, const std::string& api,
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
        auto nonce = std->to_string(this->nonce());
        auto query = this->extend({
            {"nonce", nonce}
        }, query);
        auto body = this->urlencode(query);
        auto signature = this->hmac(body, this->secret, "sha512", true);
        auto headers = {
            {"Content-Type", "application/x-www-form-urlencoded"},
            {"key", this->apiKey},
            {"sign", signature}
        };
    }

    return url;
}

std::string Bit2c::get_currency_pair(const std::string& symbol) {
    if (this->markets.find(symbol) != this->markets.end()) {
        return this->markets[symbol]["id"].get<std::string>();
    }
    throw std::runtime_error("Symbol " + symbol + " is not supported by Bit2c");
}

} // namespace ccxt
