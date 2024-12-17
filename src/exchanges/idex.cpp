#include "../../include/ccxt/exchanges/idex.h"
#include "../../include/ccxt/exchange_registry.h"
#include <future>

namespace ccxt {

const std::string idex::defaultBaseURL = "https://api.idex.io";
const std::string idex::defaultVersion = "v3";
const int idex::defaultRateLimit = 1000;
const bool idex::defaultPro = true;

idex::idex(const Config& config) : Exchange(config) {
    init();
}

void idex::init() {
    
    setBaseURL(defaultBaseURL);
    setVersion(defaultVersion);
    setRateLimit(defaultRateLimit);
    setPro(defaultPro);

    // Set up markets and currencies
    setRequiredCredentials(Json::object({
        {"apiKey", true},
        {"secret", true},
        {"walletAddress", true}
    }));

    // Set up API endpoints
    setUrls(Json::object({
        {"api", {
            {"public", "https://api.idex.io/v3"},
            {"private", "https://api.idex.io/v3"}
        }},
        {"www", "https://idex.io"},
        {"doc", "https://docs.idex.io/"},
        {"fees", "https://idex.io/fees"}
    }));

    // Set up trading fees
    setFees(Json::object({
        {"trading", Json::object({
            {"maker", 0.001},  // 0.1%
            {"taker", 0.002}   // 0.2%
        })}
    }));
}

Json idex::describeImpl() const {
    return Json::object({
        {"id", "idex"},
        {"name", "IDEX"},
        {"countries", Json::array({"US"})},
        {"rateLimit", defaultRateLimit},
        {"version", defaultVersion},
        {"certified", true},
        {"pro", defaultPro},
        {"has", Json::object({
            {"spot", true},
            {"margin", false},
            {"swap", false},
            {"future", false},
            {"option", false},
            {"createOrder", true},
            {"cancelOrder", true},
            {"fetchBalance", true},
            {"fetchMarkets", true},
            {"fetchCurrencies", true},
            {"fetchOrderBook", true},
            {"fetchTicker", true},
            {"fetchTickers", true},
            {"fetchTrades", true},
            {"fetchOHLCV", true},
            {"fetchOrder", true},
            {"fetchOrders", true},
            {"fetchOpenOrders", true},
            {"fetchClosedOrders", true},
            {"fetchMyTrades", true},
            {"fetchDeposits", true},
            {"fetchWithdrawals", true},
            {"fetchDepositAddress", true}
        })}
    });
}

// Async Market Data Methods
AsyncPullType idex::fetchMarketsAsync() const {
    return std::async(std::launch::async, [this]() {
        return this->fetchMarketsImpl();
    });
}

AsyncPullType idex::fetchCurrenciesAsync() const {
    return std::async(std::launch::async, [this]() {
        return this->fetchCurrenciesImpl();
    });
}

AsyncPullType idex::fetchTickerAsync(const std::string& symbol) const {
    return std::async(std::launch::async, [this, symbol]() {
        return this->fetchTickerImpl(symbol);
    });
}

AsyncPullType idex::fetchTickersAsync(const std::vector<std::string>& symbols) const {
    return std::async(std::launch::async, [this, symbols]() {
        return this->fetchTickersImpl(symbols);
    });
}

AsyncPullType idex::fetchOrderBookAsync(const std::string& symbol, const std::optional<int>& limit) const {
    return std::async(std::launch::async, [this, symbol, limit]() {
        return this->fetchOrderBookImpl(symbol, limit);
    });
}

AsyncPullType idex::fetchOHLCVAsync(const std::string& symbol, const std::string& timeframe,
                                      const std::optional<long long>& since, const std::optional<int>& limit) const {
    return std::async(std::launch::async, [this, symbol, timeframe, since, limit]() {
        return this->fetchOHLCVImpl(symbol, timeframe, since, limit);
    });
}

AsyncPullType idex::fetchTradesAsync(const std::string& symbol, const std::optional<long long>& since,
                                       const std::optional<int>& limit) const {
    return std::async(std::launch::async, [this, symbol, since, limit]() {
        return this->fetchTradesImpl(symbol, since, limit);
    });
}

// Async Trading Methods
AsyncPullType idex::createOrderAsync(const std::string& symbol, const std::string& type,
                                       const std::string& side, double amount, const std::optional<double>& price) {
    return std::async(std::launch::async, [this, symbol, type, side, amount, price]() {
        return this->createOrderImpl(symbol, type, side, amount, price);
    });
}

AsyncPullType idex::cancelOrderAsync(const std::string& id, const std::string& symbol) {
    return std::async(std::launch::async, [this, id, symbol]() {
        return this->cancelOrderImpl(id, symbol);
    });
}

AsyncPullType idex::fetchOrderAsync(const std::string& id, const std::string& symbol) const {
    return std::async(std::launch::async, [this, id, symbol]() {
        return this->fetchOrderImpl(id, symbol);
    });
}

AsyncPullType idex::fetchOpenOrdersAsync(const std::string& symbol, const std::optional<long long>& since,
                                           const std::optional<int>& limit) const {
    return std::async(std::launch::async, [this, symbol, since, limit]() {
        return this->fetchOpenOrdersImpl(symbol, since, limit);
    });
}

AsyncPullType idex::fetchClosedOrdersAsync(const std::string& symbol, const std::optional<long long>& since,
                                             const std::optional<int>& limit) const {
    return std::async(std::launch::async, [this, symbol, since, limit]() {
        return this->fetchClosedOrdersImpl(symbol, since, limit);
    });
}

AsyncPullType idex::fetchMyTradesAsync(const std::string& symbol, const std::optional<long long>& since,
                                         const std::optional<int>& limit) const {
    return std::async(std::launch::async, [this, symbol, since, limit]() {
        return this->fetchMyTradesImpl(symbol, since, limit);
    });
}

// Async Account Methods
AsyncPullType idex::fetchBalanceAsync() const {
    return std::async(std::launch::async, [this]() {
        return this->fetchBalanceImpl();
    });
}

AsyncPullType idex::fetchDepositAddressAsync(const std::string& code, const std::optional<std::string>& network) const {
    return std::async(std::launch::async, [this, code, network]() {
        return this->fetchDepositAddressImpl(code, network);
    });
}

AsyncPullType idex::fetchDepositsAsync(const std::optional<std::string>& code, const std::optional<long long>& since,
                                         const std::optional<int>& limit) const {
    return std::async(std::launch::async, [this, code, since, limit]() {
        return this->fetchDepositsImpl(code, since, limit);
    });
}

AsyncPullType idex::fetchWithdrawalsAsync(const std::optional<std::string>& code, const std::optional<long long>& since,
                                           const std::optional<int>& limit) const {
    return std::async(std::launch::async, [this, code, since, limit]() {
        return this->fetchWithdrawalsImpl(code, since, limit);
    });
}

// Synchronous Implementation Methods
Json idex::fetchMarketsImpl() const {
    const std::string endpoint = "/markets";
    const Json response = this->publicGetMarkets();
    return response;
}

Json idex::fetchCurrenciesImpl() const {
    const std::string endpoint = "/assets";
    const Json response = this->publicGetAssets();
    return response;
}

Json idex::fetchTickerImpl(const std::string& symbol) const {
    const std::string endpoint = "/tickers";
    const Json params = Json::object({{"market", symbol}});
    const Json response = this->publicGetTickers(params);
    return this->parseTicker(response, this->market(symbol));
}

Json idex::fetchTickersImpl(const std::vector<std::string>& symbols) const {
    const std::string endpoint = "/tickers";
    const Json response = this->publicGetTickers();
    return response;
}

Json idex::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
    const std::string endpoint = "/orderbook";
    Json params = Json::object({{"market", symbol}});
    if (limit) {
        params["limit"] = *limit;
    }
    const Json response = this->publicGetOrderbook(params);
    return response;
}

