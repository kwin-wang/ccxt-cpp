#include "exmo.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Exmo::Exmo() {
    id = "exmo";
    name = "EXMO";
    version = "v1.1";
    rateLimit = 100;  // 10 requests per second

    // Initialize API endpoints
    baseUrl = "https://api.exmo.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/27766491-1b0ea956-5eda-11e7-9225-40d67b481b8d.jpg"},
        {"api", {
            {"public", baseUrl},
            {"private", baseUrl},
            {"web", "https://exmo.me"}
        }},
        {"www", "https://exmo.me"},
        {"doc", {
            "https://exmo.me/en/api_doc",
            "https://github.com/exmo-dev/exmo_api_lib/tree/master/nodejs"
        }},
        {"fees", "https://exmo.com/en/docs/fees"}
    };

    api = {
        {"public", {
            {"get", {
                "currency",
                "currency/list/extended",
                "order_book",
                "pair_settings",
                "ticker",
                "trades"
            }}
        }},
        {"private", {
            {"post", {
                "user_info",
                "order_create",
                "order_cancel",
                "user_open_orders",
                "user_trades",
                "user_cancelled_orders",
                "order_trades",
                "required_amount",
                "deposit_address",
                "withdraw_crypt",
                "withdraw_get_txid",
                "excode_create",
                "excode_load",
                "wallet_history"
            }}
        }}
    };

    has = {
        {"CORS", false},
        {"spot", true},
        {"margin", true},
        {"swap", false},
        {"future", false},
        {"option", false},
        {"addMargin", true},
        {"cancelOrder", true},
        {"createOrder", true},
        {"createStopLimitOrder", true},
        {"createStopMarketOrder", true},
        {"createStopOrder", true},
        {"editOrder", true},
        {"fetchBalance", true},
        {"fetchCurrencies", true},
        {"fetchDepositAddress", true},
        {"fetchMarkets", true},
        {"fetchMyTrades", true},
        {"fetchOHLCV", false},
        {"fetchOpenOrders", true},
        {"fetchOrder", true},
        {"fetchOrderBook", true},
        {"fetchOrderTrades", true},
        {"fetchTicker", true},
        {"fetchTickers", true},
        {"fetchTrades", true},
        {"withdraw", true}
    };

    precisionMode = TICK_SIZE;
    
    options = {
        {"fetchTickers", {
            {"method", "publicGetTicker"}
        }},
        {"fetchOrders", {
            {"method", "privatePostUserOpenOrders"}
        }}
    };
}

json Exmo::fetchMarkets(const json& params) {
    auto response = this->publicGetPairSettings(params);
    auto result = json::array();
    
    for (const auto& [id, market] : response.items()) {
        auto symbol = id;
        auto parts = this->split(id, "_");
        auto baseId = this->safeString(parts, 0);
        auto quoteId = this->safeString(parts, 1);
        auto base = this->safeCurrencyCode(baseId);
        auto quote = this->safeCurrencyCode(quoteId);
        
        result.push_back({
            {"id", id},
            {"symbol", base + "/" + quote},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", true},
            {"type", "spot"},
            {"spot", true},
            {"margin", true},
            {"precision", {
                {"amount", this->safeInteger(market, "decimal_places")},
                {"price", this->safeInteger(market, "decimal_places")}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeNumber(market, "min_amount")},
                    {"max", this->safeNumber(market, "max_amount")}
                }},
                {"price", {
                    {"min", this->safeNumber(market, "min_price")},
                    {"max", this->safeNumber(market, "max_price")}
                }},
                {"cost", {
                    {"min", this->safeNumber(market, "min_total")},
                    {"max", this->safeNumber(market, "max_total")}
                }}
            }},
            {"info", market}
        });
    }
    
    return result;
}

json Exmo::fetchOrderBook(const String& symbol, int limit, const json& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        {"pair", market["id"]}
    };
    if (limit != 0) {
        request["limit"] = limit;
    }
    
    auto response = this->publicGetOrderBook(this->extend(request, params));
    auto orderbook = this->safeValue(response, market["id"]);
    return this->parseOrderBook(orderbook, symbol);
}

json Exmo::fetchTicker(const String& symbol, const json& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto response = this->publicGetTicker(params);
    auto ticker = this->safeValue(response, market["id"]);
    
    auto timestamp = this->safeTimestamp(ticker, "updated");
    return {
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"high", this->safeNumber(ticker, "high")},
        {"low", this->safeNumber(ticker, "low")},
        {"bid", this->safeNumber(ticker, "buy_price")},
        {"ask", this->safeNumber(ticker, "sell_price")},
        {"last", this->safeNumber(ticker, "last_trade")},
        {"close", this->safeNumber(ticker, "last_trade")},
        {"baseVolume", this->safeNumber(ticker, "vol")},
        {"quoteVolume", this->safeNumber(ticker, "vol_curr")},
        {"info", ticker}
    };
}

json Exmo::createOrder(const String& symbol, const String& type,
                      const String& side, double amount,
                      double price, const json& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        {"pair", market["id"]},
        {"quantity", this->amountToPrecision(symbol, amount)},
        {"type", side},
    };
    
    if (type == "limit") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    auto response = this->privatePostOrderCreate(this->extend(request, params));
    auto id = this->safeString(response, "order_id");
    
    return {
        {"id", id},
        {"info", response}
    };
}

json Exmo::cancelOrder(const String& id, const String& symbol, const json& params) {
    auto request = {
        {"order_id", id}
    };
    return this->privatePostOrderCancel(this->extend(request, params));
}

