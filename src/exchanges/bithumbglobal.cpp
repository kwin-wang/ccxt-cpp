#include "bithumbglobal.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Bithumbglobal::Bithumbglobal() {
    id = "bithumbglobal";
    name = "Bithumb Global";
    version = "1";
    certified = true;
    pro = false;
    hasPublicAPI = true;
    hasPrivateAPI = true;
    hasFiatAPI = true;
    hasMarginAPI = true;
    hasFuturesAPI = false;

    // Initialize URLs
    baseUrl = "https://global-openapi.bithumb.pro";
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/129991357-8fa41bc8-27d9-4424-b680-0e75845d580d.jpg"},
        {"api", {
            {"public", "https://global-openapi.bithumb.pro/openapi/v1"},
            {"private", "https://global-openapi.bithumb.pro/openapi/v1"}
        }},
        {"www", "https://www.bithumb.pro"},
        {"doc", {
            "https://github.com/bithumb-pro/bithumb.pro-official-api-docs",
            "https://api.bithumb.pro/apidoc"
        }},
        {"fees", "https://www.bithumb.pro/en-us/regulation"}
    };

    initializeApiEndpoints();
    initializeTimeframes();
    initializeMarketTypes();
    initializeOptions();
    initializeErrorCodes();
    initializeFees();
}

void Bithumbglobal::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "spot/config",
                "spot/ticker",
                "spot/orderBook",
                "spot/trades",
                "spot/kline",
                "spot/markets",
                "spot/placeConfig",
                "spot/assetConfig"
            }}
        }},
        {"private", {
            {"GET", {
                "spot/userAsset",
                "spot/orderDetail",
                "spot/openOrders",
                "spot/historyOrders",
                "spot/myTrades",
                "spot/depositHistory",
                "spot/withdrawHistory",
                "spot/depositAddress"
            }},
            {"POST", {
                "spot/placeOrder",
                "spot/cancelOrder",
                "spot/withdraw"
            }}
        }}
    };
}

void Bithumbglobal::initializeTimeframes() {
    timeframes = {
        {"1m", "1m"},
        {"3m", "3m"},
        {"5m", "5m"},
        {"15m", "15m"},
        {"30m", "30m"},
        {"1h", "1h"},
        {"2h", "2h"},
        {"4h", "4h"},
        {"6h", "6h"},
        {"8h", "8h"},
        {"12h", "12h"},
        {"1d", "1d"},
        {"3d", "3d"},
        {"1w", "1w"},
        {"1M", "1M"}
    };
}

