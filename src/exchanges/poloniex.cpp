#include "poloniex.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Poloniex::Poloniex() {
    id = "poloniex";
    name = "Poloniex";
    version = "v1";
    rateLimit = 1000;
    hasPrivateAPI = true;
    lastNonce = 0;

    // Initialize API endpoints
    baseUrl = "https://api.poloniex.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/27766817-e9456312-5ee6-11e7-9b3c-b628ca5626a5.jpg"},
        {"api", {
            {"public", baseUrl},
            {"private", baseUrl}
        }},
        {"www", "https://poloniex.com"},
        {"doc", {
            "https://docs.poloniex.com",
            "https://docs.poloniex.com/#http-api",
            "https://docs.poloniex.com/#websocket-api"
        }},
        {"fees", "https://poloniex.com/fees"}
    };

    timeframes = {
        {"1m", "MINUTE_1"},
        {"5m", "MINUTE_5"},
        {"15m", "MINUTE_15"},
        {"30m", "MINUTE_30"},
        {"1h", "HOUR_1"},
        {"2h", "HOUR_2"},
        {"4h", "HOUR_4"},
        {"6h", "HOUR_6"},
        {"12h", "HOUR_12"},
        {"1d", "DAY_1"},
        {"3d", "DAY_3"},
        {"1w", "WEEK_1"},
        {"1M", "MONTH_1"}
    };

    options = {
        {"recvWindow", "10000"},
        {"adjustForTimeDifference", true},
        {"warnOnFetchOpenOrdersWithoutSymbol", true}
    };

    initializeApiEndpoints();
}

void Poloniex::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "markets",
                "markets/{symbol}/price",
                "markets/{symbol}/orderBook",
                "markets/{symbol}/candles",
                "markets/{symbol}/trades",
                "currencies",
                "timestamp"
            }}
        }},
        {"private", {
            {"GET", {
                "accounts",
                "orders",
                "orders/{id}",
                "trades",
                "trades/{id}",
                "wallets/addresses",
                "wallets/activity",
                "wallets/fees"
            }},
            {"POST", {
                "orders",
                "orders/test",
                "wallets/addresses",
                "wallets/withdraw"
            }},
            {"DELETE", {
                "orders",
                "orders/{id}"
            }}
        }}
    };
}

json Poloniex::fetchMarkets(const json& params) {
    json response = fetch("/markets", "public", "GET", params);
    json markets = json::array();
    
    for (const auto& market : response) {
        String id = market["symbol"];
        std::vector<String> parts = this->split(id, "_");
        String baseId = parts[0];
        String quoteId = parts[1];
        String base = this->commonCurrencyCode(baseId);
        String quote = this->commonCurrencyCode(quoteId);
        String symbol = base + "/" + quote;
        bool active = market["state"] == "ONLINE";
        
        markets.push_back({
            {"id", id},
            {"symbol", symbol},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", active},
            {"type", "spot"},
            {"spot", true},
            {"future", false},
            {"swap", false},
            {"option", false},
            {"contract", false},
            {"precision", {
                {"amount", market["baseScale"].get<int>()},
                {"price", market["quoteScale"].get<int>()}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeFloat(market, "minBaseAmount")},
                    {"max", this->safeFloat(market, "maxBaseAmount")}
                }},
                {"price", {
                    {"min", this->safeFloat(market, "minPriceQuote")},
                    {"max", this->safeFloat(market, "maxPriceQuote")}
                }},
                {"cost", {
                    {"min", this->safeFloat(market, "minQuoteAmount")},
                    {"max", this->safeFloat(market, "maxQuoteAmount")}
                }}
            }},
            {"info", market}
        });
    }
    
    return markets;
}

json Poloniex::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/accounts/balances", "private", "GET", params);
    json result = {"info", response};
    
    for (const auto& balance : response) {
        String currencyId = balance["currency"];
        String code = this->commonCurrencyCode(currencyId);
        
        result[code] = {
            {"free", this->safeFloat(balance, "available")},
            {"used", this->safeFloat(balance, "onOrders")},
            {"total", this->safeFloat(balance, "total")}
        };
    }
    
    return result;
}

