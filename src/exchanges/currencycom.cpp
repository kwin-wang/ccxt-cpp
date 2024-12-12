#include "ccxt/exchanges/currencycom.h"
#include "../base/error.h"
#include <chrono>
#include <sstream>
#include <iomanip>

namespace ccxt {

const std::string currencycom::defaultBaseURL = "https://api-adapter.backend.currency.com/api/v2";
const std::string currencycom::defaultVersion = "v2";
const int currencycom::defaultRateLimit = 100;
const bool currencycom::defaultPro = true;

currencycom::currencycom(const Config& config) : Exchange(config) {
    this->init();
}

void currencycom::init() {
    
    
    this->has = {
        {"CORS", nullptr},
        {"spot", true},
        {"margin", true},
        {"swap", true},
        {"future", false},
        {"option", false},
        {"addMargin", nullptr},
        {"cancelAllOrders", nullptr},
        {"cancelOrder", true},
        {"createLimitOrder", true},
        {"createMarketOrder", true},
        {"createOrder", true},
        {"createStopLimitOrder", true},
        {"createStopMarketOrder", true},
        {"createStopOrder", true},
        {"fetchAccounts", true},
        {"fetchBalance", true},
        {"fetchCurrencies", true},
        {"fetchDepositAddress", true},
        {"fetchDeposits", true},
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
        {"fetchTrades", true},
        {"fetchWithdrawals", true}
    };

    this->urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/83718672-36745c00-a63e-11ea-81a9-677b1f789a4d.jpg"},
        {"api", {
            {"public", defaultBaseURL + "/public"},
            {"private", defaultBaseURL + "/private"}
        }},
        {"www", "https://currency.com"},
        {"doc", {
            "https://currency.com/api",
            "https://currency.com/api-documentation"
        }},
        {"fees", "https://currency.com/fees-charges"}
    };

    this->api = {
        {"public", {
            {"get", {
                "time",
                "exchangeInfo",
                "depth",
                "trades",
                "ticker/24hr",
                "ticker/price",
                "ticker/bookTicker",
                "klines"
            }}
        }},
        {"private", {
            {"get", {
                "account",
                "myTrades",
                "openOrders",
                "allOrders",
                "depositAddress",
                "depositHistory",
                "withdrawHistory",
                "positions"
            }},
            {"post", {
                "order",
                "order/test"
            }},
            {"delete", {
                "order"
            }}
        }}
    };
}

Json currencycom::describeImpl() const {
    return {
        {"id", "currencycom"},
        {"name", "Currency.com"},
        {"countries", {"BY"}},
        {"version", defaultVersion},
        {"rateLimit", defaultRateLimit},
        {"pro", defaultPro},
        {"has", this->has},
        {"urls", this->urls},
        {"api", this->api},
        {"timeframes", {
            {"1m", "1m"},
            {"5m", "5m"},
            {"15m", "15m"},
            {"30m", "30m"},
            {"1h", "1h"},
            {"4h", "4h"},
            {"1d", "1d"},
            {"1w", "1w"},
            {"1M", "1M"}
        }}
    };
}

Json currencycom::fetchMarketsImpl() const {
    Json response = this->publicGetExchangeInfo();
    Json markets = response["symbols"];
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
            {"active", true},
            {"type", "spot"},
            {"spot", true},
            {"margin", market.value("marginTrading", false)},
            {"future", false},
            {"swap", false},
            {"option", false},
            {"contract", false},
            {"precision", {
                {"amount", market["quantityPrecision"]},
                {"price", market["pricePrecision"]}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeNumber(market, "minQty")},
                    {"max", this->safeNumber(market, "maxQty")}
                }},
                {"price", {
                    {"min", this->safeNumber(market, "minPrice")},
                    {"max", this->safeNumber(market, "maxPrice")}
                }},
                {"cost", {
                    {"min", this->safeNumber(market, "minNotional")},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return result;
}

Json currencycom::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = {
        {"symbol", market["id"]}
    };
    
    if (limit.has_value()) {
        request["limit"] = limit.value();
    }
    
    Json response = this->publicGetDepth(request);
    return this->parseOrderBook(response, symbol);
}

Json currencycom::createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side,
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
    return this->parseOrder(response, market);
}

Json currencycom::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = {
        {"symbol", market["id"]},
        {"orderId", id}
    };
    
    Json response = this->privateDeleteOrder(request);
    return this->parseOrder(response, market);
}

Json currencycom::fetchBalanceImpl() const {
    Json response = this->privateGetAccount();
    Json result = {{"info", response}};
    
    Json balances = response["balances"];
    for (const auto& balance : balances) {
        std::string currencyId = balance["asset"];
        std::string code = this->safeCurrencyCode(currencyId);
        Json account = this->account();
        account["free"] = this->safeString(balance, "free");
        account["used"] = this->safeString(balance, "locked");
        result[code] = account;
    }
    
    return this->parseBalance(result);
}

Json currencycom::fetchMyTradesImpl(const std::string& symbol, const std::optional<long long>& since,
                                 const std::optional<int>& limit) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = {
        {"symbol", market["id"]}
    };
    
    if (since.has_value()) {
        request["startTime"] = since.value();
    }
    
    if (limit.has_value()) {
        request["limit"] = limit.value();
    }
    
    Json response = this->privateGetMyTrades(request);
    return this->parseTrades(response, market, since, limit);
}

Json currencycom::parseTrade(const Json& trade, const Json& market) const {
    std::string id = this->safeString(trade, "id");
    std::string orderId = this->safeString(trade, "orderId");
    long long timestamp = this->safeInteger(trade, "time");
    std::string symbol = market["symbol"];
    std::string side = this->safeStringLower(trade, "side");
    std::string type = this->safeStringLower(trade, "type");
    double price = this->safeNumber(trade, "price");
    double amount = this->safeNumber(trade, "qty");
    double cost = this->safeNumber(trade, "quoteQty");
    
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

std::string currencycom::sign(const std::string& path, const std::string& api, const std::string& method,
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
        std::string auth = query + this->secret;
        std::string signature = this->hmac(auth, this->secret, "sha256");
        query += "&signature=" + signature;
        
        if (method == "GET") {
            url += "?" + query;
        } else {
            body = query;
            headers["Content-Type"] = "application/x-www-form-urlencoded";
        }
        
        headers["X-MBX-APIKEY"] = this->apiKey;
    }
    
    return url;
}

void currencycom::handleErrors(const std::string& code, const std::string& reason, const std::string& url,
                            const std::string& method, const Json& headers, const Json& body,
                            const Json& response, const std::string& requestHeaders,
                            const std::string& requestBody) const {
    if (response.is_object() && response.contains("code")) {
        std::string errorCode = response["code"].get<std::string>();
        std::string message = response.value("msg", "Unknown error");
        
        if (errorCode != "0") {
            const std::map<std::string, ExceptionType> exceptions = {
                {"-1000", BadRequest},
                {"-1013", InvalidOrder},
                {"-1021", InvalidNonce},
                {"-1022", AuthenticationError},
                {"-1100", BadRequest},
                {"-1104", BadRequest},
                {"-1130", BadRequest},
                {"-2010", BadRequest},
                {"-2011", OrderNotFound},
                {"-2013", OrderNotFound},
                {"-2014", AuthenticationError},
                {"-2015", AuthenticationError}
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
