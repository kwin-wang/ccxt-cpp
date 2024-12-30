#include "ccxt/exchanges/fmfwio.h"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Fmfwio::Fmfwio() {
    this->id = "fmfwio";
    this->name = "FMFW.io";
    this->countries = {"KN"}; // Saint Kitts and Nevis
    this->version = "v2";
    this->rateLimit = 500;
    this->has = {
        {"cancelOrder", true},
        {"CORS", true},
        {"createOrder", true},
        {"fetchBalance", true},
        {"fetchClosedOrders", true},
        {"fetchCurrencies", true},
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
        {"logo", "https://user-images.githubusercontent.com/1294454/97296144-514fa300-1861-11eb-952b-3d55d492200b.jpg"},
        {"api", {
            {"public", "https://api.fmfw.io/api/2"},
            {"private", "https://api.fmfw.io/api/2"}
        }},
        {"www", "https://fmfw.io"},
        {"doc", {
            "https://api.fmfw.io/api/2/explore/",
            "https://github.com/fmfwio/api-docs"
        }},
        {"fees", "https://fmfw.io/fees-and-limits"}
    };

    this->api = {
        {"public", {
            {"GET", {
                "public/symbol",
                "public/ticker",
                "public/ticker/{symbol}",
                "public/orderbook/{symbol}",
                "public/trades/{symbol}",
                "public/candles/{symbol}"
            }}
        }},
        {"private", {
            {"GET", {
                "trading/balance",
                "trading/order/{clientOrderId}",
                "trading/order",
                "trading/trade",
                "payment/address/{currency}",
                "payment/transactions",
                "payment/transaction/{id}"
            }},
            {"POST", {
                "trading/order",
                "payment/address/crypto/{currency}",
                "payment/payout"
            }},
            {"DELETE", {
                "trading/order/{clientOrderId}",
                "trading/order"
            }}
        }}
    };

    this->fees = {
        {"trading", {
            {"tierBased", true},
            {"percentage", true},
            {"taker", 0.001},
            {"maker", 0.001},
            {"tiers", {
                {"taker", {
                    {0, 0.001},
                    {10, 0.0009},
                    {100, 0.0008},
                    {500, 0.0007},
                    {1000, 0.0006},
                    {5000, 0.0005},
                    {10000, 0.0004},
                    {20000, 0.0003},
                    {50000, 0.0002},
                    {100000, 0.0001}
                }},
                {"maker", {
                    {0, 0.001},
                    {10, 0.0009},
                    {100, 0.0008},
                    {500, 0.0007},
                    {1000, 0.0006},
                    {5000, 0.0005},
                    {10000, 0.0004},
                    {20000, 0.0003},
                    {50000, 0.0002},
                    {100000, 0.0001}
                }}
            }}
        }}
    };
}

nlohmann::json Fmfwio::fetch_markets() {
    auto response = this->fetch("public/symbol", "public");
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
            {"active", market["active"].get<bool>()},
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
                    {"min", this->safe_number(market, "minNotional")},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    return result;
}

nlohmann::json Fmfwio::create_order(const std::string& symbol, const std::string& type,
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

    auto response = this->fetch("trading/order", "private", "POST", request);
    return this->parse_order(response);
}

nlohmann::json Fmfwio::fetch_balance() {
    this->check_required_credentials();
    auto response = this->fetch("trading/balance", "private");
    return this->parse_balance(response);
}

