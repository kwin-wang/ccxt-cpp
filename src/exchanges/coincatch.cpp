#include "ccxt/exchanges/coincatch.h"
#include <openssl/hmac.h>

namespace ccxt {


coincatch::coincatch(const Config& config)
    : Exchange(config) {
    init();
}

void coincatch::init() {
    
    this->apiVersion = "v1";
    this->rateLimit = 50;  // 20 times per second
}

Json coincatch::describeImpl() const {
    return this->deepExtend(ExchangeImpl::describeImpl(), {
        {"id", "coincatch"},
        {"name", "CoinCatch"},
        {"countries", {"VG"}},  // British Virgin Islands
        {"version", "v1"},
        {"certified", false},
        {"pro", true},
        {"has", {
            {"CORS", nullptr},
            {"spot", true},
            {"margin", false},
            {"swap", true},
            {"future", false},
            {"option", false},
            {"addMargin", true},
            {"cancelAllOrders", true},
            {"cancelOrder", true},
            {"cancelOrders", true},
            {"createLimitBuyOrder", true},
            {"createLimitSellOrder", true},
            {"createMarketBuyOrder", true},
            {"createMarketBuyOrderWithCost", true},
            {"createMarketOrder", true},
            {"createMarketSellOrder", true},
            {"createOrder", true},
            {"createOrders", true},
            {"createOrderWithTakeProfitAndStopLoss", true},
            {"createPostOnlyOrder", true},
            {"createReduceOnlyOrder", true},
            {"createStopLimitOrder", true},
            {"createStopLossOrder", true},
            {"createStopMarketOrder", true},
            {"createStopOrder", true},
            {"createTakeProfitOrder", true},
            {"fetchBalance", true},
            {"fetchCurrencies", true},
            {"fetchDepositAddress", true},
            {"fetchDeposits", true},
            {"fetchFundingRate", true},
            {"fetchFundingRateHistory", true},
            {"fetchLedger", true},
            {"fetchLeverage", true}
        }}
    });
}

std::string coincatch::sign(const std::string& path, const std::string& api, const std::string& method,
                         const Json& params, const Json& headers, const Json& body) const {
    auto url = this->urls["api"][api] + "/" + this->version + "/" + path;
    
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

void coincatch::handleErrors(const std::string& code, const std::string& reason, const std::string& url, const std::string& method,
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
                    throw OnMaintenance(message);
                default:
                    throw ExchangeError(message);
            }
        }
    }
}

Json coincatch::fetchMarketsImpl() const {
    auto response = this->publicGetMarkets();
    return this->parseMarkets(response["data"]);
}

Json coincatch::fetchBalanceImpl() const {
    auto response = this->privateGetBalance();
    return this->parseBalance(response["data"]);
}

Json coincatch::createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side,
                            double amount, const std::optional<double>& price) {
    auto request = Json::object({
        {"symbol", symbol},
        {"side", side},
        {"type", type},
        {"amount", this->amountToPrecision(symbol, amount)}
    });

    if (price.has_value()) {
        request["price"] = this->priceToPrecision(symbol, price.value());
    }

    auto response = this->privatePostOrder(request);
    return this->parseOrder(response["data"]);
}

Json coincatch::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    auto request = Json::object({
        {"symbol", symbol},
        {"orderId", id}
    });

    auto response = this->privateDeleteOrder(request);
    return this->parseOrder(response["data"]);
}

Json coincatch::fetchLeverageImpl(const std::string& symbol, const std::optional<Json>& params) const {
    auto request = Json::object({
        {"symbol", symbol}
    });

    if (params.has_value()) {
        request = this->extend(request, params.value());
    }

    auto response = this->privateGetLeverage(request);
    return response["data"];
}

Json coincatch::fetchFundingRateImpl(const std::string& symbol, const std::optional<Json>& params) const {
    auto request = Json::object({
        {"symbol", symbol}
    });

    if (params.has_value()) {
        request = this->extend(request, params.value());
    }

    auto response = this->publicGetFundingRate(request);
    return this->parseFundingRate(response["data"]);
}

// Async Market Data Methods
boost::future<Json> coincatch::fetchMarketsAsync(const std::optional<Json>& params) const {
    return boost::async(boost::launch::async, [this, params]() {
        return this->fetchMarketsImpl();
    });
}

boost::future<Json> coincatch::fetchCurrenciesAsync(const std::optional<Json>& params) const {
    return boost::async(boost::launch::async, [this, params]() {
        return this->fetchCurrenciesImpl();
    });
}

boost::future<Json> coincatch::fetchTickerAsync(const std::string& symbol, const std::optional<Json>& params) const {
    return boost::async(boost::launch::async, [this, symbol]() {
        return this->fetchTickerImpl(symbol);
    });
}

boost::future<Json> coincatch::fetchOrderBookAsync(const std::string& symbol, const std::optional<int>& limit, const std::optional<Json>& params) const {
    return boost::async(boost::launch::async, [this, symbol, limit]() {
        return this->fetchOrderBookImpl(symbol, limit);
    });
}

