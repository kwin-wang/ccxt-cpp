#include "ccxt/exchanges/hitbtc.h"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>
#include <future>

namespace ccxt {

HitBTC::HitBTC() {
    this->id = "hitbtc";
    this->name = "HitBTC";
    this->countries = {"HK"};  // Hong Kong
    this->rateLimit = 3.333;  // 300 requests per second for trading
    this->has = {
        {"CORS", false},
        {"spot", true},
        {"margin", true},
        {"swap", true},
        {"future", false},
        {"option", false},
        {"addMargin", true},
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

    this->options = {
        {"version", "3"} // 1, 2, or 3
    };

    this->urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/27766555-8eaec20e-5edc-11e7-9c5b-6dc69fc42f5e.jpg"},
        {"api", {
            {"v1", "https://api.hitbtc.com"},
            {"v2", "https://api.hitbtc.com/api/2"},
            {"v3", "https://api.hitbtc.com/api/3"}
        }},
        {"www", "https://hitbtc.com"},
        {"doc", {
            "https://api.hitbtc.com",
            "https://api.hitbtc.com/v2",
            "https://api.hitbtc.com/v3"
        }}
    };

    this->api = {
        {"public", {
            {"GET", {
                "currency",
                "symbol",
                "ticker",
                "trades/{symbol}",
                "orderbook/{symbol}",
                "candles/{symbol}",
                "indexes/history/{symbol}",
                "derivatives/{symbol}/info",
                "futures/{symbol}/mark-price/history",
                "futures/{symbol}/premium-index/history",
                "futures/{symbol}/open-interest/history"
            }}
        }},
        {"private", {
            {"GET", {
                "spot/balance",
                "spot/order",
                "spot/order/{client_order_id}",
                "spot/fee/{symbol}",
                "margin/account",
                "margin/account/isolated/{symbol}",
                "margin/order",
                "margin/order/{client_order_id}",
                "margin/position",
                "margin/position/{symbol}",
                "wallet/balance",
                "wallet/crypto/address",
                "wallet/crypto/address/{currency}",
                "wallet/crypto/networks/{currency}",
                "wallet/transactions",
                "wallet/crypto/check-mine/{txid}",
                "wallet/crypto/check-mine/{txid}/{address}"
            }},
            {"POST", {
                "spot/order",
                "margin/order",
                "margin/position/close",
                "margin/position/close/all",
                "margin/position/reduce",
                "wallet/crypto/withdraw",
                "wallet/crypto/transfer",
                "wallet/crypto/address/new"
            }},
            {"DELETE", {
                "spot/order",
                "spot/order/{client_order_id}",
                "margin/order",
                "margin/order/{client_order_id}",
                "margin/position/reduce/{symbol}"
            }}
        }}
    };
}

std::string HitBTC::getApiVersion() {
    return this->safeString(this->options, "version", "3");
}

std::string HitBTC::getEndpoint(const std::string& path) {
    auto version = this->getApiVersion();
    auto urls = this->urls["api"];
    
    if (version == "1") {
        return urls["v1"] + path;
    } else if (version == "2") {
        return urls["v2"] + path;
    } else {
        return urls["v3"] + path;
    }
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
    auto url = this->getEndpoint(path);
    auto query = this->omit(params, this->extract_params(path));

    if (api == "private") {
        this->check_required_credentials();
        auto auth = this->config_.apiKey + ":" + this->config_.secret;
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

// Async Market Data Methods
std::future<nlohmann::json> HitBTC::fetch_markets_async() {
    return std::async(std::launch::async, [this]() {
        return this->fetch_markets();
    });
}

std::future<nlohmann::json> HitBTC::fetch_ticker_async(const std::string& symbol) {
    return std::async(std::launch::async, [this, symbol]() {
        return this->fetch_ticker(symbol);
    });
}

std::future<nlohmann::json> HitBTC::fetch_order_book_async(const std::string& symbol, int limit) {
    return std::async(std::launch::async, [this, symbol, limit]() {
        return this->fetch_order_book(symbol, limit);
    });
}

std::future<nlohmann::json> HitBTC::fetch_trades_async(const std::string& symbol, int since, int limit) {
    return std::async(std::launch::async, [this, symbol, since, limit]() {
        return this->fetch_trades(symbol, since, limit);
    });
}

std::future<nlohmann::json> HitBTC::fetch_ohlcv_async(const std::string& symbol, const std::string& timeframe,
                                                     int since, int limit) {
    return std::async(std::launch::async, [this, symbol, timeframe, since, limit]() {
        return this->fetch_ohlcv(symbol, timeframe, since, limit);
    });
}

// Async Trading Methods
std::future<nlohmann::json> HitBTC::create_order_async(const std::string& symbol, const std::string& type,
                                                      const std::string& side, double amount, double price) {
    return std::async(std::launch::async, [this, symbol, type, side, amount, price]() {
        return this->create_order(symbol, type, side, amount, price);
    });
}

std::future<nlohmann::json> HitBTC::cancel_order_async(const std::string& id, const std::string& symbol) {
    return std::async(std::launch::async, [this, id, symbol]() {
        return this->cancel_order(id, symbol);
    });
}

std::future<nlohmann::json> HitBTC::fetch_order_async(const std::string& id, const std::string& symbol) {
    return std::async(std::launch::async, [this, id, symbol]() {
        return this->fetch_order(id, symbol);
    });
}

std::future<nlohmann::json> HitBTC::fetch_orders_async(const std::string& symbol, int since, int limit) {
    return std::async(std::launch::async, [this, symbol, since, limit]() {
        return this->fetch_orders(symbol, since, limit);
    });
}

std::future<nlohmann::json> HitBTC::fetch_open_orders_async(const std::string& symbol, int since, int limit) {
    return std::async(std::launch::async, [this, symbol, since, limit]() {
        return this->fetch_open_orders(symbol, since, limit);
    });
}

std::future<nlohmann::json> HitBTC::fetch_closed_orders_async(const std::string& symbol, int since, int limit) {
    return std::async(std::launch::async, [this, symbol, since, limit]() {
        return this->fetch_closed_orders(symbol, since, limit);
    });
}

std::future<nlohmann::json> HitBTC::fetch_my_trades_async(const std::string& symbol, int since, int limit) {
    return std::async(std::launch::async, [this, symbol, since, limit]() {
        return this->fetch_my_trades(symbol, since, limit);
    });
}

// Async Account Methods
std::future<nlohmann::json> HitBTC::fetch_balance_async() {
    return std::async(std::launch::async, [this]() {
        return this->fetch_balance();
    });
}

std::future<nlohmann::json> HitBTC::fetch_deposit_address_async(const std::string& code) {
    return std::async(std::launch::async, [this, code]() {
        return this->fetch_deposit_address(code);
    });
}

std::future<nlohmann::json> HitBTC::fetch_deposits_async(const std::string& code, int since, int limit) {
    return std::async(std::launch::async, [this, code, since, limit]() {
        return this->fetch_deposits(code, since, limit);
    });
}

std::future<nlohmann::json> HitBTC::fetch_withdrawals_async(const std::string& code, int since, int limit) {
    return std::async(std::launch::async, [this, code, since, limit]() {
        return this->fetch_withdrawals(code, since, limit);
    });
}

std::future<nlohmann::json> HitBTC::withdraw_async(const std::string& code, double amount, const std::string& address,
                                                 const std::string& tag, const nlohmann::json& params) {
    return std::async(std::launch::async, [this, code, amount, address, tag, params]() {
        return this->withdraw(code, amount, address, tag, params);
    });
}

// Async Margin Trading Methods
std::future<nlohmann::json> HitBTC::fetch_margin_balance_async() {
    return std::async(std::launch::async, [this]() {
        return this->fetch_margin_balance();
    });
}

std::future<nlohmann::json> HitBTC::create_margin_order_async(const std::string& symbol, const std::string& type,
                                                             const std::string& side, double amount, double price) {
    return std::async(std::launch::async, [this, symbol, type, side, amount, price]() {
        return this->create_margin_order(symbol, type, side, amount, price);
    });
}

std::future<nlohmann::json> HitBTC::fetch_margin_orders_async(const std::string& symbol, int since, int limit) {
    return std::async(std::launch::async, [this, symbol, since, limit]() {
        return this->fetch_margin_orders(symbol, since, limit);
    });
}

std::future<nlohmann::json> HitBTC::fetch_margin_trades_async(const std::string& symbol, int since, int limit) {
    return std::async(std::launch::async, [this, symbol, since, limit]() {
        return this->fetch_margin_trades(symbol, since, limit);
    });
}

} // namespace ccxt
