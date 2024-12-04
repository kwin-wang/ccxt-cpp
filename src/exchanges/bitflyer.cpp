#include "bitflyer.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Bitflyer::Bitflyer() {
    id = "bitflyer";
    name = "bitFlyer";
    version = "1";
    rateLimit = 1000;
    certified = true;
    pro = false;
    hasPublicAPI = true;
    hasPrivateAPI = true;
    hasFuturesAPI = true;
    hasMarginAPI = true;

    // Initialize API endpoints
    baseUrl = "https://api.bitflyer.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/28051642-56154182-660e-11e7-9b0d-6042d1e6edd8.jpg"},
        {"api", {
            {"public", "https://api.bitflyer.com"},
            {"private", "https://api.bitflyer.com"}
        }},
        {"www", "https://bitflyer.com"},
        {"doc", {
            "https://lightning.bitflyer.com/docs?lang=en",
            "https://lightning.bitflyer.com/docs?lang=ja"
        }},
        {"fees", "https://bitflyer.com/en-us/commission"}
    };

    timeframes = {
        {"1m", "1"},
        {"3m", "3"},
        {"5m", "5"},
        {"15m", "15"},
        {"30m", "30"},
        {"1h", "60"},
        {"4h", "240"},
        {"6h", "360"},
        {"12h", "720"},
        {"1d", "1440"}
    };

    options = {
        {"adjustForTimeDifference", true},
        {"recvWindow", 5000},
        {"timeDifference", 0},
        {"defaultType", "spot"}
    };

    errorCodes = {
        {1000, "System error"},
        {1001, "Invalid parameter"},
        {1002, "Authentication failed"},
        {1003, "Invalid session"},
        {1004, "Too many requests"},
        {1005, "Invalid timestamp"},
        {1006, "Invalid signature"},
        {1007, "Account not found"},
        {1008, "Insufficient funds"},
        {1009, "Order not found"},
        {1010, "Market not found"},
        {1011, "Price out of range"},
        {1012, "Size out of range"},
        {1013, "Cancel not accepted"},
        {1014, "Already canceled"},
        {1015, "Invalid order type"},
        {1016, "Invalid side"},
        {1017, "Trading temporarily suspended"},
        {1018, "Market temporarily suspended"},
        {1019, "Market not available"},
        {1020, "Order not accepted"},
        {1021, "Rate limit exceeded"},
        {1022, "Position not found"},
        {1023, "Position size out of range"},
        {1024, "Position not available"},
        {1025, "Position temporarily suspended"},
        {1026, "Margin amount out of range"},
        {1027, "Margin ratio out of range"},
        {1028, "Maximum leverage out of range"},
        {1029, "Maintenance margin ratio out of range"}
    };

    initializeApiEndpoints();
}

void Bitflyer::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "getmarkets/eu",
                "getmarkets/usa",
                "getmarkets",
                "getboard",
                "getticker",
                "getexecutions",
                "gethealth",
                "getchats",
                "getboardstate",
                "getmarkets/products"
            }}
        }},
        {"private", {
            {"GET", {
                "me/getpermissions",
                "me/getbalance",
                "me/getcollateral",
                "me/getcollateralaccounts",
                "me/getaddresses",
                "me/getcoinins",
                "me/getcoinouts",
                "me/getbankaccounts",
                "me/getdeposits",
                "me/getwithdrawals",
                "me/getchildorders",
                "me/getparentorders",
                "me/getparentorder",
                "me/getexecutions",
                "me/getpositions",
                "me/gettradingcommission"
            }},
            {"POST", {
                "me/sendcoin",
                "me/withdraw",
                "me/sendchildorder",
                "me/cancelchildorder",
                "me/sendparentorder",
                "me/cancelparentorder",
                "me/cancelallchildorders"
            }}
        }}
    };
}