Json idex::fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe,
                         const std::optional<long long>& since, const std::optional<int>& limit) const {
    const std::string endpoint = "/candles";
    Json params = Json::object({
        {"market", symbol},
        {"interval", timeframe}
    });
    if (since) {
        params["start"] = *since;
    }
    if (limit) {
        params["limit"] = *limit;
    }
    const Json response = this->publicGetCandles(params);
    return response;
}

Json idex::fetchTradesImpl(const std::string& symbol, const std::optional<long long>& since,
                          const std::optional<int>& limit) const {
    const std::string endpoint = "/trades";
    Json params = Json::object({{"market", symbol}});
    if (since) {
        params["start"] = *since;
    }
    if (limit) {
        params["limit"] = *limit;
    }
    const Json response = this->publicGetTrades(params);
    return response;
}

Json idex::createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side,
                         double amount, const std::optional<double>& price) {
    Json params = Json::object({
        {"market", symbol},
        {"type", type},
        {"side", side},
        {"amount", std::to_string(amount)}
    });
    if (price) {
        params["price"] = std::to_string(*price);
    }
    const Json response = this->privatePostOrder(params);
    return this->parseOrder(response);
}

Json idex::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    const Json params = Json::object({
        {"orderId", id},
        {"market", symbol}
    });
    const Json response = this->privateDeleteOrder(params);
    return response;
}

