#include "ccxt/exchanges/hitbtc.h"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

HitBTC::HitBTC() {
    this->id = "hitbtc";
    this->name = "HitBTC";
    this->countries = {"HK"};  // Hong Kong
    this->version = "2";
    this->rateLimit = 1500;
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
        {"1m", "M1"},
        {"3m", "M3"},
        {"5m", "M5"},
        {"15m", "M15"},
        {"30m", "M30"},
        {"1h", "H1"},
        {"4h", "H4"},
        {"1d", "D1"},
        {"1w", "D7"},
        {"1M", "1M"}
    };

    this->urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/27766555-8eaec20e-5edc-11e7-9c5b-6dc69fc42f5e.jpg"},
        {"api", {
            {"public", "https://api.hitbtc.com/api/2"},
            {"private", "https://api.hitbtc.com/api/2"}
        }},
        {"www", "https://hitbtc.com"},
        {"doc", {
            "https://api.hitbtc.com",
            "https://github.com/hitbtc-com/hitbtc-api"
        }},
        {"fees", "https://hitbtc.com/fees-and-limits"}
    };

    this->api = {
        {"public", {
            {"GET", {
                "symbol",
                "symbol/{symbol}",
                "currency",
                "currency/{currency}",
                "ticker",
                "ticker/{symbol}",
                "trades/{symbol}",
                "orderbook/{symbol}",
                "candles/{symbol}"
            }}
        }},
        {"private", {
            {"GET", {
                "order",
                "order/{clientOrderId}",
                "trading/balance",
                "trading/fee/{symbol}",
                "history/trades",
                "history/order",
                "history/order/{orderId}/trades",
                "account/balance",
                "account/transactions",
                "account/transactions/{id}",
                "account/crypto/address/{currency}"
            }},
            {"POST", {
                "order",
                "account/crypto/withdraw",
                "account/crypto/address/{currency}"
            }},
            {"PUT", {
                "order/{clientOrderId}"
            }},
            {"DELETE", {
                "order",
                "order/{clientOrderId}"
            }}
        }}
    };

    this->fees = {
        {"trading", {
            {"tierBased", true},
            {"percentage", true},
            {"maker", 0.1 / 100},
            {"taker", 0.2 / 100},
            {"tiers", {
                {"maker", {
                    {0, 0.1 / 100},
                    {10, 0.08 / 100},
                    {100, 0.06 / 100},
                    {500, 0.04 / 100},
                    {1000, 0.02 / 100},
                    {5000, 0}
                }},
                {"taker", {
                    {0, 0.2 / 100},
                    {10, 0.18 / 100},
                    {100, 0.16 / 100},
                    {500, 0.14 / 100},
                    {1000, 0.12 / 100},
                    {5000, 0.1 / 100}
                }}
            }}
        }},
        {"funding", {
            {"withdraw", {}},
            {"deposit", {}}
        }}
    };
}

nlohmann::json HitBTC::fetch_markets() {
    auto response = this->fetch("symbol", "public");
    auto result = nlohmann::json::array();

    for (const auto& market : response) {
        auto id = market["id"].get<std::string>();
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
            {"active", market["trading"].get<bool>()},
            {"precision", {
                {"amount", market["quantityIncrement"].get<int>()},
                {"price", market["tickSize"].get<int>()}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safe_number(market, "quantityIncrement")},
                    {"max", nullptr}
                }},
                {"price", {
                    {"min", this->safe_number(market, "tickSize")},
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

nlohmann::json HitBTC::create_order(const std::string& symbol, const std::string& type,
                                  const std::string& side, double amount, double price) {
    this->check_required_credentials();
    auto market = this->market(symbol);
    
    auto request = {
        {"symbol", market["id"].get<std::string>()},
        {"side", side.upper()},
        {"quantity", this->amount_to_precision(symbol, amount)},
        {"type", type.upper()},
        {"clientOrderId", this->get_client_order_id()}
    };

    if (type == "limit") {
        request["price"] = this->price_to_precision(symbol, price);
    }

    auto response = this->fetch("order", "private", "POST", request);
    return this->parse_order(response);
}

nlohmann::json HitBTC::cancel_order(const std::string& id, const std::string& symbol) {
    this->check_required_credentials();
    return this->fetch("order/" + id, "private", "DELETE");
}

nlohmann::json HitBTC::fetch_balance() {
    this->check_required_credentials();
    auto response = this->fetch("trading/balance", "private");
    return this->parse_balance(response);
}

std::string HitBTC::sign(const std::string& path, const std::string& api,
                        const std::string& method, const nlohmann::json& params,
                        const std::map<std::string, std::string>& headers) {
    auto url = this->urls["api"][api].get<std::string>() + "/" + this->implode_params(path, params);
    auto query = this->omit(params, this->extract_params(path));

    if (api == "private") {
        this->check_required_credentials();
        auto auth = this->apiKey + ":" + this->secret;
        auto encoded = this->string_to_base64(auth);
        
        auto new_headers = headers;
        new_headers["Authorization"] = "Basic " + encoded;
        
        if (!query.empty()) {
            if (method == "GET" || method == "DELETE") {
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

nlohmann::json HitBTC::parse_ticker(const nlohmann::json& ticker, const nlohmann::json& market) {
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
        {"quoteVolume", this->safe_number(ticker, "volumeQuote")},
        {"info", ticker}
    };
}

nlohmann::json HitBTC::parse_balance(const nlohmann::json& response) {
    auto result = {
        {"info", response},
        {"timestamp", nullptr},
        {"datetime", nullptr}
    };

    for (const auto& balance : response) {
        auto currencyId = balance["currency"].get<std::string>();
        auto code = this->safe_currency_code(currencyId);
        auto account = this->account();
        account["free"] = this->safe_string(balance, "available");
        account["used"] = this->safe_string(balance, "reserved");
        result[code] = account;
    }

    return result;
}

std::string HitBTC::get_currency_id(const std::string& code) {
    if (this->currencies.find(code) != this->currencies.end()) {
        return this->currencies[code]["id"].get<std::string>();
    }
    return code;
}

std::string HitBTC::get_client_order_id() {
    return std::to_string(this->milliseconds());
}

bool HitBTC::is_margin_trading_enabled(const std::string& symbol) {
    auto market = this->market(symbol);
    return market["info"]["marginTrading"].get<bool>();
}

} // namespace ccxt
