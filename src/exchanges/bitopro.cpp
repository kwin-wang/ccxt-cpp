#include "bitopro.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Bitopro::Bitopro() {
    id = "bitopro";
    name = "BitoPro";
    version = "v3";
    certified = true;
    pro = false;
    hasPublicAPI = true;
    hasPrivateAPI = true;
    hasFiatAPI = true;
    hasMarginAPI = false;
    hasFuturesAPI = false;

    // Initialize URLs
    baseUrl = "https://api.bitopro.com/v3";
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/158227251-3a92a220-9222-453c-9277-977c6677fe71.jpg"},
        {"api", {
            {"public", "https://api.bitopro.com/v3"},
            {"private", "https://api.bitopro.com/v3"}
        }},
        {"www", "https://www.bitopro.com"},
        {"doc", {
            "https://github.com/bitoex/bitopro-offical-api-docs/blob/master/api/v3/rest-1/rest.md"
        }},
        {"fees", "https://www.bitopro.com/fees"}
    };

    initializeApiEndpoints();
    initializeTimeframes();
    initializeMarketTypes();
    initializeOptions();
    initializeErrorCodes();
    initializeFees();
}

void Bitopro::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "order-book/{pair}",
                "tickers/{pair}",
                "trades/{pair}",
                "trading-history/{pair}",
                "currencies",
                "provisioning/limitations-and-fees",
                "trading-pairs"
            }}
        }},
        {"private", {
            {"GET", {
                "accounts/balance",
                "orders/history",
                "orders/all/{pair}",
                "orders/{pair}",
                "orders/{pair}/{orderId}",
                "trades/{pair}",
                "wallet/withdraw/{currency}/history",
                "wallet/deposit/{currency}/history",
                "wallet/withdraw/{currency}/addresses"
            }},
            {"POST", {
                "orders/{pair}",
                "wallet/withdraw/{currency}"
            }},
            {"DELETE", {
                "orders/{pair}/{orderId}",
                "orders/{pair}"
            }}
        }}
    };
}

void Bitopro::initializeTimeframes() {
    timeframes = {
        {"1m", "1m"},
        {"5m", "5m"},
        {"15m", "15m"},
        {"30m", "30m"},
        {"1h", "1h"},
        {"4h", "4h"},
        {"6h", "6h"},
        {"12h", "12h"},
        {"1d", "1d"},
        {"1w", "1w"},
        {"1M", "1M"}
    };
}

json Bitopro::fetchMarkets(const json& params) {
    json response = fetch("/trading-pairs", "public", "GET", params);
    json result = json::array();
    
    for (const auto& market : response) {
        String id = market["pair"].get<String>();
        String baseId = market["base"].get<String>();
        String quoteId = market["quote"].get<String>();
        String base = this->safeCurrencyCode(baseId);
        String quote = this->safeCurrencyCode(quoteId);
        String symbol = base + "/" + quote;
        
        result.push_back({
            {"id", id},
            {"symbol", symbol},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", market["maintain"] == false},
            {"precision", {
                {"amount", market["orderMinAmountBase"].get<int>()},
                {"price", market["orderMinAmountQuote"].get<int>()}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeFloat(market, "orderMinAmountBase")},
                    {"max", this->safeFloat(market, "orderMaxAmountBase")}
                }},
                {"price", {
                    {"min", this->safeFloat(market, "orderMinAmountQuote")},
                    {"max", this->safeFloat(market, "orderMaxAmountQuote")}
                }},
                {"cost", {
                    {"min", this->safeFloat(market, "orderMinAmountQuote")},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return result;
}

json Bitopro::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/accounts/balance", "private", "GET", params);
    return parseBalance(response);
}

json Bitopro::parseBalance(const json& response) {
    json result = {{"info", response}};
    
    for (const auto& balance : response) {
        String currencyId = balance["currency"].get<String>();
        String code = this->safeCurrencyCode(currencyId);
        String account = {
            {"free", this->safeFloat(balance, "available")},
            {"used", this->safeFloat(balance, "locked")},
            {"total", this->safeFloat(balance, "total")}
        };
        result[code] = account;
    }
    
    return result;
}

json Bitopro::createOrder(const String& symbol, const String& type,
                         const String& side, double amount,
                         double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    String uppercaseType = type.toUpperCase();
    
    json request = {
        {"type", uppercaseType},
        {"action", side.toUpperCase()},
        {"amount", this->amountToPrecision(symbol, amount)}
    };
    
    if (uppercaseType == "LIMIT") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    String path = "/orders/" + market["id"];
    json response = fetch(path, "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response, market);
}

String Bitopro::sign(const String& path, const String& api,
                    const String& method, const json& params,
                    const std::map<String, String>& headers,
                    const json& body) {
    String url = this->urls["api"][api] + path;
    
    if (api == "private") {
        this->checkRequiredCredentials();
        String nonce = this->getNonce();
        String payload = "";
        
        if (method == "POST") {
            if (!params.empty()) {
                body = this->json(params);
                payload = body;
            }
        }
        
        String signature = this->hmac(payload, this->encode(this->config_.secret),
                                    "sha384", "hex");
        
        const_cast<std::map<String, String>&>(headers)["X-BITOPRO-APIKEY"] = this->config_.apiKey;
        const_cast<std::map<String, String>&>(headers)["X-BITOPRO-PAYLOAD"] = payload;
        const_cast<std::map<String, String>&>(headers)["X-BITOPRO-SIGNATURE"] = signature;
        
        if (method == "POST") {
            const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/json";
        }
    } else {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    }
    
    return url;
}

String Bitopro::getNonce() {
    return std::to_string(this->milliseconds());
}

json Bitopro::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "id");
    String timestamp = this->safeString(order, "createdTimestamp");
    String status = this->parseOrderStatus(this->safeString(order, "status"));
    String symbol = nullptr;
    
    if (!market.empty()) {
        symbol = market["symbol"];
    }
    
    String type = this->safeStringLower(order, "type");
    String side = this->safeStringLower(order, "action");
    
    return {
        {"id", id},
        {"clientOrderId", nullptr},
        {"timestamp", this->parse8601(timestamp)},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"type", type},
        {"timeInForce", nullptr},
        {"postOnly", nullptr},
        {"status", status},
        {"symbol", symbol},
        {"side", side},
        {"price", this->safeFloat(order, "price")},
        {"amount", this->safeFloat(order, "amount")},
        {"filled", this->safeFloat(order, "executedAmount")},
        {"remaining", this->safeFloat(order, "remainingAmount")},
        {"cost", this->safeFloat(order, "avgExecutionPrice") * this->safeFloat(order, "executedAmount")},
        {"trades", nullptr},
        {"fee", nullptr},
        {"info", order}
    };
}

String Bitopro::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"NEW", "open"},
        {"PARTIALLY_FILLED", "open"},
        {"FILLED", "closed"},
        {"CANCELLED", "canceled"},
        {"REJECTED", "rejected"}
    };
    
    return this->safeString(statuses, status, status);
}

} // namespace ccxt
