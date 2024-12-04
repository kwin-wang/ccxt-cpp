#include "ccxt/exchanges/yobit.h"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Yobit::Yobit() {
    this->id = "yobit";
    this->name = "YoBit";
    this->countries = {"RU"};  // Russia
    this->rateLimit = 3000;
    this->version = "3";
    this->has = {
        {"cancelOrder", true},
        {"CORS", false},
        {"createOrder", true},
        {"fetchBalance", true},
        {"fetchDepositAddress", true},
        {"fetchMarkets", true},
        {"fetchMyTrades", true},
        {"fetchOpenOrders", true},
        {"fetchOrder", true},
        {"fetchOrderBook", true},
        {"fetchTicker", true},
        {"fetchTickers", true},
        {"fetchTrades", true},
        {"withdraw", true}
    };

    this->urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/27766910-cdcbfdae-5eea-11e7-9859-03fea873272d.jpg"},
        {"api", {
            {"public", "https://yobit.net/api/3"},
            {"private", "https://yobit.net/tapi"}
        }},
        {"www", "https://www.yobit.net"},
        {"doc", "https://www.yobit.net/en/api/"},
        {"fees", "https://www.yobit.net/en/fees/"}
    };

    this->api = {
        {"public", {
            {"GET", {
                "info",
                "ticker/{pair}",
                "depth/{pair}",
                "trades/{pair}"
            }}
        }},
        {"private", {
            {"POST", {
                "getInfo",
                "Trade",
                "ActiveOrders",
                "OrderInfo",
                "CancelOrder",
                "TradeHistory",
                "GetDepositAddress",
                "WithdrawCoinsToAddress"
            }}
        }}
    };

    this->fees = {
        {"trading", {
            {"maker", 0.002},
            {"taker", 0.002}
        }},
        {"funding", {
            {"withdraw", {}},
            {"deposit", {}}
        }}
    };

    this->precisionMode = DECIMAL_PLACES;
}

nlohmann::json Yobit::fetch_markets() {
    auto response = this->fetch("info", "public");
    auto pairs = response["pairs"];
    auto result = nlohmann::json::array();

    for (const auto& entry : pairs.items()) {
        auto id = entry.key();
        auto market = entry.value();
        auto symbol_parts = this->split(id, '_');
        if (symbol_parts.size() != 2) continue;

        auto baseId = symbol_parts[0];
        auto quoteId = symbol_parts[1];
        auto base = this->safe_currency_code(baseId);
        auto quote = this->safe_currency_code(quoteId);
        auto symbol = base + "/" + quote;

        auto precision = {
            {"amount", this->safe_integer(market, "decimal_places")},
            {"price", this->safe_integer(market, "decimal_places")}
        };

        auto limits = {
            {"amount", {
                {"min", this->safe_number(market, "min_amount")},
                {"max", this->safe_number(market, "max_amount")}
            }},
            {"price", {
                {"min", this->safe_number(market, "min_price")},
                {"max", this->safe_number(market, "max_price")}
            }},
            {"cost", {
                {"min", this->safe_number(market, "min_total")},
                {"max", nullptr}
            }}
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
            {"limits", limits},
            {"info", market}
        });
    }
    return result;
}

nlohmann::json Yobit::create_order(const std::string& symbol, const std::string& type,
                                  const std::string& side, double amount, double price) {
    this->check_required_credentials();
    if (type != "limit") {
        throw std::runtime_error("YoBit only supports limit orders");
    }

    auto market = this->market(symbol);
    auto request = {
        {"pair", market["id"]},
        {"type", side},
        {"amount", this->amount_to_precision(symbol, amount)},
        {"rate", this->price_to_precision(symbol, price)}
    };

    auto response = this->fetch("Trade", "private", "POST", request);
    auto id = this->safe_string(response, "order_id");
    return {
        {"info", response},
        {"id", id},
        {"symbol", symbol},
        {"type", type},
        {"side", side},
        {"status", "open"}
    };
}

nlohmann::json Yobit::fetch_balance() {
    this->check_required_credentials();
    auto response = this->fetch("getInfo", "private", "POST");
    auto balances = response["return"]["funds"];
    auto result = {
        {"info", response},
        {"timestamp", nullptr},
        {"datetime", nullptr}
    };

    for (const auto& balance : balances.items()) {
        auto currencyId = balance.key();
        auto code = this->safe_currency_code(currencyId);
        auto account = this->account();
        account["free"] = this->safe_string(balances, currencyId);
        account["used"] = "0";
        result[code] = account;
    }

    return result;
}

std::string Yobit::sign(const std::string& path, const std::string& api,
                        const std::string& method, const nlohmann::json& params,
                        const std::map<std::string, std::string>& headers) {
    auto url = this->urls["api"][api].get<std::string>();
    auto query = this->omit(params, this->extract_params(path));

    if (api == "public") {
        url += "/" + this->implode_params(path, params);
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->check_required_credentials();
        auto nonce = this->get_nonce_string();
        auto body = "nonce=" + nonce;
        
        if (!query.empty()) {
            body += "&" + this->urlencode(query);
        }

        auto signature = this->hmac(body, this->secret, "sha512", "hex");
        auto new_headers = headers;
        new_headers["Content-Type"] = "application/x-www-form-urlencoded";
        new_headers["Key"] = this->apiKey;
        new_headers["Sign"] = signature;

        if (method == "POST") {
            new_headers["Content-Length"] = std::to_string(body.length());
        }
    }

    return url;
}

nlohmann::json Yobit::parse_ticker(const nlohmann::json& ticker, const nlohmann::json& market) {
    auto timestamp = this->safe_timestamp(ticker, "updated");
    auto symbol = market.empty() ? "" : market["symbol"].get<std::string>();
    
    return {
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"high", this->safe_number(ticker, "high")},
        {"low", this->safe_number(ticker, "low")},
        {"bid", this->safe_number(ticker, "buy")},
        {"ask", this->safe_number(ticker, "sell")},
        {"last", this->safe_number(ticker, "last")},
        {"close", this->safe_number(ticker, "last")},
        {"baseVolume", this->safe_number(ticker, "vol")},
        {"quoteVolume", this->safe_number(ticker, "vol_cur")},
        {"info", ticker}
    };
}

nlohmann::json Yobit::parse_trade(const nlohmann::json& trade, const nlohmann::json& market) {
    auto timestamp = this->safe_timestamp(trade, "timestamp");
    auto side = this->safe_string(trade, "type");
    auto price = this->safe_number(trade, "price");
    auto amount = this->safe_number(trade, "amount");
    auto cost = price * amount;
    
    return {
        {"info", trade},
        {"id", this->safe_string(trade, "tid")},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"symbol", market["symbol"]},
        {"type", "limit"},
        {"side", side},
        {"price", price},
        {"amount", amount},
        {"cost", cost}
    };
}

std::string Yobit::get_market_id(const std::string& symbol) {
    auto market = this->market(symbol);
    return market["id"].get<std::string>();
}

std::string Yobit::get_currency_id(const std::string& code) {
    if (this->currencies.find(code) != this->currencies.end()) {
        return this->currencies[code]["id"].get<std::string>();
    }
    return code.substr(0, 1).c_str() + code.substr(1);
}

std::string Yobit::get_nonce_string() {
    return std::to_string(this->nonce());
}

std::string Yobit::get_signature(const std::string& path, const std::string& method,
                                const nlohmann::json& params, const std::string& nonce) {
    auto body = "nonce=" + nonce;
    if (!params.empty()) {
        body += "&" + this->urlencode(params);
    }
    return this->hmac(body, this->secret, "sha512", "hex");
}

} // namespace ccxt