json Poloniex::createOrder(const String& symbol, const String& type,
                          const String& side, double amount,
                          double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"symbol", market.id},
        {"side", side.upper()},
        {"amount", this->amountToPrecision(symbol, amount)}
    };
    
    if (type == "limit") {
        request["type"] = "LIMIT";
        request["price"] = this->priceToPrecision(symbol, price);
    } else {
        request["type"] = "MARKET";
    }
    
    json response = fetch("/orders", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response, market);
}

String Poloniex::sign(const String& path, const String& api,
                      const String& method, const json& params,
                      const std::map<String, String>& headers,
                      const json& body) {
    String url = this->urls["api"][api];
    String endpoint = "/" + this->version + "/" + this->implodeParams(path, params);
    url += endpoint;
    
    json query = this->omit(params, this->extractParams(path));
    
    if (api == "public") {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->checkRequiredCredentials();
        String nonce = this->getNonce();
        String auth = endpoint + nonce;
        
        if (method == "GET" || method == "DELETE") {
            if (!query.empty()) {
                url += "?" + this->urlencode(query);
                auth += this->urlencode(query);
            }
        } else {
            if (!query.empty()) {
                body = this->json(query);
                auth += body.dump();
            }
        }
        
        String signature = this->hmac(auth, this->base64ToBinary(this->secret),
                                    "sha256", "hex");
        
        const_cast<std::map<String, String>&>(headers)["Key"] = this->apiKey;
        const_cast<std::map<String, String>&>(headers)["Signature"] = signature;
        const_cast<std::map<String, String>&>(headers)["Nonce"] = nonce;
        
        if (body != nullptr) {
            const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/json";
        }
    }
    
    return url;
}

String Poloniex::getNonce() {
    int64_t currentNonce = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    if (currentNonce <= lastNonce) {
        currentNonce = lastNonce + 1;
    }
    lastNonce = currentNonce;
    return std::to_string(currentNonce);
}

json Poloniex::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "id");
    String timestamp = this->safeString(order, "timestamp");
    String status = this->parseOrderStatus(this->safeString(order, "state"));
    String symbol = market.symbol;
    String type = this->safeStringLower(order, "type");
    String side = this->safeStringLower(order, "side");
    
    return {
        {"id", id},
        {"clientOrderId", this->safeString(order, "clientOrderId")},
        {"timestamp", this->parse8601(timestamp)},
        {"datetime", timestamp},
        {"lastTradeTimestamp", nullptr},
        {"status", status},
        {"symbol", symbol},
        {"type", type},
        {"side", side},
        {"price", this->safeFloat(order, "price")},
        {"amount", this->safeFloat(order, "amount")},
        {"filled", this->safeFloat(order, "filledAmount")},
        {"remaining", this->safeFloat(order, "remainingAmount")},
        {"cost", this->safeFloat(order, "cost")},
        {"trades", nullptr},
        {"fee", {
            {"currency", market.quote},
            {"cost", this->safeFloat(order, "fee")}
        }},
        {"info", order}
    };
}

json Poloniex::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"OPEN", "open"},
        {"PENDING", "open"},
        {"FILLED", "closed"},
        {"PARTIALLY_FILLED", "open"},
        {"CANCELLED", "canceled"},
        {"EXPIRED", "expired"}
    };
    
    return statuses.contains(status) ? statuses.at(status) : status;
}

// Trading API Methods
Json poloniex::createOrderImpl(const std::string& symbol, const std::string& type,
                             const std::string& side, double amount,
                             const std::optional<double>& price) {
    auto market = loadMarket(symbol);
    auto request = Json::object();
    request["symbol"] = market["id"];
    request["side"] = toUpper(side);
    request["type"] = toUpper(type);
    request["quantity"] = amountToPrecision(symbol, amount);
    
    if (type == "limit") {
        if (!price) {
            throw ArgumentsRequired("createOrder() requires a price argument for limit orders");
        }
        request["price"] = priceToPrecision(symbol, *price);
    }
    
    auto response = request("/orders", "private", "POST", request);
    return parseOrder(response, market);
}

