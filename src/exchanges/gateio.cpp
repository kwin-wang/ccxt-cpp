#include "gateio.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

GateIO::GateIO() {
    id = "gateio";
    name = "Gate.io";
    version = "4";
    rateLimit = 100;
    defaultType = "spot";
    settle = false;

    // Initialize API endpoints
    baseUrl = "https://api.gateio.ws";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/31784029-0313c702-b509-11e7-9ccc-bc0da6a0e435.jpg"},
        {"api", {
            {"public", "https://api.gateio.ws/api/v4"},
            {"private", "https://api.gateio.ws/api/v4"}
        }},
        {"www", "https://gate.io/"},
        {"doc", {
            "https://www.gate.io/docs/apiv4/en/index.html",
            "https://www.gate.io/api2"
        }},
        {"fees", "https://www.gate.io/fee"}
    };

    timeframes = {
        {"1m", "1m"},
        {"5m", "5m"},
        {"15m", "15m"},
        {"30m", "30m"},
        {"1h", "1h"},
        {"4h", "4h"},
        {"8h", "8h"},
        {"1d", "1d"},
        {"7d", "7d"}
    };

    initializeApiEndpoints();
}

void GateIO::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "spot/currency_pairs",
                "spot/currencies",
                "spot/tickers",
                "spot/order_book",
                "spot/trades",
                "spot/candlesticks",
                "futures/settle/contracts",
                "futures/settle/contract_stats",
                "futures/settle/positions",
                "futures/settle/position_close",
                "futures/settle/liquidates",
                "futures/settle/settlements"
            }}
        }},
        {"private", {
            {"GET", {
                "spot/accounts",
                "spot/orders",
                "spot/my_trades",
                "futures/settle/accounts",
                "futures/settle/positions",
                "futures/settle/orders"
            }},
            {"POST", {
                "spot/orders",
                "futures/settle/orders",
                "futures/settle/position_close",
                "futures/settle/leverage"
            }},
            {"DELETE", {
                "spot/orders",
                "futures/settle/orders",
                "futures/settle/position_close"
            }}
        }}
    };
}

