#include "bibox.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Bibox::Bibox() {
    id = "bibox";
    name = "Bibox";
    version = "v3";
    rateLimit = 200;

    // Initialize API endpoints
    baseUrl = "https://api.bibox.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/51840849/77257418-3262b000-6c85-11ea-8fb8-20bdf20b3592.jpg"},
        {"api", {
            {"public", "https://api.bibox.com"},
            {"private", "https://api.bibox.com"}
        }},
        {"www", "https://www.bibox.com"},
        {"doc", {
            "https://biboxcom.github.io/api/",
            "https://biboxcom.github.io/api-doc/spot/"
        }},
        {"fees", "https://bibox.zendesk.com/hc/en-us/articles/360002336133"}
    };

    timeframes = {
        {"1m", "1min"},
        {"5m", "5min"},
        {"15m", "15min"},
        {"30m", "30min"},
        {"1h", "1hour"},
        {"2h", "2hour"},
        {"4h", "4hour"},
        {"6h", "6hour"},
        {"12h", "12hour"},
        {"1d", "day"},
        {"1w", "week"}
    };

    options = {
        {"adjustForTimeDifference", true},
        {"recvWindow", "5000"}
    };

    errorCodes = {
        {2011, "Invalid symbol"},
        {2012, "Invalid amount"},
        {2013, "Invalid price"},
        {2014, "Invalid order type"},
        {2015, "Invalid side"},
        {2016, "Insufficient balance"},
        {2017, "Order does not exist"},
        {2018, "Order already cancelled"},
        {2019, "Order filled"},
        {2020, "Order partially filled"},
        {2021, "Order price too high"},
        {2022, "Order price too low"},
        {2023, "Order size too small"},
        {2024, "Order size too large"},
        {2025, "Invalid API key"},
        {2026, "Invalid signature"},
        {2027, "Invalid timestamp"},
        {2028, "Invalid recvWindow"},
        {2029, "Rate limit exceeded"}
    };

    initializeApiEndpoints();
}

void Bibox::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "v3/mdata/ping",
                "v3/mdata/time",
                "v3/mdata/marketSymbols",
                "v3/mdata/ticker",
                "v3/mdata/depth",
                "v3/mdata/deals",
                "v3/mdata/kline",
                "v3/mdata/marketAll"
            }}
        }},
        {"private", {
            {"POST", {
                "v3/orderpending/trade",
                "v3/orderpending/cancelTrade",
                "v3/orderpending/orderPendingList",
                "v3/orderpending/pendingHistoryList",
                "v3/orderpending/orderDetail",
                "v3/orderpending/order",
                "v3/transfer/mainAssets",
                "v3/transfer/coinConfig",
                "v3/transfer/transferIn",
                "v3/transfer/transferOut",
                "v3/transfer/transferInList",
                "v3/transfer/transferOutList",
                "v3/transfer/coinAddress"
            }}
        }}
    };
}

json Bibox::fetchMarkets(const json& params) {
    json response = publicRequest("/v3/mdata/marketSymbols", params);
    json result = json::array();
    
    for (const auto& market : response["result"]) {
        std::string id = this->safeString(market, "symbol");
        std::string baseId = this->safeString(market, "coin_symbol");
        std::string quoteId = this->safeString(market, "currency_symbol");
        std::string base = this->commonCurrencyCode(baseId);
        std::string quote = this->commonCurrencyCode(quoteId);
        std::string symbol = base + "/" + quote;
        
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
            {"swap", false},
            {"option", false},
            {"contract", false},
            {"precision", {
                {"amount", market["amount_precision"].get<int>()},
                {"price", market["price_precision"].get<int>()}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeFloat(market, "min_amount")},
                    {"max", this->safeFloat(market, "max_amount")}
                }},
                {"price", {
                    {"min", this->safeFloat(market, "min_price")},
                    {"max", this->safeFloat(market, "max_price")}
                }},
                {"cost", {
                    {"min", this->safeFloat(market, "min_amount") * this->safeFloat(market, "min_price")},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return result;
}

json Bibox::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = privateRequest("/v3/transfer/mainAssets", params);
    return parseBalance(response);
}

json Bibox::parseBalance(const json& response) {
    json result = {"info", response};
    json balances = response["result"];
    
    for (const auto& balance : balances) {
        std::string currencyId = balance["coin_symbol"];
        std::string code = this->commonCurrencyCode(currencyId);
        
        result[code] = {
            {"free", this->safeFloat(balance, "available")},
            {"used", this->safeFloat(balance, "freeze")},
            {"total", this->safeFloat(balance, "total")}
        };
    }
    
    return result;
}

json Bibox::createOrder(const std::string& symbol, const std::string& type,
                       const std::string& side, double amount,
                       double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"symbol", market.id},
        {"amount", this->amountToPrecision(symbol, amount)},
        {"side", side.upper()},
        {"type", type.upper()}
    };
    
    if (type == "limit") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    json response = privateRequest("/v3/orderpending/trade",
                                 this->extend(request, params));
    return this->parseOrder(response["result"], market);
}

std::string Bibox::sign(const std::string& path, const std::string& api,
                   const std::string& method, const json& params,
                   const std::map<std::string, std::string>& headers,
                   const json& body) {
    std::string url = this->urls["api"][api] + path;
    
    if (api == "public") {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    } else {
        this->checkRequiredCredentials();
        json request = this->extend({
            "apikey": this->config_.apiKey,
            "timestamp": std::to_string(this->milliseconds())
        }, params);
        
        std::string signature = this->createSignature(request);
        request["sign"] = signature;
        
        body = this->json(request);
        const_cast<std::map<std::string, std::string>&>(headers)["Content-Type"] = "application/json";
    }
    
    return url;
}

std::string Bibox::createSignature(const json& params) {
    std::string query = this->urlencode(this->keysort(params));
    return this->hmac(query, this->encode(this->config_.secret),
                     "md5", "hex");
}

json Bibox::parseOrder(const json& order, const Market& market) {
    std::string id = this->safeString(order, "id");
    std::string timestamp = this->safeString(order, "create_time");
    std::string status = this->parseOrderStatus(this->safeString(order, "status"));
    std::string symbol = market.symbol;
    std::string type = this->safeStringLower(order, "order_type");
    std::string side = this->safeStringLower(order, "order_side");
    
    return {
        {"id", id},
        {"clientOrderId", nullptr},
        {"timestamp", this->safeInteger(order, "create_time")},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"status", status},
        {"symbol", symbol},
        {"type", type},
        {"side", side},
        {"price", this->safeFloat(order, "price")},
        {"amount", this->safeFloat(order, "amount")},
        {"filled", this->safeFloat(order, "deal_amount")},
        {"remaining", this->safeFloat(order, "unexecuted")},
        {"cost", this->safeFloat(order, "deal_money")},
        {"trades", nullptr},
        {"fee", {
            {"currency", market.quote},
            {"cost", this->safeFloat(order, "fee")},
            {"rate", nullptr}
        }},
        {"info", order}
    };
}

json Bibox::parseOrderStatus(const std::string& status) {
    static const std::map<std::string, std::string> statuses = {
        {"1", "open"},
        {"2", "closed"},
        {"3", "canceled"},
        {"4", "canceled"},
        {"5", "canceled"},
        {"6", "canceled"}
    };
    
    return statuses.contains(status) ? statuses.at(status) : status;
}

} // namespace ccxt
