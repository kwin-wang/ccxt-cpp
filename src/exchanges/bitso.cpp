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
    auto response = this->request("/v3/balance", "private", "GET", params);
    return this->parseBalance(response);
}

json Bitso::parseBalance(const json& response) {
    auto result = {
        "info": response,
        "timestamp": undefined,
        "datetime": undefined
    };
    auto balances = response["payload"]["balances"];
    for (const auto& balance : balances) {
        auto currencyId = this->safeString(balance, "currency");
        auto code = this->safeCurrencyCode(currencyId);
        auto account = this->account();
        account["free"] = this->safeFloat(balance, "available");
        account["used"] = this->safeFloat(balance, "locked");
        account["total"] = this->safeFloat(balance, "total");
        result[code] = account;
    }
    return this->parseBalance(result);
}

json Bitso::createOrder(const String& symbol, const String& type, const String& side,
                       double amount, double price, const json& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        "book": market["id"],
        "side": side,
        "type": type,
        "major": this->amountToPrecision(symbol, amount)
    };
    if (type == "limit") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    auto response = this->request("/v3/orders", "private", "POST", this->extend(request, params));
    return this->parseOrder(response["payload"], market);
}

json Bitso::cancelOrder(const String& id, const String& symbol, const json& params) {
    auto request = {
        "oid": id
    };
    auto response = this->request("/v3/orders/" + id, "private", "DELETE", this->extend(request, params));
    return this->parseOrder(response["payload"]);
}

json Bitso::fetchOrder(const String& id, const String& symbol, const json& params) {
    this->loadMarkets();
    auto request = {
        "oid": id
    };
    auto response = this->request("/v3/orders/" + id, "private", "GET", this->extend(request, params));
    return this->parseOrder(response["payload"]);
}

json Bitso::fetchOrders(const String& symbol, int since, int limit, const json& params) {
    this->loadMarkets();
    auto market = undefined;
    auto request = {};
    if (symbol != "") {
        market = this->market(symbol);
        request["book"] = market["id"];
    }
    if (limit != 0) {
        request["limit"] = limit;
    }
    if (since != 0) {
        request["marker"] = since;
    }
    auto response = this->request("/v3/orders", "private", "GET", this->extend(request, params));
    return this->parseOrders(response["payload"], market, since, limit);
}

json Bitso::fetchOpenOrders(const String& symbol, int since, int limit, const json& params) {
    auto request = this->extend({
        "status": "open"
    }, params);
    return this->fetchOrders(symbol, since, limit, request);
}

json Bitso::fetchClosedOrders(const String& symbol, int since, int limit, const json& params) {
    auto request = this->extend({
        "status": "completed"
    }, params);
    return this->fetchOrders(symbol, since, limit, request);
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
        
        String signature = this->hmac(request, this->encode(this->config_.secret),
                                    "sha256", "hex");
        
        const_cast<std::map<String, String>&>(headers)["Authorization"] = "Bitso " + this->config_.apiKey + ":" + nonce + ":" + signature;
        
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

json Bitso::fetchTicker(const String& symbol, const json& params) {
    auto market = this->market(symbol);
    auto request = this->extend({
        "book": market["id"]
    }, params);
    auto response = this->request("/v3/ticker", "public", "GET", request);
    auto ticker = response["payload"];
    return this->parseTicker(ticker, market);
}

json Bitso::parseTicker(const json& ticker, const Market& market) {
    auto timestamp = this->safeInteger(ticker, "created_at");
    auto symbol = this->safeString(market, "symbol");
    return {
        "symbol": symbol,
        "timestamp": timestamp,
        "datetime": this->iso8601(timestamp),
        "high": this->safeFloat(ticker, "high"),
        "low": this->safeFloat(ticker, "low"),
        "bid": this->safeFloat(ticker, "bid"),
        "bidVolume": undefined,
        "ask": this->safeFloat(ticker, "ask"),
        "askVolume": undefined,
        "vwap": this->safeFloat(ticker, "vwap"),
        "open": undefined,
        "close": this->safeFloat(ticker, "last"),
        "last": this->safeFloat(ticker, "last"),
        "previousClose": undefined,
        "change": undefined,
        "percentage": undefined,
        "average": undefined,
        "baseVolume": this->safeFloat(ticker, "volume"),
        "quoteVolume": undefined,
        "info": ticker
    };
}

json Bitso::fetchOHLCV(const String& symbol, const String& timeframe, int since, int limit, const json& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        "book": market["id"],
        "time_bucket": this->timeframes[timeframe]
    };
    if (since != 0) {
        request["start"] = since;
    }
    if (limit != 0) {
        request["limit"] = limit;
    }
    auto response = this->request("/v3/ohlc", "public", "GET", this->extend(request, params));
    return this->parseOHLCVs(response["payload"], market, timeframe, since, limit);
}

