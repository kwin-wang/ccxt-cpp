#include "ccxt/exchanges/tradeogre.h"
#include <ctime>
#include <sstream>
#include <iomanip>

namespace ccxt {

TradeOgre::TradeOgre() {
    this->id = "tradeogre";
    this->name = "TradeOgre";
    this->countries = {"US"};
    this->rateLimit = 1000;
    this->has = {
        {"cancelOrder", true},
        {"createOrder", true},
        {"fetchBalance", true},
        {"fetchClosedOrders", false},
        {"fetchMarkets", true},
        {"fetchMyTrades", false},
        {"fetchOpenOrders", true},
        {"fetchOrder", false},
        {"fetchOrderBook", true},
        {"fetchTicker", true},
        {"fetchTickers", false},
        {"fetchTrades", false}
    };

    this->urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/94507548-a83d6a80-0218-11eb-9998-28b9cec54165.jpg"},
        {"api", {
            {"public", "https://tradeogre.com/api/v1"},
            {"private", "https://tradeogre.com/api/v1"}
        }},
        {"www", "https://tradeogre.com"},
        {"doc", {
            "https://tradeogre.com/help/api"
        }},
        {"fees", "https://tradeogre.com/help/fees"}
    };

    this->api = {
        {"public", {
            {"GET", {
                "markets",
                "orders/{market}",
                "ticker/{market}",
                "history/{market}"
            }}
        }},
        {"private", {
            {"POST", {
                "account/balances",
                "account/order",
                "order/buy",
                "order/sell",
                "order/cancel",
                "orders"
            }}
        }}
    };

    this->fees = {
        {"trading", {
            {"maker", 0.002},
            {"taker", 0.002}
        }}
    };
}

nlohmann::json TradeOgre::fetch_markets() {
    auto response = this->fetch("markets", "public");
    auto result = nlohmann::json::array();

    for (const auto& market : response.items()) {
        auto parts = this->split(market.key(), "-");
        auto baseId = parts[0];
        auto quoteId = parts[1];
        auto base = this->safe_currency_code(baseId);
        auto quote = this->safe_currency_code(quoteId);
        auto symbol = base + "/" + quote;

        result.push_back({
            {"id", market.key()},
            {"symbol", symbol},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", true},
            {"type", "spot"},
            {"spot", true},
            {"margin", false},
            {"future", false}
        });
    }
    return result;
}

nlohmann::json TradeOgre::fetch_ticker(const std::string& symbol) {
    auto market = this->market(symbol);
    auto request = {
        {"market", market["id"].get<std::string>()}
    };
    auto response = this->fetch("ticker/" + market["id"].get<std::string>(), "public");
    return this->parse_ticker(response, market);
}

nlohmann::json TradeOgre::fetch_order_book(const std::string& symbol, int limit) {
    auto market = this->market(symbol);
    auto request = {
        {"market", market["id"].get<std::string>()}
    };
    auto response = this->fetch("orders/" + market["id"].get<std::string>(), "public");
    return this->parse_order_book(response, symbol);
}

nlohmann::json TradeOgre::create_order(const std::string& symbol, const std::string& type,
                                     const std::string& side, double amount, double price) {
    this->check_required_credentials();
    auto market = this->market(symbol);
    
    if (type != "limit") {
        throw std::runtime_error("Only limit orders are supported by TradeOgre");
    }

    auto request = {
        {"market", market["id"].get<std::string>()},
        {"quantity", this->amount_to_precision(symbol, amount)},
        {"price", this->price_to_precision(symbol, price)}
    };

    auto method = (side == "buy") ? "order/buy" : "order/sell";
    auto response = this->fetch(method, "private", "POST", request);
    
    return this->parse_order(response);
}

nlohmann::json TradeOgre::cancel_order(const std::string& id, const std::string& symbol) {
    this->check_required_credentials();
    auto request = {
        {"uuid", id}
    };
    return this->fetch("order/cancel", "private", "POST", request);
}

nlohmann::json TradeOgre::fetch_balance() {
    this->check_required_credentials();
    auto response = this->fetch("account/balances", "private", "POST");
    return this->parse_balance(response);
}

nlohmann::json TradeOgre::fetch_open_orders(const std::string& symbol, int since, int limit) {
    this->check_required_credentials();
    auto response = this->fetch("orders", "private", "POST");
    return this->parse_orders(response, nullptr, since, limit);
}

std::string TradeOgre::sign(const std::string& path, const std::string& api,
                           const std::string& method, const nlohmann::json& params,
                           const std::map<std::string, std::string>& headers) {
    auto url = this->urls["api"][api].get<std::string>() + "/" + this->implode_params(path, params);
    auto query = this->omit(params, this->extract_params(path));

    if (api == "private") {
        this->check_required_credentials();
        auto auth = this->string_to_base64(this->config_.apiKey + ":" + this->config_.secret);
        auto new_headers = headers;
        new_headers["Authorization"] = "Basic " + auth;
        
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    }

    return url;
}

nlohmann::json TradeOgre::parse_ticker(const nlohmann::json& ticker, const nlohmann::json& market) {
    auto timestamp = this->milliseconds();
    auto symbol = market ? market["symbol"].get<std::string>() : "";
    
    return {
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"high", this->safe_number(ticker, "high")},
        {"low", this->safe_number(ticker, "low")},
        {"bid", this->safe_number(ticker, "bid")},
        {"ask", this->safe_number(ticker, "ask")},
        {"last", this->safe_number(ticker, "price")},
        {"volume", this->safe_number(ticker, "volume")},
        {"previousClose", nullptr},
        {"change", nullptr},
        {"percentage", nullptr},
        {"average", nullptr},
        {"baseVolume", this->safe_number(ticker, "volume")},
        {"quoteVolume", nullptr},
        {"info", ticker}
    };
}

nlohmann::json TradeOgre::parse_balance(const nlohmann::json& response) {
    auto result = {
        {"info", response},
        {"timestamp", nullptr},
        {"datetime", nullptr}
    };

    for (const auto& balance : response.items()) {
        auto currencyId = balance.key();
        auto code = this->safe_currency_code(currencyId);
        auto account = this->account();
        account["total"] = this->safe_string(balance.value(), "available");
        result[code] = account;
    }

    return result;
}

} // namespace ccxt
