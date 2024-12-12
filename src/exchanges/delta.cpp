#include "ccxt/exchanges/delta.h"
#include "../base/error.h"
#include <chrono>
#include <sstream>
#include <iomanip>

namespace ccxt {

const std::string delta::defaultBaseURL = "https://api.delta.exchange";
const std::string delta::defaultVersion = "v2";
const int delta::defaultRateLimit = 300;
const bool delta::defaultPro = false;

delta::delta(const Config& config) : Exchange(config) {
    this->init();
}

void delta::init() {
    
    
    this->has = {
        {"CORS", nullptr},
        {"spot", true},
        {"margin", false},
        {"swap", true},
        {"future", false},
        {"option", true},
        {"addMargin", true},
        {"cancelAllOrders", true},
        {"cancelOrder", true},
        {"closeAllPositions", true},
        {"closePosition", false},
        {"createOrder", true},
        {"createReduceOnlyOrder", true},
        {"editOrder", true},
        {"fetchBalance", true},
        {"fetchClosedOrders", true},
        {"fetchCurrencies", true},
        {"fetchDepositAddress", true},
        {"fetchFundingRate", true},
        {"fetchFundingRates", true},
        {"fetchGreeks", true},
        {"fetchIndexOHLCV", true},
        {"fetchLedger", true},
        {"fetchLeverage", true},
        {"fetchMarginMode", true},
        {"fetchMarkets", true},
        {"fetchMarkOHLCV", true},
        {"fetchMyTrades", true},
        {"fetchOHLCV", true},
        {"fetchOpenInterest", true},
        {"fetchOpenOrders", true},
        {"fetchOption", true},
        {"fetchOrderBook", true},
        {"fetchPosition", true},
        {"fetchPositions", true},
        {"fetchSettlementHistory", true},
        {"fetchStatus", true},
        {"fetchTicker", true},
        {"fetchTickers", true},
        {"fetchTime", true},
        {"fetchTrades", true},
        {"reduceMargin", true},
        {"setLeverage", true}
    };

    this->urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/99450025-3be60a00-2931-11eb-9302-f4fd8d8589aa.jpg"},
        {"api", {
            {"public", defaultBaseURL + "/v2"},
            {"private", defaultBaseURL + "/v2"}
        }},
        {"www", "https://www.delta.exchange"},
        {"doc", {
            "https://docs.delta.exchange"
        }},
        {"fees", "https://www.delta.exchange/fees"}
    };

    this->api = {
        {"public", {
            {"get", {
                "assets",
                "products",
                "ticker",
                "l2orderbook/{symbol}",
                "trades/{symbol}",
                "candles",
                "funding_rate/{symbol}",
                "funding_rates",
                "mark_prices/{symbol}",
                "greeks/{symbol}",
                "open_interest/{symbol}"
            }}
        }},
        {"private", {
            {"get", {
                "orders",
                "orders/history",
                "positions",
                "positions/{symbol}",
                "trades",
                "fills",
                "wallet/balances",
                "wallet/transactions",
                "wallet/deposit_address"
            }},
            {"post", {
                "orders",
                "orders/batch",
                "positions/change_margin",
                "positions/change_leverage",
                "positions/close_all"
            }},
            {"put", {
                "orders/{id}"
            }},
            {"delete", {
                "orders",
                "orders/{id}"
            }}
        }}
    };

    this->timeframes = {
        {"1m", "1m"},
        {"3m", "3m"},
        {"5m", "5m"},
        {"15m", "15m"},
        {"30m", "30m"},
        {"1h", "1h"},
        {"2h", "2h"},
        {"4h", "4h"},
        {"6h", "6h"},
        {"12h", "12h"},
        {"1d", "1d"},
        {"1w", "1w"},
        {"1M", "1M"}
    };
}

Json delta::describeImpl() const {
    return {
        {"id", "delta"},
        {"name", "Delta Exchange"},
        {"countries", {"VC"}},  // Saint Vincent and the Grenadines
        {"version", defaultVersion},
        {"rateLimit", defaultRateLimit},
        {"pro", defaultPro},
        {"has", this->has},
        {"urls", this->urls},
        {"api", this->api},
        {"timeframes", this->timeframes}
    };
}

