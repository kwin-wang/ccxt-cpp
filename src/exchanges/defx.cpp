#include "ccxt/exchanges/defx.h"
#include "../base/error.h"
#include <chrono>
#include <sstream>
#include <iomanip>

namespace ccxt {

const std::string defx::defaultBaseURL = "https://api.defx.com";
const std::string defx::defaultVersion = "v1";
const int defx::defaultRateLimit = 100;
const bool defx::defaultPro = false;

defx::defx(const Config& config) : Exchange(config) {
    this->init();
}

void defx::init() {
    
    
    this->has = {
        {"CORS", nullptr},
        {"spot", false},
        {"margin", false},
        {"swap", true},
        {"future", false},
        {"option", false},
        {"addMargin", true},
        {"cancelAllOrders", true},
        {"cancelOrder", true},
        {"closeAllPositions", true},
        {"closePosition", true},
        {"createOrder", true},
        {"createOrderWithTakeProfitAndStopLoss", true},
        {"createReduceOnlyOrder", true},
        {"createTakeProfitOrder", true},
        {"createTriggerOrder", true},
        {"fetchBalance", true},
        {"fetchCanceledOrders", true},
        {"fetchClosedOrders", true},
        {"fetchFundingRate", true},
        {"fetchLedger", true},
        {"fetchMarkets", true},
        {"fetchMyTrades", true},
        {"fetchOHLCV", true},
        {"fetchOpenOrders", true},
        {"fetchOrder", true},
        {"fetchOrderBook", true},
        {"fetchPositions", true},
        {"fetchTicker", true},
        {"fetchTickers", true},
        {"fetchTrades", true}
    };

    this->urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/84547058-5fb27d80-ad0b-11ea-8711-78ac8b3c7f31.jpg"},
        {"api", {
            {"public", defaultBaseURL + "/api/" + defaultVersion + "/public"},
            {"private", defaultBaseURL + "/api/" + defaultVersion + "/private"}
        }},
        {"www", "https://defx.com"},
        {"doc", {
            "https://docs.defx.com"
        }},
        {"fees", "https://defx.com/fees"}
    };

    this->api = {
        {"public", {
            {"get", {
                "markets",
                "ticker",
                "orderbook",
                "trades",
                "klines",
                "funding-rate"
            }}
        }},
        {"private", {
            {"get", {
                "account",
                "positions",
                "orders",
                "orders/history",
                "trades",
                "ledger"
            }},
            {"post", {
                "order",
                "order/close",
                "position/close-all"
            }},
            {"delete", {
                "order",
                "orders"
            }}
        }}
    };

    this->timeframes = {
        {"1m", "1m"},
        {"5m", "5m"},
        {"15m", "15m"},
        {"30m", "30m"},
        {"1h", "1h"},
        {"4h", "4h"},
        {"1d", "1d"},
        {"1w", "1w"},
        {"1M", "1M"}
    };
}

Json defx::describeImpl() const {
    return {
        {"id", "defx"},
        {"name", "Defx X"},
        {"version", defaultVersion},
        {"rateLimit", defaultRateLimit},
        {"pro", defaultPro},
        {"has", this->has},
        {"urls", this->urls},
        {"api", this->api},
        {"timeframes", this->timeframes}
    };
}

Json defx::fetchMarketsImpl() const {
    Json response = this->publicGetMarkets();
    Json markets = response["data"];
    Json result = Json::array();
    
    for (const auto& market : markets) {
        std::string id = market["symbol"];
        std::string baseId = market["baseAsset"];
        std::string quoteId = market["quoteAsset"];
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
            {"active", market["active"]},
            {"type", "swap"},
            {"spot", false},
            {"margin", false},
            {"swap", true},
            {"future", false},
            {"option", false},
            {"contract", true},
            {"precision", {
                {"amount", market["amountPrecision"]},
                {"price", market["pricePrecision"]}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeNumber(market, "minOrderAmount")},
                    {"max", this->safeNumber(market, "maxOrderAmount")}
                }},
                {"price", {
                    {"min", this->safeNumber(market, "minOrderPrice")},
                    {"max", this->safeNumber(market, "maxOrderPrice")}
                }},
                {"cost", {
                    {"min", this->safeNumber(market, "minOrderValue")},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return result;
}

Json defx::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = {
        {"symbol", market["id"]}
    };
    
    if (limit.has_value()) {
        request["limit"] = limit.value();
    }
    
    Json response = this->publicGetOrderbook(request);
    return this->parseOrderBook(response["data"], symbol);
}

Json defx::createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side,
                        double amount, const std::optional<double>& price) {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = {
        {"symbol", market["id"]},
        {"side", side.substr(0, 1).c_str() + side.substr(1)},  // Capitalize first letter
        {"type", type.substr(0, 1).c_str() + type.substr(1)},  // Capitalize first letter
        {"quantity", this->amountToPrecision(symbol, amount)}
    };
    
    if (price.has_value()) {
        request["price"] = this->priceToPrecision(symbol, price.value());
    }
    
    Json response = this->privatePostOrder(request);
    return this->parseOrder(response["data"], market);
}

