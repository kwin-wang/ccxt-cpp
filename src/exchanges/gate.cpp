#include "ccxt/exchanges/gate.h"
#include "ccxt/base/json.hpp"
#include "ccxt/base/errors.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace ccxt {

gate::gate(const Exchange::Config &config) : Exchange(config) {
    // Exchange info
    id = "gate";
    name = "Gate.io";
    version = "v4";
    certified = true;
    pro = true;
    countries = {"KR"};
    rateLimit = 50; // 200 requests per 10 seconds
    has = {
        {"fetchMarkets", true},
        {"fetchTicker", true},
        {"fetchOrderBook", true},
        {"fetchTrades", true},
        {"fetchOHLCV", true},
        {"createOrder", true},
        {"cancelOrder", true},
        {"fetchOrder", true},
        {"fetchOpenOrders", true},
        {"fetchClosedOrders", true},
        {"fetchMyTrades", true},
        {"fetchBalance", true},
        {"fetchTradingFees", true}
    };

    // URLs
    urls = {
        {"logo", "https://github.com/user-attachments/assets/64f988c5-07b6-4652-b5c1-679a6bf67c85"},
        {"api", {
            {"public", {
                {"wallet", "https://api.gateio.ws/api/v4"},
                {"futures", "https://api.gateio.ws/api/v4"},
                {"margin", "https://api.gateio.ws/api/v4"},
                {"delivery", "https://api.gateio.ws/api/v4"},
                {"spot", "https://api.gateio.ws/api/v4"},
                {"options", "https://api.gateio.ws/api/v4"}
            }},
            {"private", {
                {"wallet", "https://api.gateio.ws/api/v4"},
                {"futures", "https://api.gateio.ws/api/v4"},
                {"margin", "https://api.gateio.ws/api/v4"},
                {"delivery", "https://api.gateio.ws/api/v4"},
                {"spot", "https://api.gateio.ws/api/v4"},
                {"options", "https://api.gateio.ws/api/v4"}
            }}
        }},
        {"www", "https://gate.io/"},
        {"doc", "https://www.gate.io/docs/developers/apiv4/en/"}
    };

    // API endpoints
    api = {
        {"public", {
            {"GET", {
                "spot/currencies",
                "spot/currency_pairs",
                "spot/tickers",
                "spot/order_book",
                "spot/trades",
                "spot/candlesticks",
                "margin/currency_pairs",
                "margin/funding_book",
                "futures/contracts",
                "delivery/contracts",
                "options/contracts",
                "options/underlying"
            }}
        }},
        {"private", {
            {"GET", {
                "spot/accounts",
                "spot/open_orders",
                "spot/orders",
                "spot/my_trades",
                "margin/accounts",
                "margin/account_book",
                "margin/funding_accounts",
                "margin/loans",
                "margin/orders",
                "futures/accounts",
                "delivery/accounts",
                "options/accounts",
                "options/positions",
                "wallet/deposits",
                "wallet/withdrawals"
            }},
            {"POST", {
                "spot/orders",
                "margin/loans",
                "margin/orders",
                "futures/orders",
                "delivery/orders",
                "options/orders",
                "wallet/withdrawals"
            }},
            {"DELETE", {
                "spot/orders",
                "spot/orders/{order_id}",
                "margin/orders",
                "margin/orders/{order_id}",
                "futures/orders/{order_id}",
                "delivery/orders/{order_id}",
                "options/orders/{order_id}"
            }}
        }}
    };

    // Initialize markets
    if (!config.markets.empty()) {
        setMarkets(config.markets);
    }
}

Json gate::fetchMarkets(const Json &params) {
    auto response = this->publicGetSpotCurrencyPairs(params);
    return this->parseMarkets(response);
}

Json gate::fetchTicker(const std::string &symbol, const Json &params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        {"currency_pair", market["id"]}
    };
    auto response = this->publicGetSpotTickers(this->extend(request, params));
    return this->parseTicker(response, market);
}

