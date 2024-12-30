#include "bybit.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Bybit::Bybit() {
    id = "bybit";
    name = "Bybit";
    version = "v5";
    rateLimit = 100;
    unified = true;  // Use unified account
    defaultType = "spot";  // Default to spot trading

    // Initialize API endpoints
    baseUrl = "https://api.bybit.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/51840849/76547799-daff5b80-649e-11ea-87fb-3be9bac08954.jpg"},
        {"api", {
            {"public", "https://api.bybit.com"},
            {"private", "https://api.bybit.com"}
        }},
        {"www", "https://www.bybit.com"},
        {"test", "https://api-testnet.bybit.com"},
        {"doc", {
            "https://bybit-exchange.github.io/docs/v5/intro",
            "https://github.com/bybit-exchange/api-usage-examples"
        }},
        {"fees", "https://www.bybit.com/fee/trade"}
    };

    timeframes = {
        {"1m", "1"},
        {"3m", "3"},
        {"5m", "5"},
        {"15m", "15"},
        {"30m", "30"},
        {"1h", "60"},
        {"2h", "120"},
        {"4h", "240"},
        {"6h", "360"},
        {"12h", "720"},
        {"1d", "D"},
        {"1w", "W"},
        {"1M", "M"}
    };

    initializeApiEndpoints();
}

void Bybit::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "v5/market/tickers",
                "v5/market/orderbook",
                "v5/market/trades",
                "v5/market/kline",
                "v5/market/instruments-info",
                "v5/market/time",
                "v5/market/funding/history"
            }}
        }},
        {"private", {
            {"GET", {
                "v5/account/wallet-balance",
                "v5/position/list",
                "v5/order/realtime",
                "v5/order/history",
                "v5/execution/list"
            }},
            {"POST", {
                "v5/order/create",
                "v5/order/cancel",
                "v5/order/cancel-all",
                "v5/position/set-leverage",
                "v5/position/switch-isolated",
                "v5/position/set-tpsl",
                "v5/position/set-risk-limit"
            }}
        }}
    };
}

json Bybit::fetchMarkets(const json& params) {
    json response = fetch("/v5/market/instruments-info", "public", "GET", params);
    json markets = json::array();
    
    for (const auto& category : {"spot", "linear", "inverse"}) {
        json request = {{"category", category}};
        json categoryResponse = fetch("/v5/market/instruments-info", "public", "GET", 
                                   this->extend(request, params));
        
        for (const auto& market : categoryResponse["result"]["list"]) {
            String id = market["symbol"];
            String baseId = market["baseCoin"];
            String quoteId = market["quoteCoin"];
            String base = this->commonCurrencyCode(baseId);
            String quote = this->commonCurrencyCode(quoteId);
            String type = category;
            
            markets.push_back({
                {"id", id},
                {"symbol", base + "/" + quote},
                {"base", base},
                {"quote", quote},
                {"baseId", baseId},
                {"quoteId", quoteId},
                {"active", market["status"] == "Trading"},
                {"type", type},
                {"spot", type == "spot"},
                {"margin", type != "spot"},
                {"future", false},
                {"swap", type != "spot"},
                {"option", false},
                {"contract", type != "spot"},
                {"linear", type == "linear"},
                {"inverse", type == "inverse"},
                {"contractSize", market.contains("lotSize") ? market["lotSize"].get<double>() : 1.0},
                {"precision", {
                    {"amount", market["lotSizeFilter"]["minOrderQty"].get<double>()},
                    {"price", market["priceFilter"]["tickSize"].get<double>()}
                }},
                {"limits", {
                    {"amount", {
                        {"min", market["lotSizeFilter"]["minOrderQty"]},
                        {"max", market["lotSizeFilter"]["maxOrderQty"]}
                    }},
                    {"price", {
                        {"min", market["priceFilter"]["minPrice"]},
                        {"max", market["priceFilter"]["maxPrice"]}
                    }},
                    {"cost", {
                        {"min", market["lotSizeFilter"]["minOrderAmt"]},
                        {"max", nullptr}
                    }}
                }},
                {"info", market}
            });
        }
    }
    
    return markets;
}

json Bybit::fetchTicker(const String& symbol, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"category", market.type},
        {"symbol", market.id}
    };
    
    json response = fetch("/v5/market/tickers", "public", "GET", 
                         this->extend(request, params));
    json ticker = response["result"]["list"][0];
    
    return {
        {"symbol", symbol},
        {"timestamp", this->safeInteger(ticker, "time")},
        {"datetime", this->iso8601(this->safeInteger(ticker, "time"))},
        {"high", this->safeFloat(ticker, "highPrice24h")},
        {"low", this->safeFloat(ticker, "lowPrice24h")},
        {"bid", this->safeFloat(ticker, "bid1Price")},
        {"bidVolume", this->safeFloat(ticker, "bid1Size")},
        {"ask", this->safeFloat(ticker, "ask1Price")},
        {"askVolume", this->safeFloat(ticker, "ask1Size")},
        {"vwap", nullptr},
        {"open", this->safeFloat(ticker, "prevPrice24h")},
        {"close", this->safeFloat(ticker, "lastPrice")},
        {"last", this->safeFloat(ticker, "lastPrice")},
        {"previousClose", nullptr},
        {"change", nullptr},
        {"percentage", this->safeFloat(ticker, "price24hPcnt")},
        {"average", nullptr},
        {"baseVolume", this->safeFloat(ticker, "volume24h")},
        {"quoteVolume", this->safeFloat(ticker, "turnover24h")},
        {"info", ticker}
    };
}

