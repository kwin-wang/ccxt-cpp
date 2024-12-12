#include "ccxt/exchanges/coinspot.h"
#include <openssl/hmac.h>

namespace ccxt {

const std::string coinspot::defaultBaseURL = "https://www.coinspot.com.au";
const int coinspot::defaultRateLimit = 1000;
const bool coinspot::defaultPro = false;

coinspot::coinspot(const Config& config)
    : Exchange(config) {
    init();
}

void coinspot::init() {
    
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
        {"margin", false},
        {"swap", false},
        {"future", false},
        {"option", false},
        {"cancelOrder", true},
        {"createOrder", true},
        {"createMarketOrder", false},
        {"fetchBalance", true},
        {"fetchMarkets", true},
        {"fetchMyTrades", true},
        {"fetchOrderBook", true},
        {"fetchTicker", true},
        {"fetchTickers", true},
        {"fetchTrades", true}
    };

    this->markets = {
        {"ADA/AUD", this->safeMarketStructure({{"id", "ada"}, {"symbol", "ADA/AUD"}, {"base", "ADA"}, {"quote", "AUD"}, {"baseId", "ada"}, {"quoteId", "aud"}, {"type", "spot"}, {"spot", true}})},
        {"BTC/AUD", this->safeMarketStructure({{"id", "btc"}, {"symbol", "BTC/AUD"}, {"base", "BTC"}, {"quote", "AUD"}, {"baseId", "btc"}, {"quoteId", "aud"}, {"type", "spot"}, {"spot", true}})},
        {"ETH/AUD", this->safeMarketStructure({{"id", "eth"}, {"symbol", "ETH/AUD"}, {"base", "ETH"}, {"quote", "AUD"}, {"baseId", "eth"}, {"quoteId", "aud"}, {"type", "spot"}, {"spot", true}})},
        {"XRP/AUD", this->safeMarketStructure({{"id", "xrp"}, {"symbol", "XRP/AUD"}, {"base", "XRP"}, {"quote", "AUD"}, {"baseId", "xrp"}, {"quoteId", "aud"}, {"type", "spot"}, {"spot", true}})},
        {"LTC/AUD", this->safeMarketStructure({{"id", "ltc"}, {"symbol", "LTC/AUD"}, {"base", "LTC"}, {"quote", "AUD"}, {"baseId", "ltc"}, {"quoteId", "aud"}, {"type", "spot"}, {"spot", true}})},
        {"DOGE/AUD", this->safeMarketStructure({{"id", "doge"}, {"symbol", "DOGE/AUD"}, {"base", "DOGE"}, {"quote", "AUD"}, {"baseId", "doge"}, {"quoteId", "aud"}, {"type", "spot"}, {"spot", true}})}
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
        {"cointype", market["id"]}
    });
    Json response = this->privatePostOrders(request);
    return this->parseOrderBook(response, market, limit, "buyorders", "sellorders", "rate", "amount");
}

Json coinspot::parseOrderBook(const Json& orderBook, const Json& market, const std::optional<int>& limit,
                           const std::string& buyKey, const std::string& sellKey,
                           const std::string& priceKey, const std::string& amountKey) const {
    Json result = Json::object({
        {"symbol", market["symbol"]},
        {"bids", Json::array()},
        {"asks", Json::array()},
        {"timestamp", nullptr},
        {"datetime", nullptr},
        {"nonce", nullptr}
    });

    if (orderBook.contains(buyKey)) {
        for (const auto& bid : orderBook[buyKey]) {
            result["bids"].push_back({
                this->safeNumber(bid, priceKey),
                this->safeNumber(bid, amountKey)
            });
        }
    }

    if (orderBook.contains(sellKey)) {
        for (const auto& ask : orderBook[sellKey]) {
            result["asks"].push_back({
                this->safeNumber(ask, priceKey),
                this->safeNumber(ask, amountKey)
            });
        }
    }

    return result;
}

Json coinspot::fetchTradesImpl(const std::string& symbol, const std::optional<long long>& since, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json::object({
        {"cointype", market["id"]}
    });
    Json response = this->privatePostOrdersHistory(request);
    Json trades = this->safeList(response, "orders", Json::array());
    return this->parseTrades(trades, market, since, limit);
}

