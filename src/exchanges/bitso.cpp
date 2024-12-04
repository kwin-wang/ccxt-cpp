#include "bitso.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Bitso::Bitso() {
    id = "bitso";
    name = "Bitso";
    version = "v3";
    rateLimit = 2000;
    certified = true;
    pro = false;

    // Initialize API endpoints
    baseUrl = "https://api.bitso.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/27766335-715ce7aa-5ed5-11e7-88a8-173a27bb30fe.jpg"},
        {"api", {
            {"public", "https://api.bitso.com"},
            {"private", "https://api.bitso.com"}
        }},
        {"www", "https://bitso.com"},
        {"doc", {
            "https://bitso.com/api_info",
            "https://bitso.com/developers"
        }},
        {"referral", "https://bitso.com/?ref=testuser"},
        {"fees", "https://bitso.com/fees"}
    };

    timeframes = {
        {"1m", "60"},
        {"5m", "300"},
        {"15m", "900"},
        {"30m", "1800"},
        {"1h", "3600"},
        {"4h", "14400"},
        {"12h", "43200"},
        {"1d", "86400"},
        {"1w", "604800"}
    };

    options = {
        {"adjustForTimeDifference", true},
        {"recvWindow", 5000},
        {"timeDifference", 0}
    };

    errorCodes = {
        {0, "Success"},
        {1, "General error"},
        {2, "Authentication error"},
        {3, "Invalid Request"},
        {4, "Rate limit exceeded"},
        {5, "Invalid parameters"},
        {6, "Resource not found"},
        {7, "Operation not allowed"},
        {8, "Insufficient funds"},
        {9, "Order not found"},
        {10, "Order already cancelled"},
        {11, "Order already filled"}
    };

    currencyIds = {
        {"BTC", "btc"},
        {"ETH", "eth"},
        {"XRP", "xrp"},
        {"LTC", "ltc"},
        {"BCH", "bch"},
        {"TUSD", "tusd"},
        {"MANA", "mana"},
        {"DAI", "dai"},
        {"MXN", "mxn"},
        {"USD", "usd"}
    };

    initializeApiEndpoints();
}

void Bitso::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "v3/available_books",
                "v3/ticker",
                "v3/order_book",
                "v3/trades",
                "v3/ohlc"
            }}
        }},
        {"private", {
            {"GET", {
                "v3/account_status",
                "v3/balance",
                "v3/fees",
                "v3/funding_destination",
                "v3/fundings",
                "v3/ledger",
                "v3/open_orders",
                "v3/orders",
                "v3/user_trades",
                "v3/withdrawals"
            }},
            {"POST", {
                "v3/orders",
                "v3/funding_destinations",
                "v3/spei_withdrawal",
                "v3/debit_card_withdrawal",
                "v3/crypto_withdrawal"
            }},
            {"DELETE", {
                "v3/orders/{oid}",
                "v3/orders/all"
            }}
        }}
    };
}

json Bitso::fetchMarkets(const json& params) {
    json response = fetch("/v3/available_books", "public", "GET", params);
    json result = json::array();
    
    for (const auto& market : response["payload"]) {
        String id = this->safeString(market, "book");
        String baseId = id.substr(0, 3);
        String quoteId = id.substr(3);
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
            {"active", true},
            {"type", "spot"},
            {"spot", true},
            {"future", false},
            {"option", false},
            {"margin", false},
            {"contract", false},
            {"precision", {
                {"amount", this->safeInteger(market, "amount_decimals")},
                {"price", this->safeInteger(market, "price_decimals")}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeFloat(market, "minimum_amount")},
                    {"max", this->safeFloat(market, "maximum_amount")}
                }},
                {"price", {
                    {"min", this->safeFloat(market, "minimum_price")},
                    {"max", this->safeFloat(market, "maximum_price")}
                }},
                {"cost", {
                    {"min", this->safeFloat(market, "minimum_value")},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return result;
}

json Bitso::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/v3/balance", "private", "GET", params);
    return parseBalance(response);
}

json Bitso::parseBalance(const json& response) {
    json result = {{"info", response}};
    json balances = this->safeValue(response, "payload", json::array());
    
    for (const auto& balance : balances) {
        String currencyId = this->safeString(balance, "currency");
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

json Bitso::createOrder(const String& symbol, const String& type,
                       const String& side, double amount,
                       double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"book", market["id"]},
        {"side", side},
        {"type", type},
        {"major", this->amountToPrecision(symbol, amount)}
    };
    
    if (type == "limit") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    json response = fetch("/v3/orders", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response["payload"], market);
}

String Bitso::sign(const String& path, const String& api,
                  const String& method, const json& params,
                  const std::map<String, String>& headers,
                  const json& body) {
    String url = this->urls["api"][api] + "/" + path;
    
    if (api == "public") {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    } else {
        this->checkRequiredCredentials();
        String nonce = this->nonce().str();
        String request = nonce + method + "/" + path;
        
        if (method == "POST") {
            if (!params.empty()) {
                body = this->json(params);
                request += body;
            }
        } else {
            if (!params.empty()) {
                String query = this->urlencode(params);
                url += "?" + query;
                request += "?" + query;
            }
        }
        
        String signature = this->hmac(request, this->encode(this->secret),
                                    "sha256", "hex");
        
        const_cast<std::map<String, String>&>(headers)["Authorization"] = "Bitso " + this->apiKey + ":" + nonce + ":" + signature;
        
        if (method == "POST") {
            const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/json";
        }
    }
    
    return url;
}

String Bitso::createNonce() {
    return std::to_string(this->milliseconds());
}

json Bitso::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "oid");
    String timestamp = this->safeString(order, "created_at");
    String status = this->parseOrderStatus(this->safeString(order, "status"));
    String symbol = nullptr;
    
    if (!market.empty()) {
        symbol = market["symbol"];
    }
    
    String type = this->safeString(order, "type");
    String side = this->safeString(order, "side");
    
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
        {"cost", this->safeFloat(order, "value")},
        {"amount", this->safeFloat(order, "original_amount")},
        {"filled", this->safeFloat(order, "filled_amount")},
        {"remaining", this->safeFloat(order, "unfilled_amount")},
        {"trades", nullptr},
        {"fee", {
            {"currency", market["quote"]},
            {"cost", this->safeFloat(order, "fees_amount")},
            {"rate", this->safeFloat(order, "fees_rate")}
        }},
        {"info", order}
    };
}

String Bitso::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"queued", "open"},
        {"active", "open"},
        {"partially filled", "open"},
        {"completed", "closed"},
        {"cancelled", "canceled"}
    };
    
    return this->safeString(statuses, status, status);
}

} // namespace ccxt
