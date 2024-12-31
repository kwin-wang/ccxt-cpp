#include "ccxt/exchanges/bittrue.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

BitTrue::BitTrue() {
    id = "bittrue";
    name = "BitTrue";
    version = "v1";
    certified = true;
    pro = true;

    // Initialize URLs
    baseUrl = "https://openapi.bittrue.com";
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/87295558-132aaf80-c50e-11ea-9801-a2fb0c57c799.jpg"},
        {"api", {
            {"public", "https://openapi.bittrue.com/api/v1"},
            {"private", "https://openapi.bittrue.com/api/v1"},
            {"sapi", "https://openapi.bittrue.com/sapi/v1"}
        }},
        {"www", "https://www.bittrue.com"},
        {"doc", {
            "https://github.com/bittrue/bittrue-official-api-docs"
        }},
        {"fees", "https://www.bittrue.com/fee"}
    };

    // Initialize API endpoints
    api = {
        {"public", {
            {"GET", {
                "ping",
                "time",
                "exchangeInfo",
                "depth",
                "trades",
                "historicalTrades",
                "aggTrades",
                "klines",
                "ticker/24hr",
                "ticker/price",
                "ticker/bookTicker"
            }}
        }},
        {"private", {
            {"GET", {
                "order",
                "openOrders",
                "allOrders",
                "account",
                "myTrades"
            }},
            {"POST", {
                "order",
                "order/test"
            }},
            {"DELETE", {
                "order"
            }}
        }},
        {"sapi", {
            {"GET", {
                "asset/getUserAsset",
                "capital/config/getall",
                "capital/deposit/address",
                "capital/deposit/hisrec",
                "capital/withdraw/history"
            }},
            {"POST", {
                "capital/withdraw/apply"
            }}
        }}
    };

    // Initialize timeframes for OHLCV data
    timeframes = {
        {"1m", "1m"},
        {"3m", "3m"},
        {"5m", "5m"},
        {"15m", "15m"},
        {"30m", "30m"},
        {"1h", "1h"},
        {"2h", "2h"},
        {"4h", "4h"},
        {"6h", "6h"},
        {"8h", "8h"},
        {"12h", "12h"},
        {"1d", "1d"},
        {"3d", "3d"},
        {"1w", "1w"},
        {"1M", "1M"}
    };
}

void BitTrue::sign(Request& request, const std::string& path, const std::string& api,
                  const std::string& method, const json& params, const json& headers,
                  const json& body) {
    if (api == "private" || api == "sapi") {
        if (this->config_.apiKey.empty()) {
            throw AuthenticationError("Authentication failed: API key required for private endpoints");
        }

        std::string timestamp = getNonce();
        std::string querystd::string = "";

        if (!params.empty()) {
            querystd::string = this->urlencode(params);
        }

        std::string payload = timestamp + method + path;
        if (!querystd::string.empty()) {
            payload += "?" + querystd::string;
        }

        std::string signature = getSignature(timestamp, method, path, querystd::string);

        request.headers["X-BT-APIKEY"] = this->config_.apiKey;
        request.headers["X-BT-TIMESTAMP"] = timestamp;
        request.headers["X-BT-SIGNATURE"] = signature;
    }
}

std::string BitTrue::getSignature(const std::string& timestamp, const std::string& method,
                                const std::string& path, const std::string& body) {
    std::string message = timestamp + method + path + body;
    unsigned char* digest = HMAC(EVP_sha256(),
                               this->config_.secret.c_str(), this->config_.secret.length(),
                               (unsigned char*)message.c_str(), message.length(),
                               nullptr, nullptr);
    
    std::stringstream ss;
    for(int i = 0; i < 32; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
    }
    return ss.str();
}

std::string BitTrue::getNonce() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return std::to_string(ms.count());
}

