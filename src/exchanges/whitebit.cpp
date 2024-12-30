#include "ccxt/exchanges/whitebit.h"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

WhiteBIT::WhiteBIT() {
    this->id = "whitebit";
    this->name = "WhiteBIT";
    this->countries = {"EE"};  // Estonia
    this->version = "2";
    this->rateLimit = 500;
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
        {"logo", "https://user-images.githubusercontent.com/1294454/66732963-8eb7dd00-ee66-11e9-849b-10d9282bb9e0.jpg"},
        {"api", {
            {"public", "https://whitebit.com/api/v2"},
            {"private", "https://whitebit.com/api/v2"},
            {"v4", "https://whitebit.com/api/v4"}
        }},
        {"www", "https://www.whitebit.com"},
        {"doc", {
            "https://github.com/whitebit-exchange/api-docs",
            "https://documenter.getpostman.com/view/7473075/Szzj8dgv?version=latest"
        }},
        {"fees", "https://whitebit.com/fee-schedule"}
    };

    this->api = {
        {"public", {
            {"GET", {
                "markets",
                "ticker",
                "assets",
                "fee",
                "depth/{market}",
                "trades/{market}",
                "kline/{market}"
            }}
        }},
        {"private", {
            {"POST", {
                "account/balance",
                "order/new",
                "order/cancel",
                "orders",
                "account/order_history",
                "account/executed_history",
                "account/deposit_address",
                "main_account/address",
                "main_account/history",
                "main_account/withdraw"
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

nlohmann::json WhiteBIT::fetch_markets() {
    auto response = this->fetch("markets", "public");
    auto result = nlohmann::json::array();

    for (const auto& market : response) {
        auto id = market["name"].get<std::string>();
        auto parts = this->split(id, "_");
        auto baseId = parts[0];
        auto quoteId = parts[1];
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
                {"amount", market["precision"].get<int>()},
                {"price", market["price_precision"].get<int>()}
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

nlohmann::json WhiteBIT::create_order(const std::string& symbol, const std::string& type,
                                    const std::string& side, double amount, double price) {
    this->check_required_credentials();
    auto market = this->market(symbol);
    
    auto request = {
        {"market", market["id"].get<std::string>()},
        {"side", side},
        {"amount", this->amount_to_precision(symbol, amount)},
        {"type", type},
        {"client_order_id", this->get_client_order_id()}
    };

    if (type == "limit") {
        request["price"] = this->price_to_precision(symbol, price);
    }

    auto response = this->fetch("order/new", "private", "POST", request);
    return this->parse_order(response);
}

nlohmann::json WhiteBIT::cancel_order(const std::string& id, const std::string& symbol) {
    this->check_required_credentials();
    auto request = {
        {"orderId", std::stoi(id)}
    };
    if (!symbol.empty()) {
        auto market = this->market(symbol);
        request["market"] = market["id"].get<std::string>();
    }
    return this->fetch("order/cancel", "private", "POST", request);
}

nlohmann::json WhiteBIT::fetch_balance() {
    this->check_required_credentials();
    auto response = this->fetch("account/balance", "private", "POST");
    return this->parse_balance(response);
}

std::string WhiteBIT::sign(const std::string& path, const std::string& api,
                          const std::string& method, const nlohmann::json& params,
                          const std::map<std::string, std::string>& headers) {
    auto url = this->urls["api"][api].get<std::string>() + "/" + this->implode_params(path, params);
    auto query = this->omit(params, this->extract_params(path));

    if (api == "private") {
        this->check_required_credentials();
        auto nonce = this->get_nonce_string();
        auto body = "";
        
        if (!query.empty()) {
            body = this->json(query);
        }

        auto auth = nonce + method + "/api/v2/" + path + body;
        auto signature = this->hmac(auth, this->config_.secret, "sha512", "hex");
        
        auto new_headers = headers;
        new_headers["Content-Type"] = "application/json";
        new_headers["X-TXC-APIKEY"] = this->config_.apiKey;
        new_headers["X-TXC-PAYLOAD"] = nonce;
        new_headers["X-TXC-SIGNATURE"] = signature;
        
        if (method == "POST") {
            new_headers["Content-Length"] = std::to_string(body.length());
        }
    } else {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    }

    return url;
}

nlohmann::json WhiteBIT::parse_ticker(const nlohmann::json& ticker, const nlohmann::json& market) {
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
        {"last", this->safe_number(ticker, "last")},
        {"close", this->safe_number(ticker, "last")},
        {"baseVolume", this->safe_number(ticker, "volume")},
        {"quoteVolume", this->safe_number(ticker, "volume_quote")},
        {"info", ticker}
    };
}

nlohmann::json WhiteBIT::parse_balance(const nlohmann::json& response) {
    auto result = {
        {"info", response},
        {"timestamp", nullptr},
        {"datetime", nullptr}
    };

    for (const auto& balance : response["balances"].items()) {
        auto currencyId = balance.key();
        auto code = this->safe_currency_code(currencyId);
        auto account = this->account();
        account["free"] = this->safe_string(balance.value(), "available");
        account["used"] = this->safe_string(balance.value(), "freeze");
        result[code] = account;
    }

    return result;
}

std::string WhiteBIT::get_currency_id(const std::string& code) {
    if (this->currencies.find(code) != this->currencies.end()) {
        return this->currencies[code]["id"].get<std::string>();
    }
    return code;
}

std::string WhiteBIT::get_client_order_id() {
    return std::to_string(this->milliseconds());
}

std::string WhiteBIT::get_nonce_string() {
    return std::to_string(this->nonce());
}

std::string WhiteBIT::get_request_body_signature(const std::string& body) {
    return this->hmac(body, this->config_.secret, "sha512", "hex");
}

} // namespace ccxt
