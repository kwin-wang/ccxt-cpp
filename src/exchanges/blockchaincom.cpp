#include "ccxt/exchanges/blockchaincom.h"
#include "../base/json_helper.h"
#include <openssl/hmac.h>
#include <sstream>
#include <iomanip>

namespace ccxt {

const std::string blockchaincom::defaultBaseURL = "https://api.blockchain.com/v3/exchange";
const std::string blockchaincom::defaultVersion = "v3";
const int blockchaincom::defaultRateLimit = 500;
const bool blockchaincom::defaultPro = true;

ExchangeRegistry::Factory blockchaincom::factory("blockchaincom", blockchaincom::createInstance);

blockchaincom::blockchaincom(const Config& config) : ExchangeImpl(config) {
    init();
}

void blockchaincom::init() {
    ExchangeImpl::init();
    
    // Set exchange properties
    this->id = "blockchaincom";
    this->name = "Blockchain.com";
    this->countries = {"LX"};
    this->version = defaultVersion;
    this->rateLimit = defaultRateLimit;
    this->pro = defaultPro;
    
    if (this->urls.empty()) {
        this->urls = {
            {"logo", "https://github.com/user-attachments/assets/975e3054-3399-4363-bcee-ec3c6d63d4e8"},
            {"api", {
                {"public", defaultBaseURL},
                {"private", defaultBaseURL}
            }},
            {"www", "https://blockchain.com"},
            {"doc", {"https://api.blockchain.com/v3"}},
            {"fees", "https://exchange.blockchain.com/fees"}
        };
    }

    this->has = {
        {"CORS", false},
        {"spot", true},
        {"margin", std::nullopt},
        {"swap", false},
        {"future", false},
        {"option", false},
        {"cancelAllOrders", true},
        {"cancelOrder", true},
        {"createOrder", true},
        {"createStopLimitOrder", true},
        {"createStopMarketOrder", true},
        {"createStopOrder", true},
        {"fetchBalance", true},
        {"fetchCanceledOrders", true},
        {"fetchClosedOrders", true},
        {"fetchDeposit", true},
        {"fetchDepositAddress", true},
        {"fetchDeposits", true},
        {"fetchL2OrderBook", true},
        {"fetchL3OrderBook", true},
        {"fetchMarkets", true},
        {"fetchMyTrades", true},
        {"fetchOpenOrders", true},
        {"fetchOrder", true},
        {"fetchOrderBook", true},
        {"fetchTicker", true},
        {"fetchTickers", true},
        {"fetchTradingFees", true},
        {"fetchWithdrawal", true},
        {"fetchWithdrawals", true},
        {"fetchWithdrawalWhitelist", true},
        {"withdraw", true}
    };
}

std::string blockchaincom::sign(const std::string& path, const std::string& api, const std::string& method,
                               const Json& params, const Json& headers, const Json& body) const {
    std::string url = this->urls["api"][api] + "/" + path;
    
    if (api == "public") {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    } else {
        this->checkRequiredCredentials();
        
        long long timestamp = this->milliseconds();
        std::string nonce = std::to_string(timestamp);
        
        std::string auth = nonce + method + "/" + path;
        if (!params.empty()) {
            auth += this->json(params);
        }
        
        std::string signature = this->hmac(auth, this->secret, "SHA256", "hex");
        
        headers["X-API-Token"] = this->apiKey;
        headers["X-Timestamp"] = nonce;
        headers["X-Signature"] = signature;
        headers["Content-Type"] = "application/json";
    }

    return url;
}

void blockchaincom::handleErrors(const std::string& code, const std::string& reason, const std::string& url,
                                const std::string& method, const Json& headers, const Json& body,
                                const Json& response, const std::string& requestHeaders,
                                const std::string& requestBody) const {
    if (response.is_object() && response.contains("error")) {
        std::string message = this->safeString(response, "message", "Unknown error");
        std::string errorCode = this->safeString(response, "error");
        
        if (errorCode == "UNAUTHORIZED") {
            throw AuthenticationError(message);
        } else if (errorCode == "INSUFFICIENT_FUNDS") {
            throw InsufficientFunds(message);
        } else if (errorCode == "ORDER_NOT_FOUND") {
            throw OrderNotFound(message);
        }
        
        throw ExchangeError(message);
    }
}

Json blockchaincom::fetchMarketsImpl() const {
    Json response = this->publicGetSymbols();
    return this->parseMarkets(response);
}

Json blockchaincom::fetchTickerImpl(const std::string& symbol) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json response = this->publicGetTicker(Json::object({
        {"symbol", market["id"]}
    }));
    return this->parseTicker(response, market);
}

Json blockchaincom::fetchTickersImpl(const std::vector<std::string>& symbols) const {
    this->loadMarkets();
    Json response = this->publicGetTickers();
    return this->parseTickers(response, symbols);
}

Json blockchaincom::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json request = Json::object({
        {"symbol", this->marketId(symbol)}
    });
    if (limit) {
        request["limit"] = *limit;
    }
    Json response = this->publicGetL2OrderBook(request);
    return this->parseOrderBook(response, symbol);
}

Json blockchaincom::fetchL3OrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json request = Json::object({
        {"symbol", this->marketId(symbol)}
    });
    if (limit) {
        request["limit"] = *limit;
    }
    Json response = this->publicGetL3OrderBook(request);
    return this->parseOrderBook(response, symbol, undefined, "bids", "asks", "px", "qty");
}

