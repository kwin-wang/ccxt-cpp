#include "krakenpro.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

KrakenPro::KrakenPro() {
    id = "krakenpro";
    name = "Kraken Futures";
    version = "v3";
    rateLimit = 200;
    testnet = false;
    defaultType = "futures";

    // Initialize API endpoints
    baseUrl = testnet ? "https://demo-futures.kraken.com" : "https://futures.kraken.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/51840849/76173629-fc67fb00-61b1-11ea-84fe-f2de582f58a3.jpg"},
        {"api", {
            {"public", baseUrl},
            {"private", baseUrl}
        }},
        {"www", "https://futures.kraken.com"},
        {"doc", {
            "https://support.kraken.com/hc/en-us/categories/360001806372-Futures-API",
            "https://futures.kraken.com/derivatives/api/v3/swagger-ui/index.html"
        }},
        {"fees", "https://futures.kraken.com/derivatives/api/v3/feeschedules"},
        {"test", "https://demo-futures.kraken.com"}
    };

    timeframes = {
        {"1m", "1m"},
        {"5m", "5m"},
        {"15m", "15m"},
        {"30m", "30m"},
        {"1h", "1h"},
        {"4h", "4h"},
        {"12h", "12h"},
        {"1d", "1d"},
        {"1w", "1w"},
        {"2w", "2w"}
    };

    options = {
        {"version", "v3"},
        {"defaultType", "futures"}
    };

    initializeApiEndpoints();
}

void KrakenPro::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "derivatives/api/v3/instruments",
                "derivatives/api/v3/tickers",
                "derivatives/api/v3/orderbook",
                "derivatives/api/v3/history",
                "derivatives/api/v3/charts",
                "derivatives/api/v3/charts/v2",
                "derivatives/api/v3/statistics",
                "derivatives/api/v3/margins",
                "derivatives/api/v3/funding_rates",
                "derivatives/api/v3/trade_history",
                "derivatives/api/v3/funding_history",
                "derivatives/api/v3/feeschedules"
            }}
        }},
        {"private", {
            {"GET", {
                "derivatives/api/v3/accounts",
                "derivatives/api/v3/openpositions",
                "derivatives/api/v3/notifications",
                "derivatives/api/v3/accounts/pnl_history",
                "derivatives/api/v3/orders",
                "derivatives/api/v3/fills",
                "derivatives/api/v3/transfers",
                "derivatives/api/v3/withdrawals",
                "derivatives/api/v3/deposits",
                "derivatives/api/v3/leverage"
            }},
            {"POST", {
                "derivatives/api/v3/sendorder",
                "derivatives/api/v3/cancelorder",
                "derivatives/api/v3/cancelallorders",
                "derivatives/api/v3/cancelallordersafter",
                "derivatives/api/v3/batchorder",
                "derivatives/api/v3/withdrawal",
                "derivatives/api/v3/transfer",
                "derivatives/api/v3/leverage"
            }}
        }}
    };
}