boost::future<Json> coincatch::fetchTradesAsync(const std::string& symbol, const std::optional<long long>& since, const std::optional<int>& limit, const std::optional<Json>& params) const {
    return boost::async(boost::launch::async, [this, symbol, since, limit]() {
        return this->fetchTradesImpl(symbol, since, limit);
    });
}

boost::future<Json> coincatch::fetchBalanceAsync(const std::optional<Json>& params) const {
    return boost::async(boost::launch::async, [this]() {
        return this->fetchBalanceImpl();
    });
}

boost::future<Json> coincatch::fetchLedgerAsync(const std::optional<std::string>& code, const std::optional<long long>& since, const std::optional<int>& limit, const std::optional<Json>& params) const {
    return boost::async(boost::launch::async, [this, code, since, limit]() {
        return this->fetchLedgerImpl(code, since, limit);
    });
}

boost::future<Json> coincatch::fetchLeverageAsync(const std::string& symbol, const std::optional<Json>& params) const {
    return boost::async(boost::launch::async, [this, symbol, params]() {
        return this->fetchLeverageImpl(symbol, params);
    });
}

boost::future<Json> coincatch::fetchFundingRateAsync(const std::string& symbol, const std::optional<Json>& params) const {
    return boost::async(boost::launch::async, [this, symbol, params]() {
        return this->fetchFundingRateImpl(symbol, params);
    });
}

boost::future<Json> coincatch::fetchFundingRateHistoryAsync(const std::string& symbol, const std::optional<long long>& since, const std::optional<int>& limit, const std::optional<Json>& params) const {
    return boost::async(boost::launch::async, [this, symbol, since, limit]() {
        return this->fetchFundingRateHistoryImpl(symbol, since, limit);
    });
}

// Async Trading Methods
boost::future<Json> coincatch::createOrderAsync(const std::string& symbol, const std::string& type, const std::string& side, double amount, const std::optional<double>& price, const std::optional<Json>& params) {
    return boost::async(boost::launch::async, [this, symbol, type, side, amount, price]() {
        return this->createOrderImpl(symbol, type, side, amount, price);
    });
}

boost::future<Json> coincatch::createMarketOrderWithCostAsync(const std::string& symbol, const std::string& side, double cost, const std::optional<Json>& params) {
    return boost::async(boost::launch::async, [this, symbol, side, cost, params]() {
        return this->createMarketOrderWithCostImpl(symbol, side, cost, params);
    });
}

boost::future<Json> coincatch::createStopLimitOrderAsync(const std::string& symbol, const std::string& side, double amount, double price, double stopPrice, const std::optional<Json>& params) {
    return boost::async(boost::launch::async, [this, symbol, side, amount, price, stopPrice, params]() {
        return this->createStopLimitOrderImpl(symbol, side, amount, price, stopPrice, params);
    });
}

boost::future<Json> coincatch::createStopMarketOrderAsync(const std::string& symbol, const std::string& side, double amount, double stopPrice, const std::optional<Json>& params) {
    return boost::async(boost::launch::async, [this, symbol, side, amount, stopPrice, params]() {
        return this->createStopMarketOrderImpl(symbol, side, amount, stopPrice, params);
    });
}

boost::future<Json> coincatch::createTakeProfitOrderAsync(const std::string& symbol, const std::string& type, const std::string& side, double amount, const std::optional<double>& price, const std::optional<Json>& params) {
    return boost::async(boost::launch::async, [this, symbol, type, side, amount, price, params]() {
        return this->createTakeProfitOrderImpl(symbol, type, side, amount, price, params);
    });
}

boost::future<Json> coincatch::cancelOrderAsync(const std::string& id, const std::string& symbol, const std::optional<Json>& params) {
    return boost::async(boost::launch::async, [this, id, symbol]() {
        return this->cancelOrderImpl(id, symbol);
    });
}

boost::future<Json> coincatch::cancelOrdersAsync(const std::vector<std::string>& ids, const std::string& symbol, const std::optional<Json>& params) {
    return boost::async(boost::launch::async, [this, ids, symbol, params]() {
        return this->cancelOrdersImpl(ids, symbol, params);
    });
}

boost::future<Json> coincatch::cancelAllOrdersAsync(const std::optional<std::string>& symbol, const std::optional<Json>& params) {
    return boost::async(boost::launch::async, [this, symbol]() {
        return this->cancelAllOrdersImpl(symbol);
    });
}

// Async Account Methods
boost::future<Json> coincatch::fetchDepositAddressAsync(const std::string& code, const std::optional<std::string>& network, const std::optional<Json>& params) const {
    return boost::async(boost::launch::async, [this, code, network]() {
        return this->fetchDepositAddressImpl(code, network);
    });
}

boost::future<Json> coincatch::fetchDepositsAsync(const std::optional<std::string>& code, const std::optional<long long>& since, const std::optional<int>& limit, const std::optional<Json>& params) const {
    return boost::async(boost::launch::async, [this, code, since, limit]() {
        return this->fetchDepositsImpl(code, since, limit);
    });
}

boost::future<Json> coincatch::addMarginAsync(const std::string& symbol, double amount, const std::optional<Json>& params) {
    return boost::async(boost::launch::async, [this, symbol, amount, params]() {
        return this->addMarginImpl(symbol, amount, params);
    });
}

} // namespace ccxt