json Bybit::fetchBalance(const json& params) {
    this->loadMarkets();
    String type = this->safeString(params, "type", defaultType);
    
    json request = {
        {"accountType", type == "spot" ? "SPOT" : "CONTRACT"},
        {"coin", ""}  // Empty string to get all currencies
    };
    
    json response = fetch("/v5/account/wallet-balance", "private", "GET",
                         this->extend(request, params));
    json result = {
        {"info", response},
        {"timestamp", nullptr},
        {"datetime", nullptr}
    };
    
    for (const auto& balance : response["result"]["list"]) {
        String currency = balance["coin"];
        double total = std::stod(balance["walletBalance"].get<String>());
        double free = std::stod(balance["availableToWithdraw"].get<String>());
        double used = total - free;
        
        result[currency] = {
            {"free", free},
            {"used", used},
            {"total", total}
        };
    }
    
    return result;
}

json Bybit::createOrder(const String& symbol, const String& type,
                       const String& side, double amount,
                       double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"category", market.type},
        {"symbol", market.id},
        {"side", side.substr(0, 1).upper() + side.substr(1).lower()},  // Capitalize first letter
        {"orderType", type.upper()},
        {"qty", this->amountToPrecision(symbol, amount)}
    };
    
    if (type == "limit") {
        if (price == 0) {
            throw InvalidOrder("For limit orders, price cannot be zero");
        }
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    json response = fetch("/v5/order/create", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response["result"], market);
}

json Bybit::fetchPositions(const String& symbol, const json& params) {
    this->loadMarkets();
    json request = {};
    
    if (!symbol.empty()) {
        Market market = this->market(symbol);
        request["symbol"] = market.id;
        request["category"] = market.type;
    }
    
    json response = fetch("/v5/position/list", "private", "GET",
                         this->extend(request, params));
    return this->parsePositions(response["result"]["list"]);
}

String Bybit::sign(const String& path, const String& api,
                   const String& method, const json& params,
                   const std::map<String, String>& headers,
                   const json& body) {
    String endpoint = "/" + this->implodeParams(path, params);
    String url = this->urls["api"][api] + endpoint;
    
    if (api == "private") {
        String timestamp = std::to_string(this->nonce());
        String queryString = this->rawencode(this->keysort(params));
        String auth = timestamp + this->config_.apiKey + queryString;
        
        if (method == "POST") {
            auth += body.dump();
        }
        
        String signature = this->hmac(auth, this->config_.secret, "sha256", "hex");
        
        const_cast<std::map<String, String>&>(headers)["X-BAPI-API-KEY"] = this->config_.apiKey;
        const_cast<std::map<String, String>&>(headers)["X-BAPI-TIMESTAMP"] = timestamp;
        const_cast<std::map<String, String>&>(headers)["X-BAPI-SIGN"] = signature;
        
        if (method == "POST") {
            const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/json";
        }
    }
    
    return url;
}

String Bybit::createSignature(const String& timestamp, const String& method,
                            const String& path, const String& queryString,
                            const String& body) {
    String message = timestamp + this->config_.apiKey + queryString + body;
    
    unsigned char* hmac = nullptr;
    unsigned int hmacLen = 0;
    
    HMAC_CTX* ctx = HMAC_CTX_new();
    HMAC_Init_ex(ctx, secret.c_str(), secret.length(), EVP_sha256(), nullptr);
    HMAC_Update(ctx, (unsigned char*)message.c_str(), message.length());
    HMAC_Final(ctx, hmac, &hmacLen);
    HMAC_CTX_free(ctx);
    
    return this->toHex(hmac, hmacLen);
}

json Bybit::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"Created", "open"},
        {"New", "open"},
        {"PartiallyFilled", "open"},
        {"Filled", "closed"},
        {"Cancelled", "canceled"},
        {"Rejected", "rejected"}
    };
    
    return statuses.contains(status) ? statuses.at(status) : status;
}

json Bybit::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "orderId");
    String symbol = market.symbol;
    String timestamp = this->safeString(order, "createdTime");
    String status = this->parseOrderStatus(this->safeString(order, "orderStatus"));
    
    return {
        {"id", id},
        {"clientOrderId", this->safeString(order, "orderLinkId")},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"symbol", symbol},
        {"type", this->safeStringLower(order, "orderType")},
        {"side", this->safeStringLower(order, "side")},
        {"price", this->safeFloat(order, "price")},
        {"amount", this->safeFloat(order, "qty")},
        {"cost", this->safeFloat(order, "cumExecValue")},
        {"average", this->safeFloat(order, "avgPrice")},
        {"filled", this->safeFloat(order, "cumExecQty")},
        {"remaining", nullptr},
        {"status", status},
        {"fee", nullptr},
        {"trades", nullptr},
        {"info", order}
    };
}

} // namespace ccxt
