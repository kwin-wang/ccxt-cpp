#include "okcoin.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

OKCoin::OKCoin() {
    id = "okcoin";
    name = "OKCoin";
    version = "v5";
    rateLimit = 1000;
    hasPrivateAPI = true;

    // Initialize API endpoints
    baseUrl = "https://www.okcoin.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/27766791-89ffb502-5ee5-11e7-8a5b-c5950b68ac65.jpg"},
        {"api", {
            {"public", "https://www.okcoin.com"},
            {"private", "https://www.okcoin.com"}
        }},
        {"www", "https://www.okcoin.com"},
        {"doc", {
            "https://www.okcoin.com/docs/en/",
            "https://www.okcoin.com/docs/en/#rest-api"
        }},
        {"fees", "https://www.okcoin.com/fees.html"}
    };

    timeframes = {
        {"1m", "1m"},
        {"3m", "3m"},
        {"5m", "5m"},
        {"15m", "15m"},
        {"30m", "30m"},
        {"1h", "1H"},
        {"2h", "2H"},
        {"4h", "4H"},
        {"6h", "6H"},
        {"12h", "12H"},
        {"1d", "1D"},
        {"1w", "1W"},
        {"1M", "1M"},
        {"3M", "3M"},
        {"6M", "6M"},
        {"1y", "1Y"}
    };

    options = {
        {"adjustForTimeDifference", true},
        {"recvWindow", "5000"},
        {"timeDifference", 0}
    };

    initializeApiEndpoints();
}

void OKCoin::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "api/v5/market/tickers",
                "api/v5/market/ticker",
                "api/v5/market/index-tickers",
                "api/v5/market/books",
                "api/v5/market/candles",
                "api/v5/market/history-candles",
                "api/v5/public/instruments",
                "api/v5/public/delivery-exercise-history",
                "api/v5/public/open-interest",
                "api/v5/public/funding-rate",
                "api/v5/public/funding-rate-history",
                "api/v5/public/price-limit",
                "api/v5/public/opt-summary",
                "api/v5/public/estimated-price",
                "api/v5/public/discount-rate-interest-free-quota",
                "api/v5/public/time",
                "api/v5/public/mark-price",
                "api/v5/public/position-tiers",
                "api/v5/public/interest-rate-loan-quota"
            }}
        }},
        {"private", {
            {"GET", {
                "api/v5/account/balance",
                "api/v5/account/positions",
                "api/v5/account/bills",
                "api/v5/account/config",
                "api/v5/account/max-size",
                "api/v5/account/max-avail-size",
                "api/v5/account/leverage-info",
                "api/v5/account/max-loan",
                "api/v5/account/trade-fee",
                "api/v5/account/interest-accrued",
                "api/v5/account/interest-rate",
                "api/v5/account/interest-limits",
                "api/v5/asset/balances",
                "api/v5/asset/deposit-address",
                "api/v5/asset/deposit-history",
                "api/v5/asset/withdrawal-history",
                "api/v5/trade/order",
                "api/v5/trade/orders-pending",
                "api/v5/trade/orders-history",
                "api/v5/trade/orders-history-archive",
                "api/v5/trade/fills",
                "api/v5/trade/fills-history"
            }},
            {"POST", {
                "api/v5/account/set-leverage",
                "api/v5/account/set-position-mode",
                "api/v5/asset/withdrawal",
                "api/v5/asset/transfer",
                "api/v5/trade/order",
                "api/v5/trade/batch-orders",
                "api/v5/trade/cancel-order",
                "api/v5/trade/cancel-batch-orders",
                "api/v5/trade/amend-order",
                "api/v5/trade/amend-batch-orders",
                "api/v5/trade/close-position"
            }}
        }}
    };
}