Json coinspot::parseTrade(const Json& trade, const Json& market) const {
    //
    //     {
    //         "amount": 0.00102091,
    //         "rate": 21549.09999991,
    //         "total": 21.99969168,
    //         "coin": "BTC",
    //         "solddate": 1604890646143,
    //         "market": "BTC/AUD"
    //     }
    //
    std::string marketId = this->safeString(trade, "coin");
    market = this->safeMarket(marketId, market);
    long long timestamp = this->safeInteger(trade, "solddate");
    double price = this->safeNumber(trade, "rate");
    double amount = this->safeNumber(trade, "amount");
    double cost = this->safeNumber(trade, "total");

    return Json::object({
        {"info", trade},
        {"symbol", market["symbol"]},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"id", nullptr},
        {"order", nullptr},
        {"type", nullptr},
        {"side", nullptr},
        {"takerOrMaker", nullptr},
        {"price", price},
        {"amount", amount},
        {"cost", cost},
        {"fee", nullptr}
    });
}

Json coinspot::parseBalance(const Json& response) const {
    Json result = Json::object({{"info", response}});
    Json balances = this->safeValue2(response, "balance", "balances");

    if (balances.is_array()) {
        for (const auto& currencies : balances) {
            for (const auto& currencyId : currencies.items()) {
                std::string code = this->safeCurrencyCode(currencyId.key());
                Json balance = currencyId.value();
                Json account = this->account();
                account["total"] = this->safeString(balance, "balance");
                result[code] = account;
            }
        }
    } else {
        for (const auto& currencyId : balances.items()) {
            std::string code = this->safeCurrencyCode(currencyId.key());
            Json account = this->account();
            account["total"] = this->safeString(balances, currencyId.key());
            result[code] = account;
        }
    }

    return this->safeBalance(result);
}

Json coinspot::fetchBalanceImpl() const {
    std::string method = this->safeString(this->options, "fetchBalance", "private_post_my_balances");
    Json response = this->call(method, Json::object());
    return this->parseBalance(response);
}

Json coinspot::createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side, double amount, const std::optional<double>& price) {
    if (type != "limit") {
        throw InvalidOrder("Coinspot only supports limit orders");
    }

    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json::object({
        {"cointype", market["id"]},
        {"amount", this->amountToPrecision(symbol, amount)},
        {"rate", this->priceToPrecision(symbol, price.value_or(0))}
    });

    std::string method = (side == "buy") ? "private_post_my_buy" : "private_post_my_sell";
    Json response = this->call(method, request);
    return this->parseOrder(response, market);
}

Json coinspot::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json::object({
        {"id", id},
        {"cointype", market["id"]}
    });

    std::string method = "private_post_my_buy_cancel";  // or private_post_my_sell_cancel
    Json response = this->call(method, request);
    return this->parseOrder(response, market);
}

Json coinspot::fetchMyTradesImpl(const std::string& symbol, const std::optional<long long>& since, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json request = Json::object();
    Json market;

    if (!symbol.empty()) {
        market = this->market(symbol);
    }

    if (since.has_value()) {
        request["startdate"] = this->ymd(since.value());
    }

    Json response = this->privatePostRoMyTransactions(request);
    return this->parseTrades(response, market, since, limit);
}

std::string coinspot::sign(const std::string& path, const std::string& api, const std::string& method,
                         const Json& params, const Json& headers, const Json& body) const {
    std::string url = this->urls["api"][api] + "/" + path;
    
    if (api == "private") {
        this->checkRequiredCredentials();
        long long nonce = this->nonce();
        std::string request = this->json(this->extend(params, {
            "nonce", nonce
        }));
        std::string secret = this->base64ToBinary(this->secret);
        std::string auth = this->hmac(request, secret, "sha512", "base64");
        Json newHeaders = Json::object({
            {"Content-Type", "application/json"},
            {"sign", auth},
            {"key", this->apiKey}
        });
        headers.update(newHeaders);
        body = request;
    }

    return url;
}

void coinspot::handleErrors(const std::string& code, const std::string& reason, const std::string& url, const std::string& method,
                         const Json& headers, const Json& body, const Json& response, const std::string& requestHeaders,
                         const std::string& requestBody) const {
    if (response.is_object() && response.contains("status")) {
        std::string status = response["status"].get<std::string>();
        if (status == "error") {
            std::string message = response.value("message", "Unknown error");
            throw ExchangeError(message);
        }
    }
}

} // namespace ccxt
