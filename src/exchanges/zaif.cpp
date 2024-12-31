#include "zaif.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Zaif::Zaif() {
    id = "zaif";
    name = "Zaif";
    version = "1";
    rateLimit = 2000;
    certified = true;
    pro = false;

    // Initialize API endpoints
    baseUrl = "https://api.zaif.jp";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/27766927-39ca2ada-5eeb-11e7-972f-1b4199518ca6.jpg"},
        {"api", {
            {"public", "https://api.zaif.jp/api/1"},
            {"private", "https://api.zaif.jp/tapi"},
            {"futures", "https://api.zaif.jp/fapi/1"}
        }},
        {"www", "https://zaif.jp"},
        {"doc", {
            "https://techbureau-api-document.readthedocs.io/ja/latest/index.html",
            "https://corp.zaif.jp/api-docs",
            "https://corp.zaif.jp/api-docs/api_links",
            "https://www.npmjs.com/package/zaif.jp"
        }},
        {"fees", "https://zaif.jp/fee"}
    };

    timeframes = {
        {"1m", "1min"},
        {"5m", "5min"},
        {"15m", "15min"},
        {"30m", "30min"},
        {"1h", "1hour"},
        {"4h", "4hour"},
        {"8h", "8hour"},
        {"12h", "12hour"},
        {"1d", "1day"},
        {"1w", "1week"}
    };

    options = {
        {"adjustForTimeDifference", true},
        {"fetchOrdersMethod", "private_post_active_orders"}
    };

    errorCodes = {
        {1, "System error"},
        {2, "Invalid parameter"},
        {3, "Authentication failed"},
        {4, "Authentication required"},
        {5, "Invalid address"},
        {6, "Invalid amount"},
        {7, "Order not found"},
        {8, "Order canceled"},
        {9, "Market not found"},
        {10, "Market suspended"},
        {11, "Market closed"},
        {12, "Insufficient funds"},
        {13, "Invalid order type"},
        {14, "Invalid side"},
        {15, "Invalid leverage level"},
        {16, "Invalid position"},
        {17, "Position not found"},
        {18, "Position closed"},
        {19, "Position liquidated"},
        {20, "Margin call"}
    };

    initializeApiEndpoints();
}

void Zaif::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "depth/{pair}",
                "ticker/{pair}",
                "trades/{pair}",
                "currency_pairs/{pair}",
                "currencies/{currency}",
                "exchange/orders/rate"
            }}
        }},
        {"private", {
            {"POST", {
                "get_info",
                "trade",
                "active_orders",
                "cancel_order",
                "withdraw",
                "deposit_history",
                "withdraw_history",
                "trade_history",
                "margin/positions",
                "margin/position_history",
                "margin/active_positions"
            }}
        }},
        {"futures", {
            {"GET", {
                "groups/{group_id}",
                "last_price/{group_id}/{pair}",
                "ticker/{group_id}/{pair}",
                "trades/{group_id}/{pair}",
                "depth/{group_id}/{pair}"
            }},
            {"POST", {
                "create_position",
                "change_position",
                "cancel_position",
                "position_history",
                "active_positions"
            }}
        }}
    };
}

json Zaif::fetchMarkets(const json& params) {
    json response = fetch("/currency_pairs/all", "public", "GET", params);
    json result = json::array();
    
    for (const auto& market : response) {
        std::string id = this->safeString(market, "currency_pair");
        std::string baseId = this->safeString(market, "base_currency");
        std::string quoteId = this->safeString(market, "quote_currency");
        std::string base = this->safeCurrencyCode(baseId);
        std::string quote = this->safeCurrencyCode(quoteId);
        bool isToken = this->safeValue(market, "is_token", false);
        
        result.push_back({
            {"id", id},
            {"symbol", base + "/" + quote},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", true},
            {"type", isToken ? "token" : "spot"},
            {"spot", !isToken},
            {"margin", true},
            {"future", false},
            {"option", false},
            {"contract", false},
            {"precision", {
                {"amount", this->safeInteger(market, "item_unit_step")},
                {"price", this->safeInteger(market, "aux_unit_step")}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeFloat(market, "item_unit_min")},
                    {"max", nullptr}
                }},
                {"price", {
                    {"min", this->safeFloat(market, "aux_unit_min")},
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

json Zaif::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/get_info", "private", "POST", params);
    return parseBalance(response);
}

json Zaif::parseBalance(const json& response) {
    json data = this->safeValue(response, "return", {});
    json funds = this->safeValue(data, "funds", {});
    json deposit = this->safeValue(data, "deposit", {});
    json result = {{"info", response}};
    
    for (const auto& [currency, balance] : funds.items()) {
        std::string code = this->safeCurrencyCode(currency);
        json account = {
            {"free", this->safeFloat(funds, currency)},
            {"used", 0.0},
            {"total", this->safeFloat(deposit, currency)}
        };
        account["used"] = account["total"].get<double>() - account["free"].get<double>();
        result[code] = account;
    }
    
    return result;
}

json Zaif::createOrder(const std::string& symbol, const std::string& type,
                      const std::string& side, double amount,
                      double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"currency_pair", market["id"]},
        {"action", side},
        {"amount", this->amountToPrecision(symbol, amount)}
    };
    
    if (type == "limit") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    json response = fetch("/trade", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response["return"], market);
}

std::string Zaif::sign(const std::string& path, const std::string& api,
                 const std::string& method, const json& params,
                 const std::map<std::string, std::string>& headers,
                 const json& body) {
    std::string url = this->urls["api"][api];
    std::string query = this->omit(params, this->extractParams(path));
    
    if (api == "public") {
        url += this->implodeParams(path, params);
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->checkRequiredCredentials();
        std::string nonce = this->nonce().str();
        json request = this->extend({
            "method", path,
            "nonce", nonce
        }, query);
        
        std::string body = this->urlencode(request);
        std::string signature = this->hmac(body, this->encode(this->config_.secret),
                                    "sha512", "hex");
        
        const_cast<std::map<std::string, std::string>&>(headers)["Key"] = this->config_.apiKey;
        const_cast<std::map<std::string, std::string>&>(headers)["Sign"] = signature;
        
        if (method == "POST") {
            const_cast<std::map<std::string, std::string>&>(headers)["Content-Type"] = "application/x-www-form-urlencoded";
        }
    }
    
    return url;
}

std::string Zaif::createNonce() {
    return std::to_string(this->milliseconds());
}

json Zaif::parseOrder(const json& order, const Market& market) {
    std::string timestamp = this->safeString(order, "timestamp");
    std::string status = this->parseOrderStatus(this->safeString(order, "status"));
    std::string symbol = nullptr;
    
    if (!market.empty()) {
        symbol = market["symbol"];
    }
    
    std::string type = this->safeString(order, "type");
    std::string side = this->safeString(order, "action");
    
    return {
        {"id", this->safeString(order, "order_id")},
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
        {"amount", this->safeFloat(order, "amount")},
        {"filled", this->safeFloat(order, "executed")},
        {"remaining", this->safeFloat(order, "remaining")},
        {"trades", nullptr},
        {"fee", nullptr},
        {"info", order}
    };
}

std::string Zaif::parseOrderStatus(const std::string& status) {
    static const std::map<std::string, std::string> statuses = {
        {"active", "open"},
        {"cancelled", "canceled"},
        {"executed", "closed"},
        {"partially_executed", "open"}
    };
    
    return this->safeString(statuses, status, status);
}

} // namespace ccxt
