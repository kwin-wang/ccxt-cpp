#include "ccxt/exchanges/coinspot.h"
#include <openssl/hmac.h>

namespace ccxt {

const std::string coinspot::defaultBaseURL = "https://www.coinspot.com.au";
const int coinspot::defaultRateLimit = 1000;
const bool coinspot::defaultPro = false;

ExchangeRegistry::Factory coinspot::factory("coinspot", &coinspot::createInstance);

coinspot::coinspot(const Config& config)
    : ExchangeImpl(config) {
    init();
}

void coinspot::init() {
    ExchangeImpl::init();
    this->id = "coinspot";
    this->name = "CoinSpot";
    this->countries = {"AU"};  // Australia
    this->rateLimit = defaultRateLimit;
    this->pro = defaultPro;

    if (this->urls.empty()) {
        this->urls["api"] = {
            {"public", defaultBaseURL + "/pubapi"},
            {"private", defaultBaseURL + "/api"}
        };
    }

    this->has = {
        {"CORS", nullptr},
        {"spot", true},
        {"cancelOrder", true},
        {"createOrder", true},
        {"fetchBalance", true},
        {"fetchMarkets", true},
        {"fetchMyTrades", true},
        {"fetchOHLCV", false},
        {"fetchOpenOrders", false},
        {"fetchClosedOrders", false},
        {"fetchOrder", false},
        {"fetchOrderBook", true}
    };
}

Json coinspot::describeImpl() const {
    return Json::object({
        {"id", this->id},
        {"name", this->name},
        {"countries", this->countries},
        {"rateLimit", this->rateLimit},
        {"pro", this->pro},
        {"has", this->has}
    });
}

Json coinspot::fetchMarketsImpl() const {
    Json response = this->publicGetMarkets();
    return this->parseMarkets(response);
}

Json coinspot::fetchTickerImpl(const std::string& symbol) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json response = this->publicGetTicker(Json::object({
        {"symbol", market["id"]}
    }));
    return this->parseTicker(response, market);
}

Json coinspot::fetchTickersImpl(const std::vector<std::string>& symbols) const {
    this->loadMarkets();
    Json response = this->publicGetTickers();
    return this->parseTickers(response, symbols);
}

Json coinspot::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json::object({
        {"symbol", market["id"]}
    });
    if (limit.has_value()) {
        request["limit"] = limit.value();
    }
    Json response = this->publicGetOrderBook(request);
    return this->parseOrderBook(response, market);
}

Json coinspot::createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side, double amount, const std::optional<double>& price) {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json::object({
        {"symbol", market["id"]},
        {"type", type},
        {"side", side},
        {"amount", this->amountToPrecision(symbol, amount)}
    });
    if (price.has_value()) {
        request["price"] = this->priceToPrecision(symbol, price.value());
    }
    Json response = this->privatePostOrder(request);
    return this->parseOrder(response, market);
}

Json coinspot::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json::object({
        {"symbol", market["id"]},
        {"orderId", id}
    });
    Json response = this->privateDeleteOrder(request);
    return this->parseOrder(response, market);
}

Json coinspot::fetchMyTradesImpl(const std::string& symbol, const std::optional<long long>& since, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json::object({
        {"symbol", market["id"]}
    });
    if (since.has_value()) {
        request["since"] = since.value();
    }
    if (limit.has_value()) {
        request["limit"] = limit.value();
    }
    Json response = this->privateGetMyTrades(request);
    return this->parseTrades(response, market, since, limit);
}

Json coinspot::fetchBalanceImpl() const {
    Json response = this->privateGetBalance();
    return this->parseBalance(response);
}

std::string coinspot::sign(const std::string& path, const std::string& api, const std::string& method,
                         const Json& params, const Json& headers, const Json& body) const {
    auto url = this->urls["api"][api] + "/" + path;
    
    if (api == "public") {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
        return url;
    }

    this->checkRequiredCredentials();
    auto nonce = std::to_string(this->milliseconds());
    auto auth = nonce + method + "/" + path;
    
    if (!params.empty()) {
        auth += "?" + this->urlencode(params);
    }

    auto signature = this->hmac(auth, this->secret, "sha256", "hex");
    
    return url;
}

void coinspot::handleErrors(const std::string& code, const std::string& reason, const std::string& url, const std::string& method,
                         const Json& headers, const Json& body, const Json& response, const std::string& requestHeaders,
                         const std::string& requestBody) const {
    if (response.is_object() && response.contains("code")) {
        auto errorCode = response["code"].get<int>();
        auto message = response.value("message", "Unknown error");

        if (errorCode != 0) {
            switch (errorCode) {
                case 10001:
                    throw InvalidOrder(message);
                case 10002:
                    throw OrderNotFound(message);
                case 10003:
                    throw InsufficientFunds(message);
                case 10004:
                    throw AuthenticationError(message);
                case 10005:
                    throw PermissionDenied(message);
                case 10006:
                    throw BadRequest(message);
                case 10007:
                    throw RateLimitExceeded(message);
                case 10008:
                    throw ExchangeError(message);
                default:
                    throw ExchangeError(message);
            }
        }
    }
}

} // namespace ccxt