Json defx::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = {
        {"symbol", market["id"]},
        {"orderId", id}
    };
    
    Json response = this->privateDeleteOrder(request);
    return this->parseOrder(response["data"], market);
}

Json defx::fetchBalanceImpl() const {
    Json response = this->privateGetAccount();
    Json result = {{"info", response}};
    
    Json balances = response["data"]["balances"];
    for (const auto& balance : balances) {
        std::string currencyId = balance["asset"];
        std::string code = this->safeCurrencyCode(currencyId);
        Json account = this->account();
        account["free"] = this->safeString(balance, "available");
        account["used"] = this->safeString(balance, "frozen");
        result[code] = account;
    }
    
    return this->parseBalance(result);
}

Json defx::fetchMyTradesImpl(const std::string& symbol, const std::optional<long long>& since,
                          const std::optional<int>& limit) const {
    this->loadMarkets();
    Json request = Json::object();
    Json market = nullptr;
    
    if (!symbol.empty()) {
        market = this->market(symbol);
        request["symbol"] = market["id"];
    }
    
    if (since.has_value()) {
        request["startTime"] = since.value();
    }
    
    if (limit.has_value()) {
        request["limit"] = limit.value();
    }
    
    Json response = this->privateGetTrades(request);
    return this->parseTrades(response["data"], market, since, limit);
}

Json defx::parseTrade(const Json& trade, const Json& market) const {
    std::string id = this->safeString(trade, "id");
    std::string orderId = this->safeString(trade, "orderId");
    long long timestamp = this->safeInteger(trade, "time");
    std::string symbol = market["symbol"];
    std::string side = this->safeStringLower(trade, "side");
    std::string type = this->safeStringLower(trade, "type");
    double price = this->safeNumber(trade, "price");
    double amount = this->safeNumber(trade, "quantity");
    double cost = this->safeNumber(trade, "quoteQuantity");
    
    return {
        {"info", trade},
        {"id", id},
        {"order", orderId},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"symbol", symbol},
        {"type", type},
        {"side", side},
        {"takerOrMaker", nullptr},
        {"price", price},
        {"amount", amount},
        {"cost", cost},
        {"fee", nullptr}
    };
}

std::string defx::sign(const std::string& path, const std::string& api, const std::string& method,
                     const Json& params, const Json& headers, const Json& body) const {
    std::string url = this->urls["api"][api] + "/" + path;
    
    if (api == "public") {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    } else {
        this->checkRequiredCredentials();
        long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        Json allParams = this->extend({
            {"timestamp", timestamp}
        }, params);
        
        std::string query = this->urlencode(allParams);
        std::string auth = query + this->config_.secret;
        std::string signature = this->hmac(auth, this->config_.secret, "sha256");
        query += "&signature=" + signature;
        
        if (method == "GET") {
            url += "?" + query;
        } else {
            body = query;
            headers["Content-Type"] = "application/x-www-form-urlencoded";
        }
        
        headers["X-API-KEY"] = this->config_.apiKey;
    }
    
    return url;
}

void defx::handleErrors(const std::string& code, const std::string& reason, const std::string& url,
                      const std::string& method, const Json& headers, const Json& body,
                      const Json& response, const std::string& requestHeaders,
                      const std::string& requestBody) const {
    if (response.is_object() && response.contains("code")) {
        std::string errorCode = response["code"].get<std::string>();
        std::string message = response.value("message", "Unknown error");
        
        if (errorCode != "0" && errorCode != "200") {
            const std::map<std::string, ExceptionType> exceptions = {
                {"400", BadRequest},
                {"401", AuthenticationError},
                {"403", AuthenticationError},
                {"404", BadRequest},
                {"405", BadRequest},
                {"429", BadRequest},
                {"500", ExchangeError},
                {"503", ExchangeError},
                {"1001", BadRequest},
                {"1002", AuthenticationError},
                {"1003", InvalidOrder},
                {"1004", InvalidOrder},
                {"1005", InvalidOrder}
            };
            
            auto it = exceptions.find(errorCode);
            if (it != exceptions.end()) {
                throw it->second(message);
            }
            
            throw ExchangeError(message);
        }
    }
}

} // namespace ccxt
