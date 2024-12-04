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
        String id = this->safeString(market, "id");
        String baseId = this->safeString(market, "base_currency");
        String quoteId = this->safeString(market, "quoted_currency");
        String base = this->commonCurrencyCode(baseId);
        String quote = this->commonCurrencyCode(quoteId);
        String symbol = base + "/" + quote;
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
        String currencyId = balance["currency"];
        String code = this->commonCurrencyCode(currencyId);
        
        result[code] = {
            {"free", this->safeFloat(balance, "available_balance")},
            {"used", this->safeFloat(balance, "reserved_balance")},
            {"total", this->safeFloat(balance, "balance")}
        };
    }
    
    return result;
}

json Liquid::createOrder(const String& symbol, const String& type,
                        const String& side, double amount,
                        double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"product_id", market.id},
        {"side", side.lower()},
        {"quantity", this->amountToPrecision(symbol, amount)}
    };
    
    String orderType = type.lower();
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

String Liquid::sign(const String& path, const String& api,
                    const String& method, const json& params,
                    const std::map<String, String>& headers,
                    const json& body) {
    String url = this->urls["api"][api];
    String endpoint = "/" + this->implodeParams(path, params);
    url += endpoint;
    
    json query = this->omit(params, this->extractParams(path));
    
    if (api == "public") {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->checkRequiredCredentials();
        String nonce = this->getNonce();
        String authPath = "/" + this->version + endpoint;
        
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
        
        String signature = this->createSignature(authPath, method, nonce,
                                               body.empty() ? "" : body.dump());
        
        const_cast<std::map<String, String>&>(headers)["X-Quoine-API-Version"] = this->version;
        const_cast<std::map<String, String>&>(headers)["X-Quoine-Auth"] = signature;
        const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/json";
        
        if (!body.empty()) {
            const_cast<std::map<String, String>&>(headers)["Content-Length"] = std::to_string(body.dump().length());
        }
    }
    
    return url;
}

String Liquid::getNonce() {
    int64_t currentNonce = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    if (currentNonce <= lastNonce) {
        currentNonce = lastNonce + 1;
    }
    lastNonce = currentNonce;
    return std::to_string(currentNonce);
}

String Liquid::createSignature(const String& path, const String& method,
                              const String& nonce, const String& body) {
    String message = nonce + "|" + method + "|" + path + "|" + body;
    return this->hmac(message, this->base64ToBinary(this->secret),
                     "sha256", "hex");
}

json Liquid::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "id");
    String timestamp = this->safeString(order, "created_at");
    String status = this->parseOrderStatus(this->safeString(order, "status"));
    String type = this->safeStringLower(order, "order_type");
    String side = this->safeStringLower(order, "side");
    
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

json Liquid::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"live", "open"},
        {"filled", "closed"},
        {"cancelled", "canceled"}
    };
    
    return statuses.contains(status) ? statuses.at(status) : status;
}

} // namespace ccxt
