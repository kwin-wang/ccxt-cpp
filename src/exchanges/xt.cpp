#include "ccxt/exchanges/xt.h"
#include "ccxt/error.h"
#include <algorithm>

namespace ccxt {

Xt::Xt() {
    // Basic exchange properties
    id = "xt";
    name = "XT.COM";
    countries = {"Seychelles", "United Kingdom", "Singapore"};
    version = "v1";
    rateLimit = 50;
    certified = false;
    pro = false;
    has = {
        {"CORS", false},
        {"spot", true},
        {"margin", true},
        {"swap", true},
        {"future", false},
        {"option", false},
        {"cancelAllOrders", true},
        {"cancelOrder", true},
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
        {"fetchWithdrawals", true}
    };

    // API endpoint versions
    v1 = "v1";
    v2 = "v2";

    initializeApiEndpoints();
}

void Xt::initializeApiEndpoints() {
    // URLs
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/129991357-8f47464b-d0f4-41d6-8a82-34122f0d1398.jpg"},
        {"api", {
            {"public", "https://api.xt.com/data/api"},
            {"private", "https://api.xt.com/trade/api"}
        }},
        {"www", "https://www.xt.com"},
        {"doc", {
            "https://doc.xt.com",
            "https://doc.xt.com/en/home/index.html"
        }},
        {"fees", "https://www.xt.com/fee"}
    };

    // API endpoints
    api = {
        {"public", {
            {"get", {
                {v1 + "/getMarketConfig"},
                {v1 + "/getAllCurrencyDetail"},
                {v1 + "/getDepth"},
                {v1 + "/getTicker"},
                {v1 + "/getMarketTicker"},
                {v1 + "/getTrades"},
                {v1 + "/getKLine"}
            }}
        }},
        {"private", {
            {"get", {
                {v1 + "/balance"},
                {v1 + "/order/openOrders"},
                {v1 + "/order/completedOrders"},
                {v1 + "/order/orderInfo"},
                {v1 + "/order/myTrades"},
                {v1 + "/deposit/list"},
                {v1 + "/withdraw/list"}
            }},
            {"post", {
                {v1 + "/order/create"},
                {v1 + "/order/cancel"},
                {v1 + "/order/cancelAll"}
            }}
        }}
    };

    // Timeframes
    timeframes = {
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
}

// Market Data Methods - Sync
json Xt::fetchMarkets(const json& params) {
    auto response = this->publicGetV1GetMarketConfig(params);
    auto data = this->safeValue(response, "result", json::array());
    return this->parseMarkets(data);
}

json Xt::fetchCurrencies(const json& params) {
    auto response = this->publicGetV1GetAllCurrencyDetail(params);
    auto data = this->safeValue(response, "result", json::array());
    return this->parseCurrencies(data);
}

json Xt::fetchTicker(const String& symbol, const json& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        {"market", market["id"]}
    };
    auto response = this->publicGetV1GetTicker(this->extend(request, params));
    auto data = this->safeValue(response, "result");
    return this->parseTicker(data, market);
}

json Xt::fetchOrderBook(const String& symbol, int limit, const json& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        {"market", market["id"]}
    };
    if (limit != 0) {
        request["limit"] = limit;
    }
    auto response = this->publicGetV1GetDepth(this->extend(request, params));
    auto data = this->safeValue(response, "result");
    return this->parseOrderBook(data, market["symbol"]);
}

// Trading Methods
json Xt::createOrder(const String& symbol, const String& type, const String& side, 
                    const double& amount, const double& price, const json& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]},
        {"type", type},
        {"side", side},
        {"amount", this->amountToPrecision(symbol, amount)},
        {"price", this->priceToPrecision(symbol, price)}
    };
    
    auto response = this->privatePostOrder(this->extend(request, params));
    auto order = this->parseOrder(response, market);
    return this->extend(order, {
        {"type", type},
        {"side", side},
        {"price", price},
        {"amount", amount}
    });
}

json Xt::cancelOrder(const String& id, const String& symbol, const json& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]},
        {"order_id", id}
    };
    return this->privateDeleteOrder(this->extend(request, params));
}

json Xt::fetchOrder(const String& id, const String& symbol, const json& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]},
        {"order_id", id}
    };
    auto response = this->privateGetOrder(this->extend(request, params));
    return this->parseOrder(response, market);
}

json Xt::fetchOrders(const String& symbol, const long* since,
                    const long* limit, const json& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]}
    };
    if (since != nullptr) {
        request["start_time"] = since;
    }
    if (limit != nullptr) {
        request["limit"] = limit;
    }
    auto response = this->privateGetOrders(this->extend(request, params));
    return this->parseOrders(response, market, since, limit);
}

