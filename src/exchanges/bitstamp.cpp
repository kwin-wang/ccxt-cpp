#include "bitstamp.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Bitstamp::Bitstamp() {
    id = "bitstamp";
    name = "Bitstamp";
    version = "v2";
    rateLimit = 1000;

    // Initialize API endpoints
    baseUrl = "https://www.bitstamp.net/api";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/27786377-8c8ab57e-5fe9-11e7-8ea4-2b05b6bcceec.jpg"},
        {"api", {
            {"public", baseUrl},
            {"private", baseUrl},
            {"v1", baseUrl + "/v1"},
            {"v2", baseUrl + "/v2"}
        }},
        {"www", "https://www.bitstamp.net"},
        {"doc", {
            "https://www.bitstamp.net/api",
            "https://support.bitstamp.net/hc/en-us/articles/360024386139-API-Guide"
        }},
        {"fees", "https://www.bitstamp.net/fee-schedule/"}
    };

    timeframes = {
        {"1m", "60"},
        {"3m", "180"},
        {"5m", "300"},
        {"15m", "900"},
        {"30m", "1800"},
        {"1h", "3600"},
        {"2h", "7200"},
        {"4h", "14400"},
        {"6h", "21600"},
        {"12h", "43200"},
        {"1d", "86400"},
        {"3d", "259200"}
    };

    options = {
        {"adjustForTimeDifference", true}
    };

    initializeApiEndpoints();
}

void Bitstamp::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "v2/trading-pairs-info",
                "v2/ticker/{pair}",
                "v2/ticker_hour/{pair}",
                "v2/order_book/{pair}",
                "v2/transactions/{pair}",
                "v2/ohlc/{pair}",
                "v2/eur_usd"
            }}
        }},
        {"private", {
            {"POST", {
                "v2/balance",
                "v2/user_transactions",
                "v2/open_orders/all",
                "v2/order_status",
                "v2/cancel_order",
                "v2/cancel_all_orders",
                "v2/buy/{pair}",
                "v2/buy/market/{pair}",
                "v2/buy/instant/{pair}",
                "v2/sell/{pair}",
                "v2/sell/market/{pair}",
                "v2/sell/instant/{pair}",
                "v2/transfer-to-main",
                "v2/transfer-from-main",
                "v2/withdrawal-requests",
                "v2/withdrawal/open",
                "v2/withdrawal/status",
                "v2/withdrawal/cancel",
                "v2/liquidation_address/new",
                "v2/liquidation_address/info",
                "v2/bitcoin_deposit_address"
            }}
        }}
    };
}

json Bitstamp::fetchMarkets(const json& params) {
    json response = fetch("/v2/trading-pairs-info", "public", "GET", params);
    json markets = json::array();
    
    for (const auto& market : response) {
        String id = market["url_symbol"];
        std::vector<String> parts = this->split(market["name"], "/");
        String baseId = parts[0];
        String quoteId = parts[1];
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
                {"amount", this->safeInteger(market, "base_decimals")},
                {"price", this->safeInteger(market, "counter_decimals")}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeFloat(market, "minimum_order")},
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
    
    return markets;
}

json Bitstamp::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/v2/balance", "private", "POST", params);
    json result = {"info", response};
    
    std::vector<String> currencyIds = {
        "btc", "eth", "eur", "usd", "gbp", "ltc", "xrp", "bch", "xlm", "link", "pax",
        "omg", "usdc", "aave", "algo", "audio", "crv", "dai", "enj", "grt", "uni",
        "yfi", "comp", "snx", "doge", "chz", "matic", "mkr", "sushi", "uma", "dot",
        "bat", "usdt"
    };
    
    for (const auto& currencyId : currencyIds) {
        String code = this->commonCurrencyCode(currencyId.upper());
        String account = this->account();
        
        String free = currencyId + "_available";
        String total = currencyId + "_balance";
        String used = currencyId + "_reserved";
        
        if (response.contains(total) || response.contains(free)) {
            account["free"] = this->safeString(response, free);
            account["total"] = this->safeString(response, total);
            account["used"] = this->safeString(response, used);
            result[code] = account;
        }
    }
    
    return this->parseBalance(result);
}

json Bitstamp::createOrder(const String& symbol, const String& type,
                          const String& side, double amount,
                          double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    String method = "privatePostV2" + this->capitalize(side);
    String request = {"pair", market.id};
    
    if (type == "market") {
        method += "Market";
    } else if (type == "instant") {
        method += "Instant";
    }
    
    method += this->capitalize(market.id);
    
    if (type != "market") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    request["amount"] = this->amountToPrecision(symbol, amount);
    
    json response = this->call(method, this->extend(request, params));
    return this->parseOrder(response, market);
}

String Bitstamp::sign(const String& path, const String& api,
                      const String& method, const json& params,
                      const std::map<String, String>& headers,
                      const json& body) {
    String url = this->urls["api"][api] + "/" + this->implodeParams(path, params);
    String query = this->omit(params, this->extractParams(path));
    
    if (api == "public") {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->checkRequiredCredentials();
        
        String nonce = this->getNonce();
        String timestamp = std::to_string(this->milliseconds());
        String contentType = "application/x-www-form-urlencoded";
        String auth = nonce + this->customerId + this->apiKey;
        
        query = this->extend({
            "key": this->apiKey,
            "signature": this->hmac(auth, this->secret, "sha256", "hex"),
            "nonce": nonce
        }, query);
        
        body = this->urlencode(query);
        
        const_cast<std::map<String, String>&>(headers)["Content-Type"] = contentType;
        const_cast<std::map<String, String>&>(headers)["X-Auth"] = "BITSTAMP " + this->apiKey;
        const_cast<std::map<String, String>&>(headers)["X-Auth-Signature"] = this->hmac(body, this->secret, "sha256", "hex");
        const_cast<std::map<String, String>&>(headers)["X-Auth-Nonce"] = nonce;
        const_cast<std::map<String, String>&>(headers)["X-Auth-Timestamp"] = timestamp;
        const_cast<std::map<String, String>&>(headers)["X-Auth-Version"] = version;
    }
    
    return url;
}

String Bitstamp::getNonce() {
    return std::to_string(this->milliseconds());
}

json Bitstamp::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "id");
    String timestamp = this->safeInteger(order, "datetime");
    String status = this->parseOrderStatus(this->safeString(order, "status"));
    String side = this->safeStringLower(order, "type");
    String type = "limit";  // Bitstamp只支持限价单
    
    if (side == "0") {
        side = "buy";
    } else if (side == "1") {
        side = "sell";
    }
    
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
        {"amount", this->safeFloat(order, "amount")},
        {"filled", nullptr},
        {"remaining", nullptr},
        {"cost", nullptr},
        {"trades", nullptr},
        {"fee", nullptr},
        {"info", order}
    };
}

json Bitstamp::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"In Queue", "open"},
        {"Open", "open"},
        {"Finished", "closed"},
        {"Canceled", "canceled"}
    };
    
    return statuses.contains(status) ? statuses.at(status) : status;
}

} // namespace ccxt
