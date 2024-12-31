#include "bitget.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Bitget::Bitget() {
    id = "bitget";
    name = "Bitget";
    version = "v2";
    rateLimit = 50;
    unifiedMargin = false;

    // Initialize API endpoints
    baseUrl = "https://api.bitget.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/51840849/88317935-a8a21c80-cd22-11ea-8e2b-4b9fac5975eb.jpg"},
        {"api", {
            {"public", baseUrl},
            {"private", baseUrl}
        }},
        {"www", "https://www.bitget.com"},
        {"doc", {
            "https://bitgetlimited.github.io/apidoc/en/mix",
            "https://bitgetlimited.github.io/apidoc/en/spot",
            "https://bitgetlimited.github.io/apidoc/en/broker"
        }},
        {"fees", "https://www.bitget.com/fee"}
    };

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
        {"12h", "12h"},
        {"1d", "1d"},
        {"3d", "3d"},
        {"1w", "1w"},
        {"1M", "1M"}
    };

    options = {
        {"defaultType", "spot"},
        {"marginMode", "unified"},
        {"broker", "CCXT"}
    };

    initializeApiEndpoints();
}

void Bitget::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "api/spot/v1/public/time",
                "api/spot/v1/public/currencies",
                "api/spot/v1/public/products",
                "api/spot/v1/public/product",
                "api/spot/v1/market/ticker",
                "api/spot/v1/market/tickers",
                "api/spot/v1/market/fills",
                "api/spot/v1/market/candles",
                "api/spot/v1/market/depth",
                "api/mix/v1/market/contracts",
                "api/mix/v1/market/ticker",
                "api/mix/v1/market/tickers",
                "api/mix/v1/market/fills",
                "api/mix/v1/market/candles",
                "api/mix/v1/market/depth",
                "api/mix/v1/market/funding-time",
                "api/mix/v1/market/funding-rate",
                "api/mix/v1/market/history/funding-rate"
            }}
        }},
        {"private", {
            {"GET", {
                "api/spot/v1/account/assets",
                "api/spot/v1/account/bills",
                "api/spot/v1/trade/orders/current",
                "api/spot/v1/trade/orders/history",
                "api/spot/v1/trade/fills",
                "api/mix/v1/account/accounts",
                "api/mix/v1/account/account",
                "api/mix/v1/account/positions",
                "api/mix/v1/account/position",
                "api/mix/v1/order/current",
                "api/mix/v1/order/history",
                "api/mix/v1/order/fills",
                "api/mix/v1/position/history-positions"
            }},
            {"POST", {
                "api/spot/v1/trade/orders",
                "api/spot/v1/trade/batch-orders",
                "api/spot/v1/trade/cancel-order",
                "api/spot/v1/trade/cancel-batch-orders",
                "api/mix/v1/order/placeOrder",
                "api/mix/v1/order/batch-orders",
                "api/mix/v1/order/cancel-order",
                "api/mix/v1/order/cancel-batch-orders",
                "api/mix/v1/position/margin",
                "api/mix/v1/position/leverage",
                "api/mix/v1/position/margin-mode",
                "api/mix/v1/position/hold-mode",
                "api/mix/v1/account/setPositionMode",
                "api/mix/v1/account/setMarginMode"
            }}
        }}
    };
}