json OKCoin::fetchMarkets(const json& params) {
    json response = fetch("/api/v5/public/instruments", "public", "GET",
                         this->extend({{"instType", "SPOT"}}, params));
    json markets = json::array();
    
    for (const auto& market : response["data"]) {
        std::string id = market["instId"];
        std::string baseId = market["baseCcy"];
        std::string quoteId = market["quoteCcy"];
        std::string base = this->commonCurrencyCode(baseId);
        std::string quote = this->commonCurrencyCode(quoteId);
        std::string symbol = base + "/" + quote;
        bool active = market["state"] == "live";
        
        markets.push_back({
            {"id", id},
            {"symbol", symbol},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", active},
            {"type", "spot"},
            {"spot", true},
            {"future", false},
            {"swap", false},
            {"option", false},
            {"contract", false},
            {"precision", {
                {"amount", this->safeInteger(market, "lotSz")},
                {"price", this->safeInteger(market, "tickSz")}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeFloat(market, "minSz")},
                    {"max", nullptr}
                }},
                {"price", {
                    {"min", nullptr},
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
    
    return markets;
}

json OKCoin::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/api/v5/account/balance", "private", "GET", params);
    json result = {"info", response};
    
    for (const auto& balance : response["data"][0]["details"]) {
        std::string currencyId = balance["ccy"];
        std::string code = this->commonCurrencyCode(currencyId);
        
        result[code] = {
            {"free", this->safeFloat(balance, "availBal")},
            {"used", this->safeFloat(balance, "frozenBal")},
            {"total", this->safeFloat(balance, "cashBal")}
        };
    }
    
    return result;
}

json OKCoin::createOrder(const std::string& symbol, const std::string& type,
                        const std::string& side, double amount,
                        double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"instId", market.id},
        {"tdMode", "cash"},
        {"side", side.lower()},
        {"ordType", type.lower()},
        {"sz", this->amountToPrecision(symbol, amount)}
    };
    
    if (type == "limit") {
        request["px"] = this->priceToPrecision(symbol, price);
    }
    
    json response = fetch("/api/v5/trade/order", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response["data"][0], market);
}

std::string OKCoin::sign(const std::string& path, const std::string& api,
                    const std::string& method, const json& params,
                    const std::map<std::string, std::string>& headers,
                    const json& body) {
    std::string url = this->urls["api"][api] + "/" + this->version + "/" + this->implodeParams(path, params);
    std::string query = this->omit(params, this->extractParams(path));
    
    if (api == "public") {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->checkRequiredCredentials();
        std::string timestamp = this->getTimestamp();
        std::string auth = timestamp + method + "/" + path;
        
        if (method == "GET") {
            if (!query.empty()) {
                url += "?" + this->urlencode(query);
                auth += "?" + this->urlencode(query);
            }
        } else {
            if (!query.empty()) {
                body = this->json(query);
                auth += body.dump();
            }
        }
        
        std::string signature = this->hmac(auth, this->base64ToBinary(this->config_.secret),
                                    "sha256", "base64");
        
        const_cast<std::map<std::string, std::string>&>(headers)["OK-ACCESS-KEY"] = this->config_.apiKey;
        const_cast<std::map<std::string, std::string>&>(headers)["OK-ACCESS-SIGN"] = signature;
        const_cast<std::map<std::string, std::string>&>(headers)["OK-ACCESS-TIMESTAMP"] = timestamp;
        const_cast<std::map<std::string, std::string>&>(headers)["OK-ACCESS-PASSPHRASE"] = this->password;
        
        if (!body.empty()) {
            const_cast<std::map<std::string, std::string>&>(headers)["Content-Type"] = "application/json";
        }
    }
    
    return url;
}

std::string OKCoin::getTimestamp() {
    return std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count());
}

