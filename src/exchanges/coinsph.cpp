#include "ccxt/exchanges/coinsph.h"
#include <openssl/hmac.h>

namespace ccxt {

const std::string coinsph::defaultBaseURL = "https://api.coins.ph";
const std::string coinsph::defaultVersion = "v1";
const int coinsph::defaultRateLimit = 50;
const bool coinsph::defaultPro = false;


coinsph::coinsph(const Config& config)
    : Exchange(config) {
    init();
}

void coinsph::init() {
    
    this->id = "coinsph";
    this->name = "Coins.ph";
    this->countries = {"PH"};  // Philippines
    this->version = defaultVersion;
    this->rateLimit = defaultRateLimit;
    this->pro = defaultPro;

    if (this->urls.empty()) {
        this->urls["api"] = {
            {"public", "https://api.pro.coins.ph"},
            {"private", "https://api.pro.coins.ph"}
        };
    }

    this->has = {
        {"CORS", nullptr},
        {"spot", true},
        {"margin", false},
        {"swap", false},
        {"future", false},
        {"option", false},
        {"cancelAllOrders", true},
        {"cancelOrder", true},
        {"createOrder", true},
        {"fetchBalance", true},
        {"fetchMarkets", true},
        {"fetchMyTrades", true},
        {"fetchOHLCV", true},
        {"fetchOpenOrders", true},
        {"fetchClosedOrders", true},
        {"fetchOrder", true},
        {"fetchOrderBook", true},
        {"fetchStatus", true},
        {"fetchTicker", true},
        {"fetchTickers", true},
        {"fetchTime", true},
        {"fetchTrades", true},
        {"fetchTradingFee", true},
        {"fetchTradingFees", true},
        {"fetchWithdrawals", true},
        {"withdraw", true}
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
        {"8h", "8h"},
        {"12h", "12h"},
        {"1d", "1d"},
        {"3d", "3d"},
        {"1w", "1w"},
        {"1M", "1M"}
    };
}

Json coinsph::describeImpl() const {
    return Json::object({
        {"id", this->id},
        {"name", this->name},
        {"countries", this->countries},
        {"version", this->version},
        {"rateLimit", this->rateLimit},
        {"pro", this->pro},
        {"has", this->has}
    });
}

Json coinsph::fetchMarketsImpl() const {
    Json response = this->publicGetMarkets();
    return this->parseMarkets(response);
}

Json coinsph::fetchTickerImpl(const std::string& symbol) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json response = this->publicGetTicker(Json::object({
        {"symbol", market["id"]}
    }));
    return this->parseTicker(response, market);
}

Json coinsph::fetchTickersImpl(const std::vector<std::string>& symbols) const {
    this->loadMarkets();
    Json response = this->publicGetTickers();
    return this->parseTickers(response, symbols);
}

Json coinsph::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
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

Json coinsph::fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe, const std::optional<long long>& since, const std::optional<int>& limit) const {
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

Json coinsph::createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side, double amount, const std::optional<double>& price) {
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

Json coinsph::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json::object({
        {"symbol", market["id"]},
        {"orderId", id}
    });
    Json response = this->privateDeleteOrder(request);
    return this->parseOrder(response, market);
}

Json coinsph::fetchOrderImpl(const std::string& id, const std::string& symbol) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json::object({
        {"symbol", market["id"]},
        {"orderId", id}
    });
    Json response = this->privateGetOrder(request);
    return this->parseOrder(response, market);
}