Json poloniex::createMarketBuyOrderWithCostImpl(const std::string& symbol,
                                              double cost,
                                              const Json& params) {
    auto market = loadMarket(symbol);
    auto request = Json::object();
    request["symbol"] = market["id"];
    request["side"] = "BUY";
    request["type"] = "MARKET";
    request["cost"] = costToPrecision(symbol, cost);
    
    auto response = request("/orders", "private", "POST", request);
    return parseOrder(response, market);
}

Json poloniex::createStopOrderImpl(const std::string& symbol, const std::string& type,
                                 const std::string& side, double amount,
                                 const std::optional<double>& price,
                                 const Json& params) {
    auto market = loadMarket(symbol);
    auto request = Json::object();
    request["symbol"] = market["id"];
    request["side"] = toUpper(side);
    request["type"] = toUpper(type);
    request["quantity"] = amountToPrecision(symbol, amount);
    
    if (type == "limit") {
        if (!price) {
            throw ArgumentsRequired("createStopOrder() requires a price argument for limit orders");
        }
        request["price"] = priceToPrecision(symbol, *price);
    }
    
    if (!params.contains("triggerPrice")) {
        throw ArgumentsRequired("createStopOrder() requires a triggerPrice parameter");
    }
    request["triggerPrice"] = priceToPrecision(symbol, params["triggerPrice"].get<double>());
    
    auto response = request("/orders", "private", "POST", request);
    return parseOrder(response, market);
}

Json poloniex::editOrderImpl(const std::string& id, const std::string& symbol,
                           const std::string& type, const std::string& side,
                           const std::optional<double>& amount,
                           const std::optional<double>& price,
                           const Json& params) {
    auto market = loadMarket(symbol);
    auto request = Json::object();
    request["orderId"] = id;
    
    if (amount) {
        request["quantity"] = amountToPrecision(symbol, *amount);
    }
    if (price) {
        request["price"] = priceToPrecision(symbol, *price);
    }
    
    auto response = request("/orders/" + id, "private", "PUT", request);
    return parseOrder(response, market);
}

Json poloniex::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    auto request = Json::object();
    request["orderId"] = id;
    
    auto response = request("/orders/" + id, "private", "DELETE", request);
    return parseOrder(response);
}

Json poloniex::cancelAllOrdersImpl(const std::string& symbol) {
    auto request = Json::object();
    if (!symbol.empty()) {
        auto market = loadMarket(symbol);
        request["symbol"] = market["id"];
    }
    
    auto response = request("/orders", "private", "DELETE", request);
    return parseOrders(response);
}

Json poloniex::fetchOrderImpl(const std::string& id, const std::string& symbol) const {
    auto market = loadMarket(symbol);
    auto request = Json::object();
    request["orderId"] = id;
    
    auto response = request("/orders/" + id, "private", "GET", request);
    return parseOrder(response, market);
}

Json poloniex::fetchOrdersImpl(const std::string& symbol,
                             const std::optional<long long>& since,
                             const std::optional<int>& limit) const {
    auto request = Json::object();
    if (!symbol.empty()) {
        auto market = loadMarket(symbol);
        request["symbol"] = market["id"];
    }
    if (since) {
        request["startTime"] = *since;
    }
    if (limit) {
        request["limit"] = *limit;
    }
    
    auto response = request("/orders/history", "private", "GET", request);
    return parseOrders(response);
}

Json poloniex::fetchOpenOrdersImpl(const std::string& symbol,
                                 const std::optional<long long>& since,
                                 const std::optional<int>& limit) const {
    auto request = Json::object();
    if (!symbol.empty()) {
        auto market = loadMarket(symbol);
        request["symbol"] = market["id"];
    }
    if (since) {
        request["startTime"] = *since;
    }
    if (limit) {
        request["limit"] = *limit;
    }
    
    auto response = request("/orders", "private", "GET", request);
    return parseOrders(response);
}