json OKCoin::parseOrder(const json& order, const Market& market) {
    std::string id = this->safeString(order, "ordId");
    std::string timestamp = this->safeString(order, "cTime");
    std::string lastTradeTimestamp = this->safeString(order, "uTime");
    std::string status = this->parseOrderStatus(this->safeString(order, "state"));
    std::string symbol = market.symbol;
    std::string type = this->safeStringLower(order, "ordType");
    std::string side = this->safeStringLower(order, "side");
    
    return {
        {"id", id},
        {"clientOrderId", this->safeString(order, "clOrdId")},
        {"timestamp", this->parse8601(timestamp)},
        {"datetime", timestamp},
        {"lastTradeTimestamp", this->parse8601(lastTradeTimestamp)},
        {"status", status},
        {"symbol", symbol},
        {"type", type},
        {"side", side},
        {"price", this->safeFloat(order, "px")},
        {"amount", this->safeFloat(order, "sz")},
        {"filled", this->safeFloat(order, "accFillSz")},
        {"remaining", this->safeFloat(order, "sz") - this->safeFloat(order, "accFillSz")},
        {"cost", this->safeFloat(order, "fillPx") * this->safeFloat(order, "accFillSz")},
        {"trades", nullptr},
        {"fee", {
            {"currency", market.quote},
            {"cost", this->safeFloat(order, "fee")}
        }},
        {"info", order}
    };
}

json OKCoin::parseOrderStatus(const std::string& status) {
    static const std::map<std::string, std::string> statuses = {
        {"live", "open"},
        {"partially_filled", "open"},
        {"filled", "closed"},
        {"canceled", "canceled"},
        {"cancelled", "canceled"},
        {"failure", "rejected"}
    };
    
    return statuses.contains(status) ? statuses.at(status) : status;
}

// Async REST API methods
json OKCoin::fetchTickerAsync(const std::string& symbol, const json& params) {
    return std::async(std::launch::async, [this, symbol, params]() {
        return this->fetchTicker(symbol, params);
    });
}

json OKCoin::fetchBalanceAsync(const json& params) {
    return std::async(std::launch::async, [this, params]() {
        return this->fetchBalance(params);
    });
}

json OKCoin::createOrderAsync(const std::string& symbol, const std::string& type,
                            const std::string& side, double amount,
                            double price, const json& params) {
    return std::async(std::launch::async, [this, symbol, type, side, amount, price, params]() {
        return this->createOrder(symbol, type, side, amount, price, params);
    });
}

json OKCoin::cancelOrderAsync(const std::string& id, const std::string& symbol,
                            const json& params) {
    return std::async(std::launch::async, [this, id, symbol, params]() {
        return this->cancelOrder(id, symbol, params);
    });
}

json OKCoin::fetchOrderAsync(const std::string& id, const std::string& symbol,
                           const json& params) {
    return std::async(std::launch::async, [this, id, symbol, params]() {
        return this->fetchOrder(id, symbol, params);
    });
}

json OKCoin::fetchOrdersAsync(const std::string& symbol, int since,
                            int limit, const json& params) {
    return std::async(std::launch::async, [this, symbol, since, limit, params]() {
        return this->fetchOrders(symbol, since, limit, params);
    });
}

json OKCoin::fetchOpenOrdersAsync(const std::string& symbol, int since,
                                int limit, const json& params) {
    return std::async(std::launch::async, [this, symbol, since, limit, params]() {
        return this->fetchOpenOrders(symbol, since, limit, params);
    });
}

json OKCoin::fetchClosedOrdersAsync(const std::string& symbol, int since,
                                  int limit, const json& params) {
    return std::async(std::launch::async, [this, symbol, since, limit, params]() {
        return this->fetchClosedOrders(symbol, since, limit, params);
    });
}

json OKCoin::fetchMyTradesAsync(const std::string& symbol, int since,
                               int limit, const json& params) {
    return std::async(std::launch::async, [this, symbol, since, limit, params]() {
        return this->fetchMyTrades(symbol, since, limit, params);
    });
}

json OKCoin::fetchMarketsAsync(const json& params) {
    return std::async(std::launch::async, [this, params]() {
        return this->fetchMarkets(params);
    });
}

json OKCoin::fetchOrderBookAsync(const std::string& symbol, int limit,
                               const json& params) {
    return std::async(std::launch::async, [this, symbol, limit, params]() {
        return this->fetchOrderBook(symbol, limit, params);
    });
}

json OKCoin::fetchOHLCVAsync(const std::string& symbol, const std::string& timeframe,
                            int since, int limit, const json& params) {
    return std::async(std::launch::async, [this, symbol, timeframe, since, limit, params]() {
        return this->fetchOHLCV(symbol, timeframe, since, limit, params);
    });
}

} // namespace ccxt