std::string Fmfwio::sign(const std::string& path, const std::string& api,
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
        auto auth = timestamp + method + "/api/2/" + path;
        
        if (!query.empty()) {
            auth += this->urlencode(this->keysort(query));
        }

        auto signature = this->hmac(auth, this->config_.secret, "sha256", "hex");
        auto new_headers = headers;
        new_headers["FW-API-KEY"] = this->config_.apiKey;
        new_headers["FW-API-TIMESTAMP"] = timestamp;
        new_headers["FW-API-SIGNATURE"] = signature;

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

nlohmann::json Fmfwio::parse_ticker(const nlohmann::json& ticker, const nlohmann::json& market) {
    auto timestamp = this->safe_timestamp(ticker, "timestamp");
    auto symbol = market.empty() ? "" : market["symbol"].get<std::string>();
    
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

nlohmann::json Fmfwio::parse_trade(const nlohmann::json& trade, const nlohmann::json& market) {
    auto timestamp = this->safe_timestamp(trade, "timestamp");
    auto side = this->safe_string(trade, "side");
    auto price = this->safe_number(trade, "price");
    auto amount = this->safe_number(trade, "quantity");
    auto cost = price * amount;
    
    return {
        {"info", trade},
        {"id", this->safe_string(trade, "id")},
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

nlohmann::json Fmfwio::parse_balance(const nlohmann::json& response) {
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

std::string Fmfwio::get_market_id(const std::string& symbol) {
    auto market = this->market(symbol);
    return market["id"].get<std::string>();
}

std::string Fmfwio::get_currency_id(const std::string& code) {
    if (this->currencies.find(code) != this->currencies.end()) {
        return this->currencies[code]["id"].get<std::string>();
    }
    return code;
}

std::string Fmfwio::get_order_id() {
    return std::to_string(this->milliseconds());
}

std::string Fmfwio::get_signature(const std::string& path, const std::string& method,
                                 const nlohmann::json& params, const std::string& timestamp) {
    auto auth = timestamp + method + "/api/2/" + path;
    
    if (!params.empty()) {
        auth += this->urlencode(this->keysort(params));
    }

    return this->hmac(auth, this->config_.secret, "sha256", "hex");
}

nlohmann::json Fmfwio::parse_order(const nlohmann::json& order, const nlohmann::json& market) {
    auto timestamp = this->safe_timestamp(order, "created_at");
    auto updated = this->safe_timestamp(order, "updated_at");
    auto marketId = this->safe_string(order, "symbol");
    auto symbol = this->safe_symbol(marketId, market);
    auto amount = this->safe_number(order, "quantity");
    auto filled = this->safe_number(order, "cumulative_quantity");
    auto status = this->parse_order_status(this->safe_string(order, "status"));
    auto side = this->safe_string(order, "side");
    auto type = this->safe_string(order, "type");
    auto price = this->safe_number(order, "price");
    
    return {
        {"id", this->safe_string(order, "client_order_id")},
        {"clientOrderId", this->safe_string(order, "client_order_id")},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", updated},
        {"status", status},
        {"symbol", symbol},
        {"type", type},
        {"timeInForce", this->safe_string(order, "time_in_force")},
        {"postOnly", this->safe_value(order, "post_only")},
        {"side", side},
        {"price", price},
        {"stopPrice", this->safe_number(order, "stop_price")},
        {"amount", amount},
        {"filled", filled},
        {"remaining", amount - filled},
        {"cost", filled * price},
        {"trades", nullptr},
        {"fee", nullptr},
        {"info", order}
    };
}

std::string Fmfwio::parse_order_status(const std::string& status) {
    const std::map<std::string, std::string> statuses = {
        {"new", "open"},
        {"suspended", "open"},
        {"partiallyFilled", "open"},
        {"filled", "closed"},
        {"canceled", "canceled"},
        {"expired", "expired"}
    };
    return this->safe_string(statuses, status, status);
}

nlohmann::json Fmfwio::parse_trade(const nlohmann::json& trade, const nlohmann::json& market) {
    auto timestamp = this->safe_timestamp(trade, "timestamp");
    auto id = this->safe_string(trade, "id");
    auto orderId = this->safe_string(trade, "order_id");
    auto marketId = this->safe_string(trade, "symbol");
    auto symbol = this->safe_symbol(marketId, market);
    auto side = this->safe_string(trade, "side");
    auto price = this->safe_number(trade, "price");
    auto amount = this->safe_number(trade, "quantity");
    auto cost = price * amount;
    
    auto fee = nullptr;
    if (trade.contains("fee")) {
        auto feeCost = this->safe_number(trade["fee"], "cost");
        auto feeCurrency = this->safe_string(trade["fee"], "currency");
        fee = {
            {"cost", feeCost},
            {"currency", this->safe_currency_code(feeCurrency)}
        };
    }
    
    return {
        {"info", trade},
        {"id", id},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"symbol", symbol},
        {"order", orderId},
        {"type", "limit"},
        {"side", side},
        {"takerOrMaker", this->safe_string(trade, "liquidity")},
        {"price", price},
        {"amount", amount},
        {"cost", cost},
        {"fee", fee}
    };
}

nlohmann::json Fmfwio::fetch_my_trades(const std::string& symbol, int since, int limit, const nlohmann::json& params) {
    this->load_markets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]}
    };
    
    if (since != 0) {
        request["from"] = since;
    }
    if (limit != 0) {
        request["limit"] = limit;
    }
    
    auto response = this->private_get_spot_order_trade_list(this->extend(request, params));
    return this->parse_trades(response["result"], market, since, limit);
}

nlohmann::json Fmfwio::fetch_open_orders(const std::string& symbol, int since, int limit, const nlohmann::json& params) {
    this->load_markets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]}
    };
    
    if (limit != 0) {
        request["limit"] = limit;
    }
    
    auto response = this->private_get_spot_order_list_open(this->extend(request, params));
    return this->parse_orders(response["result"], market, since, limit);
}

nlohmann::json Fmfwio::fetch_closed_orders(const std::string& symbol, int since, int limit, const nlohmann::json& params) {
    this->load_markets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]}
    };
    
    if (since != 0) {
        request["from"] = since;
    }
    if (limit != 0) {
        request["limit"] = limit;
    }
    
    auto response = this->private_get_spot_order_list_closed(this->extend(request, params));
    return this->parse_orders(response["result"], market, since, limit);
}

nlohmann::json Fmfwio::fetch_order(const std::string& id, const std::string& symbol, const nlohmann::json& params) {
    this->load_markets();
    auto market = this->market(symbol);
    auto request = {
        {"client_order_id", id}
    };
    
    auto response = this->private_get_spot_order_client_order_id(this->extend(request, params));
    return this->parse_order(response["result"], market);
}

} // namespace ccxt