json Bitso::parseOHLCV(const json& ohlcv, const Market& market) {
    return {
        this->safeTimestamp(ohlcv, "bucket_start_time"),
        this->safeFloat(ohlcv, "open"),
        this->safeFloat(ohlcv, "high"),
        this->safeFloat(ohlcv, "low"),
        this->safeFloat(ohlcv, "close"),
        this->safeFloat(ohlcv, "volume")
    };
}

json Bitso::fetchTrades(const String& symbol, int since, int limit, const json& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        "book": market["id"]
    };
    if (limit != 0) {
        request["limit"] = limit;
    }
    auto response = this->request("/v3/trades", "public", "GET", this->extend(request, params));
    return this->parseTrades(response["payload"], market, since, limit);
}

json Bitso::parseTrade(const json& trade, const Market& market) {
    auto timestamp = this->safeTimestamp(trade, "created_at");
    auto side = this->safeString(trade, "maker_side");
    if (side == "buy") {
        side = "sell";
    } else if (side == "sell") {
        side = "buy";
    }
    auto price = this->safeFloat(trade, "price");
    auto amount = this->safeFloat(trade, "amount");
    auto cost = undefined;
    if (price != undefined && amount != undefined) {
        cost = price * amount;
    }
    return {
        "info": trade,
        "id": this->safeString(trade, "tid"),
        "timestamp": timestamp,
        "datetime": this->iso8601(timestamp),
        "symbol": market["symbol"],
        "order": undefined,
        "type": undefined,
        "side": side,
        "takerOrMaker": undefined,
        "price": price,
        "amount": amount,
        "cost": cost,
        "fee": undefined
    };
}

json Bitso::fetchMyTrades(const String& symbol, int since, int limit, const json& params) {
    this->loadMarkets();
    auto market = undefined;
    auto request = {};
    if (symbol != "") {
        market = this->market(symbol);
        request["book"] = market["id"];
    }
    if (limit != 0) {
        request["limit"] = limit;
    }
    if (since != 0) {
        request["marker"] = since;
    }
    auto response = this->request("/v3/user_trades", "private", "GET", this->extend(request, params));
    return this->parseTrades(response["payload"], market, since, limit);
}

json Bitso::fetchDeposits(const String& code, int since, int limit, const json& params) {
    this->loadMarkets();
    auto currency = undefined;
    auto request = {};
    if (code != "") {
        currency = this->currency(code);
        request["currency"] = currency["id"];
    }
    if (limit != 0) {
        request["limit"] = limit;
    }
    if (since != 0) {
        request["marker"] = since;
    }
    auto response = this->request("/v3/fundings", "private", "GET", this->extend(request, params));
    return this->parseTransactions(response["payload"], currency, since, limit, {"deposit"});
}

json Bitso::fetchWithdrawals(const String& code, int since, int limit, const json& params) {
    this->loadMarkets();
    auto currency = undefined;
    auto request = {};
    if (code != "") {
        currency = this->currency(code);
        request["currency"] = currency["id"];
    }
    if (limit != 0) {
        request["limit"] = limit;
    }
    if (since != 0) {
        request["marker"] = since;
    }
    auto response = this->request("/v3/withdrawals", "private", "GET", this->extend(request, params));
    return this->parseTransactions(response["payload"], currency, since, limit, {"withdrawal"});
}

json Bitso::fetchDepositAddress(const String& code, const json& params) {
    this->loadMarkets();
    auto currency = this->currency(code);
    auto request = {
        "fund_currency": currency["id"]
    };
    auto response = this->request("/v3/funding_destination", "private", "GET", this->extend(request, params));
    auto address = this->safeString(response["payload"], "account_identifier");
    auto tag = this->safeString(response["payload"], "payment_id");
    return {
        "currency": code,
        "address": address,
        "tag": tag,
        "info": response
    };
}

json Bitso::withdraw(const String& code, double amount, const String& address,
                    const String& tag, const json& params) {
    this->checkAddress(address);
    this->loadMarkets();
    auto currency = this->currency(code);
    auto request = {
        "currency": currency["id"],
        "amount": this->currencyToPrecision(code, amount),
        "address": address
    };
    if (tag != "") {
        request["payment_id"] = tag;
    }
    auto response = this->request("/v3/withdrawals", "private", "POST", this->extend(request, params));
    return {
        "info": response,
        "id": this->safeString(response["payload"], "wid")
    };
}

} // namespace ccxt
