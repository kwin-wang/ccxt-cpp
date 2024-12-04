#include "ccxt/exchanges/bigone.h"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

BigONE::BigONE() {
    this->id = "bigone";
    this->name = "BigONE";
    this->countries = {"SG"};  // Singapore
    this->version = "v3";
    this->rateLimit = 1000;
    this->has = {
        {"cancelAllOrders", true},
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
        {"fetchTrades", true},
        {"fetchWithdrawals", true},
        {"withdraw", true}
    };

    this->timeframes = {
        {"1m", "1m"},
        {"5m", "5m"},
        {"15m", "15m"},
        {"30m", "30m"},
        {"1h", "1h"},
        {"2h", "2h"},
        {"4h", "4h"},
        {"6h", "6h"},
        {"12h", "12h"},
        {"1d", "1d"},
        {"1w", "1w"},
        {"1M", "1M"}
    };

    this->urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/69354403-1d532180-0c91-11ea-88ed-44c06cefdf87.jpg"},
        {"api", {
            {"public", "https://big.one/api/v3"},
            {"private", "https://big.one/api/v3"}
        }},
        {"www", "https://big.one"},
        {"doc", {
            "https://open.big.one/docs/api.html"
        }},
        {"fees", "https://bigone.zendesk.com/hc/en-us/articles/115001933374-BigONE-Fee-Policy"}
    };

    this->api = {
        {"public", {
            {"GET", {
                "ping",
                "asset_pairs",
                "asset_pairs/{asset_pair_name}/depth",
                "asset_pairs/{asset_pair_name}/trades",
                "asset_pairs/{asset_pair_name}/ticker",
                "asset_pairs/{asset_pair_name}/candles",
                "asset_pairs/tickers"
            }}
        }},
        {"private", {
            {"GET", {
                "accounts",
                "orders",
                "orders/{id}",
                "trades",
                "withdrawals",
                "deposits"
            }},
            {"POST", {
                "orders",
                "orders/{id}/cancel",
                "orders/cancel_all",
                "withdrawals"
            }}
        }}
    };

    this->fees = {
        {"trading", {
            {"tierBased", false},
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

nlohmann::json BigONE::fetch_markets() {
    auto response = this->fetch("asset_pairs", "public");
    auto result = nlohmann::json::array();

    for (const auto& market : response["data"]) {
        auto id = market["name"].get<std::string>();
        auto baseId = market["base_asset"].get<std::string>();
        auto quoteId = market["quote_asset"].get<std::string>();
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
                {"amount", market["base_scale"].get<int>()},
                {"price", market["quote_scale"].get<int>()}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safe_number(market, "minimum_amount")},
                    {"max", nullptr}
                }},
                {"price", {
                    {"min", this->safe_number(market, "minimum_price")},
                    {"max", nullptr}
                }},
                {"cost", {
                    {"min", this->safe_number(market, "minimum_value")},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    return result;
}

nlohmann::json BigONE::create_order(const std::string& symbol, const std::string& type,
                                  const std::string& side, double amount, double price) {
    this->check_required_credentials();
    auto market = this->market(symbol);
    
    auto request = {
        {"asset_pair_name", market["id"].get<std::string>()},
        {"side", side.upper()},
        {"amount", this->amount_to_precision(symbol, amount)},
        {"order_type", type.upper()}
    };

    if (type == "LIMIT") {
        request["price"] = this->price_to_precision(symbol, price);
    }

    auto response = this->fetch("orders", "private", "POST", request);
    return this->parse_order(response["data"]);
}

nlohmann::json BigONE::cancel_order(const std::string& id, const std::string& symbol) {
    this->check_required_credentials();
    auto request = {
        {"id", id}
    };
    return this->fetch("orders/" + id + "/cancel", "private", "POST");
}

nlohmann::json BigONE::fetch_balance() {
    this->check_required_credentials();
    auto response = this->fetch("accounts", "private");
    return this->parse_balance(response);
}

std::string BigONE::sign(const std::string& path, const std::string& api,
                        const std::string& method, const nlohmann::json& params,
                        const std::map<std::string, std::string>& headers) {
    auto url = this->urls["api"][api].get<std::string>() + "/" + this->implode_params(path, params);
    auto query = this->omit(params, this->extract_params(path));

    if (api == "private") {
        this->check_required_credentials();
        auto timestamp = std::to_string(this->nonce());
        auto auth = timestamp + method + "/" + this->version + "/" + path;
        
        if (!query.empty()) {
            if (method == "GET") {
                auth += "?" + this->urlencode(query);
                url += "?" + this->urlencode(query);
            } else {
                auth += this->json(query);
            }
        }

        auto signature = this->hmac(auth, this->secret, "sha256", "hex");
        auto new_headers = headers;
        new_headers["Authorization"] = "Bearer " + this->apiKey;
        new_headers["Big-Device-Id"] = this->get_signed_token();
        new_headers["Content-Type"] = "application/json";
        new_headers["timestamp"] = timestamp;
        new_headers["signature"] = signature;
    } else {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    }

    return url;
}

nlohmann::json BigONE::parse_ticker(const nlohmann::json& ticker, const nlohmann::json& market) {
    auto timestamp = this->safe_timestamp(ticker, "timestamp");
    auto symbol = market ? market["symbol"].get<std::string>() : "";
    
    return {
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"high", this->safe_number(ticker, "high")},
        {"low", this->safe_number(ticker, "low")},
        {"bid", this->safe_number(ticker, "bid")},
        {"ask", this->safe_number(ticker, "ask")},
        {"last", this->safe_number(ticker, "close")},
        {"close", this->safe_number(ticker, "close")},
        {"baseVolume", this->safe_number(ticker, "volume")},
        {"quoteVolume", this->safe_number(ticker, "volume_24h")},
        {"info", ticker}
    };
}

nlohmann::json BigONE::parse_balance(const nlohmann::json& response) {
    auto result = {
        {"info", response},
        {"timestamp", nullptr},
        {"datetime", nullptr}
    };

    for (const auto& balance : response["data"]) {
        auto currencyId = balance["asset_id"].get<std::string>();
        auto code = this->safe_currency_code(currencyId);
        auto account = this->account();
        account["free"] = this->safe_string(balance, "balance");
        account["used"] = this->safe_string(balance, "locked_balance");
        result[code] = account;
    }

    return result;
}

std::string BigONE::get_asset_id(const std::string& code) {
    if (this->currencies.find(code) != this->currencies.end()) {
        return this->currencies[code]["id"].get<std::string>();
    }
    return code;
}

std::string BigONE::get_order_id() {
    return std::to_string(this->nonce());
}

std::string BigONE::get_signed_token() {
    auto timestamp = std::to_string(this->nonce());
    return this->hmac(timestamp, this->secret, "sha256", "hex");
}

} // namespace ccxt