json GateIO::fetchMarkets(const json& params) {
    json response = fetch("/spot/currency_pairs", "public", "GET", params);
    json markets = json::array();
    
    // Spot markets
    for (const auto& market : response) {
        String id = market["id"];
        String baseId = market["base"];
        String quoteId = market["quote"];
        String base = this->commonCurrencyCode(baseId);
        String quote = this->commonCurrencyCode(quoteId);
        
        markets.push_back({
            {"id", id},
            {"symbol", base + "/" + quote},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", market["trade_status"] == "tradable"},
            {"type", "spot"},
            {"spot", true},
            {"future", false},
            {"swap", false},
            {"precision", {
                {"amount", market["amount_precision"]},
                {"price", market["precision"]}
            }},
            {"limits", {
                {"amount", {
                    {"min", market["min_amount"]},
                    {"max", nullptr}
                }},
                {"price", {
                    {"min", market["min_price"]},
                    {"max", market["max_price"]}
                }},
                {"cost", {
                    {"min", market["min_amount"] * market["min_price"]},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    // Futures markets
    if (this->has["fetchFuturesMarkets"]) {
        for (const auto& settle : {"btc", "usdt", "usd"}) {
            json request = {{"settle", settle}};
            json futuresResponse = fetch("/futures/" + String(settle) + "/contracts", 
                                      "public", "GET", request);
            
            for (const auto& market : futuresResponse) {
                String id = market["name"];
                String baseId = market["underlying"];
                String quoteId = settle;
                String base = this->commonCurrencyCode(baseId);
                String quote = this->commonCurrencyCode(quoteId);
                String type = market["type"];
                
                markets.push_back({
                    {"id", id},
                    {"symbol", base + "/" + quote},
                    {"base", base},
                    {"quote", quote},
                    {"baseId", baseId},
                    {"quoteId", quoteId},
                    {"active", market["trade_status"] == "tradable"},
                    {"type", type},
                    {"spot", false},
                    {"future", type == "futures"},
                    {"swap", type == "swap"},
                    {"linear", settle == "usdt"},
                    {"inverse", settle == "btc"},
                    {"settle", settle},
                    {"contractSize", market["contract_size"]},
                    {"precision", {
                        {"amount", market["order_size_min"]},
                        {"price", market["order_price_round"]}
                    }},
                    {"limits", {
                        {"amount", {
                            {"min", market["order_size_min"]},
                            {"max", market["order_size_max"]}
                        }},
                        {"price", {
                            {"min", market["order_price_min"]},
                            {"max", market["order_price_max"]}
                        }},
                        {"leverage", {
                            {"min", market["leverage_min"]},
                            {"max", market["leverage_max"]}
                        }}
                    }},
                    {"info", market}
                });
            }
        }
    }
    
    return markets;
}

json GateIO::fetchTicker(const String& symbol, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {{"currency_pair", market.id}};
    json response = fetch("/spot/tickers", "public", "GET", 
                         this->extend(request, params));
    json ticker = response[0];
    
    return {
        {"symbol", symbol},
        {"timestamp", this->safeTimestamp(ticker, "timestamp")},
        {"datetime", this->iso8601(this->safeTimestamp(ticker, "timestamp"))},
        {"high", this->safeFloat(ticker, "high_24h")},
        {"low", this->safeFloat(ticker, "low_24h")},
        {"bid", this->safeFloat(ticker, "highest_bid")},
        {"bidVolume", nullptr},
        {"ask", this->safeFloat(ticker, "lowest_ask")},
        {"askVolume", nullptr},
        {"vwap", nullptr},
        {"open", this->safeFloat(ticker, "open_24h")},
        {"close", this->safeFloat(ticker, "last")},
        {"last", this->safeFloat(ticker, "last")},
        {"previousClose", nullptr},
        {"change", this->safeFloat(ticker, "change_percentage")},
        {"percentage", this->safeFloat(ticker, "change_percentage")},
        {"average", nullptr},
        {"baseVolume", this->safeFloat(ticker, "base_volume")},
        {"quoteVolume", this->safeFloat(ticker, "quote_volume")},
        {"info", ticker}
    };
}

json GateIO::fetchBalance(const json& params) {
    this->loadMarkets();
    String type = this->safeString(params, "type", defaultType);
    
    String path = type == "spot" ? "/spot/accounts" : 
                 "/futures/" + String(settle ? "settle" : "usdt") + "/accounts";
    
    json response = fetch(path, "private", "GET", params);
    json result = {
        {"info", response},
        {"timestamp", nullptr},
        {"datetime", nullptr}
    };
    
    for (const auto& balance : response) {
        String currency = balance["currency"];
        double total = std::stod(balance["available"].get<String>()) + 
                      std::stod(balance["locked"].get<String>());
        double free = std::stod(balance["available"].get<String>());
        double used = std::stod(balance["locked"].get<String>());
        
        result[currency] = {
            {"free", free},
            {"used", used},
            {"total", total}
        };
    }
    
    return result;
}

json GateIO::createOrder(const String& symbol, const String& type,
                        const String& side, double amount,
                        double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"currency_pair", market.id},
        {"side", side},
        {"type", type},
        {"amount", this->amountToPrecision(symbol, amount)}
    };
    
    if (type == "limit") {
        if (price == 0) {
            throw InvalidOrder("For limit orders, price cannot be zero");
        }
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    String path = market.type == "spot" ? "/spot/orders" : 
                 "/futures/" + String(settle ? "settle" : "usdt") + "/orders";
    
    json response = fetch(path, "private", "POST", this->extend(request, params));
    return this->parseOrder(response, market);
}

String GateIO::sign(const String& path, const String& api,
                    const String& method, const json& params,
                    const std::map<String, String>& headers,
                    const json& body) {
    String endpoint = "/" + this->version + path;
    String url = this->urls["api"][api] + endpoint;
    
    if (api == "private") {
        String timestamp = std::to_string(this->nonce());
        String queryString = this->rawencode(this->keysort(params));
        String auth = timestamp + method + endpoint;
        
        if (!queryString.empty()) {
            auth += "?" + queryString;
        }
        
        if (method == "POST" || method == "PUT" || method == "DELETE") {
            if (!body.empty()) {
                auth += body.dump();
            }
        }
        
        String signature = this->hmac(auth, this->secret, "sha512", "hex");
        
        const_cast<std::map<String, String>&>(headers)["KEY"] = this->apiKey;
        const_cast<std::map<String, String>&>(headers)["Timestamp"] = timestamp;
        const_cast<std::map<String, String>&>(headers)["SIGN"] = signature;
        
        if (method == "POST" || method == "PUT" || method == "DELETE") {
            const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/json";
        }
    }
    
    return url;
}

String GateIO::createSignature(const String& method, const String& path,
                             const String& queryString, const String& body) {
    String message = method + "\n" + path + "\n" + queryString + "\n" + body + "\n";
    
    unsigned char* hmac = nullptr;
    unsigned int hmacLen = 0;
    
    HMAC_CTX* ctx = HMAC_CTX_new();
    HMAC_Init_ex(ctx, secret.c_str(), secret.length(), EVP_sha512(), nullptr);
    HMAC_Update(ctx, (unsigned char*)message.c_str(), message.length());
    HMAC_Final(ctx, hmac, &hmacLen);
    HMAC_CTX_free(ctx);
    
    return this->toHex(hmac, hmacLen);
}

json GateIO::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"open", "open"},
        {"closed", "closed"},
        {"cancelled", "canceled"}
    };
    
    return statuses.contains(status) ? statuses.at(status) : status;
}

json GateIO::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "id");
    String timestamp = this->safeString(order, "create_time");
    String status = this->parseOrderStatus(this->safeString(order, "status"));
    
    return {
        {"id", id},
        {"clientOrderId", this->safeString(order, "text")},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"status", status},
        {"symbol", market.symbol},
        {"type", this->safeString(order, "type")},
        {"side", this->safeString(order, "side")},
        {"price", this->safeFloat(order, "price")},
        {"amount", this->safeFloat(order, "amount")},
        {"filled", this->safeFloat(order, "filled_total")},
        {"remaining", this->safeFloat(order, "left")},
        {"cost", this->safeFloat(order, "filled_total") * this->safeFloat(order, "avg_deal_price")},
        {"average", this->safeFloat(order, "avg_deal_price")},
        {"fee", {
            {"cost", this->safeFloat(order, "fee")},
            {"currency", market.quote}
        }},
        {"trades", nullptr},
        {"info", order}
    };
}

} // namespace ccxt
