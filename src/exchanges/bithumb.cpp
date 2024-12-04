#include "bithumb.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Bithumb::Bithumb() {
    id = "bithumb";
    name = "Bithumb";
    version = "1";
    rateLimit = 500;
    isPublicAPI = true;

    // Initialize API endpoints
    baseUrl = "https://api.bithumb.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/51840849/77257418-3262b000-6c85-11ea-8fb8-20bdf20b3592.jpg"},
        {"api", {
            {"public", baseUrl},
            {"private", baseUrl}
        }},
        {"www", "https://www.bithumb.com"},
        {"doc", {
            "https://apidocs.bithumb.com",
            "https://api.bithumb.com/apidoc"
        }},
        {"fees", "https://www.bithumb.com/customer_support/info_fee"}
    };

    timeframes = {
        {"1m", "1m"},
        {"3m", "3m"},
        {"5m", "5m"},
        {"10m", "10m"},
        {"30m", "30m"},
        {"1h", "1h"},
        {"6h", "6h"},
        {"12h", "12h"},
        {"1d", "24h"},
        {"1w", "1w"},
        {"1M", "1M"}
    };

    options = {
        {"recvWindow", "5000"},
        {"adjustForTimeDifference", true}
    };

    initializeApiEndpoints();
}

void Bithumb::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "public/ticker/{currency}",
                "public/ticker/all",
                "public/orderbook/{currency}",
                "public/orderbook/all",
                "public/transaction_history/{currency}",
                "public/candlestick/{currency}/{interval}",
                "public/assetsstatus",
                "public/btci"
            }}
        }},
        {"private", {
            {"POST", {
                "info/account",
                "info/balance",
                "info/wallet_address",
                "info/ticker",
                "info/orders",
                "info/user_transactions",
                "trade/place",
                "trade/cancel",
                "trade/btc_withdrawal",
                "trade/krw_withdrawal",
                "trade/market_buy",
                "trade/market_sell"
            }}
        }}
    };
}

json Bithumb::fetchMarkets(const json& params) {
    json response = fetch("/public/ticker/all", "public", "GET", params);
    json data = response["data"];
    json markets = json::array();
    
    for (const auto& item : data.items()) {
        if (item.key() == "date") continue;
        
        String id = item.key();
        String baseId = id;
        String quoteId = "KRW";
        String base = this->commonCurrencyCode(baseId);
        String quote = this->commonCurrencyCode(quoteId);
        String symbol = base + "/" + quote;
        
        markets.push_back({
            {"id", id},
            {"symbol", symbol},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", true},
            {"type", "spot"},
            {"spot", true},
            {"future", false},
            {"swap", false},
            {"option", false},
            {"contract", false},
            {"precision", {
                {"amount", 4},
                {"price", 4}
            }},
            {"limits", {
                {"amount", {
                    {"min", 0.0001},
                    {"max", nullptr}
                }},
                {"price", {
                    {"min", nullptr},
                    {"max", nullptr}
                }},
                {"cost", {
                    {"min", 500},  // 500 KRW
                    {"max", nullptr}
                }}
            }},
            {"info", item.value()}
        });
    }
    
    return markets;
}

json Bithumb::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/info/balance", "private", "POST", params);
    json balances = response["data"];
    json result = {"info", response};
    
    for (const auto& item : balances.items()) {
        String currencyId = item.key();
        if (currencyId.find("total_") == 0) {
            String code = this->commonCurrencyCode(currencyId.substr(6));
            String account = this->account();
            
            account["total"] = this->safeString(balances, currencyId);
            account["free"] = this->safeString(balances, code.lower() + "_available");
            account["used"] = this->safeString(balances, code.lower() + "_in_use");
            
            result[code] = account;
        }
    }
    
    return this->parseBalance(result);
}

json Bithumb::createOrder(const String& symbol, const String& type,
                         const String& side, double amount,
                         double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"order_currency", market.baseId},
        {"payment_currency", market.quoteId},
        {"units", this->amountToPrecision(symbol, amount)}
    };
    
    String method = "trade/";
    if (type == "limit") {
        method += side;
        request["price"] = this->priceToPrecision(symbol, price);
    } else {
        method += "market_" + side;
    }
    
    json response = fetch("/" + method, "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response["data"], market);
}

String Bithumb::sign(const String& path, const String& api,
                     const String& method, const json& params,
                     const std::map<String, String>& headers,
                     const json& body) {
    String endpoint = "/" + this->implodeParams(path, params);
    String url = this->urls["api"][api] + endpoint;
    
    if (api == "public") {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    } else {
        this->checkRequiredCredentials();
        
        String nonce = this->getNonce();
        String query = this->urlencode(this->extend({
            "endpoint": endpoint
        }, params));
        
        String auth = endpoint + ";" + query + ";" + nonce;
        String signature = this->hmac(auth, this->base64ToBinary(this->secret),
                                    "sha512", "hex");
        
        const_cast<std::map<String, String>&>(headers)["Api-Key"] = this->apiKey;
        const_cast<std::map<String, String>&>(headers)["Api-Sign"] = signature;
        const_cast<std::map<String, String>&>(headers)["Api-Nonce"] = nonce;
        const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/x-www-form-urlencoded";
        
        body = query;
    }
    
    return url;
}

String Bithumb::getNonce() {
    return std::to_string(this->milliseconds());
}

json Bithumb::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "order_id");
    String timestamp = this->safeString(order, "order_date");
    if (timestamp.empty()) {
        timestamp = this->milliseconds();
    }
    
    String type = "limit";  // Bithumb只支持限价单
    String side = this->parseOrderSide(this->safeString(order, "type"));
    String status = this->parseOrderStatus(this->safeString(order, "status"));
    
    return {
        {"id", id},
        {"clientOrderId", nullptr},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"status", status},
        {"symbol", market.symbol},
        {"type", type},
        {"side", side},
        {"price", this->safeFloat(order, "price")},
        {"amount", this->safeFloat(order, "units")},
        {"filled", this->safeFloat(order, "units_traded")},
        {"remaining", nullptr},
        {"cost", nullptr},
        {"trades", nullptr},
        {"fee", nullptr},
        {"info", order}
    };
}

json Bithumb::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"placed", "open"},
        {"completed", "closed"},
        {"cancelled", "canceled"},
        {"cancel", "canceled"}
    };
    
    return statuses.contains(status) ? statuses.at(status) : status;
}

json Bithumb::parseOrderSide(const String& orderType) {
    if (orderType == "ask" || orderType == "sell") {
        return "sell";
    } else if (orderType == "bid" || orderType == "buy") {
        return "buy";
    }
    return orderType;
}

} // namespace ccxt