json Bithumbglobal::fetchMarkets(const json& params) {
    json response = fetch("/spot/config", "public", "GET", params);
    json spotConfig = response["data"];
    json result = json::array();
    
    for (const auto& market : spotConfig["spotConfig"]) {
        std::string id = market["symbol"].get<std::string>();
        std::vector<std::string> parts = this->split(id, "-");
        std::string baseId = parts[0];
        std::string quoteId = parts[1];
        std::string base = this->safeCurrencyCode(baseId);
        std::string quote = this->safeCurrencyCode(quoteId);
        std::string symbol = base + "/" + quote;
        
        result.push_back({
            {"id", id},
            {"symbol", symbol},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", true},
            {"precision", {
                {"amount", market["amountPrecision"].get<int>()},
                {"price", market["pricePrecision"].get<int>()}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeFloat(market, "minAmount")},
                    {"max", this->safeFloat(market, "maxAmount")}
                }},
                {"price", {
                    {"min", this->safeFloat(market, "minPrice")},
                    {"max", this->safeFloat(market, "maxPrice")}
                }},
                {"cost", {
                    {"min", this->safeFloat(market, "minValue")},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return result;
}

json Bithumbglobal::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/spot/userAsset", "private", "GET", params);
    return parseBalance(response["data"]);
}

json Bithumbglobal::parseBalance(const json& response) {
    json result = {{"info", response}};
    
    for (const auto& balance : response) {
        std::string currencyId = balance["coinType"].get<std::string>();
        std::string code = this->safeCurrencyCode(currencyId);
        std::string account = {
            {"free", this->safeFloat(balance, "available")},
            {"used", this->safeFloat(balance, "frozen")},
            {"total", this->safeFloat(balance, "total")}
        };
        result[code] = account;
    }
    
    return result;
}

json Bithumbglobal::createOrder(const std::string& symbol, const std::string& type,
                              const std::string& side, double amount,
                              double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    std::string uppercaseType = type.toUpperCase();
    
    json request = {
        {"symbol", market["id"]},
        {"type", uppercaseType},
        {"side", side.toUpperCase()},
        {"quantity", this->amountToPrecision(symbol, amount)}
    };
    
    if (uppercaseType == "LIMIT") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    json response = fetch("/spot/placeOrder", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response["data"], market);
}

std::string Bithumbglobal::sign(const std::string& path, const std::string& api,
                          const std::string& method, const json& params,
                          const std::map<std::string, std::string>& headers,
                          const json& body) {
    std::string url = this->urls["api"][api] + path;
    
    if (api == "private") {
        this->checkRequiredCredentials();
        std::string timestamp = std::to_string(this->milliseconds());
        std::string nonce = this->uuid();
        
        json payload = {
            {"apiKey", this->config_.apiKey},
            {"timestamp", timestamp},
            {"nonce", nonce}
        };
        
        if (method == "GET") {
            if (!params.empty()) {
                payload = this->extend(payload, params);
            }
        } else {
            if (!params.empty()) {
                payload = this->extend(payload, params);
            }
            body = this->json(payload);
        }
        
        std::string payloadstd::string = this->json(this->keysort(payload));
        std::string signature = this->hmac(payloadstd::string, this->encode(this->config_.secret),
                                    "sha256", "hex");
        
        if (method == "GET") {
            if (!params.empty()) {
                url += "?" + this->urlencode(payload);
            }
        }
        
        const_cast<std::map<std::string, std::string>&>(headers)["Api-Key"] = this->config_.apiKey;
        const_cast<std::map<std::string, std::string>&>(headers)["Api-Timestamp"] = timestamp;
        const_cast<std::map<std::string, std::string>&>(headers)["Api-Nonce"] = nonce;
        const_cast<std::map<std::string, std::string>&>(headers)["Api-Signature"] = signature;
        
        if (method != "GET") {
            const_cast<std::map<std::string, std::string>&>(headers)["Content-Type"] = "application/json";
        }
    } else {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    }
    
    return url;
}

std::string Bithumbglobal::getNonce() {
    return this->uuid();
}

json Bithumbglobal::parseOrder(const json& order, const Market& market) {
    std::string id = this->safeString(order, "orderId");
    std::string timestamp = this->safeString(order, "createTime");
    std::string status = this->parseOrderStatus(this->safeString(order, "status"));
    std::string symbol = nullptr;
    
    if (!market.empty()) {
        symbol = market["symbol"];
    } else {
        std::string marketId = this->safeString(order, "symbol");
        if (marketId != nullptr) {
            if (this->markets_by_id.contains(marketId)) {
                market = this->markets_by_id[marketId];
                symbol = market["symbol"];
            } else {
                symbol = marketId;
            }
        }
    }
    
    std::string type = this->safeStringLower(order, "type");
    std::string side = this->safeStringLower(order, "side");
    
    return {
        {"id", id},
        {"clientOrderId", this->safeString(order, "clientOrderId")},
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
        {"amount", this->safeFloat(order, "quantity")},
        {"filled", this->safeFloat(order, "executedQty")},
        {"remaining", this->safeFloat(order, "quantity") - this->safeFloat(order, "executedQty")},
        {"cost", this->safeFloat(order, "executedQty") * this->safeFloat(order, "price")},
        {"trades", nullptr},
        {"fee", nullptr},
        {"info", order}
    };
}

std::string Bithumbglobal::parseOrderStatus(const std::string& status) {
    static const std::map<std::string, std::string> statuses = {
        {"NEW", "open"},
        {"PARTIALLY_FILLED", "open"},
        {"FILLED", "closed"},
        {"CANCELED", "canceled"},
        {"PENDING_CANCEL", "canceling"},
        {"REJECTED", "rejected"},
        {"EXPIRED", "expired"}
    };
    
    return this->safeString(statuses, status, status);
}

} // namespace ccxt