Json poloniex::fetchOrderTradesImpl(const std::string& id,
                                  const std::string& symbol,
                                  const std::optional<long long>& since,
                                  const std::optional<int>& limit) const {
    auto market = loadMarket(symbol);
    auto request = Json::object();
    request["orderId"] = id;
    
    if (since) {
        request["startTime"] = *since;
    }
    if (limit) {
        request["limit"] = *limit;
    }
    
    auto response = request("/trades/order/" + id, "private", "GET", request);
    return parseTrades(response, market);
}

Json poloniex::fetchMyTradesImpl(const std::string& symbol,
                               const std::optional<long long>& since,
                               const std::optional<int>& limit) const {
    auto request = Json::object();
    if (!symbol.empty()) {
        auto market = loadMarket(symbol);
        request["symbol"] = market["id"];
    }
    if (since) {
        request["startTime"] = *since;
    }
    if (limit) {
        request["limit"] = *limit;
    }
    
    auto response = request("/trades", "private", "GET", request);
    return parseTrades(response);
}

// Account API Methods
Json poloniex::fetchBalanceImpl() const {
    auto response = request("/accounts", "private", "GET");
    return parseBalance(response);
}

Json poloniex::createDepositAddressImpl(const std::string& code,
                                      const Json& params) {
    auto currency = loadCurrency(code);
    auto request = Json::object();
    request["currency"] = currency["id"];
    
    auto response = request("/wallets/address", "private", "POST", request);
    return parseDepositAddress(response, currency);
}

Json poloniex::fetchDepositAddressImpl(const std::string& code,
                                     const std::optional<std::string>& network) const {
    auto currency = loadCurrency(code);
    auto request = Json::object();
    request["currency"] = currency["id"];
    
    auto response = request("/wallets/addresses", "private", "GET", request);
    if (network) {
        for (const auto& item : response) {
            if (item["network"].get<std::string>() == *network) {
                return parseDepositAddress(item, currency);
            }
        }
        throw NotSupported("fetchDepositAddress() network " + *network + " not supported");
    }
    return parseDepositAddress(response[0], currency);
}

Json poloniex::fetchDepositsImpl(const std::optional<std::string>& code,
                                const std::optional<long long>& since,
                                const std::optional<int>& limit) const {
    auto request = Json::object();
    if (code) {
        auto currency = loadCurrency(*code);
        request["currency"] = currency["id"];
    }
    if (since) {
        request["startTime"] = *since;
    }
    if (limit) {
        request["limit"] = *limit;
    }
    
    auto response = request("/wallets/activity", "private", "GET", request);
    return parseTransactions(response, code, since, limit, {{"type", "deposit"}});
}

Json poloniex::fetchWithdrawalsImpl(const std::optional<std::string>& code,
                                   const std::optional<long long>& since,
                                   const std::optional<int>& limit) const {
    auto request = Json::object();
    if (code) {
        auto currency = loadCurrency(*code);
        request["currency"] = currency["id"];
    }
    if (since) {
        request["startTime"] = *since;
    }
    if (limit) {
        request["limit"] = *limit;
    }
    
    auto response = request("/wallets/activity", "private", "GET", request);
    return parseTransactions(response, code, since, limit, {{"type", "withdrawal"}});
}

Json poloniex::fetchDepositWithdrawFeesImpl(const std::vector<std::string>& codes) const {
    auto response = request("/currencies", "public", "GET");
    return parseDepositWithdrawFees(response, codes);
}

Json poloniex::transferImpl(const std::string& code,
                          double amount,
                          const std::string& fromAccount,
                          const std::string& toAccount,
                          const Json& params) {
    auto currency = loadCurrency(code);
    auto request = Json::object();
    request["currency"] = currency["id"];
    request["amount"] = amountToPrecision(code, amount);
    request["fromAccount"] = fromAccount;
    request["toAccount"] = toAccount;
    
    auto response = request("/accounts/transfer", "private", "POST", request);
    return parseTransfer(response, currency);
}

