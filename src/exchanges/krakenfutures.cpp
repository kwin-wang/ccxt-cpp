#include "ccxt/exchanges/krakenfutures.h"
#include "../base/crypto.h"
#include "../base/error.h"
#include <sstream>
#include <iomanip>
#include <chrono>

namespace ccxt {

KrakenFutures::KrakenFutures(const ExchangeConfig& config) : Exchange(config) {
    // Initialize exchange-specific configurations
    this->has = {
        {"CORS", nullptr},
        {"spot", false},
        {"margin", false},
        {"swap", true},
        {"future", true},
        {"option", false},
        {"cancelAllOrders", true},
        {"cancelAllOrdersAfter", true},
        {"cancelOrder", true},
        {"cancelOrders", true}
    };

    this->urls = {
        {"logo", "https://user-images.githubusercontent.com/51840849/76173629-fc67fb00-61b1-11ea-84fe-f2de582f58a3.jpg"},
        {"api", {
            {"public", "https://futures.kraken.com/derivatives/api/v3"},
            {"private", "https://futures.kraken.com/derivatives/api/v3"}
        }},
        {"www", "https://futures.kraken.com/"},
        {"doc", "https://support.kraken.com/hc/en-us/categories/360001806372-Futures-API"},
        {"fees", "https://support.kraken.com/hc/en-us/articles/360022835771-Transaction-fees-and-rebates"}
    };

    this->api = {
        {"public", {
            {"get", {
                "instruments",
                "instruments/{instrument_name}",
                "tickers",
                "tickers/{symbol}",
                "orderbook/{symbol}",
                "history",
                "history/{symbol}",
                "charts",
                "charts/{symbol}",
                "markets",
                "markets/{symbol}",
                "derivatives/api/v3/openapi.json"
            }}
        }},
        {"private", {
            {"get", {
                "accounts",
                "accounts/balances",
                "accounts/positions",
                "accounts/margins",
                "accounts/notifications",
                "accounts/overview",
                "accounts/pnl",
                "accounts/pnlhistory",
                "accounts/transfers",
                "accounts/withdrawals",
                "accounts/deposits",
                "wallets/accounts",
                "wallets/history",
                "orders",
                "orders/history",
                "fills",
                "fills/history",
                "sendorder",
                "cancelorder",
                "cancelallorders",
                "cancelallordersafter"
            }},
            {"post", {
                "sendorder",
                "withdrawal",
                "transfer",
                "cancelorder",
                "cancelallorders",
                "cancelallordersafter",
                "batchorder",
                "accounts/leverage"
            }}
        }}
    };
}

std::string KrakenFutures::getNonce() const {
    return std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

std::string KrakenFutures::getSignature(const std::string& path, const std::string& method,
                                      const std::string& nonce, const std::string& body) const {
    std::string message = nonce + method + path;
    if (!body.empty()) {
        message += body;
    }
    return hmacSha256(message, this->secret);
}

json KrakenFutures::signRequest(const std::string& path, const std::string& api,
                              const std::string& method, const Params& params,
                              const json& headers, const std::string& body) {
    auto nonce = this->getNonce();
    auto signature = this->getSignature(path, method, nonce, body);
    
    json resultHeaders = headers;
    resultHeaders["APIKey"] = this->apiKey;
    resultHeaders["Nonce"] = nonce;
    resultHeaders["Authent"] = signature;
    
    std::string url = this->urls["api"][api] + path;
    if (!params.empty() && method == "GET") {
        url += "?" + this->urlencode(params);
    }

    return {
        {"url", url},
        {"method", method},
        {"body", body},
        {"headers", resultHeaders}
    };
}

OrderBook KrakenFutures::fetchOrderBook(const std::string& symbol, int limit, const Params& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = Params();
    
    auto response = this->publicGetOrderbookSymbol(this->extend({
        "symbol": market["id"]
    }, request));

    return this->parseOrderBook(response, symbol);
}

Order KrakenFutures::createOrder(const std::string& symbol, const std::string& type,
                               const std::string& side, double amount, double price,
                               const Params& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    
    auto request = {
        {"orderType", type},
        {"symbol", market["id"].get<std::string>()},
        {"side", side},
        {"size", this->amountToPrecision(symbol, amount)}
    };

    if (type == "limit") {
        request["limitPrice"] = this->priceToPrecision(symbol, price);
    }

    auto response = this->privatePostSendorder(this->extend(request, params));
    return this->parseOrder(response["result"]);
}

Balance KrakenFutures::fetchBalance(const Params& params) {
    this->loadMarkets();
    auto response = this->privateGetAccountsBalances(params);
    return this->parseBalance(response["accounts"]);
}

void KrakenFutures::handleErrors(const json& response) {
    if (response.contains("error")) {
        auto error = response["error"];
        auto code = this->safeString(error, "code");
        auto message = this->safeString(error, "message", "Unknown error");
        
        if (code == "authenticationError") {
            throw AuthenticationError(message);
        } else if (code == "insufficientAvailableFunds") {
            throw InsufficientFunds(message);
        } else if (code == "orderNotFound") {
            throw OrderNotFound(message);
        } else if (code == "rateLimitExceeded") {
            throw RateLimitExceeded(message);
        } else if (code == "nonceDuplicate") {
            throw InvalidNonce(message);
        }
        
        throw ExchangeError(message);
    }
}

std::string KrakenFutures::getSettlementCurrency(const std::string& symbol) const {
    auto market = this->market(symbol);
    return market.value("settlementCurrency", "USD");
}

bool KrakenFutures::checkRequiredSymbol(const std::string& symbol) const {
    if (symbol.empty()) {
        throw ArgumentsRequired("Symbol is required for this operation");
    }
    return true;
}

void KrakenFutures::validateLeverageInput(double leverage, const std::string& symbol) const {
    auto market = this->market(symbol);
    auto maxLeverage = market.value("maxLeverage", 50.0);
    if (leverage > maxLeverage) {
        throw BadRequest("Leverage exceeds maximum allowed value of " + std::to_string(maxLeverage));
    }
}

} // namespace ccxt
