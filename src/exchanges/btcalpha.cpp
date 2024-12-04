#include "ccxt/exchanges/btcalpha.h"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

BTCAlpha::BTCAlpha() {
    this->id = "btcalpha";
    this->name = "BTC-Alpha";
    this->countries = {"US"};
    this->rateLimit = 1000;
    this->version = "v1";
    this->has = {
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
        {"fetchTicker", true},
        {"fetchTickers", true},
        {"fetchTrades", true},
        {"fetchWithdrawals", true},
        {"withdraw", true}
    };

    this->timeframes = {
        {"1m", "1"},
        {"5m", "5"},
        {"15m", "15"},
        {"30m", "30"},
        {"1h", "60"},
        {"4h", "240"},
        {"1d", "D"}
    };

    this->urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/42625213-dabaa5da-85cf-11e8-8f99-aa8f8f7699f0.jpg"},
        {"api", "https://btc-alpha.com/api"},
        {"www", "https://btc-alpha.com"},
        {"doc", "https://btc-alpha.github.io/api-docs"},
        {"fees", "https://btc-alpha.com/fees/"}
    };

    this->api = {
        {"public", {
            {"GET", {
                "currencies/",
                "pairs/",
                "orderbook/{pair}/",
                "exchanges/",
                "charts/{pair}/{type}/chart/",
                "ticker/",
                "ticker/{pair}/"
            }}
        }},
        {"private", {
            {"GET", {
                "wallets/",
                "orders/own/",
                "orders/own/{id}/",
                "deposits/",
                "withdraws/",
                "orders/",
                "orders/{id}/"
            }},
            {"POST", {
                "order/",
                "orders/{id}/cancel/",
                "withdraws/payment/"
            }}
        }}
    };

    this->fees = {
        {"trading", {
            {"maker", 0.002},
            {"taker", 0.002}
        }},
        {"funding", {
            {"withdraw", {
                "BTC": 0.00135,
                "LTC": 0.0035,
                "XMR": 0.018,
                "ZEC": 0.002,
                "ETH": 0.01,
                "ETC": 0.01,
                "SIB": 1.5,
                "CCRB": 4,
                "PZM": 0.05,
                "ITI": 0.05,
                "DCY": 5,
                "R": 5,
                "ATB": 0.05,
                "BRIA": 0.05,
                "KZC": 0.05,
                "HWC": 1,
                "SPA": 1,
                "SMS": 0.05,
                "REC": 0.05,
                "SUP": 1,
                "BQ": 100,
                "GDS": 0.05,
                "EVN": 300,
                "TRKC": 0.01,
                "UNI": 1,
                "STN": 1,
                "BCH": 0.001,
                "QBIC": 0.05
            }}
        }}
    };
}

nlohmann::json BTCAlpha::fetch_markets() {
    auto response = this->fetch("pairs/", "public");
    auto result = nlohmann::json::array();

    for (const auto& market : response) {
        auto id = market["name"].get<std::string>();
        auto baseId = market["currency1"].get<std::string>();
        auto quoteId = market["currency2"].get<std::string>();
        auto base = this->safe_currency_code(baseId);
        auto quote = this->safe_currency_code(quoteId);
        auto symbol = base + "/" + quote;
        auto precision = {
            {"amount", market["price_precision"].get<int>()},
            {"price", market["price_precision"].get<int>()}
        };

        result.push_back({
            {"id", id},
            {"symbol", symbol},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", true},
            {"precision", precision},
            {"limits", {
                {"amount", {
                    {"min", this->safe_number(market, "minimum_order_size")},
                    {"max", this->safe_number(market, "maximum_order_size")}
                }},
                {"price", {
                    {"min", std::pow(10, -precision["price"].get<int>())},
                    {"max", nullptr}
                }},
                {"cost", {
                    {"min", nullptr},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    return result;
}

nlohmann::json BTCAlpha::create_order(const std::string& symbol, const std::string& type,
                                    const std::string& side, double amount, double price) {
    this->check_required_credentials();
    auto market = this->market(symbol);
    
    auto request = {
        {"pair", market["id"].get<std::string>()},
        {"type", side},
        {"amount", this->amount_to_precision(symbol, amount)},
        {"price", this->price_to_precision(symbol, price)}
    };

    auto response = this->fetch("order/", "private", "POST", request);
    return this->parse_order(response);
}

nlohmann::json BTCAlpha::cancel_order(const std::string& id, const std::string& symbol) {
    this->check_required_credentials();
    return this->fetch("orders/" + id + "/cancel/", "private", "POST");
}

nlohmann::json BTCAlpha::fetch_balance() {
    this->check_required_credentials();
    auto response = this->fetch("wallets/", "private");
    return this->parse_balance(response);
}

std::string BTCAlpha::sign(const std::string& path, const std::string& api,
                          const std::string& method, const nlohmann::json& params,
                          const std::map<std::string, std::string>& headers) {
    auto url = this->urls["api"].get<std::string>() + "/" + this->version + "/" + this->implode_params(path, params);
    auto query = this->omit(params, this->extract_params(path));

    if (api == "private") {
        this->check_required_credentials();
        auto nonce = std::to_string(this->nonce());
        auto auth = nonce + this->uid + this->apiKey;
        auto signature = this->hmac(auth, this->secret, "sha256", "hex");
        
        auto new_headers = headers;
        new_headers["Api-Key"] = this->apiKey;
        new_headers["Api-Nonce"] = nonce;
        new_headers["Api-Signature"] = signature;
        
        if (!query.empty()) {
            if (method == "GET") {
                url += "?" + this->urlencode(query);
            } else {
                new_headers["Content-Type"] = "application/json";
            }
        }
    } else {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    }

    return url;
}

nlohmann::json BTCAlpha::parse_ticker(const nlohmann::json& ticker, const nlohmann::json& market) {
    auto timestamp = this->safe_timestamp(ticker, "timestamp");
    auto symbol = market ? market["symbol"].get<std::string>() : "";
    
    return {
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"high", this->safe_number(ticker, "high")},
        {"low", this->safe_number(ticker, "low")},
        {"bid", this->safe_number(ticker, "buy_price")},
        {"ask", this->safe_number(ticker, "sell_price")},
        {"last", this->safe_number(ticker, "last_price")},
        {"close", this->safe_number(ticker, "last_price")},
        {"previousClose", nullptr},
        {"change", nullptr},
        {"percentage", nullptr},
        {"average", nullptr},
        {"baseVolume", this->safe_number(ticker, "vol")},
        {"quoteVolume", nullptr},
        {"info", ticker}
    };
}

nlohmann::json BTCAlpha::parse_balance(const nlohmann::json& response) {
    auto result = {
        {"info", response},
        {"timestamp", nullptr},
        {"datetime", nullptr}
    };

    for (const auto& balance : response) {
        auto currencyId = balance["currency"].get<std::string>();
        auto code = this->safe_currency_code(currencyId);
        auto account = this->account();
        account["free"] = this->safe_string(balance, "balance");
        account["used"] = this->safe_string(balance, "reserve");
        result[code] = account;
    }

    return result;
}

std::string BTCAlpha::get_currency_id(const std::string& code) {
    if (this->currencies.find(code) != this->currencies.end()) {
        return this->currencies[code]["id"].get<std::string>();
    }
    return code;
}

} // namespace ccxt