Json coinsph::fetchOpenOrdersImpl(const std::string& symbol, const std::optional<long long>& since, const std::optional<int>& limit) const {
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

Json coinsph::fetchClosedOrdersImpl(const std::string& symbol, const std::optional<long long>& since, const std::optional<int>& limit) const {
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

Json coinsph::fetchMyTradesImpl(const std::string& symbol, const std::optional<long long>& since, const std::optional<int>& limit) const {
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

Json coinsph::fetchBalanceImpl() const {
    Json response = this->privateGetBalance();
    return this->parseBalance(response);
}

Json coinsph::fetchDepositAddressImpl(const std::string& code, const std::optional<std::string>& network) const {
    Json request = Json::object({
        {"currency", code}
    });
    if (network.has_value()) {
        request["network"] = network.value();
    }
    Json response = this->privateGetDepositAddress(request);
    return this->parseDepositAddress(response);
}

Json coinsph::fetchDepositsImpl(const std::optional<std::string>& code, const std::optional<long long>& since, const std::optional<int>& limit) const {
    Json request = Json::object();
    if (code.has_value()) {
        request["currency"] = code.value();
    }
    if (since.has_value()) {
        request["since"] = since.value();
    }
    if (limit.has_value()) {
        request["limit"] = limit.value();
    }
    Json response = this->privateGetDeposits(request);
    return this->parseTransactions(response);
}

Json coinsph::fetchTimeImpl() const {
    Json response = this->publicGetOpenApiV1Time();
    return response["serverTime"];
}

Json coinsph::fetchStatusImpl() const {
    Json response = this->publicGetOpenApiV1Ping();
    return {
        {"status", "ok"},
        {"updated", this->milliseconds()},
        {"eta", nullptr},
        {"url", nullptr}
    };
}

Json coinsph::fetchTradingFeeImpl(const std::string& symbol) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json response = this->privateGetOpenApiV1AssetTradeFee(Json::object({
        {"symbol", market["id"]}
    }));
    return this->parseTradingFee(response, market);
}

Json coinsph::fetchTradingFeesImpl() const {
    this->loadMarkets();
    Json response = this->privateGetOpenApiV1AssetTradeFee();
    Json result = Json::object();
    for (const auto& entry : response.items()) {
        const std::string& marketId = entry.key();
        if (this->markets_by_id.count(marketId)) {
            Json market = this->markets_by_id[marketId];
            result[market["symbol"]] = this->parseTradingFee(entry.value(), market);
        }
    }
    return result;
}

Json coinsph::fetchOrderTradesImpl(const std::string& id, const std::string& symbol) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json::object({
        {"orderId", id},
        {"symbol", market["id"]}
    });
    Json response = this->privateGetOpenApiV1MyTrades(request);
    return this->parseTrades(response, market);
}

Json coinsph::cancelAllOrdersImpl(const std::string& symbol) {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json::object({
        {"symbol", market["id"]}
    });
    Json response = this->privateDeleteOpenApiV1OpenOrders(request);
    return this->parseOrders(response, market);
}

Json coinsph::fetchWithdrawalsImpl(const std::optional<std::string>& code, const std::optional<long long>& since, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json request = Json::object();
    Json currency;
    if (code.has_value()) {
        currency = this->currency(code.value());
        request["coin"] = currency["id"];
    }
    if (since.has_value()) {
        request["startTime"] = since.value();
    }
    if (limit.has_value()) {
        request["limit"] = limit.value();
    }
    Json response = this->privateGetOpenApiWalletV1WithdrawHistory(request);
    return this->parseTransactions(response, currency, since, limit);
}

Json coinsph::withdrawImpl(const std::string& code, double amount, const std::string& address, const std::optional<std::string>& tag) {
    this->loadMarkets();
    Json currency = this->currency(code);
    Json request = Json::object({
        {"coin", currency["id"]},
        {"amount", this->currencyToPrecision(code, amount)},
        {"address", address}
    });
    if (tag.has_value()) {
        request["addressTag"] = tag.value();
    }
    Json response = this->privatePostOpenApiWalletV1WithdrawApply(request);
    return this->parseTransaction(response, currency);
}

std::string coinsph::sign(const std::string& path, const std::string& api, const std::string& method,
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

    auto signature = this->hmac(auth, this->config_.secret, "sha256", "hex");
    
    return url;
}

void coinsph::handleErrors(const std::string& code, const std::string& reason, const std::string& url, const std::string& method,
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

std::string coinsph::parseOrderSide(const std::string& status) const {
    static const std::map<std::string, std::string> statuses = {
        {"BUY", "buy"},
        {"SELL", "sell"}
    };
    return statuses.at(status);
}

std::string coinsph::encodeOrderSide(const std::string& status) const {
    static const std::map<std::string, std::string> statuses = {
        {"buy", "BUY"},
        {"sell", "SELL"}
    };
    return statuses.at(status);
}

std::string coinsph::parseOrderType(const std::string& status) const {
    static const std::map<std::string, std::string> statuses = {
        {"LIMIT", "limit"},
        {"MARKET", "market"},
        {"STOP_LOSS", "stop_loss"},
        {"STOP_LOSS_LIMIT", "stop_loss_limit"},
        {"TAKE_PROFIT", "take_profit"},
        {"TAKE_PROFIT_LIMIT", "take_profit_limit"},
        {"LIMIT_MAKER", "limit_maker"}
    };
    return statuses.at(status);
}

std::string coinsph::encodeOrderType(const std::string& status) const {
    static const std::map<std::string, std::string> statuses = {
        {"limit", "LIMIT"},
        {"market", "MARKET"},
        {"stop_loss", "STOP_LOSS"},
        {"stop_loss_limit", "STOP_LOSS_LIMIT"},
        {"take_profit", "TAKE_PROFIT"},
        {"take_profit_limit", "TAKE_PROFIT_LIMIT"},
        {"limit_maker", "LIMIT_MAKER"}
    };
    return statuses.at(status);
}

std::string coinsph::parseOrderStatus(const std::string& status) const {
    static const std::map<std::string, std::string> statuses = {
        {"NEW", "open"},
        {"PARTIALLY_FILLED", "open"},
        {"FILLED", "closed"},
        {"CANCELED", "canceled"},
        {"PENDING_CANCEL", "canceling"},
        {"REJECTED", "rejected"},
        {"EXPIRED", "expired"}
    };
    return statuses.at(status);
}

std::string coinsph::parseOrderTimeInForce(const std::string& status) const {
    static const std::map<std::string, std::string> statuses = {
        {"GTC", "GTC"},
        {"IOC", "IOC"},
        {"FOK", "FOK"}
    };
    return statuses.at(status);
}

std::string coinsph::parseTransactionStatus(const std::string& status) const {
    static const std::map<std::string, std::string> statuses = {
        {"PENDING", "pending"},
        {"COMPLETED", "ok"},
        {"FAILED", "failed"},
        {"CANCELLED", "canceled"}
    };
    return statuses.at(status);
}

std::string coinsph::urlEncodeQuery(const Json& query) const {
    std::string result;
    for (const auto& entry : query.items()) {
        if (!result.empty()) {
            result += "&";
        }
        result += entry.key() + "=" + std::to_string(entry.value());
    }
    return result;
}

Json coinsph::parseArrayParam(const Json& array, const std::string& key) const {
    if (array.is_array()) {
        Json result = Json::array();
        for (const auto& item : array) {
            result.push_back(item[key]);
        }
        return result;
    }
    return array;
}

Json coinsph::parseTradingFee(const Json& fee, const Json& market) const {
    return Json::object({
        {"info", fee},
        {"symbol", market["symbol"]},
        {"maker", this->safeNumber(fee, "makerCommission")},
        {"taker", this->safeNumber(fee, "takerCommission")}
    });
}

Json coinsph::parseDepositAddress(const Json& depositAddress, const Json& currency) const {
    return Json::object({
        {"currency", currency["code"]},
        {"address", this->safeString(depositAddress, "address")},
        {"tag", this->safeString(depositAddress, "tag")},
        {"network", this->safeString(depositAddress, "network")},
        {"info", depositAddress}
    });
}

} // namespace ccxt
