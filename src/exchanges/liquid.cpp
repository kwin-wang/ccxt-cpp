#include "liquid.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Liquid::Liquid() {
    id = "liquid";
    name = "Liquid";
    version = "2";
    rateLimit = 1000;
    hasPrivateAPI = true;
    lastNonce = 0;

    // Initialize API endpoints
    baseUrl = "https://api.liquid.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/45798859-1a872600-bcb4-11e8-8746-69291ce87b04.jpg"},
        {"api", {
            {"public", "https://api.liquid.com"},
            {"private", "https://api.liquid.com"}
        }},
        {"www", "https://www.liquid.com"},
        {"doc", {
            "https://developers.liquid.com"
        }},
        {"fees", "https://help.liquid.com/getting-started-with-liquid/the-platform/fee-structure"}
    };

    timeframes = {
        {"1m", "1"},
        {"5m", "5"},
        {"15m", "15"},
        {"30m", "30"},
        {"1h", "60"},
        {"4h", "240"},
        {"6h", "360"},
        {"12h", "720"},
        {"1d", "1440"},
        {"2d", "2880"},
        {"3d", "4320"},
        {"1w", "10080"},
        {"2w", "20160"},
        {"1M", "43200"}
    };

    options = {
        {"adjustForTimeDifference", true},
        {"recvWindow", "5000"},
        {"timeDifference", 0}
    };

    initializeApiEndpoints();
}

void Liquid::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "products",
                "products/{id}",
                "products/{id}/price_levels",
                "executions",
                "ir_ladders/{currency}",
                "fees/trading",
                "currencies",
                "currencies/{id}",
                "pairs",
                "products/{id}/tickers",
                "products/tickers"
            }}
        }},
        {"private", {
            {"GET", {
                "accounts",
                "accounts/balance",
                "crypto_accounts",
                "executions/me",
                "fiat_accounts",
                "loan_bids",
                "loans",
                "orders",
                "orders/{id}",
                "trades",
                "trades/{id}/loans",
                "trading_accounts",
                "trading_accounts/{id}",
                "transactions",
                "withdrawals",
                "withdrawals/{id}"
            }},
            {"POST", {
                "orders",
                "withdrawals",
                "trades/{id}/close",
                "trades/{id}/adjust_margin"
            }},
            {"PUT", {
                "orders/{id}/cancel"
            }}
        }}
    };
}

json Liquid::fetchMarkets(const json& params) {
    json response = fetch("/products", "public", "GET", params);
    json markets = json::array();
    
    for (const auto& market : response) {
        std::string id = this->safeString(market, "id");
        std::string baseId = this->safeString(market, "base_currency");
        std::string quoteId = this->safeString(market, "quoted_currency");
        std::string base = this->commonCurrencyCode(baseId);
        std::string quote = this->commonCurrencyCode(quoteId);
        std::string symbol = base + "/" + quote;
        bool active = market["disabled"].get<bool>() == false;
        
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
                {"amount", market["currency_precision"].get<int>()},
                {"price", market["price_precision"].get<int>()}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeFloat(market, "minimum_order_quantity")},
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

json Liquid::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/accounts/balance", "private", "GET", params);
    json result = {"info", response};
    
    for (const auto& balance : response) {
        std::string currencyId = balance["currency"];
        std::string code = this->commonCurrencyCode(currencyId);
        
        result[code] = {
            {"free", this->safeFloat(balance, "available_balance")},
            {"used", this->safeFloat(balance, "reserved_balance")},
            {"total", this->safeFloat(balance, "balance")}
        };
    }
    
    return result;
}

json Liquid::createOrder(const std::string& symbol, const std::string& type,
                        const std::string& side, double amount,
                        double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"product_id", market.id},
        {"side", side.lower()},
        {"quantity", this->amountToPrecision(symbol, amount)}
    };
    
    std::string orderType = type.lower();
    if (orderType == "limit") {
        request["order_type"] = "limit";
        request["price"] = this->priceToPrecision(symbol, price);
    } else {
        request["order_type"] = "market";
    }
    
    json response = fetch("/orders", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response, market);
}

std::string Liquid::sign(const std::string& path, const std::string& api,
                    const std::string& method, const json& params,
                    const std::map<std::string, std::string>& headers,
                    const json& body) {
    std::string url = this->urls["api"][api];
    std::string endpoint = "/" + this->implodeParams(path, params);
    url += endpoint;
    
    json query = this->omit(params, this->extractParams(path));
    
    if (api == "public") {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->checkRequiredCredentials();
        std::string nonce = this->getNonce();
        std::string authPath = "/" + this->version + endpoint;
        
        if (method == "GET") {
            if (!query.empty()) {
                url += "?" + this->urlencode(query);
                authPath += "?" + this->urlencode(query);
            }
        } else {
            if (!query.empty()) {
                body = this->json(query);
            }
        }
        
        std::string signature = this->createSignature(authPath, method, nonce,
                                               body.empty() ? "" : body.dump());
        
        const_cast<std::map<std::string, std::string>&>(headers)["X-Quoine-API-Version"] = this->version;
        const_cast<std::map<std::string, std::string>&>(headers)["X-Quoine-Auth"] = signature;
        const_cast<std::map<std::string, std::string>&>(headers)["Content-Type"] = "application/json";
        
        if (!body.empty()) {
            const_cast<std::map<std::string, std::string>&>(headers)["Content-Length"] = std::to_string(body.dump().length());
        }
    }
    
    return url;
}

std::string Liquid::getNonce() {
    int64_t currentNonce = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    if (currentNonce <= lastNonce) {
        currentNonce = lastNonce + 1;
    }
    lastNonce = currentNonce;
    return std::to_string(currentNonce);
}

std::string Liquid::createSignature(const std::string& path, const std::string& method,
                              const std::string& nonce, const std::string& body) {
    std::string message = nonce + "|" + method + "|" + path + "|" + body;
    return this->hmac(message, this->base64ToBinary(this->config_.secret),
                     "sha256", "hex");
}

json Liquid::parseOrder(const json& order, const Market& market) {
    std::string id = this->safeString(order, "id");
    std::string timestamp = this->safeString(order, "created_at");
    std::string status = this->parseOrderStatus(this->safeString(order, "status"));
    std::string type = this->safeStringLower(order, "order_type");
    std::string side = this->safeStringLower(order, "side");
    
    return {
        {"id", id},
        {"clientOrderId", nullptr},
        {"timestamp", this->parse8601(timestamp)},
        {"datetime", timestamp},
        {"lastTradeTimestamp", nullptr},
        {"status", status},
        {"symbol", market.symbol},
        {"type", type},
        {"side", side},
        {"price", this->safeFloat(order, "price")},
        {"amount", this->safeFloat(order, "quantity")},
        {"filled", this->safeFloat(order, "filled_quantity")},
        {"remaining", this->safeFloat(order, "unfilled_quantity")},
        {"cost", this->safeFloat(order, "price") * this->safeFloat(order, "filled_quantity")},
        {"trades", nullptr},
        {"fee", nullptr},
        {"info", order}
    };
}

json Liquid::parseOrderStatus(const std::string& status) {
    static const std::map<std::string, std::string> statuses = {
        {"live", "open"},
        {"filled", "closed"},
        {"cancelled", "canceled"}
    };
    
    return statuses.contains(status) ? statuses.at(status) : status;
}

} // namespace ccxt