json Bitflyer::fetchMarkets(const json& params) {
    json response = fetch("/getmarkets", "public", "GET", params);
    json result = json::array();
    
    for (const auto& market : response) {
        String id = market["product_code"].get<String>();
        String type = "spot";
        bool spot = true;
        bool future = false;
        bool margin = false;
        
        if (id.find("_") != String::npos) {
            if (id.find("FX_") != String::npos) {
                type = "margin";
                spot = false;
                margin = true;
            } else {
                type = "future";
                spot = false;
                future = true;
            }
        }
        
        std::vector<String> parts;
        std::stringstream ss(id);
        String part;
        while (std::getline(ss, part, '_')) {
            parts.push_back(part);
        }
        
        String baseId = parts[0];
        String quoteId = parts.size() > 1 ? parts[1] : "JPY";
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
            {"type", type},
            {"spot", spot},
            {"margin", margin},
            {"future", future},
            {"active", true},
            {"precision", {
                {"amount", 8},
                {"price", 8}
            }},
            {"limits", {
                {"amount", {
                    {"min", market["min_trade_size"].get<double>()},
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
    
    return result;
}

json Bitflyer::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/me/getbalance", "private", "GET", params);
    return parseBalance(response);
}

json Bitflyer::parseBalance(const json& response) {
    json result = {{"info", response}};
    
    for (const auto& balance : response) {
        String currencyId = balance["currency_code"].get<String>();
        String code = this->safeCurrencyCode(currencyId);
        String account = {
            {"free", this->safeFloat(balance, "available")},
            {"used", this->safeFloat(balance, "amount") - this->safeFloat(balance, "available")},
            {"total", this->safeFloat(balance, "amount")}
        };
        result[code] = account;
    }
    
    return result;
}

json Bitflyer::createOrder(const String& symbol, const String& type,
                          const String& side, double amount,
                          double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"product_code", market["id"]},
        {"child_order_type", type.toUpperCase()},
        {"side", side.toUpperCase()},
        {"size", this->amountToPrecision(symbol, amount)}
    };
    
    if (type == "LIMIT") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    json response = fetch("/me/sendchildorder", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response, market);
}

String Bitflyer::sign(const String& path, const String& api,
                      const String& method, const json& params,
                      const std::map<String, String>& headers,
                      const json& body) {
    String url = this->urls["api"][api] + path;
    
    if (api == "private") {
        this->checkRequiredCredentials();
        String timestamp = std::to_string(this->milliseconds());
        String auth = timestamp + method + path;
        
        if (method == "POST") {
            body = this->json(params);
            auth += body;
        } else {
            if (!params.empty()) {
                String query = this->urlencode(this->keysort(params));
                url += "?" + query;
                auth += "?" + query;
            }
        }
        
        String signature = this->hmac(auth, this->encode(this->secret),
                                    "sha256", "hex");
        
        const_cast<std::map<String, String>&>(headers)["ACCESS-KEY"] = this->apiKey;
        const_cast<std::map<String, String>&>(headers)["ACCESS-TIMESTAMP"] = timestamp;
        const_cast<std::map<String, String>&>(headers)["ACCESS-SIGN"] = signature;
        const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/json";
    } else {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    }
    
    return url;
}

String Bitflyer::getNonce() {
    return std::to_string(this->milliseconds());
}

json Bitflyer::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "child_order_acceptance_id");
    String timestamp = this->safeString(order, "child_order_date");
    String status = this->parseOrderStatus(this->safeString(order, "child_order_state"));
    String symbol = nullptr;
    
    if (!market.empty()) {
        symbol = market["symbol"];
    }
    
    String type = this->safeStringLower(order, "child_order_type");
    String side = this->safeStringLower(order, "side");
    
    return {
        {"id", id},
        {"clientOrderId", nullptr},
        {"datetime", this->iso8601(timestamp)},
        {"timestamp", this->parse8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"type", type},
        {"timeInForce", nullptr},
        {"postOnly", nullptr},
        {"status", status},
        {"symbol", symbol},
        {"side", side},
        {"price", this->safeFloat(order, "price")},
        {"stopPrice", nullptr},
        {"cost", nullptr},
        {"amount", this->safeFloat(order, "size")},
        {"filled", this->safeFloat(order, "executed_size")},
        {"remaining", this->safeFloat(order, "outstanding_size")},
        {"trades", nullptr},
        {"fee", {
            {"currency", market["quote"]},
            {"cost", this->safeFloat(order, "total_commission")},
            {"rate", nullptr}
        }},
        {"info", order}
    };
}

String Bitflyer::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"ACTIVE", "open"},
        {"COMPLETED", "closed"},
        {"CANCELED", "canceled"},
        {"EXPIRED", "expired"},
        {"REJECTED", "rejected"}
    };
    
    return this->safeString(statuses, status, status);
}

} // namespace ccxt
