#include "ccxt/exchanges/zb.h"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

ZB::ZB() {
    this->id = "zb";
    this->name = "ZB";
    this->countries = {"CN"};  // China
    this->version = "1";
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
        {"fetchFundingRate", true},
        {"fetchFundingRates", true},
        {"fetchFundingHistory", true}
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
        {"3d", "3day"},
        {"1w", "1week"}
    };

    this->urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/32859187-cd5214f0-ca5e-11e7-967d-96568e2e2bd1.jpg"},
        {"api", {
            {"public", "http://api.zb.com/data/v1"},
            {"private", "https://trade.zb.com/api/v1"},
            {"trade", "https://trade.zb.com/api"}
        }},
        {"www", "https://www.zb.com"},
        {"doc", {
            "https://www.zb.com/i/developer",
            "https://github.com/ZBFuture/docs/blob/main/API%20V2%20_en.md"
        }},
        {"fees", "https://www.zb.com/i/rate"}
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
                "getOrdersNew",
                "getOrdersIgnoreTradeType",
                "getUnfinishedOrdersIgnoreTradeType",
                "getAccountInfo",
                "getUserAddress",
                "getWithdrawAddress",
                "getWithdrawRecord",
                "getChargeRecord",
                "withdraw"
            }}
        }},
        {"trade", {
            {"GET", {
                "getFundingRate",
                "getFundingRates",
                "getFundingHistory"
            }}
        }}
    };

    this->fees = {
        {"trading", {
            {"tierBased", false},
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

nlohmann::json ZB::fetch_markets() {
    auto response = this->fetch("markets", "public");
    auto result = nlohmann::json::array();

    for (const auto& entry : response.items()) {
        auto id = entry.key();
        auto market = entry.value();
        auto symbol = id;
        auto parts = this->split(id, "_");
        
        if (parts.size() == 2) {
            auto baseId = parts[0];
            auto quoteId = parts[1];
            auto base = this->safe_currency_code(baseId);
            auto quote = this->safe_currency_code(quoteId);
            symbol = base + "/" + quote;

            result.push_back({
                {"id", id},
                {"symbol", symbol},
                {"base", base},
                {"quote", quote},
                {"baseId", baseId},
                {"quoteId", quoteId},
                {"active", true},
                {"precision", {
                    {"amount", market["amountScale"].get<int>()},
                    {"price", market["priceScale"].get<int>()}
                }},
                {"limits", {
                    {"amount", {
                        {"min", std::pow(10, -market["amountScale"].get<int>())},
                        {"max", nullptr}
                    }},
                    {"price", {
                        {"min", std::pow(10, -market["priceScale"].get<int>())},
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
    }
    return result;
}

nlohmann::json ZB::create_order(const std::string& symbol, const std::string& type,
                               const std::string& side, double amount, double price) {
    this->check_required_credentials();
    auto market = this->market(symbol);
    
    auto request = {
        {"method", "order"},
        {"price", this->price_to_precision(symbol, price)},
        {"amount", this->amount_to_precision(symbol, amount)},
        {"tradeType", (side == "buy") ? "1" : "0"},
        {"currency", market["id"]}
    };

    auto response = this->fetch("order", "private", "POST", request);
    return this->parse_order(response);
}

nlohmann::json ZB::fetch_balance() {
    this->check_required_credentials();
    auto response = this->fetch("getAccountInfo", "private", "POST");
    return this->parse_balance(response);
}

nlohmann::json ZB::fetch_funding_rate(const std::string& symbol) {
    this->check_required_credentials();
    auto market = this->market(symbol);
    auto request = {{"symbol", market["id"]}};
    auto response = this->fetch("getFundingRate", "trade", "GET", request);
    return this->parse_funding_rate(response, market);
}

std::string ZB::sign(const std::string& path, const std::string& api,
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
        query["accesskey"] = this->apiKey;
        query["method"] = path;

        auto nonce = this->nonce();
        auto sorted = this->keysort(query);
        auto auth = this->urlencode(sorted);
        auto signature = this->hmac(auth, this->secret, "md5");
        sorted["sign"] = signature;
        sorted["reqTime"] = std::to_string(nonce);

        if (method == "GET") {
            url += "?" + this->urlencode(sorted);
        } else {
            auto body = this->json(sorted);
            auto new_headers = headers;
            new_headers["Content-Type"] = "application/json";
            new_headers["Content-Length"] = std::to_string(body.length());
        }
    }

    return url;
}

nlohmann::json ZB::parse_ticker(const nlohmann::json& ticker, const nlohmann::json& market) {
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
        {"quoteVolume", nullptr},
        {"info", ticker}
    };
}

nlohmann::json ZB::parse_balance(const nlohmann::json& response) {
    auto balances = response["result"]["coins"];
    auto result = {
        {"info", response},
        {"timestamp", nullptr},
        {"datetime", nullptr}
    };

    for (const auto& balance : balances) {
        auto currencyId = balance["key"].get<std::string>();
        auto code = this->safe_currency_code(currencyId);
        auto account = this->account();
        account["free"] = this->safe_string(balance, "available");
        account["used"] = this->safe_string(balance, "freez");
        result[code] = account;
    }

    return result;
}

nlohmann::json ZB::parse_funding_rate(const nlohmann::json& fundingRate, const nlohmann::json& market) {
    return {
        {"info", fundingRate},
        {"symbol", market["symbol"].get<std::string>()},
        {"timestamp", this->safe_timestamp(fundingRate, "timestamp")},
        {"datetime", this->iso8601(this->safe_timestamp(fundingRate, "timestamp"))},
        {"fundingRate", this->safe_number(fundingRate, "fundingRate")},
        {"fundingTimestamp", this->safe_timestamp(fundingRate, "nextFundingTime")},
        {"fundingDatetime", this->iso8601(this->safe_timestamp(fundingRate, "nextFundingTime"))}
    };
}

std::string ZB::get_market_id(const std::string& symbol) {
    auto market = this->market(symbol);
    return market["id"].get<std::string>();
}

std::string ZB::get_currency_id(const std::string& code) {
    if (this->currencies.find(code) != this->currencies.end()) {
        return this->currencies[code]["id"].get<std::string>();
    }
    return code;
}

std::string ZB::get_order_id() {
    return std::to_string(this->milliseconds());
}

std::string ZB::get_signed_params(const nlohmann::json& params) {
    auto sorted = this->keysort(params);
    return this->urlencode(sorted);
}

} // namespace ccxt