Json blockchaincom::createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side,
                                  double amount, const std::optional<double>& price) {
    this->loadMarkets();
    Json market = this->market(symbol);
    
    Json request = Json::object({
        {"symbol", market["id"]},
        {"side", side.substr(0, 1).upper() + side.substr(1)},  // Capitalize first letter
        {"orderType", type.upper()},
        {"qty", this->amountToPrecision(symbol, amount)}
    });

    if (type == "limit") {
        if (!price) {
            throw ArgumentsRequired("createOrder() requires a price argument for limit orders");
        }
        request["price"] = this->priceToPrecision(symbol, *price);
    }

    Json response = this->privatePostOrders(request);
    return this->parseOrder(response);
}

Json blockchaincom::createStopOrderImpl(const std::string& symbol, const std::string& type, const std::string& side,
                                      double amount, double price, const std::optional<Json>& params) {
    this->loadMarkets();
    Json market = this->market(symbol);
    
    Json request = Json::object({
        {"symbol", market["id"]},
        {"side", side.substr(0, 1).upper() + side.substr(1)},
        {"orderType", type.upper()},
        {"qty", this->amountToPrecision(symbol, amount)},
        {"stopPrice", this->priceToPrecision(symbol, price)}
    });

    if (type == "limit") {
        if (!params || !params->contains("price")) {
            throw ArgumentsRequired("createStopOrder() requires a price parameter for stop limit orders");
        }
        request["price"] = this->priceToPrecision(symbol, (*params)["price"].get<double>());
    }

    Json response = this->privatePostOrders(request);
    return this->parseOrder(response);
}

Json blockchaincom::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    this->loadMarkets();
    Json request = Json::object({
        {"orderId", id}
    });
    return this->privateDeleteOrdersOrderId(request);
}

Json blockchaincom::cancelAllOrdersImpl(const std::optional<std::string>& symbol) {
    this->loadMarkets();
    Json request = Json::object();
    if (symbol) {
        Json market = this->market(*symbol);
        request["symbol"] = market["id"];
    }
    return this->privateDeleteOrders(request);
}

Json blockchaincom::fetchBalanceImpl() const {
    this->loadMarkets();
    Json response = this->privateGetBalance();
    return this->parseBalance(response);
}

Json blockchaincom::fetchDepositAddressImpl(const std::string& code, const std::optional<std::string>& network) const {
    this->loadMarkets();
    Json currency = this->currency(code);
    Json request = Json::object({
        {"currency", currency["id"]}
    });
    Json response = this->privateGetDepositAddress(request);
    return this->parseDepositAddress(response, currency);
}

Json blockchaincom::withdrawImpl(const std::string& code, double amount, const std::string& address,
                                const std::optional<std::string>& tag) {
    this->checkAddress(address);
    this->loadMarkets();
    Json currency = this->currency(code);
    
    Json request = Json::object({
        {"currency", currency["id"]},
        {"amount", this->currencyToPrecision(code, amount)},
        {"address", address}
    });

    if (tag) {
        request["memo"] = *tag;
    }

    Json response = this->privatePostWithdraw(request);
    return this->parseTransaction(response, currency);
}

Json blockchaincom::parseTicker(const Json& ticker, const Json& market) const {
    long long timestamp = this->milliseconds();
    std::string symbol = this->safeString(market, "symbol");
    return Json::object({
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"high", this->safeString(ticker, "high24h")},
        {"low", this->safeString(ticker, "low24h")},
        {"bid", this->safeString(ticker, "bid")},
        {"ask", this->safeString(ticker, "ask")},
        {"last", this->safeString(ticker, "last_trade_price")},
        {"baseVolume", this->safeString(ticker, "volume24h")},
        {"info", ticker}
    });
}

Json blockchaincom::parseOrder(const Json& order, const Json& market) const {
    std::string id = this->safeString(order, "orderId");
    std::string timestamp = this->safeString(order, "timestamp");
    std::string status = this->parseOrderStatus(this->safeString(order, "status"));
    std::string symbol = this->safeString(market, "symbol");
    std::string type = this->safeStringLower(order, "orderType");
    std::string side = this->safeStringLower(order, "side");
    
    return Json::object({
        {"id", id},
        {"timestamp", this->parse8601(timestamp)},
        {"datetime", timestamp},
        {"status", status},
        {"symbol", symbol},
        {"type", type},
        {"side", side},
        {"price", this->safeNumber(order, "price")},
        {"amount", this->safeNumber(order, "qty")},
        {"filled", this->safeNumber(order, "filledQty")},
        {"remaining", this->safeNumber(order, "remainingQty")},
        {"info", order}
    });
}

Json blockchaincom::parseTransaction(const Json& transaction, const Json& currency) const {
    std::string id = this->safeString(transaction, "withdrawalId");
    long long timestamp = this->safeTimestamp(transaction, "timestamp");
    std::string status = this->parseTransactionStatus(this->safeString(transaction, "status"));
    
    return Json::object({
        {"id", id},
        {"info", transaction},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"currency", currency["code"]},
        {"amount", this->safeNumber(transaction, "amount")},
        {"address", this->safeString(transaction, "address")},
        {"tag", this->safeString(transaction, "memo")},
        {"status", status},
        {"fee", this->safeNumber(transaction, "fee")}
    });
}

Json blockchaincom::parseDepositAddress(const Json& depositAddress, const Json& currency) const {
    return Json::object({
        {"currency", currency["code"]},
        {"address", this->safeString(depositAddress, "address")},
        {"tag", this->safeString(depositAddress, "memo")},
        {"network", this->safeString(depositAddress, "network")},
        {"info", depositAddress}
    });
}

} // namespace ccxt
