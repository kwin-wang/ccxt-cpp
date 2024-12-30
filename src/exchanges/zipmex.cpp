#include "ccxt/exchanges/zipmex.h"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Zipmex::Zipmex() {
    this->id = "zipmex";
    this->name = "Zipmex";
    this->countries = {"SG", "AU", "ID", "TH"};  // Singapore, Australia, Indonesia, Thailand
    this->version = "v1";
    this->rateLimit = 100;
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
        {"fetchLeverageTiers", true},
        {"fetchPositions", true}
    };

    this->timeframes = {
        {"1m", "1m"},
        {"5m", "5m"},
        {"15m", "15m"},
        {"30m", "30m"},
        {"1h", "1h"},
        {"4h", "4h"},
        {"1d", "1d"},
        {"1w", "1w"}
    };

    this->urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/146103275-c39a34d9-68a4-4cd2-b1f1-c684548d311b.jpg"},
        {"api", {
            {"public", "https://api.zipmex.com/api/v1/public"},
            {"private", "https://api.zipmex.com/api/v1/private"}
        }},
        {"www", "https://zipmex.com"},
        {"doc", {
            "https://docs.zipmex.com/",
            "https://github.com/zipmex/zipmex-api-docs"
        }},
        {"fees", "https://zipmex.com/fee-schedule"}
    };

    this->api = {
        {"public", {
            {"GET", {
                "markets",
                "ticker",
                "orderbook",
                "trades",
                "klines",
                "leverage/tiers"
            }}
        }},
        {"private", {
            {"POST", {
                "account/balances",
                "orders/create",
                "orders/cancel",
                "orders/status",
                "orders/list",
                "orders/open",
                "orders/history",
                "trades/list",
                "deposit/address",
                "deposit/history",
                "withdraw/history",
                "withdraw/create",
                "positions/list",
                "position/status"
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

nlohmann::json Zipmex::fetch_markets() {
    auto response = this->fetch("markets", "public");
    auto result = nlohmann::json::array();

    for (const auto& market : response["data"]) {
        auto id = market["symbol"].get<std::string>();
        auto baseId = market["base_currency"].get<std::string>();
        auto quoteId = market["quote_currency"].get<std::string>();
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
            {"active", market["active"].get<bool>()},
            {"precision", {
                {"amount", market["amount_precision"].get<int>()},
                {"price", market["price_precision"].get<int>()}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safe_number(market, "min_amount")},
                    {"max", this->safe_number(market, "max_amount")}
                }},
                {"price", {
                    {"min", this->safe_number(market, "min_price")},
                    {"max", this->safe_number(market, "max_price")}
                }},
                {"cost", {
                    {"min", this->safe_number(market, "min_notional")},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    return result;
}

nlohmann::json Zipmex::create_order(const std::string& symbol, const std::string& type,
                                   const std::string& side, double amount, double price) {
    this->check_required_credentials();
    auto market = this->market(symbol);
    
    auto request = {
        {"symbol", market["id"]},
        {"side", side},
        {"type", type},
        {"quantity", this->amount_to_precision(symbol, amount)}
    };

    if (type == "limit") {
        request["price"] = this->price_to_precision(symbol, price);
    }

    auto response = this->fetch("orders/create", "private", "POST", request);
    return this->parse_order(response["data"]);
}

nlohmann::json Zipmex::fetch_balance() {
    this->check_required_credentials();
    auto response = this->fetch("account/balances", "private", "POST");
    return this->parse_balance(response);
}

nlohmann::json Zipmex::fetch_positions(const std::vector<std::string>& symbols) {
    this->check_required_credentials();
    auto request = nlohmann::json::object();
    
    if (!symbols.empty()) {
        auto market_ids = std::vector<std::string>();
        for (const auto& symbol : symbols) {
            auto market = this->market(symbol);
            market_ids.push_back(market["id"].get<std::string>());
        }
        request["symbols"] = market_ids;
    }

    auto response = this->fetch("positions/list", "private", "POST", request);
    return this->parse_positions(response["data"]);
}

std::string Zipmex::sign(const std::string& path, const std::string& api,
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
        auto auth = method + timestamp + "/api/v1/private/" + path;
        
        if (!sorted.empty()) {
            auth += this->urlencode(sorted);
        }

        auto signature = this->hmac(auth, this->config_.secret, "sha256", "hex");
        auto new_headers = headers;
        new_headers["ZM-API-KEY"] = this->config_.apiKey;
        new_headers["ZM-API-TIMESTAMP"] = timestamp;
        new_headers["ZM-API-SIGNATURE"] = signature;

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

nlohmann::json Zipmex::parse_ticker(const nlohmann::json& ticker, const nlohmann::json& market) {
    auto timestamp = this->safe_timestamp(ticker, "timestamp");
    auto symbol = market.empty() ? "" : market["symbol"].get<std::string>();
    
    return {
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"high", this->safe_number(ticker, "high_24h")},
        {"low", this->safe_number(ticker, "low_24h")},
        {"bid", this->safe_number(ticker, "best_bid")},
        {"ask", this->safe_number(ticker, "best_ask")},
        {"last", this->safe_number(ticker, "last_price")},
        {"close", this->safe_number(ticker, "last_price")},
        {"baseVolume", this->safe_number(ticker, "volume_24h")},
        {"quoteVolume", this->safe_number(ticker, "quote_volume_24h")},
        {"info", ticker}
    };
}

nlohmann::json Zipmex::parse_balance(const nlohmann::json& response) {
    auto balances = response["data"];
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
        account["used"] = this->safe_string(balance, "reserved");
        result[code] = account;
    }

    return result;
}

nlohmann::json Zipmex::parse_position(const nlohmann::json& position, const nlohmann::json& market) {
    auto symbol = market.empty() ? position["symbol"].get<std::string>() : market["symbol"].get<std::string>();
    auto timestamp = this->safe_timestamp(position, "timestamp");
    
    return {
        {"info", position},
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"contracts", this->safe_number(position, "contracts")},
        {"contractSize", this->safe_number(position, "contract_size")},
        {"side", position["side"].get<std::string>()},
        {"notional", this->safe_number(position, "notional")},
        {"leverage", this->safe_number(position, "leverage")},
        {"collateral", this->safe_number(position, "collateral")},
        {"entryPrice", this->safe_number(position, "entry_price")},
        {"markPrice", this->safe_number(position, "mark_price")},
        {"liquidationPrice", this->safe_number(position, "liquidation_price")},
        {"unrealizedPnl", this->safe_number(position, "unrealized_pnl")},
        {"percentage", this->safe_number(position, "roi")}
    };
}

std::string Zipmex::get_market_id(const std::string& symbol) {
    auto market = this->market(symbol);
    return market["id"].get<std::string>();
}

std::string Zipmex::get_currency_id(const std::string& code) {
    if (this->currencies.find(code) != this->currencies.end()) {
        return this->currencies[code]["id"].get<std::string>();
    }
    return code;
}

std::string Zipmex::get_order_id() {
    return std::to_string(this->milliseconds());
}

std::string Zipmex::get_signature(const std::string& path, const std::string& method,
                                 const nlohmann::json& params, const std::string& timestamp) {
    auto auth = method + timestamp + "/api/v1/private/" + path;
    
    if (!params.empty()) {
        auth += this->urlencode(this->keysort(params));
    }

    return this->hmac(auth, this->config_.secret, "sha256", "hex");
}

} // namespace ccxt