json BitTrue::fetchMarkets(const json& params) {
    json response = fetch("/exchangeInfo", "public", "GET", params);
    json result = json::array();
    
    for (const auto& market : response["symbols"]) {
        std::string id = market["symbol"].get<std::string>();
        std::string baseId = market["baseAsset"].get<std::string>();
        std::string quoteId = market["quoteAsset"].get<std::string>();
        std::string base = this->safeCurrencyCode(baseId);
        std::string quote = this->safeCurrencyCode(quoteId);
        std::string symbol = base + "/" + quote;
        
        result.push_back({
            {"id", id},
            {"symbol", symbol},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", market["status"] == "TRADING"},
            {"precision", {
                {"amount", market["baseAssetPrecision"].get<int>()},
                {"price", market["quotePrecision"].get<int>()}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeFloat(market, "minQty")},
                    {"max", this->safeFloat(market, "maxQty")}
                }},
                {"price", {
                    {"min", this->safeFloat(market, "minPrice")},
                    {"max", this->safeFloat(market, "maxPrice")}
                }},
                {"cost", {
                    {"min", this->safeFloat(market, "minNotional")},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return result;
}

json BitTrue::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/account", "private", "GET", params);
    return parseBalance(response);
}

json BitTrue::parseBalance(const json& response) {
    json result = {{"info", response}};
    
    for (const auto& balance : response["balances"]) {
        std::string currencyId = balance["asset"].get<std::string>();
        std::string code = this->safeCurrencyCode(currencyId);
        json account = {
            {"free", this->safeFloat(balance, "free")},
            {"used", this->safeFloat(balance, "locked")},
            {"total", this->safeFloat(balance, "free") + this->safeFloat(balance, "locked")}
        };
        result[code] = account;
    }
    
    return result;
}

json BitTrue::createOrder(const std::string& symbol, const std::string& type,
                        const std::string& side, double amount,
                        double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    std::string uppercaseType = this->uppercase(type);
    
    json request = {
        {"symbol", market["id"]},
        {"type", uppercaseType},
        {"side", this->uppercase(side)},
        {"quantity", this->amountToPrecision(symbol, amount)}
    };
    
    if (uppercaseType == "LIMIT") {
        request["price"] = this->priceToPrecision(symbol, price);
        request["timeInForce"] = "GTC";
    }
    
    json response = fetch("/order", "private", "POST", this->extend(request, params));
    return this->parseOrder(response, market);
}

json BitTrue::parseOrder(const json& order, const Market& market) {
    std::string id = this->safeString(order, "orderId");
    std::string timestamp = this->safeString(order, "time");
    std::string status = this->parseOrderStatus(this->safeString(order, "status"));
    std::string symbol = "";
    
    if (!market.empty()) {
        symbol = market["symbol"];
    } else {
        std::string marketId = this->safeString(order, "symbol");
        if (marketId != "") {
            if (this->markets_by_id.contains(marketId)) {
                market = this->markets_by_id[marketId];
                symbol = market["symbol"];
            } else {
                symbol = marketId;
            }
        }
    }
    
    std::string type = this->safeStringLower(order, "type");
    std::string side = this->safeStringLower(order, "side");
    
    return {
        {"id", id},
        {"clientOrderId", this->safeString(order, "clientOrderId")},
        {"timestamp", this->parse8601(timestamp)},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"type", type},
        {"timeInForce", this->safeString(order, "timeInForce")},
        {"postOnly", nullptr},
        {"status", status},
        {"symbol", symbol},
        {"side", side},
        {"price", this->safeFloat(order, "price")},
        {"amount", this->safeFloat(order, "origQty")},
        {"filled", this->safeFloat(order, "executedQty")},
        {"remaining", this->safeFloat(order, "origQty") - this->safeFloat(order, "executedQty")},
        {"cost", this->safeFloat(order, "cummulativeQuoteQty")},
        {"trades", nullptr},
        {"fee", nullptr},
        {"info", order}
    };
}

std::string BitTrue::parseOrderStatus(const std::string& status) {
    static const std::map<std::string, std::string> statuses = {
        {"NEW", "open"},
        {"PARTIALLY_FILLED", "open"},
        {"FILLED", "closed"},
        {"CANCELED", "canceled"},
        {"PENDING_CANCEL", "canceling"},
        {"REJECTED", "rejected"},
        {"EXPIRED", "expired"}
    };
    
    return this->safeString(statuses, status, status);
}

} // namespace ccxt
