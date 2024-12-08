#include "ccxt/exchanges/delta.h"
#include <openssl/hmac.h>

namespace ccxt {

const std::string delta::defaultBaseURL = "https://api.delta.exchange";
const std::string delta::defaultVersion = "v2";
const int delta::defaultRateLimit = 300;
const bool delta::defaultPro = true;

ExchangeRegistry::Factory delta::factory("delta", &delta::createInstance);

delta::delta(const Config& config)
    : ExchangeImpl(config) {
    init();
}

void delta::init() {
    ExchangeImpl::init();
    this->id = "delta";
    this->name = "Delta Exchange";
    this->countries = {"VC"};  // Saint Vincent and the Grenadines
    this->rateLimit = defaultRateLimit;
    this->pro = defaultPro;

    if (this->urls.empty()) {
        this->urls["api"] = {
            {"public", defaultBaseURL + "/public"},
            {"private", defaultBaseURL + "/private"}
        };
    }

    this->has = {
        {"CORS", nullptr},
        {"spot", true},
        {"swap", true},
        {"option", true},
        {"addMargin", true},
        {"cancelAllOrders", true},
        {"cancelOrder", true},
        {"closeAllPositions", true},
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
        {"fetchTrades", true}
    };
}

Json delta::describeImpl() const {
    return Json::object({
        {"id", this->id},
        {"name", this->name},
        {"countries", this->countries},
        {"rateLimit", this->rateLimit},
        {"pro", this->pro},
        {"has", this->has}
    });
}

Json delta::fetchMarketsImpl() const {
    Json response = this->publicGetMarkets();
    return this->parseMarkets(response);
}

Json delta::fetchCurrenciesImpl() const {
    Json response = this->publicGetCurrencies();
    return this->parseCurrencies(response);
}

Json delta::fetchTickerImpl(const std::string& symbol) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json response = this->publicGetTicker(Json::object({
        {"symbol", market["id"]}
    }));
    return this->parseTicker(response, market);
}

Json delta::fetchTickersImpl(const std::vector<std::string>& symbols) const {
    this->loadMarkets();
    Json response = this->publicGetTickers();
    return this->parseTickers(response, symbols);
}

Json delta::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
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

Json delta::fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe, const std::optional<long long>& since, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json::object({
        {"symbol", market["id"]},
        {"timeframe", this->timeframes.at(timeframe)}
    });
    if (since.has_value()) {
        request["since"] = since.value();
    }
    if (limit.has_value()) {
        request["limit"] = limit.value();
    }
    Json response = this->publicGetOHLCV(request);
    return this->parseOHLCV(response, market, timeframe, since, limit);
}

Json delta::fetchFundingRatesImpl() const {
    Json response = this->publicGetFundingRates();
    return this->parseFundingRates(response);
}

Json delta::fetchGreeksImpl() const {
    Json response = this->publicGetGreeks();
    return this->parseGreeks(response);
}

Json delta::createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side, double amount, const std::optional<double>& price) {
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

Json delta::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json::object({
        {"symbol", market["id"]},
        {"orderId", id}
    });
    Json response = this->privateDeleteOrder(request);
    return this->parseOrder(response, market);
}

Json delta::fetchOrderImpl(const std::string& id, const std::string& symbol) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json::object({
        {"symbol", market["id"]},
        {"orderId", id}
    });
    Json response = this->privateGetOrder(request);
    return this->parseOrder(response, market);
}

Json delta::fetchOpenOrdersImpl(const std::string& symbol, const std::optional<long long>& since, const std::optional<int>& limit) const {
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
    Json response = this->privateGetOpenOrders(request);
    return this->parseOrders(response, market, since, limit);
}

Json delta::fetchClosedOrdersImpl(const std::string& symbol, const std::optional<long long>& since, const std::optional<int>& limit) const {
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
    Json response = this->privateGetClosedOrders(request);
    return this->parseOrders(response, market, since, limit);
}

Json delta::fetchMyTradesImpl(const std::string& symbol, const std::optional<long long>& since, const std::optional<int>& limit) const {
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

Json delta::fetchBalanceImpl() const {
    Json response = this->privateGetBalance();
    return this->parseBalance(response);
}

Json delta::fetchLedgerImpl(const std::optional<std::string>& code, const std::optional<long long>& since, const std::optional<int>& limit) const {
    Json request = Json::object();
    if (code.has_value()) {
        request["code"] = code.value();
    }
    if (since.has_value()) {
        request["since"] = since.value();
    }
    if (limit.has_value()) {
        request["limit"] = limit.value();
    }
    Json response = this->privateGetLedger(request);
    return this->parseLedger(response);
}

Json delta::fetchDepositAddressImpl(const std::string& code, const std::optional<std::string>& network) const {
    Json request = Json::object({
        {"currency", code}
    });
    if (network.has_value()) {
        request["network"] = network.value();
    }
    Json response = this->privateGetDepositAddress(request);
    return this->parseDepositAddress(response);
}

std::string delta::sign(const std::string& path, const std::string& api, const std::string& method,
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

void delta::handleErrors(const std::string& code, const std::string& reason, const std::string& url, const std::string& method,
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