json Bitget::fetchMarkets(const json& params) {
    std::string type = this->getDefaultType();
    std::string path = type == "spot" ? "/api/spot/v1/public/products" : "/api/mix/v1/market/contracts";
    json response = fetch(path, "public", "GET", params);
    json markets = json::array();
    
    for (const auto& market : response["data"]) {
        std::string id = market["symbol"];
        std::string baseId = market["baseCoin"];
        std::string quoteId = market["quoteCoin"];
        std::string base = this->commonCurrencyCode(baseId);
        std::string quote = this->commonCurrencyCode(quoteId);
        std::string symbol = base + "/" + quote;
        bool active = market["status"] == "online";
        
        markets.push_back({
            {"id", id},
            {"symbol", symbol},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", active},
            {"type", type},
            {"spot", type == "spot"},
            {"future", type == "future"},
            {"swap", type == "swap"},
            {"option", false},
            {"contract", type != "spot"},
            {"linear", type != "spot" ? market["marginCoin"] == quoteId : false},
            {"inverse", type != "spot" ? market["marginCoin"] == baseId : false},
            {"contractSize", type != "spot" ? market["contractSize"] : nullptr},
            {"precision", {
                {"amount", market["quantityScale"]},
                {"price", market["priceScale"]}
            }},
            {"limits", {
                {"amount", {
                    {"min", market["minTradeAmount"]},
                    {"max", market["maxTradeAmount"]}
                }},
                {"price", {
                    {"min", market["minTradePrice"]},
                    {"max", market["maxTradePrice"]}
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

json Bitget::fetchBalance(const json& params) {
    this->loadMarkets();
    std::string type = this->getDefaultType();
    std::string path = type == "spot" ? "/api/spot/v1/account/assets" : "/api/mix/v1/account/accounts";
    
    json response = fetch(path, "private", "GET", params);
    json balances = response["data"];
    json result = {"info", response};
    
    for (const auto& balance : balances) {
        std::string currencyId = balance["coinName"];
        std::string code = this->commonCurrencyCode(currencyId);
        
        result[code] = {
            {"free", this->safeFloat(balance, "available")},
            {"used", this->safeFloat(balance, "frozen")},
            {"total", this->safeFloat(balance, "total")}
        };
    }
    
    return result;
}

json Bitget::createOrder(const std::string& symbol, const std::string& type,
                        const std::string& side, double amount,
                        double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    std::string orderType = type == "market" ? "market" : "limit";
    
    json request = {
        {"symbol", market.id},
        {"side", side.upper()},
        {"orderType", orderType},
        {"size", this->amountToPrecision(symbol, amount)}
    };
    
    if (type != "market") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    std::string path = market.type == "spot" ? "/api/spot/v1/trade/orders" : "/api/mix/v1/order/placeOrder";
    json response = fetch(path, "private", "POST", this->extend(request, params));
    return this->parseOrder(response["data"], market);
}

std::string Bitget::sign(const std::string& path, const std::string& api,
                    const std::string& method, const json& params,
                    const std::map<std::string, std::string>& headers,
                    const json& body) {
    std::string url = this->urls["api"][api] + "/" + this->implodeParams(path, params);
    std::string timestamp = this->getTimestamp();
    
    if (api == "public") {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    } else {
        this->checkRequiredCredentials();
        
        std::string query = "";
        if (method == "GET") {
            if (!params.empty()) {
                query = this->urlencode(this->keysort(params));
                url += "?" + query;
            }
        } else {
            if (!params.empty()) {
                body = this->json(params);
                query = body.dump();
            }
        }
        
        std::string auth = timestamp + method + path + query;
        std::string signature = this->hmac(auth, this->config_.secret, "sha256", "base64");
        
        const_cast<std::map<std::string, std::string>&>(headers)["ACCESS-KEY"] = this->config_.apiKey;
        const_cast<std::map<std::string, std::string>&>(headers)["ACCESS-SIGN"] = signature;
        const_cast<std::map<std::string, std::string>&>(headers)["ACCESS-TIMESTAMP"] = timestamp;
        const_cast<std::map<std::string, std::string>&>(headers)["ACCESS-PASSPHRASE"] = this->password;
        const_cast<std::map<std::string, std::string>&>(headers)["Content-Type"] = "application/json";
        
        if (!unifiedMargin) {
            const_cast<std::map<std::string, std::string>&>(headers)["X-CHANNEL-API-CODE"] = this->safeString(this->options, "broker", "CCXT");
        }
    }
    
    return url;
}

std::string Bitget::getTimestamp() {
    return std::to_string(this->milliseconds());
}

std::string Bitget::getDefaultType() {
    return this->safeString(this->options, "defaultType", "spot");
}

json Bitget::parseOrder(const json& order, const Market& market) {
    std::string id = this->safeString(order, "orderId");
    std::string timestamp = this->safeInteger(order, "cTime");
    std::string status = this->parseOrderStatus(this->safeString(order, "status"));
    std::string symbol = market.symbol;
    std::string type = this->safeStringLower(order, "orderType");
    std::string side = this->safeStringLower(order, "side");
    
    return {
        {"id", id},
        {"clientOrderId", this->safeString(order, "clientOid")},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"status", status},
        {"symbol", symbol},
        {"type", type},
        {"side", side},
        {"price", this->safeFloat(order, "price")},
        {"amount", this->safeFloat(order, "size")},
        {"filled", this->safeFloat(order, "filledQty")},
        {"remaining", this->safeFloat(order, "remainSize")},
        {"cost", this->safeFloat(order, "filledAmount")},
        {"trades", nullptr},
        {"fee", nullptr},
        {"info", order}
    };
}

json Bitget::parseOrderStatus(const std::string& status) {
    static const std::map<std::string, std::string> statuses = {
        {"new", "open"},
        {"partial-filled", "open"},
        {"filled", "closed"},
        {"canceled", "canceled"},
        {"cancelled", "canceled"}
    };
    
    return statuses.contains(status) ? statuses.at(status) : status;
}

} // namespace ccxt