Json delta::fetchMarketsImpl() const {
    Json response = this->publicGetProducts();
    Json result = Json::array();
    
    for (const auto& market : response["result"]) {
        std::string id = market["symbol"];
        std::string baseId = market["base_currency"];
        std::string quoteId = market["quote_currency"];
        std::string base = this->safeCurrencyCode(baseId);
        std::string quote = this->safeCurrencyCode(quoteId);
        std::string symbol = base + "/" + quote;
        std::string type = market["contract_type"].get<std::string>();
        bool linear = type == "perpetual_futures";
        bool inverse = type == "inverse_perpetual_futures";
        bool spot = type == "spot";
        bool swap = linear || inverse;
        bool option = type == "call_options" || type == "put_options";
        
        result.push_back({
            {"id", id},
            {"symbol", symbol},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", market["active"]},
            {"type", type},
            {"spot", spot},
            {"margin", false},
            {"swap", swap},
            {"future", false},
            {"option", option},
            {"linear", linear},
            {"inverse", inverse},
            {"contract", swap || option},
            {"contractSize", this->safeNumber(market, "contract_value")},
            {"precision", {
                {"amount", market["size_precision"]},
                {"price", market["price_precision"]}
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeNumber(market, "min_size")},
                    {"max", this->safeNumber(market, "max_size")}
                }},
                {"price", {
                    {"min", this->safeNumber(market, "min_price")},
                    {"max", this->safeNumber(market, "max_price")}
                }},
                {"cost", {
                    {"min", this->safeNumber(market, "min_notional")},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return result;
}

Json delta::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = {
        {"symbol", market["id"]}
    };
    
    if (limit.has_value()) {
        request["depth"] = limit.value();
    }
    
    Json response = this->publicGetL2orderbookSymbol(request);
    return this->parseOrderBook(response["result"], symbol);
}

Json delta::createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side,
                         double amount, const std::optional<double>& price) {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = {
        {"symbol", market["id"]},
        {"side", side.substr(0, 1).c_str() + side.substr(1)},  // Capitalize first letter
        {"order_type", type.substr(0, 1).c_str() + type.substr(1)},  // Capitalize first letter
        {"size", this->amountToPrecision(symbol, amount)}
    };
    
    if (price.has_value()) {
        request["price"] = this->priceToPrecision(symbol, price.value());
    }
    
    Json response = this->privatePostOrders(request);
    return this->parseOrder(response["result"], market);
}

Json delta::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = {
        {"symbol", market["id"]},
        {"order_id", id}
    };
    
    Json response = this->privateDeleteOrdersId(request);
    return this->parseOrder(response["result"], market);
}

Json delta::fetchBalanceImpl() const {
    Json response = this->privateGetWalletBalances();
    Json result = {{"info", response}};
    
    Json balances = response["result"];
    for (const auto& balance : balances) {
        std::string currencyId = balance["asset"];
        std::string code = this->safeCurrencyCode(currencyId);
        Json account = this->account();
        account["free"] = this->safeString(balance, "available_balance");
        account["used"] = this->safeString(balance, "order_margin");
        account["total"] = this->safeString(balance, "balance");
        result[code] = account;
    }
    
    return this->parseBalance(result);
}

Json delta::fetchMyTradesImpl(const std::string& symbol, const std::optional<long long>& since,
                           const std::optional<int>& limit) const {
    this->loadMarkets();
    Json request = Json::object();
    Json market = nullptr;
    
    if (!symbol.empty()) {
        market = this->market(symbol);
        request["symbol"] = market["id"];
    }
    
    if (since.has_value()) {
        request["start_time"] = since.value();
    }
    
    if (limit.has_value()) {
        request["limit"] = limit.value();
    }
    
    Json response = this->privateGetTrades(request);
    return this->parseTrades(response["result"], market, since, limit);
}

Json delta::parseTrade(const Json& trade, const Json& market) const {
    std::string id = this->safeString(trade, "id");
    std::string orderId = this->safeString(trade, "order_id");
    long long timestamp = this->safeInteger(trade, "created_at");
    std::string symbol = market["symbol"];
    std::string side = this->safeStringLower(trade, "side");
    std::string type = this->safeStringLower(trade, "order_type");
    double price = this->safeNumber(trade, "price");
    double amount = this->safeNumber(trade, "size");
    double cost = price * amount;
    
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
        {"fee", {
            {"cost", this->safeNumber(trade, "fee")},
            {"currency", this->safeCurrencyCode(trade["fee_currency"])}
        }}
    };
}

std::string delta::sign(const std::string& path, const std::string& api, const std::string& method,
                      const Json& params, const Json& headers, const Json& body) const {
    std::string url = this->urls["api"][api] + "/" + this->implodeParams(path, params);
    Json query = this->omit(params, this->extractParams(path));
    
    if (api == "public") {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->checkRequiredCredentials();
        long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        Json auth = {
            {"api-key", this->apiKey},
            {"timestamp", timestamp}
        };
        
        std::string payload;
        if (method == "GET" || method == "DELETE") {
            if (!query.empty()) {
                url += "?" + this->urlencode(query);
            }
            payload = std::to_string(timestamp) + method + "/v2/" + path;
        } else {
            auth["request-body"] = this->json(query);
            payload = std::to_string(timestamp) + method + "/v2/" + path + this->json(query);
        }
        
        auth["signature"] = this->hmac(payload, this->secret, "sha256", "hex");
        headers["api-key"] = this->apiKey;
        headers["timestamp"] = std::to_string(timestamp);
        headers["signature"] = auth["signature"];
        headers["Content-Type"] = "application/json";
        
        if (method != "GET" && method != "DELETE") {
            body = this->json(query);
        }
    }
    
    return url;
}

void delta::handleErrors(const std::string& code, const std::string& reason, const std::string& url,
                       const std::string& method, const Json& headers, const Json& body,
                       const Json& response, const std::string& requestHeaders,
                       const std::string& requestBody) const {
    if (response.is_object() && response.contains("error")) {
        Json error = response["error"];
        std::string errorCode = this->safeString(error, "code");
        std::string message = this->safeString(error, "message", "Unknown error");
        
        if (!errorCode.empty()) {
            const std::map<std::string, ExceptionType> exceptions = {
                {"invalid_parameter", BadRequest},
                {"invalid_signature", AuthenticationError},
                {"missing_required_param", ArgumentsRequired},
                {"not_found", OrderNotFound},
                {"insufficient_margin", InsufficientFunds},
                {"invalid_order", InvalidOrder},
                {"server_error", ExchangeError},
                {"rate_limit_exceeded", ExchangeError}
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
