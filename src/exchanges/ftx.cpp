#include "ftx.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

FTX::FTX() {
    id = "ftx";
    name = "FTX";
    version = "v1";
    rateLimit = 50;
    testnet = false;
    subAccountName = "";

    // Initialize API endpoints
    baseUrl = testnet ? "https://ftx.us/api" : "https://ftx.com/api";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/67149189-df896480-f2b0-11e9-8816-41593e17f9ec.jpg"},
        {"api", {
            {"public", baseUrl},
            {"private", baseUrl}
        }},
        {"www", "https://ftx.com"},
        {"doc", {
            "https://docs.ftx.com/",
            "https://ftx-us.zendesk.com/hc/en-us"
        }},
        {"fees", "https://ftx.com/fees"},
        {"test", "https://ftx.us/api"}
    };

    timeframes = {
        {"15s", "15"},
        {"1m", "60"},
        {"5m", "300"},
        {"15m", "900"},
        {"1h", "3600"},
        {"4h", "14400"},
        {"1d", "86400"}
    };

    options = {
        {"defaultType", "spot"},
        {"adjustForTimeDifference", true},
        {"recvWindow", "5000"}
    };

    initializeApiEndpoints();
}

void FTX::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "markets",
                "markets/{market_name}",
                "markets/{market_name}/orderbook",
                "markets/{market_name}/trades",
                "markets/{market_name}/candles",
                "futures",
                "funding_rates",
                "indexes/{index_name}/weights",
                "expired_futures",
                "indexes/{market_name}/candles",
                "stats/24h_volume",
                "stats/latency_stats",
                "stats/markets_summary"
            }}
        }},
        {"private", {
            {"GET", {
                "account",
                "positions",
                "wallet/balances",
                "wallet/all_balances",
                "wallet/deposit_address/{coin}",
                "wallet/deposits",
                "wallet/withdrawals",
                "orders",
                "orders/history",
                "fills",
                "funding_payments",
                "lt/balances",
                "lt/creations",
                "lt/redemptions",
                "subaccounts",
                "subaccounts/{nickname}/balances"
            }},
            {"POST", {
                "account/leverage",
                "orders",
                "orders/{order_id}/modify",
                "orders/{order_id}/trigger",
                "orders/by_client_id/{client_order_id}/modify",
                "conditional_orders",
                "conditional_orders/{order_id}/modify",
                "wallet/withdrawals",
                "wallet/saved_addresses",
                "lt/{token_name}/create",
                "lt/{token_name}/redeem",
                "subaccounts",
                "subaccounts/update_name",
                "subaccounts/transfer"
            }},
            {"DELETE", {
                "orders/{order_id}",
                "orders/by_client_id/{client_order_id}",
                "orders",
                "conditional_orders/{order_id}"
            }}
        }}
    };
}