Json idex::fetchOrderImpl(const std::string& id, const std::string& symbol) const {
    const Json params = Json::object({
        {"orderId", id},
        {"market", symbol}
    });
    const Json response = this->privateGetOrder(params);
    return this->parseOrder(response);
}

Json idex::fetchOpenOrdersImpl(const std::string& symbol, const std::optional<long long>& since,
                             const std::optional<int>& limit) const {
    Json params = Json::object();
    if (!symbol.empty()) {
        params["market"] = symbol;
    }
    if (since) {
        params["start"] = *since;
    }
    if (limit) {
        params["limit"] = *limit;
    }
    const Json response = this->privateGetOrders(params);
    return response;
}

Json idex::fetchClosedOrdersImpl(const std::string& symbol, const std::optional<long long>& since,
                               const std::optional<int>& limit) const {
    Json params = Json::object({{"status", "closed"}});
    if (!symbol.empty()) {
        params["market"] = symbol;
    }
    if (since) {
        params["start"] = *since;
    }
    if (limit) {
        params["limit"] = *limit;
    }
    const Json response = this->privateGetOrders(params);
    return response;
}

Json idex::fetchMyTradesImpl(const std::string& symbol, const std::optional<long long>& since,
                           const std::optional<int>& limit) const {
    Json params = Json::object();
    if (!symbol.empty()) {
        params["market"] = symbol;
    }
    if (since) {
        params["start"] = *since;
    }
    if (limit) {
        params["limit"] = *limit;
    }
    const Json response = this->privateGetTrades(params);
    return response;
}

Json idex::fetchBalanceImpl() const {
    const Json response = this->privateGetBalances();
    return this->parseBalance(response);
}

Json idex::fetchDepositAddressImpl(const std::string& code, const std::optional<std::string>& network) const {
    Json params = Json::object({{"asset", code}});
    if (network) {
        params["network"] = *network;
    }
    const Json response = this->privateGetDepositAddress(params);
    return response;
}

Json idex::fetchDepositsImpl(const std::optional<std::string>& code, const std::optional<long long>& since,
                           const std::optional<int>& limit) const {
    Json params = Json::object();
    if (code) {
        params["asset"] = *code;
    }
    if (since) {
        params["start"] = *since;
    }
    if (limit) {
        params["limit"] = *limit;
    }
    const Json response = this->privateGetDeposits(params);
    return response;
}

Json idex::fetchWithdrawalsImpl(const std::optional<std::string>& code, const std::optional<long long>& since,
                             const std::optional<int>& limit) const {
    Json params = Json::object();
    if (code) {
        params["asset"] = *code;
    }
    if (since) {
        params["start"] = *since;
    }
    if (limit) {
        params["limit"] = *limit;
    }
    const Json response = this->privateGetWithdrawals(params);
    return response;
}

std::string idex::sign(const std::string& path, const std::string& api, const std::string& method,
                      const Json& params, const Json& headers, const Json& body) const {
    std::string endpoint = "/" + this->version + path;
    std::string url = this->urls["api"][api] + endpoint;

    if (api == "public") {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    } else {
        this->checkRequiredCredentials();
        
        const long long timestamp = this->milliseconds();
        std::string payload = std::to_string(timestamp) + method + endpoint;
        
        if (!params.empty()) {
            if (method == "GET" || method == "DELETE") {
                url += "?" + this->urlencode(params);
                payload += this->urlencode(params);
            } else {
                payload += this->json(params);
            }
        }

        const std::string signature = this->hmac(payload, this->secret, "sha256", true);
        
        headers["IDEX-API-Key"] = this->apiKey;
        headers["IDEX-TIMESTAMP"] = std::to_string(timestamp);
        headers["IDEX-SIGNATURE"] = signature;
        headers["Content-Type"] = "application/json";
    }

    return url;
}

void idex::handleErrors(const std::string& code, const std::string& reason, const std::string& url, const std::string& method,
                       const Json& headers, const Json& body, const Json& response, const std::string& requestHeaders,
                       const std::string& requestBody) const {
    if (response.contains("code") && response.contains("message")) {
        const std::string errorCode = response["code"].get<std::string>();
        const std::string message = response["message"].get<std::string>();
        
        if (errorCode == "INVALID_SIGNATURE") {
            throw AuthenticationError(message);
        } else if (errorCode == "INVALID_API_KEY") {
            throw AuthenticationError(message);
        } else if (errorCode == "INVALID_PARAMETER") {
            throw BadRequest(message);
        } else if (errorCode == "INSUFFICIENT_FUNDS") {
            throw InsufficientFunds(message);
        } else if (errorCode == "ORDER_NOT_FOUND") {
            throw OrderNotFound(message);
        } else {
            throw ExchangeError(message);
        }
    }
}

} // namespace ccxt