json Xt::fetchOpenOrders(const String& symbol, const long* since,
                        const long* limit, const json& params) {
    auto request = {{"status", "NEW"}};
    return this->fetchOrders(symbol, since, limit, this->extend(request, params));
}

json Xt::fetchClosedOrders(const String& symbol, const long* since,
                          const long* limit, const json& params) {
    auto request = {{"status", "FILLED"}};
    return this->fetchOrders(symbol, since, limit, this->extend(request, params));
}

// Account Methods
json Xt::fetchBalance(const json& params) {
    this->loadMarkets();
    auto response = this->privateGetBalance(params);
    return this->parseBalance(response);
}

json Xt::fetchDeposits(const Nullable<String>& code, const long* since,
                       const long* limit, const json& params) {
    this->loadMarkets();
    auto request = {};
    auto currency = nullptr;
    if (code != nullptr) {
        currency = this->currency(code);
        request["currency"] = currency["id"];
    }
    if (since != nullptr) {
        request["start_time"] = since;
    }
    if (limit != nullptr) {
        request["limit"] = limit;
    }
    auto response = this->privateGetDeposits(this->extend(request, params));
    return this->parseTransactions(response, currency, since, limit, {{"type", "deposit"}});
}

json Xt::fetchWithdrawals(const Nullable<String>& code, const long* since,
                         const long* limit, const json& params) {
    this->loadMarkets();
    auto request = {};
    auto currency = nullptr;
    if (code != nullptr) {
        currency = this->currency(code);
        request["currency"] = currency["id"];
    }
    if (since != nullptr) {
        request["start_time"] = since;
    }
    if (limit != nullptr) {
        request["limit"] = limit;
    }
    auto response = this->privateGetWithdrawals(this->extend(request, params));
    return this->parseTransactions(response, currency, since, limit, {{"type", "withdrawal"}});
}

// Async Methods
AsyncPullType Xt::createOrderAsync(const String& symbol, const String& type, const String& side,
                                     const double& amount, const double& price, const json& params) {
    return std::async(std::launch::async, [this, symbol, type, side, amount, price, params]() {
        return this->createOrder(symbol, type, side, amount, price, params);
    });
}

AsyncPullType Xt::cancelOrderAsync(const String& id, const String& symbol, const json& params) {
    return std::async(std::launch::async, [this, id, symbol, params]() {
        return this->cancelOrder(id, symbol, params);
    });
}

AsyncPullType Xt::fetchOrderAsync(const String& id, const String& symbol, const json& params) {
    return std::async(std::launch::async, [this, id, symbol, params]() {
        return this->fetchOrder(id, symbol, params);
    });
}

AsyncPullType Xt::fetchOrdersAsync(const String& symbol, const long* since,
                                     const long* limit, const json& params) {
    return std::async(std::launch::async, [this, symbol, since, limit, params]() {
        return this->fetchOrders(symbol, since, limit, params);
    });
}

AsyncPullType Xt::fetchBalanceAsync(const json& params) {
    return std::async(std::launch::async, [this, params]() {
        return this->fetchBalance(params);
    });
}

// Helper Methods
json Xt::parseMarket(const json& market) {
    auto id = this->safeString(market, "market");
    auto baseId = this->safeString(market, "stock");
    auto quoteId = this->safeString(market, "money");
    auto base = this->safeCurrencyCode(baseId);
    auto quote = this->safeCurrencyCode(quoteId);
    auto symbol = base + "/" + quote;
    
    return {
        {"id", id},
        {"symbol", symbol},
        {"base", base},
        {"quote", quote},
        {"baseId", baseId},
        {"quoteId", quoteId},
        {"active", true},
        {"precision", {
            {"amount", this->safeInteger(market, "amount_scale")},
            {"price", this->safeInteger(market, "price_scale")}
        }},
        {"limits", {
            {"amount", {
                {"min", this->safeNumber(market, "min_amount")},
                {"max", nullptr}
            }},
            {"price", {
                {"min", this->safeNumber(market, "min_price")},
                {"max", nullptr}
            }},
            {"cost", {
                {"min", this->safeNumber(market, "min_money")},
                {"max", nullptr}
            }}
        }},
        {"info", market}
    };
}

String Xt::sign(const String& path, const String& api, const String& method,
                const json& params, const json& headers, const String& body) {
    auto url = this->urls["api"][api];
    url += "/" + path;

    if (api == "public") {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    } else {
        this->checkRequiredCredentials();
        auto timestamp = std::to_string(this->milliseconds());
        auto auth = timestamp + method + "/api/" + path;
        
        if (method == "POST") {
            body = this->json(params);
            auth += body;
        } else {
            if (!params.empty()) {
                auto query = this->urlencode(params);
                url += "?" + query;
                auth += "?" + query;
            }
        }

        auto signature = this->hmac(this->encode(auth), this->encode(this->config_.secret));
        auto newHeaders = {
            {"xt-validate-appkey", this->config_.apiKey},
            {"xt-validate-timestamp", timestamp},
            {"xt-validate-signature", signature},
            {"Content-Type", "application/json"}
        };
        headers = this->extend(headers, newHeaders);
    }

    return url;
}

