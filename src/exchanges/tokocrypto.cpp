#include "ccxt/exchanges/tokocrypto.h"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Tokocrypto::Tokocrypto() {
    this->id = "tokocrypto";
    this->name = "Tokocrypto";
    this->countries = {"ID"};  // Indonesia
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
        {"withdraw", true}
    };

    this->timeframes = {
        {"1m", "1m"},
        {"3m", "3m"},
        {"5m", "5m"},
        {"15m", "15m"},
        {"30m", "30m"},
        {"1h", "1h"},
        {"2h", "2h"},
        {"4h", "4h"},
        {"6h", "6h"},
        {"8h", "8h"},
        {"12h", "12h"},
        {"1d", "1d"},
        {"3d", "3d"},
        {"1w", "1w"},
        {"1M", "1M"}
    };

    this->urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/183870484-d3398d0c-f6a1-4cce-91b8-d58792308716.jpg"},
        {"api", {
            {"public", "https://www.tokocrypto.com/api/v1/public"},
            {"private", "https://www.tokocrypto.com/api/v1/private"},
            {"v1", "https://www.tokocrypto.com/api/v1"}
        }},
        {"www", "https://www.tokocrypto.com"},
        {"doc", {
            "https://www.tokocrypto.com/apidocs/",
            "https://github.com/tokocrypto/openapi-v1"
        }},
        {"fees", "https://www.tokocrypto.com/fees/trading"}
    };

    this->api = {
        {"public", {
            {"GET", {
                "market/pairs",
                "market/ticker",
                "market/depth",
                "market/trades",
                "market/kline"
            }}
        }},
        {"private", {
            {"POST", {
                "account/spot/asset/list",
                "order/spot/create",
                "order/spot/cancel",
                "order/spot/detail",
                "order/spot/list",
                "order/spot/open",
                "trade/spot/list",
                "wallet/deposit/address",
                "wallet/deposit/list",
                "wallet/withdraw/list",
                "wallet/withdraw/create"
            }}
        }}
    };

    this->fees = {
        {"trading", {
            {"tierBased", true},
            {"percentage", true},
            {"maker", 0.001},
            {"taker", 0.001}
        }},
        {"funding", {
            {"tierBased", false},
            {"percentage", false},
            {"withdraw", {}},
            {"deposit", {}}
        }}
    };
}

nlohmann::json Tokocrypto::fetch_markets() {
    auto response = this->fetch("market/pairs", "public");
    auto result = nlohmann::json::array();

    for (const auto& market : response["data"]) {
        auto id = market["symbol"].get<std::string>();
        auto baseId = market["baseAsset"].get<std::string>();
        auto quoteId = market["quoteAsset"].get<std::string>();
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
            {"active", true},
            {"precision", {
                {"amount", market["baseAssetPrecision"].get<int>()},
                {"price", market["quoteAssetPrecision"].get<int>()}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safe_number(market, "minQty")},
                    {"max", this->safe_number(market, "maxQty")}
                }},
                {"price", {
                    {"min", this->safe_number(market, "minPrice")},
                    {"max", this->safe_number(market, "maxPrice")}
                }},
                {"cost", {
                    {"min", this->safe_number(market, "minNotional")},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    return result;
}

nlohmann::json Tokocrypto::create_order(const std::string& symbol, const std::string& type,
                                       const std::string& side, double amount, double price) {
    this->check_required_credentials();
    auto market = this->market(symbol);
    
    auto request = {
        {"symbol", market["id"]},
        {"side", side.substr(0, 1).c_str()},  // Convert to uppercase B/S
        {"type", type},
        {"quantity", this->amount_to_precision(symbol, amount)}
    };

    if (type == "limit") {
        request["price"] = this->price_to_precision(symbol, price);
    }

    auto response = this->fetch("order/spot/create", "private", "POST", request);
    return this->parse_order(response["data"]);
}

nlohmann::json Tokocrypto::fetch_balance() {
    this->check_required_credentials();
    auto response = this->fetch("account/spot/asset/list", "private", "POST");
    return this->parse_balance(response);
}

std::string Tokocrypto::sign(const std::string& path, const std::string& api,
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
        auto sorted = this->keysort(query);
        auto auth = timestamp + method + "/api/v1/" + path;
        
        if (!sorted.empty()) {
            auth += this->urlencode(sorted);
        }

        auto signature = this->hmac(auth, this->config_.secret, "sha256", "hex");
        auto new_headers = headers;
        new_headers["TOK-API-KEY"] = this->config_.apiKey;
        new_headers["TOK-API-TIMESTAMP"] = timestamp;
        new_headers["TOK-API-SIGNATURE"] = signature;

        if (method == "POST") {
            new_headers["Content-Type"] = "application/json";
            if (!query.empty()) {
                auto body = this->json(query);
                new_headers["Content-Length"] = std::to_string(body.length());
            }
        } else if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    }

    return url;
}

nlohmann::json Tokocrypto::parse_ticker(const nlohmann::json& ticker, const nlohmann::json& market) {
    auto timestamp = this->safe_timestamp(ticker, "time");
    auto symbol = market.empty() ? "" : market["symbol"].get<std::string>();
    
    return {
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"high", this->safe_number(ticker, "highPrice")},
        {"low", this->safe_number(ticker, "lowPrice")},
        {"bid", this->safe_number(ticker, "bidPrice")},
        {"ask", this->safe_number(ticker, "askPrice")},
        {"last", this->safe_number(ticker, "lastPrice")},
        {"close", this->safe_number(ticker, "lastPrice")},
        {"baseVolume", this->safe_number(ticker, "volume")},
        {"quoteVolume", this->safe_number(ticker, "quoteVolume")},
        {"info", ticker}
    };
}

nlohmann::json Tokocrypto::parse_balance(const nlohmann::json& response) {
    auto balances = response["data"];
    auto result = {
        {"info", response},
        {"timestamp", nullptr},
        {"datetime", nullptr}
    };

    for (const auto& balance : balances) {
        auto currencyId = balance["asset"].get<std::string>();
        auto code = this->safe_currency_code(currencyId);
        auto account = this->account();
        account["free"] = this->safe_string(balance, "free");
        account["used"] = this->safe_string(balance, "locked");
        result[code] = account;
    }

    return result;
}

std::string Tokocrypto::get_market_id(const std::string& symbol) {
    auto market = this->market(symbol);
    return market["id"].get<std::string>();
}

std::string Tokocrypto::get_currency_id(const std::string& code) {
    if (this->currencies.find(code) != this->currencies.end()) {
        return this->currencies[code]["id"].get<std::string>();
    }
    return code;
}

std::string Tokocrypto::get_order_id() {
    return std::to_string(this->milliseconds());
}

std::string Tokocrypto::get_signature(const std::string& path, const std::string& method,
                                     const nlohmann::json& params, const std::string& timestamp) {
    auto auth = timestamp + method + "/api/v1/" + path;
    
    if (!params.empty()) {
        auth += this->urlencode(this->keysort(params));
    }

    return this->hmac(auth, this->config_.secret, "sha256", "hex");
}

} // namespace ccxt