Json poloniex::withdrawImpl(const std::string& code,
                          double amount,
                          const std::string& address,
                          const std::optional<std::string>& tag,
                          const Json& params) {
    auto currency = loadCurrency(code);
    auto request = Json::object();
    request["currency"] = currency["id"];
    request["amount"] = amountToPrecision(code, amount);
    request["address"] = address;
    if (tag) {
        request["paymentId"] = *tag;
    }
    
    auto response = request("/wallets/withdraw", "private", "POST", request);
    return parseTransaction(response, currency);
}

// Helper Methods
Json poloniex::parseOrder(const Json& order, const Json& market) const {
    auto id = safeString(order, "orderId");
    auto timestamp = safeInteger(order, "createTime");
    auto status = parseOrderStatus(safeString(order, "state"));
    auto symbol = safeString(market, "symbol");
    auto type = safeStringLower(order, "type");
    auto side = safeStringLower(order, "side");
    auto price = safeString(order, "price");
    auto amount = safeString(order, "quantity");
    auto filled = safeString(order, "filledQuantity");
    auto remaining = safeString(order, "remainingQuantity");
    auto cost = safeString(order, "amount");
    
    return {
        {"id", id},
        {"info", order},
        {"timestamp", timestamp},
        {"datetime", iso8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"status", status},
        {"symbol", symbol},
        {"type", type},
        {"timeInForce", safeString(order, "timeInForce")},
        {"postOnly", safeValue(order, "postOnly")},
        {"side", side},
        {"price", price},
        {"stopPrice", safeString(order, "triggerPrice")},
        {"amount", amount},
        {"cost", cost},
        {"filled", filled},
        {"remaining", remaining},
        {"trades", nullptr},
        {"fee", nullptr},
        {"average", nullptr},
        {"clientOrderId", safeString(order, "clientOrderId")}
    };
}

std::string poloniex::parseOrderStatus(const std::string& status) const {
    static const std::map<std::string, std::string> statuses = {
        {"NEW", "open"},
        {"PARTIALLY_FILLED", "open"},
        {"FILLED", "closed"},
        {"PENDING_CANCEL", "canceling"},
        {"CANCELLED", "canceled"},
        {"EXPIRED", "expired"},
        {"REJECTED", "rejected"}
    };
    
    return safeString(statuses, status, status);
}

Json poloniex::parseTrade(const Json& trade, const Json& market) const {
    auto id = safeString2(trade, "id", "tradeID");
    auto orderId = safeString(trade, "orderId");
    auto timestamp = safeInteger2(trade, "ts", "createTime");
    auto symbol = safeString(market, "symbol");
    auto side = safeStringLower2(trade, "side", "takerSide");
    auto type = safeStringLower(trade, "type");
    auto price = safeString(trade, "price");
    auto amount = safeString(trade, "quantity");
    auto cost = safeString(trade, "amount");
    
    Json fee = nullptr;
    auto feeCurrencyId = safeString(trade, "feeCurrency");
    auto feeAmount = safeString(trade, "feeAmount");
    if (feeCurrencyId && feeAmount) {
        fee = {
            {"currency", safeCurrencyCode(feeCurrencyId)},
            {"cost", feeAmount}
        };
    }
    
    return {
        {"id", id},
        {"info", trade},
        {"timestamp", timestamp},
        {"datetime", iso8601(timestamp)},
        {"symbol", symbol},
        {"order", orderId},
        {"type", type},
        {"side", side},
        {"takerOrMaker", safeStringLower(trade, "matchRole")},
        {"price", price},
        {"amount", amount},
        {"cost", cost},
        {"fee", fee}
    };
}

// Async implementations for trading API
std::future<Json> poloniex::createOrderAsync(const std::string& symbol,
                                           const std::string& type,
                                           const std::string& side,
                                           double amount,
                                           const std::optional<double>& price) {
    return std::async(std::launch::async,
                     [this, symbol, type, side, amount, price]() {
                         return createOrderImpl(symbol, type, side, amount, price);
                     });
}

} // namespace ccxt