// Parse Methods
json Xt::parseTicker(const json& ticker, const Market& market) {
    auto timestamp = this->safeInteger(ticker, "timestamp");
    auto symbol = market.symbol;
    auto last = this->safeString(ticker, "last");
    
    return {
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"high", this->safeString(ticker, "high")},
        {"low", this->safeString(ticker, "low")},
        {"bid", this->safeString(ticker, "buy")},
        {"bidVolume", nullptr},
        {"ask", this->safeString(ticker, "sell")},
        {"askVolume", nullptr},
        {"vwap", nullptr},
        {"open", this->safeString(ticker, "open")},
        {"close", last},
        {"last", last},
        {"previousClose", nullptr},
        {"change", nullptr},
        {"percentage", this->safeString(ticker, "change")},
        {"average", nullptr},
        {"baseVolume", this->safeString(ticker, "vol")},
        {"quoteVolume", this->safeString(ticker, "quoteVol")},
        {"info", ticker}
    };
}

json Xt::parseOrder(const json& order, const Market& market) {
    auto status = this->parseOrderStatus(this->safeString(order, "status"));
    auto symbol = this->safeString(market, "symbol");
    auto timestamp = this->safeInteger(order, "create_time");
    auto price = this->safeString(order, "price");
    auto amount = this->safeString(order, "amount");
    auto filled = this->safeString(order, "deal_amount");
    auto remaining = this->safeString(order, "left");
    auto cost = this->safeString(order, "deal_money");
    
    return {
        {"id", this->safeString(order, "id")},
        {"clientOrderId", this->safeString(order, "client_id")},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", this->safeInteger(order, "update_time")},
        {"status", status},
        {"symbol", symbol},
        {"type", this->safeStringLower(order, "type")},
        {"side", this->safeStringLower(order, "side")},
        {"price", price},
        {"amount", amount},
        {"filled", filled},
        {"remaining", remaining},
        {"cost", cost},
        {"trades", nullptr},
        {"fee", {
            {"cost", this->safeString(order, "deal_fee")},
            {"currency", this->safeString(market, "quote")}
        }},
        {"info", order}
    };
}

json Xt::parseTrade(const json& trade, const Market& market) {
    auto timestamp = this->safeInteger(trade, "time");
    auto side = this->safeStringLower(trade, "side");
    auto id = this->safeString(trade, "id");
    auto orderId = this->safeString(trade, "order_id");
    auto priceString = this->safeString(trade, "price");
    auto amountString = this->safeString(trade, "amount");
    auto price = this->parseNumber(priceString);
    auto amount = this->parseNumber(amountString);
    auto cost = this->parseNumber(Precise::stringMul(priceString, amountString));
    auto feeCost = this->safeNumber(trade, "fee");
    auto fee = nullptr;
    
    if (feeCost != nullptr) {
        auto feeCurrencyId = this->safeString(trade, "fee_currency");
        auto feeCurrencyCode = this->safeCurrencyCode(feeCurrencyId);
        fee = {
            {"cost", feeCost},
            {"currency", feeCurrencyCode}
        };
    }
    
    return {
        {"id", id},
        {"info", trade},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"symbol", market.symbol},
        {"order", orderId},
        {"type", nullptr},
        {"side", side},
        {"takerOrMaker", nullptr},
        {"price", price},
        {"amount", amount},
        {"cost", cost},
        {"fee", fee}
    };
}

json Xt::parseBalance(const json& response) {
    auto result = {
        {"info", response}
    };
    auto balances = this->safeValue(response, "balances", json::array());
    
    for (const auto& balance : balances) {
        auto currencyId = this->safeString(balance, "currency");
        auto code = this->safeCurrencyCode(currencyId);
        auto account = this->account();
        
        account["free"] = this->safeString(balance, "available");
        account["used"] = this->safeString(balance, "frozen");
        account["total"] = this->safeString(balance, "balance");
        
        result[code] = account;
    }
    
    return result;
}

String Xt::parseOrderStatus(const String& status) {
    auto statuses = {
        {"NEW", "open"},
        {"FILLED", "closed"},
        {"PARTIALLY_FILLED", "open"},
        {"CANCELED", "canceled"},
        {"PENDING_CANCEL", "canceling"},
        {"REJECTED", "rejected"}
    };
    return this->safeString(statuses, status, status);
}

} // namespace ccxt