json KrakenPro::fetchMarkets(const json& params) {
    json response = fetch("/derivatives/api/v3/instruments", "public", "GET", params);
    json markets = json::array();
    
    for (const auto& market : response["instruments"]) {
        String id = market["symbol"];
        String baseId = market["underlying"];
        String quoteId = market["quoteCurrency"];
        String base = this->commonCurrencyCode(baseId);
        String quote = this->commonCurrencyCode(quoteId);
        String type = market["type"].get<String>();
        bool active = market["tradeable"];
        
        markets.push_back({
            {"id", id},
            {"symbol", base + "/" + quote},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", active},
            {"type", type},
            {"spot", false},
            {"future", type == "future"},
            {"swap", type == "perpetual"},
            {"option", false},
            {"linear", market["isInverse"] == false},
            {"inverse", market["isInverse"] == true},
            {"contract", true},
            {"contractSize", market["contractSize"]},
            {"expiry", market["expiry"]},
            {"expiryDatetime", this->iso8601(market["expiry"])},
            {"precision", {
                {"amount", market["contractSize"]},
                {"price", market["tickSize"]}
            }},
            {"limits", {
                {"amount", {
                    {"min", market["minOrderSize"]},
                    {"max", market["maxOrderSize"]}
                }},
                {"price", {
                    {"min", market["tickSize"]},
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
    
    return markets;
}

json KrakenPro::fetchTicker(const String& symbol, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {{"symbol", market.id}};
    json response = fetch("/derivatives/api/v3/tickers", "public", "GET",
                         this->extend(request, params));
    json ticker = response["tickers"][0];
    
    return {
        {"symbol", symbol},
        {"timestamp", ticker["time"]},
        {"datetime", this->iso8601(ticker["time"])},
        {"high", ticker["high24h"]},
        {"low", ticker["low24h"]},
        {"bid", ticker["bid"]},
        {"bidVolume", ticker["bidSize"]},
        {"ask", ticker["ask"]},
        {"askVolume", ticker["askSize"]},
        {"vwap", ticker["vwap"]},
        {"open", ticker["open24h"]},
        {"close", ticker["last"]},
        {"last", ticker["last"]},
        {"previousClose", nullptr},
        {"change", nullptr},
        {"percentage", nullptr},
        {"average", nullptr},
        {"baseVolume", ticker["volume"]},
        {"quoteVolume", nullptr},
        {"info", ticker}
    };
}

json KrakenPro::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/derivatives/api/v3/accounts", "private", "GET", params);
    json result = {"info", response};
    
    for (const auto& balance : response["accounts"]) {
        String currencyId = balance["currency"];
        String code = this->commonCurrencyCode(currencyId);
        
        result[code] = {
            {"free", balance["availableBalance"]},
            {"used", balance["initialMargin"]},
            {"total", balance["equity"]}
        };
    }
    
    return result;
}

json KrakenPro::createOrder(const String& symbol, const String& type,
                           const String& side, double amount,
                           double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"symbol", market.id},
        {"side", side.upper()},
        {"size", amount},
        {"orderType", type.upper()}
    };
    
    if (type == "limit") {
        if (price == 0) {
            throw InvalidOrder("For limit orders, price cannot be zero");
        }
        request["limitPrice"] = price;
    }
    
    json response = fetch("/derivatives/api/v3/sendorder", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response["sendStatus"], market);
}

String KrakenPro::sign(const String& path, const String& api,
                       const String& method, const json& params,
                       const std::map<String, String>& headers,
                       const json& body) {
    String url = this->urls["api"][api] + "/" + path;
    String timestamp = std::to_string(this->milliseconds());
    
    if (api == "public") {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    } else {
        this->checkRequiredCredentials();
        
        String nonce = this->nonce().str();
        String auth = path + nonce + timestamp;
        
        if (method == "GET") {
            if (!params.empty()) {
                String query = this->urlencode(this->keysort(params));
                url += "?" + query;
                auth += query;
            }
        } else {
            if (!params.empty()) {
                body = this->json(params);
                auth += body.dump();
            }
        }
        
        String signature = this->hmac(auth, this->base64ToBinary(this->config_.secret),
                                    "sha512", "base64");
        
        const_cast<std::map<String, String>&>(headers)["APIKey"] = this->config_.apiKey;
        const_cast<std::map<String, String>&>(headers)["Nonce"] = nonce;
        const_cast<std::map<String, String>&>(headers)["Authent"] = signature;
        
        if (method != "GET") {
            const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/json";
        }
    }
    
    return url;
}

json KrakenPro::parseOrder(const json& order, const Market& market) {
    String id = this->getOrderId(order);
    String timestamp = this->safeInteger(order, "receivedTime");
    String status = this->parseOrderStatus(this->safeString(order, "status"));
    
    return {
        {"id", id},
        {"clientOrderId", this->safeString(order, "cliOrdId")},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", this->safeInteger(order, "lastModifiedTime")},
        {"status", status},
        {"symbol", market.symbol},
        {"type", this->safeStringLower(order, "orderType")},
        {"side", this->safeStringLower(order, "side")},
        {"price", this->safeFloat(order, "limitPrice")},
        {"amount", this->safeFloat(order, "size")},
        {"filled", this->safeFloat(order, "filledSize")},
        {"remaining", this->safeFloat(order, "unfilledSize")},
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

String KrakenPro::getOrderId(const json& order) {
    if (order.contains("orderId")) {
        return order["orderId"];
    } else if (order.contains("orderid")) {
        return order["orderid"];
    } else if (order.contains("order_id")) {
        return order["order_id"];
    }
    return "";
}

json KrakenPro::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"placed", "open"},
        {"cancelled", "canceled"},
        {"untriggered", "open"},
        {"triggered", "open"},
        {"filled", "closed"}
    };
    
    return statuses.contains(status) ? statuses.at(status) : status;
}

} // namespace ccxt
