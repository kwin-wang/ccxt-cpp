#include "ccxt/exchanges/blockchaincom.h"
#include "ccxt/base/json_helper.h"
#include <openssl/hmac.h>
#include <sstream>
#include <iomanip>

namespace ccxt {

const std::string blockchaincom::defaultBaseURL = "https://api.blockchain.com/v3/exchange";
const std::string blockchaincom::defaultVersion = "v3";
const int blockchaincom::defaultRateLimit = 500;
const bool blockchaincom::defaultPro = true;

blockchaincom::blockchaincom(const Config& config) : Exchange(config) {
    init();
}

void blockchaincom::init() {
    
    
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
        
        std::string signature = this->hmac(auth, this->config_.secret, "SHA256", "hex");
        
        headers["X-API-Token"] = this->config_.apiKey;
        headers["X-Timestamp"] = nonce;
        headers["X-Signature"] = signature;
        headers["Content-Type"] = "application/json";
    }

    return url;
}

void blockchaincom::handleErrors(const std::string& httpCode, const std::string& reason, const std::string& url,
                            const std::string& method, const Json& headers, const Json& body,
                            const Json& response, const std::string& requestHeaders,
                            const std::string& requestBody) {
    if (response.is_null()) {
        return;
    }
    
    // Check for error message in response
    std::string message = this->safeString(response, "message");
    if (!message.empty()) {
        if (message == "Invalid API key") {
            throw AuthenticationError(message);
        }
        if (message == "Insufficient funds") {
            throw InsufficientFunds(message);
        }
        if (message == "Order not found") {
            throw OrderNotFound(message);
        }
        if (message.find("Required parameter") != std::string::npos) {
            throw ArgumentsRequired(message);
        }
        // Generic exchange error
        throw ExchangeError(message);
    }
}

Json blockchaincom::fetchMarketsImpl() const {
    Json response = this->fetch("/markets", "public", "GET", Json::object());
    return this->parseMarkets(response);
}

Json blockchaincom::fetchTickerImpl(const std::string& symbol) const {
    this->loadMarkets();
    std::string market_id = this->marketId(symbol);
    Json response = this->fetch("/tickers/" + market_id, "public", "GET", Json::object());
    return this->parseTicker(response, this->market(symbol));
}

Json blockchaincom::fetchTickersImpl(const std::vector<std::string>& symbols) const {
    this->loadMarkets();
    Json response = this->fetch("/tickers", "public", "GET", Json::object());
    return this->parseTickers(response, symbols);
}

Json blockchaincom::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
    this->loadMarkets();
    std::string market_id = this->marketId(symbol);
    Json request;
    if (limit.has_value()) {
        request["depth"] = limit.value();
    }
    Json response = this->fetch("/l2/" + market_id, "public", "GET", request);
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
    std::string market_id = this->marketId(symbol);
    
    Json request = Json::object({
        {"symbol", market_id},
        {"side", side.c_str()},
        {"orderType", type.c_str()},
        {"quantity", this->amountToPrecision(symbol, amount)}
    });
    
    if (type == "limit") {
        if (!price.has_value()) {
            throw ArgumentsRequired("createOrder() requires a price argument for limit orders");
        }
        request["price"] = this->priceToPrecision(symbol, price.value());
    }
    
    Json response = this->fetch("/orders", "private", "POST", request);
    return this->parseOrder(response);
}

Json blockchaincom::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    this->loadMarkets();
    Json request = Json::object({
        {"orderId", id}
    });
    if (!symbol.empty()) {
        request["symbol"] = this->marketId(symbol);
    }
    Json response = this->fetch("/orders/" + id, "private", "DELETE", request);
    return this->parseOrder(response);
}

Json blockchaincom::fetchOrderImpl(const std::string& id, const std::string& symbol) const {
    this->loadMarkets();
    Json request = Json::object({
        {"orderId", id}
    });
    if (!symbol.empty()) {
        request["symbol"] = this->marketId(symbol);
    }
    Json response = this->fetch("/orders/" + id, "private", "GET", request);
    return this->parseOrder(response);
}

Json blockchaincom::fetchOpenOrdersImpl(const std::optional<std::string>& symbol,
                                      const std::optional<long long>& since,
                                      const std::optional<int>& limit) const {
    this->loadMarkets();
    Json request;
    if (symbol.has_value()) {
        request["symbol"] = this->marketId(symbol.value());
    }
    if (limit.has_value()) {
        request["limit"] = limit.value();
    }
    Json response = this->fetch("/orders", "private", "GET", request);
    return this->parseOrders(response, symbol);
}

Json blockchaincom::fetchBalanceImpl() const {
    this->loadMarkets();
    Json response = this->fetch("/accounts", "private", "GET", Json::object());
    return this->parseBalance(response);
}

Json blockchaincom::fetchDepositAddressImpl(const std::string& code, const std::optional<std::string>& network) const {
    this->loadMarkets();
    Json currency = this->currency(code);
    Json request = Json::object({
        {"currency", this->safeString(currency, "id")}
    });
    
    if (network.has_value()) {
        request["network"] = network.value();
    }
    
    Json response = this->fetch("/deposits/address", "private", "GET", request);
    return this->parseDepositAddress(response, currency);
}