json Exmo::fetchBalance(const json& params) {
    this->loadMarkets();
    auto response = this->privatePostUserInfo(params);
    auto result = {"info", response};
    
    auto balances = this->safeValue(response, "balances", {});
    auto reserved = this->safeValue(response, "reserved", {});
    
    for (const auto& [currencyId, balance] : balances.items()) {
        auto code = this->safeCurrencyCode(currencyId);
        auto account = this->account();
        account["free"] = this->safeString(balances, currencyId);
        account["used"] = this->safeString(reserved, currencyId);
        result[code] = account;
    }
    
    return this->parseBalance(result);
}

json Exmo::parseOrder(const json& order, const Market& market) {
    auto id = this->safeString(order, "order_id");
    auto timestamp = this->safeTimestamp(order, "created");
    auto symbol = market["symbol"];
    auto side = this->safeString(order, "type");
    auto price = this->safeNumber(order, "price");
    auto amount = this->safeNumber(order, "quantity");
    auto remaining = this->safeNumber(order, "amount");
    auto filled = amount - remaining;
    
    return {
        {"id", id},
        {"clientOrderId", nullptr},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"status", "open"},
        {"symbol", symbol},
        {"type", "limit"},
        {"timeInForce", nullptr},
        {"postOnly", nullptr},
        {"side", side},
        {"price", price},
        {"stopPrice", nullptr},
        {"amount", amount},
        {"filled", filled},
        {"remaining", remaining},
        {"cost", price * filled},
        {"trades", nullptr},
        {"fee", nullptr},
        {"info", order}
    };
}

json Exmo::parseTrade(const json& trade, const Market& market) {
    auto timestamp = this->safeTimestamp(trade, "date");
    auto price = this->safeNumber(trade, "price");
    auto amount = this->safeNumber(trade, "quantity");
    auto id = this->safeString(trade, "trade_id");
    auto orderId = this->safeString(trade, "order_id");
    auto side = this->safeString(trade, "type");
    auto symbol = market["symbol"];
    
    auto fee = nullptr;
    if (trade.contains("commission")) {
        auto feeCost = this->safeNumber(trade, "commission");
        auto feeCurrency = this->safeString(trade, "commission_currency");
        fee = {
            {"cost", feeCost},
            {"currency", this->safeCurrencyCode(feeCurrency)}
        };
    }
    
    return {
        {"id", id},
        {"info", trade},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"symbol", symbol},
        {"order", orderId},
        {"type", "limit"},
        {"side", side},
        {"takerOrMaker", nullptr},
        {"price", price},
        {"amount", amount},
        {"cost", price * amount},
        {"fee", fee}
    };
}

json Exmo::fetchMyTrades(const String& symbol, int since, int limit, const json& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        {"pair", market["id"]}
    };
    
    if (limit != 0) {
        request["limit"] = limit;
    }
    
    auto response = this->privatePostUserTrades(this->extend(request, params));
    auto trades = this->safeValue(response, market["id"], {});
    return this->parseTrades(trades, market, since, limit);
}

json Exmo::fetchOpenOrders(const String& symbol, int since, int limit, const json& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        {"pair", market["id"]}
    };
    
    auto response = this->privatePostUserOpenOrders(this->extend(request, params));
    auto orders = this->safeValue(response, market["id"], {});
    return this->parseOrders(orders, market, since, limit);
}

json Exmo::fetchOrderTrades(const String& id, const String& symbol, int since, int limit, const json& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        {"order_id", id}
    };
    
    auto response = this->privatePostOrderTrades(this->extend(request, params));
    auto trades = this->safeValue(response, "trades", {});
    return this->parseTrades(trades, market, since, limit);
}

String Exmo::sign(const String& path, const String& api,
                  const String& method, const json& params,
                  const std::map<String, String>& headers,
                  const json& body) {
    auto url = this->urls["api"][api] + "/" + this->version + "/" + path;
    
    if (api == "public") {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    } else {
        this->checkRequiredCredentials();
        auto nonce = std::to_string(this->nonce());
        auto body = this->urlencode(this->extend({
            "nonce", nonce
        }, params));
        
        auto signature = this->hmac(body, this->encode(this->config_.secret),
                                  "sha512", "hex");
        
        auto headers = {
            {"Content-Type", "application/x-www-form-urlencoded"},
            {"Key", this->config_.apiKey},
            {"Sign", signature}
        };
    }
    
    return url;
}

void Exmo::handleErrors(const json& httpCode, const String& reason, const String& url,
                       const String& method, const std::map<String, String>& headers,
                       const String& body, const json& response,
                       const json& requestHeaders, const json& requestBody) {
    if (response.empty()) {
        return;
    }
    
    if (response.contains("result")) {
        auto result = this->safeValue(response, "result", false);
        if (result == false) {
            auto code = this->safeString(response, "error");
            auto feedback = this->id + " " + body;
            
            if (code == "40015") {
                throw InvalidNonce(feedback);
            } else if (code == "40017") {
                throw AuthenticationError(feedback);
            } else if (code == "40021") {
                throw PermissionDenied(feedback);
            } else if (code == "50052") {
                throw InsufficientFunds(feedback);
            } else if (code == "50054") {
                throw InvalidOrder(feedback);
            } else if (code == "50173") {
                throw OrderNotFound(feedback);
            } else if (code == "50319") {
                throw InvalidOrder(feedback);
            } else if (code == "50321") {
                throw InvalidOrder(feedback);
            }
            
            throw ExchangeError(feedback);
        }
    }
}
} // namespace ccxt