json FTX::fetchMarkets(const json& params) {
    json response = fetch("/markets", "public", "GET", params);
    json markets = json::array();
    
    for (const auto& market : response["result"]) {
        std::string id = market["name"];
        std::string type = market["type"];
        std::string baseId = market["baseCurrency"];
        std::string quoteId = market["quoteCurrency"];
        std::string base = this->commonCurrencyCode(baseId);
        std::string quote = this->commonCurrencyCode(quoteId);
        bool active = market["enabled"];
        
        markets.push_back({
            {"id", id},
            {"symbol", base + "/" + quote},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", active},
            {"type", type},
            {"spot", type == "spot"},
            {"future", type == "future"},
            {"swap", type == "perpetual"},
            {"option", false},
            {"contract", type != "spot"},
            {"linear", type != "spot"},
            {"inverse", false},
            {"contractSize", market["contractSize"]},
            {"expiry", market["expiry"]},
            {"expiryDatetime", this->iso8601(market["expiry"])},
            {"precision", {
                {"amount", market["sizeIncrement"]},
                {"price", market["priceIncrement"]}
            }},
            {"limits", {
                {"amount", {
                    {"min", market["minProvideSize"]},
                    {"max", nullptr}
                }},
                {"price", {
                    {"min", market["priceIncrement"]},
                    {"max", nullptr}
                }},
                {"cost", {
                    {"min", market["minProvideSize"] * market["priceIncrement"]},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return markets;
}

json FTX::fetchBalance(const json& params) {
    this->loadMarkets();
    std::string subAccount = this->getSubAccountName(params);
    std::string path = subAccount.empty() ? "/wallet/balances" : "/subaccounts/" + subAccount + "/balances";
    
    json response = fetch(path, "private", "GET", params);
    json balances = response["result"];
    json result = {"info", response};
    
    for (const auto& balance : balances) {
        std::string currencyId = balance["coin"];
        std::string code = this->commonCurrencyCode(currencyId);
        
        result[code] = {
            {"free", balance["free"]},
            {"used", balance["total"] - balance["free"]},
            {"total", balance["total"]}
        };
    }
    
    return result;
}

json FTX::createOrder(const std::string& symbol, const std::string& type,
                     const std::string& side, double amount,
                     double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"market", market.id},
        {"side", side},
        {"price", type == "limit" ? this->priceToPrecision(symbol, price) : nullptr},
        {"type", type},
        {"size", this->amountToPrecision(symbol, amount)}
    };
    
    if (type == "limit") {
        request["postOnly"] = this->safeValue(params, "postOnly", false);
        request["ioc"] = this->safeValue(params, "ioc", false);
    }
    
    json response = fetch("/orders", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response["result"], market);
}

std::string FTX::sign(const std::string& path, const std::string& api,
                 const std::string& method, const json& params,
                 const std::map<std::string, std::string>& headers,
                 const json& body) {
    std::string url = this->urls["api"][api] + path;
    std::string timestamp = std::to_string(this->milliseconds());
    
    if (api == "public") {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    } else {
        this->checkRequiredCredentials();
        
        std::string auth = timestamp + method + path;
        
        if (method == "GET") {
            if (!params.empty()) {
                std::string query = this->urlencode(this->keysort(params));
                url += "?" + query;
                auth += "?" + query;
            }
        } else {
            if (!params.empty()) {
                body = this->json(params);
                auth += body.dump();
            }
        }
        
        std::string signature = this->hmac(auth, this->config_.secret, "sha256", "hex");
        
        const_cast<std::map<std::string, std::string>&>(headers)["FTX-KEY"] = this->config_.apiKey;
        const_cast<std::map<std::string, std::string>&>(headers)["FTX-SIGN"] = signature;
        const_cast<std::map<std::string, std::string>&>(headers)["FTX-TS"] = timestamp;
        
        if (!subAccountName.empty()) {
            const_cast<std::map<std::string, std::string>&>(headers)["FTX-SUBACCOUNT"] = 
                this->urlencode(subAccountName);
        }
        
        if (method != "GET") {
            const_cast<std::map<std::string, std::string>&>(headers)["Content-Type"] = "application/json";
        }
    }
    
    return url;
}

std::string FTX::getSubAccountName(const json& params) {
    return this->safeString(params, "subAccountName", subAccountName);
}

json FTX::parseOrder(const json& order, const Market& market) {
    std::string id = this->safeString(order, "id");
    std::string timestamp = this->safeInteger(order, "createdAt");
    std::string status = this->parseOrderStatus(this->safeString(order, "status"));
    
    return {
        {"id", id},
        {"clientOrderId", this->safeString(order, "clientId")},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"status", status},
        {"symbol", market.symbol},
        {"type", this->safeString(order, "type")},
        {"side", this->safeString(order, "side")},
        {"price", this->safeFloat(order, "price")},
        {"amount", this->safeFloat(order, "size")},
        {"filled", this->safeFloat(order, "filledSize")},
        {"remaining", this->safeFloat(order, "remainingSize")},
        {"cost", this->safeFloat(order, "filledSize") * this->safeFloat(order, "avgFillPrice")},
        {"average", this->safeFloat(order, "avgFillPrice")},
        {"trades", nullptr},
        {"fee", {
            {"cost", this->safeFloat(order, "fee")},
            {"currency", market.quote}
        }},
        {"info", order}
    };
}

json FTX::parseOrderStatus(const std::string& status) {
    static const std::map<std::string, std::string> statuses = {
        {"new", "open"},
        {"open", "open"},
        {"closed", "closed"},
        {"triggered", "closed"},
        {"cancelled", "canceled"},
        {"canceled", "canceled"}
    };
    
    return statuses.contains(status) ? statuses.at(status) : status;
}

} // namespace ccxt