Json blockchaincom::fetchDepositsImpl(const std::optional<std::string>& code,
                                    const std::optional<long long>& since,
                                    const std::optional<int>& limit) const {
    this->loadMarkets();
    Json request;
    if (code.has_value()) {
        Json currency = this->currency(code.value());
        request["currency"] = this->safeString(currency, "id");
    }
    if (since.has_value()) {
        request["from"] = this->iso8601(since.value());
    }
    if (limit.has_value()) {
        request["limit"] = limit.value();
    }
    Json response = this->fetch("/deposits", "private", "GET", request);
    return this->parseTransactions(response, code);
}

Json blockchaincom::fetchWithdrawalsImpl(const std::optional<std::string>& code,
                                       const std::optional<long long>& since,
                                       const std::optional<int>& limit) const {
    this->loadMarkets();
    Json request;
    if (code.has_value()) {
        Json currency = this->currency(code.value());
        request["currency"] = this->safeString(currency, "id");
    }
    if (since.has_value()) {
        request["from"] = this->iso8601(since.value());
    }
    if (limit.has_value()) {
        request["limit"] = limit.value();
    }
    Json response = this->fetch("/withdrawals", "private", "GET", request);
    return this->parseTransactions(response, code);
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

Json blockchaincom::fetchTradingFeesImpl() const {
    this->loadMarkets();
    Json response = this->fetch("/fees", "public", "GET", Json::object());
    
    Json result = Json::object();
    result["info"] = response;
    result["maker"] = this->safeNumber(response, "makerRate");
    result["taker"] = this->safeNumber(response, "takerRate");
    result["percentage"] = true;
    result["tierBased"] = true;
    
    return result;
}

Json blockchaincom::parseMarket(const Json& market) const {
    std::string id = this->safeString(market, "symbol");
    std::string baseId = this->safeString(market, "baseCurrency");
    std::string quoteId = this->safeString(market, "quoteCurrency");
    std::string base = this->safeCurrencyCode(baseId);
    std::string quote = this->safeCurrencyCode(quoteId);
    std::string symbol = base + "/" + quote;
    
    return Json::object({
        {"id", id},
        {"symbol", symbol},
        {"base", base},
        {"quote", quote},
        {"baseId", baseId},
        {"quoteId", quoteId},
        {"active", true},
        {"type", "spot"},
        {"spot", true},
        {"precision", Json::object({
            {"amount", this->safeInteger(market, "quantityPrecision", 8)},
            {"price", this->safeInteger(market, "pricePrecision", 8)}
        })},
        {"limits", Json::object({
            {"amount", Json::object({
                {"min", this->safeNumber(market, "minQuantity")},
                {"max", this->safeNumber(market, "maxQuantity")}
            })},
            {"price", Json::object({
                {"min", this->safeNumber(market, "minPrice")},
                {"max", this->safeNumber(market, "maxPrice")}
            })}
        })},
        {"info", market}
    });
}

Json blockchaincom::parseTicker(const Json& ticker, const Json& market) const {
    long long timestamp = this->milliseconds();
    std::string symbol = this->safeString(market, "symbol");
    
    return Json::object({
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"high", this->safeNumber(ticker, "high24h")},
        {"low", this->safeNumber(ticker, "low24h")},
        {"bid", this->safeNumber(ticker, "bid")},
        {"bidVolume", this->safeNumber(ticker, "bidSize")},
        {"ask", this->safeNumber(ticker, "ask")},
        {"askVolume", this->safeNumber(ticker, "askSize")},
        {"vwap", this->safeNumber(ticker, "volumeWeightedPrice")},
        {"open", this->safeNumber(ticker, "open24h")},
        {"close", this->safeNumber(ticker, "lastPrice")},
        {"last", this->safeNumber(ticker, "lastPrice")},
        {"previousClose", nullptr},
        {"change", nullptr},
        {"percentage", nullptr},
        {"average", nullptr},
        {"baseVolume", this->safeNumber(ticker, "volume24h")},
        {"quoteVolume", nullptr},
        {"info", ticker}
    });
}

Json blockchaincom::parseOrder(const Json& order, const Json& market) const {
    std::string id = this->safeString(order, "orderId");
    std::string symbol = this->safeString(market, "symbol");
    long long timestamp = this->safeTimestamp(order, "timestamp");
    std::string side = this->safeStringLower(order, "side");
    std::string type = this->safeStringLower(order, "orderType");
    
    return Json::object({
        {"id", id},
        {"clientOrderId", this->safeString(order, "clientOrderId")},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"symbol", symbol},
        {"type", type},
        {"timeInForce", this->safeString(order, "timeInForce")},
        {"postOnly", this->safeValue(order, "postOnly")},
        {"side", side},
        {"price", this->safeNumber(order, "price")},
        {"stopPrice", this->safeNumber(order, "stopPrice")},
        {"amount", this->safeNumber(order, "quantity")},
        {"cost", nullptr},
        {"average", this->safeNumber(order, "avgFillPrice")},
        {"filled", this->safeNumber(order, "filledQuantity")},
        {"remaining", nullptr},
        {"status", this->parseOrderStatus(this->safeString(order, "status"))},
        {"fee", nullptr},
        {"trades", nullptr},
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
