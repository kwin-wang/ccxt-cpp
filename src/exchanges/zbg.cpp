#include "ccxt/exchanges/zbg.h"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

ZBG::ZBG() {
    this->id = "zbg";
    this->name = "ZBG";
    this->countries = {"CN"};  // China
    this->version = "v1";
    this->rateLimit = 1000;
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
        {"fetchOrders", true},
        {"fetchTicker", true},
        {"fetchTickers", true},
        {"fetchTrades", true},
        {"fetchWithdrawals", true},
        {"withdraw", true},
        {"fetchMarginBalance", true},
        {"createMarginOrder", true}
    };

    this->timeframes = {
        {"1m", "1min"},
        {"3m", "3min"},
        {"5m", "5min"},
        {"15m", "15min"},
        {"30m", "30min"},
        {"1h", "1hour"},
        {"2h", "2hour"},
        {"4h", "4hour"},
        {"6h", "6hour"},
        {"12h", "12hour"},
        {"1d", "1day"},
        {"1w", "1week"}
    };

    this->urls = {
        {"logo", "https://user-images.githubusercontent.com/51840849/87295551-102fbf00-c50e-11ea-90a9-462eebba5829.jpg"},
        {"api", {
            {"public", "https://www.zbg.com/exchange/api/v1/public"},
            {"private", "https://www.zbg.com/exchange/api/v1/private"},
            {"market", "https://www.zbg.com/exchange/api/v1/market"}
        }},
        {"www", "https://www.zbg.com"},
        {"doc", {
            "https://www.zbg.com/docs/guide/",
            "https://github.com/ZBGCoin/API"
        }},
        {"fees", "https://www.zbg.com/help/rate"}
    };

    this->api = {
        {"public", {
            {"GET", {
                "markets",
                "ticker",
                "depth",
                "trades",
                "kline"
            }}
        }},
        {"private", {
            {"POST", {
                "order",
                "cancelOrder",
                "getOrder",
                "getOrders",
                "getOpenOrders",
                "getFinishedOrders",
                "getUserAddress",
                "getWithdrawAddress",
                "getWithdrawRecord",
                "getDepositRecord",
                "withdraw"
            }}
        }},
        {"market", {
            {"GET", {
                "getAllAssets",
                "getAllSymbols"
            }}
        }}
    };

    this->fees = {
        {"trading", {
            {"tierBased", true},
            {"percentage", true},
            {"maker", 0.002},
            {"taker", 0.002}
        }},
        {"funding", {
            {"tierBased", false},
            {"percentage", false},
            {"withdraw", {}},
            {"deposit", {}}
        }}
    };
}

nlohmann::json ZBG::fetch_markets() {
    auto response = this->fetch("getAllSymbols", "market");
    auto result = nlohmann::json::array();

    for (const auto& market : response["datas"]) {
        auto id = market["symbol"].get<std::string>();
        auto baseId = market["baseCurrency"].get<std::string>();
        auto quoteId = market["quoteCurrency"].get<std::string>();
        auto base = this->safe_currency_code(baseId);
        auto quote = this->safe_currency_code(quoteId);
        auto symbol = base + "/" + quote;

        result.push_back({
            {"id", id},
            {"symbol", symbol},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", market["enable"].get<bool>()},
            {"precision", {
                {"amount", market["amountPrecision"].get<int>()},
                {"price", market["pricePrecision"].get<int>()}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safe_number(market, "minAmount")},
                    {"max", this->safe_number(market, "maxAmount")}
                }},
                {"price", {
                    {"min", this->safe_number(market, "minPrice")},
                    {"max", this->safe_number(market, "maxPrice")}
                }},
                {"cost", {
                    {"min", this->safe_number(market, "minTotal")},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    return result;
}

nlohmann::json ZBG::create_order(const std::string& symbol, const std::string& type,
                                const std::string& side, double amount, double price) {
    this->check_required_credentials();
    auto market = this->market(symbol);
    
    auto request = {
        {"symbol", market["id"]},
        {"side", side},
        {"type", type},
        {"volume", this->amount_to_precision(symbol, amount)}
    };

    if (type == "limit") {
        request["price"] = this->price_to_precision(symbol, price);
    }

    auto response = this->fetch("order", "private", "POST", request);
    return this->parse_order(response["datas"]);
}

nlohmann::json ZBG::fetch_balance() {
    this->check_required_credentials();
    auto response = this->fetch("getBalance", "private", "POST");
    return this->parse_balance(response);
}

std::string ZBG::sign(const std::string& path, const std::string& api,
                      const std::string& method, const nlohmann::json& params,
                      const std::map<std::string, std::string>& headers) {
    auto url = this->urls["api"][api].get<std::string>() + "/" + this->implode_params(path, params);
    auto query = this->omit(params, this->extract_params(path));

    if (api == "public" || api == "market") {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->check_required_credentials();
        auto timestamp = std::to_string(this->milliseconds());
        auto auth = timestamp + method + "/api/v1/private/" + path;
        
        if (!query.empty()) {
            auth += this->urlencode(this->keysort(query));
        }

        auto signature = this->hmac(auth, this->config_.secret, "sha256", "hex");
        auto new_headers = headers;
        new_headers["X-BH-APIKEY"] = this->config_.apiKey;
        new_headers["X-BH-TIMESTAMP"] = timestamp;
        new_headers["X-BH-SIGNATURE"] = signature;

        if (method == "POST") {
            new_headers["Content-Type"] = "application/x-www-form-urlencoded";
        }

        if (!query.empty()) {
            if (method == "GET") {
                url += "?" + this->urlencode(query);
            } else {
                auto body = this->urlencode(query);
                new_headers["Content-Length"] = std::to_string(body.length());
            }
        }
    }

    return url;
}

nlohmann::json ZBG::parse_ticker(const nlohmann::json& ticker, const nlohmann::json& market) {
    auto timestamp = this->safe_timestamp(ticker, "timestamp");
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
        {"quoteVolume", this->safe_number(ticker, "quoteVol")},
        {"info", ticker}
    };
}

nlohmann::json ZBG::parse_balance(const nlohmann::json& response) {
    auto balances = response["datas"]["list"];
    auto result = {
        {"info", response},
        {"timestamp", nullptr},
        {"datetime", nullptr}
    };

    for (const auto& balance : balances) {
        auto currencyId = balance["currency"].get<std::string>();
        auto code = this->safe_currency_code(currencyId);
        auto account = this->account();
        account["free"] = this->safe_string(balance, "available");
        account["used"] = this->safe_string(balance, "frozen");
        result[code] = account;
    }

    return result;
}

std::string ZBG::get_market_id(const std::string& symbol) {
    auto market = this->market(symbol);
    return market["id"].get<std::string>();
}

std::string ZBG::get_currency_id(const std::string& code) {
    if (this->currencies.find(code) != this->currencies.end()) {
        return this->currencies[code]["id"].get<std::string>();
    }
    return code;
}

std::string ZBG::get_order_id() {
    return std::to_string(this->milliseconds());
}

std::string ZBG::get_signed_params(const nlohmann::json& params) {
    auto sorted = this->keysort(params);
    return this->urlencode(sorted);
}

std::string ZBG::get_signature(const std::string& path, const std::string& method,
                              const nlohmann::json& params, const std::string& timestamp) {
    auto auth = timestamp + method + "/api/v1/private/" + path;
    
    if (!params.empty()) {
        auth += this->urlencode(this->keysort(params));
    }

    return this->hmac(auth, this->config_.secret, "sha256", "hex");
}

} // namespace ccxt