Json gate::fetchOrderBook(const std::string &symbol, const int limit, const Json &params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        {"currency_pair", market["id"]}
    };
    if (limit != 0) {
        request["limit"] = limit;
    }
    auto response = this->publicGetSpotOrderBook(this->extend(request, params));
    return this->parseOrderBook(response, symbol);
}

Json gate::createOrder(const std::string &symbol, const std::string &type, const std::string &side,
                      double amount, double price, const Json &params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        {"currency_pair", market["id"]},
        {"side", side},
        {"type", type},
        {"amount", this->amountToPrecision(symbol, amount)}
    };
    if (type == "limit") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    auto response = this->privatePostSpotOrders(this->extend(request, params));
    return this->parseOrder(response, market);
}

Json gate::cancelOrder(const std::string &id, const std::string &symbol, const Json &params) {
    if (symbol.empty()) {
        throw ArgumentsRequired("cancelOrder requires a symbol argument");
    }
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        {"order_id", id},
        {"currency_pair", market["id"]}
    };
    return this->privateDeleteSpotOrdersOrderId(this->extend(request, params));
}

Json gate::fetchBalance(const Json &params) {
    this->loadMarkets();
    auto response = this->privateGetSpotAccounts(params);
    return this->parseBalance(response);
}

Json gate::sign(const std::string &path, const std::string &api, const std::string &method,
                const Json &params, const Json &headers, const Json &body) {
    auto url = this->urls["api"][api]["spot"] + "/" + this->implodeParams(path, params);
    auto query = this->omit(params, this->extractParams(path));

    if (api == "public") {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->checkRequiredCredentials();
        auto nonce = std::to_string(this->nonce());
        auto bodyString = body.empty() ? "" : this->json(body);
        auto auth = method + "\n" + url + "\n" + bodyString + "\n" + nonce;
        auto signature = this->hmac(auth, this->config_.secret, "SHA512", "hex");
        
        auto newHeaders = Json::object();
        newHeaders["KEY"] = this->config_.apiKey;
        newHeaders["Timestamp"] = nonce;
        newHeaders["SIGN"] = signature;
        
        if (!bodyString.empty()) {
            newHeaders["Content-Type"] = "application/json";
        }
        
        return {"url": url, "method": method, "body": bodyString, "headers": newHeaders};
    }
    
    return {"url": url, "method": method};
}

void gate::handleErrors(const Json &response) {
    if (response.find("code") != response.end() && response["code"] != 0) {
        auto code = std::to_string(response["code"].get<int>());
        auto message = response.value("message", "Unknown error");
        
        auto ExceptionClass = ExchangeError;
        if (code == "401" || code == "403") {
            ExceptionClass = AuthenticationError;
        } else if (code == "429") {
            ExceptionClass = RateLimitExceeded;
        } else if (code == "404") {
            ExceptionClass = OrderNotFound;
        } else if (code == "400") {
            ExceptionClass = BadRequest;
        }
        
        throw ExceptionClass(message);
    }
}

Json gate::parseOrder(const Json &order, const Json &market) {
    auto timestamp = this->safeInteger(order, "create_time_ms");
    auto price = this->safeString(order, "price");
    auto amount = this->safeString(order, "amount");
    auto filled = this->safeString(order, "filled_total");
    auto status = this->getOrderStatus(this->safeString(order, "status"));
    
    return {
        {"id", this->safeString(order, "id")},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"status", status},
        {"symbol", market["symbol"]},
        {"type", this->safeString(order, "type")},
        {"side", this->safeString(order, "side")},
        {"price", price},
        {"amount", amount},
        {"filled", filled},
        {"remaining", this->safeString(order, "left")},
        {"cost", this->safeString(order, "filled_total")},
        {"trades", nullptr},
        {"fee", nullptr},
        {"info", order}
    };
}

std::string gate::getOrderStatus(const std::string &status) {
    static std::map<std::string, std::string> statuses = {
        {"open", "open"},
        {"closed", "closed"},
        {"cancelled", "canceled"}
    };
    return statuses.value(status, status);
}

} // namespace ccxt
