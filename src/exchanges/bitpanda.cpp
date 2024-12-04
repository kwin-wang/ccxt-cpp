#include "bitpanda.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Bitpanda::Bitpanda() {
    id = "bitpanda";
    name = "Bitpanda Pro";
    version = "v1";
    certified = true;
    pro = true;
    hasPublicAPI = true;
    hasPrivateAPI = true;
    hasFiatAPI = true;
    hasMarginAPI = false;
    hasFuturesAPI = false;

    // Initialize URLs
    baseUrl = "https://api.exchange.bitpanda.com/public/v1";
    urls = {
        {"logo", "https://user-images.githubusercontent.com/51840849/87591171-9a377d80-c6f0-11ea-94ac-97a126eac3bc.jpg"},
        {"api", {
            {"public", "https://api.exchange.bitpanda.com/public/v1"},
            {"private", "https://api.exchange.bitpanda.com/public/v1"}
        }},
        {"www", "https://www.bitpanda.com/en/pro"},
        {"doc", {
            "https://developers.bitpanda.com/exchange/",
            "https://api.exchange.bitpanda.com/public/v1"
        }},
        {"fees", "https://www.bitpanda.com/en/pro/fees"}
    };

    initializeApiEndpoints();
    initializeTimeframes();
    initializeMarketTypes();
    initializeOptions();
    initializeErrorCodes();
    initializeFees();
}

void Bitpanda::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "time",
                "currencies",
                "candlesticks/{instrument_code}",
                "fees",
                "instruments",
                "order-book/{instrument_code}",
                "market-ticker",
                "market-ticker/{instrument_code}",
                "price-ticks/{instrument_code}",
                "trades/{instrument_code}"
            }}
        }},
        {"private", {
            {"GET", {
                "account/balances",
                "account/deposit/crypto/{currency}",
                "account/deposit/fiat/EUR",
                "account/deposits",
                "account/orders",
                "account/orders/{order_id}",
                "account/orders/trades",
                "account/trades",
                "account/trading-volume",
                "account/withdrawals"
            }},
            {"POST", {
                "account/deposit/crypto",
                "account/withdraw/crypto",
                "account/withdraw/fiat",
                "account/orders"
            }},
            {"DELETE", {
                "account/orders",
                "account/orders/{order_id}"
            }}
        }}
    };
}

void Bitpanda::initializeTimeframes() {
    timeframes = {
        {"1m", "1"},
        {"5m", "5"},
        {"15m", "15"},
        {"30m", "30"},
        {"1h", "60"},
        {"4h", "240"},
        {"1d", "1D"},
        {"1w", "1W"},
        {"1M", "1M"}
    };
}

json Bitpanda::fetchMarkets(const json& params) {
    json response = fetch("/instruments", "public", "GET", params);
    json result = json::array();
    
    for (const auto& market : response) {
        String id = market["instrument_code"].get<String>();
        String baseId = market["base"]["code"].get<String>();
        String quoteId = market["quote"]["code"].get<String>();
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
            {"active", market["state"].get<String>() == "ACTIVE"},
            {"precision", {
                {"amount", market["amount_precision"].get<int>()},
                {"price", market["price_precision"].get<int>()}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeFloat(market, "min_size")},
                    {"max", this->safeFloat(market, "max_size")}
                }},
                {"price", {
                    {"min", this->safeFloat(market, "min_price")},
                    {"max", this->safeFloat(market, "max_price")}
                }},
                {"cost", {
                    {"min", this->safeFloat(market, "min_value")},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return result;
}

json Bitpanda::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/account/balances", "private", "GET", params);
    return parseBalance(response);
}

json Bitpanda::parseBalance(const json& response) {
    json result = {{"info", response}};
    
    for (const auto& balance : response) {
        String currencyId = balance["currency_code"].get<String>();
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

json Bitpanda::createOrder(const String& symbol, const String& type,
                          const String& side, double amount,
                          double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    String uppercaseType = type.toUpperCase();
    
    json request = {
        {"instrument_code", market["id"]},
        {"type", uppercaseType},
        {"side", side.toUpperCase()},
        {"amount", this->amountToPrecision(symbol, amount)}
    };
    
    if (uppercaseType == "LIMIT") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    json response = fetch("/account/orders", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response, market);
}

String Bitpanda::sign(const String& path, const String& api,
                      const String& method, const json& params,
                      const std::map<String, String>& headers,
                      const json& body) {
    String url = this->urls["api"][api];
    String endpoint = this->implodeParams(path, params);
    url += "/" + this->implodeParams(endpoint, params);
    json query = this->omit(params, this->extractParams(path));
    
    if (api == "public") {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->checkRequiredCredentials();
        String nonce = this->uuid();
        String timestamp = std::to_string(this->milliseconds());
        
        const_cast<std::map<String, String>&>(headers)["Accept"] = "application/json";
        const_cast<std::map<String, String>&>(headers)["Authorization"] = "Bearer " + this->apiKey;
        const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/json";
        
        if (method == "GET") {
            if (!query.empty()) {
                url += "?" + this->urlencode(query);
            }
        } else {
            if (!query.empty()) {
                body = this->json(query);
            }
        }
    }
    
    return url;
}

String Bitpanda::getNonce() {
    return this->uuid();
}

json Bitpanda::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "order_id");
    String timestamp = this->safeString(order, "time");
    String status = this->parseOrderStatus(this->safeString(order, "status"));
    String symbol = nullptr;
    
    if (!market.empty()) {
        symbol = market["symbol"];
    } else {
        String marketId = this->safeString(order, "instrument_code");
        if (marketId != nullptr) {
            if (this->markets_by_id.contains(marketId)) {
                market = this->markets_by_id[marketId];
                symbol = market["symbol"];
            } else {
                symbol = marketId;
            }
        }
    }
    
    String type = this->safeStringLower(order, "type");
    String side = this->safeStringLower(order, "side");
    
    return {
        {"id", id},
        {"clientOrderId", this->safeString(order, "client_id")},
        {"timestamp", this->parse8601(timestamp)},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"type", type},
        {"timeInForce", this->safeString(order, "time_in_force")},
        {"postOnly", nullptr},
        {"status", status},
        {"symbol", symbol},
        {"side", side},
        {"price", this->safeFloat(order, "price")},
        {"amount", this->safeFloat(order, "amount")},
        {"filled", this->safeFloat(order, "filled_amount")},
        {"remaining", this->safeFloat(order, "remaining_amount")},
        {"cost", this->safeFloat(order, "filled_amount") * this->safeFloat(order, "price")},
        {"trades", nullptr},
        {"fee", nullptr},
        {"info", order}
    };
}

String Bitpanda::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"OPEN", "open"},
        {"FILLED", "closed"},
        {"CANCELLED", "canceled"},
        {"PARTIALLY_FILLED", "open"},
        {"REJECTED", "rejected"}
    };
    
    return this->safeString(statuses, status, status);
}

} // namespace ccxt
