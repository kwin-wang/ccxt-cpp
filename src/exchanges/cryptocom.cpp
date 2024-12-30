#include "ccxt/exchanges/cryptocom.h"
#include "../base/crypto.h"
#include "../base/error.h"
#include <sstream>
#include <iomanip>
#include <chrono>

namespace ccxt {

CryptoCom::CryptoCom(const ExchangeConfig& config) : Exchange(config) {
    // Initialize exchange-specific configurations
    this->has = {
        {"CORS", false},
        {"spot", true},
        {"margin", true},
        {"swap", true},
        {"future", true},
        {"option", true},
        {"addMargin", false},
        {"cancelAllOrders", true},
        {"cancelOrder", true}
    };

    this->urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/147792121-38ed5e36-c229-48d6-b49a-48d05fc19ed4.jpeg"},
        {"api", {
            {"public", "https://api.crypto.com/v2"},
            {"private", "https://api.crypto.com/v2"}
        }},
        {"www", "https://crypto.com/exchange"},
        {"doc", {
            "https://exchange-docs.crypto.com/exchange/v1/rest-ws/index.html",
            "https://exchange-docs.crypto.com/spot/index.html",
            "https://exchange-docs.crypto.com/derivatives/index.html"
        }},
        {"fees", "https://crypto.com/exchange/document/fees-limits"}
    };

    this->api = {
        {"public", {
            {"get", {
                "public/auth",
                "public/get-instruments",
                "public/get-book",
                "public/get-candlestick",
                "public/get-ticker",
                "public/get-trades",
                "public/get-expired-instruments",
                "public/get-valuations",
                "public/get-insurance",
                "public/get-funding-rate-history"
            }}
        }},
        {"private", {
            {"post", {
                "private/create-order",
                "private/cancel-order",
                "private/cancel-all-orders",
                "private/get-order-history",
                "private/get-open-orders",
                "private/get-order-detail",
                "private/get-trades",
                "private/get-positions",
                "private/get-account-summary",
                "private/get-subaccount-balances",
                "private/get-currency-networks",
                "private/get-deposit-address",
                "private/get-deposit-history",
                "private/get-withdrawal-history",
                "private/create-withdrawal",
                "private/cancel-withdrawal",
                "private/get-risk-state",
                "private/get-account-positions",
                "private/get-account-trades"
            }}
        }}
    };
}

std::string CryptoCom::getSignature(const std::string& requestPath, const std::string& method,
                                  const std::string& paramsStr, const std::string& timestamp) const {
    std::string signStr = timestamp + method + requestPath + paramsStr;
    return hmacSha256(signStr, this->config_.secret);
}

json CryptoCom::signRequest(const std::string& path, const std::string& api,
                          const std::string& method, const Params& params,
                          const json& headers, const std::string& body) {
    this->checkRequiredCredentials();
    
    auto timestamp = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    
    std::string requestPath = this->getRequestPath(api, path);
    std::string paramsStr = this->json_encode(params);
    auto signature = this->getSignature(requestPath, method, paramsStr, timestamp);
    
    json resultHeaders = headers;
    resultHeaders["api-key"] = this->config_.apiKey;
    resultHeaders["api-timestamp"] = timestamp;
    resultHeaders["api-signature"] = signature;
    resultHeaders["Content-Type"] = "application/json";
    
    return {
        {"url", this->urls["api"][api] + requestPath},
        {"method", method},
        {"body", body},
        {"headers", resultHeaders}
    };
}

OrderBook CryptoCom::fetchOrderBook(const std::string& symbol, int limit, const Params& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    
    auto request = {
        {"instrument_name", market["id"].get<std::string>()}
    };
    
    if (limit != 0) {
        request["depth"] = std::to_string(limit);
    }
    
    auto response = this->publicGetPublicGetBook(this->extend(request, params));
    auto orderbook = this->parseOrderBook(response["result"]["data"], symbol);
    orderbook.timestamp = this->safeInteger(response["result"], "t");
    return orderbook;
}

Order CryptoCom::createOrder(const std::string& symbol, const std::string& type,
                           const std::string& side, double amount, double price,
                           const Params& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    
    auto request = {
        {"instrument_name", market["id"].get<std::string>()},
        {"side", side},
        {"type", type},
        {"quantity", this->amountToPrecision(symbol, amount)}
    };
    
    if (type == "LIMIT") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    std::string timeInForce = this->getTimeInForce(params);
    if (!timeInForce.empty()) {
        request["time_in_force"] = timeInForce;
    }
    
    auto response = this->privatePostPrivateCreateOrder(this->extend(request, params));
    return this->parseOrder(response["result"]["data"]);
}

Balance CryptoCom::fetchBalance(const Params& params) {
    this->loadMarkets();
    auto response = this->privatePostPrivateGetAccountSummary(params);
    return this->parseBalance(response["result"]["accounts"]);
}

void CryptoCom::handleErrors(const json& response) {
    if (response.contains("code")) {
        auto code = response["code"].get<int>();
        auto message = this->safeString(response, "msg", "Unknown error");
        
        if (code != 0) {
            if (code == 10001) {
                throw AuthenticationError(message);
            } else if (code == 10002) {
                throw PermissionDenied(message);
            } else if (code == 10003) {
                throw AccountNotEnabled(message);
            } else if (code == 10004) {
                throw InsufficientFunds(message);
            } else if (code == 10005) {
                throw InvalidOrder(message);
            } else if (code == 10006) {
                throw OrderNotFound(message);
            } else if (code == 10007) {
                throw RateLimitExceeded(message);
            } else if (code == 20001) {
                throw BadSymbol(message);
            }
            
            throw ExchangeError(message);
        }
    }
}

std::string CryptoCom::getRequestPath(const std::string& api, const std::string& path) const {
    return "/" + path;
}

std::string CryptoCom::getInstrumentType(const std::string& symbol) const {
    auto market = this->market(symbol);
    return market.value("type", "SPOT");
}

void CryptoCom::validateSymbol(const std::string& symbol) const {
    if (symbol.empty()) {
        throw ArgumentsRequired("Symbol is required");
    }
    if (!this->markets.contains(symbol)) {
        throw BadSymbol("Symbol " + symbol + " is not supported by Crypto.com");
    }
}

void CryptoCom::checkRequiredCredentials() const {
    if (this->config_.apiKey.empty()) {
        throw AuthenticationError("API Key is required for private endpoints");
    }
    if (this->config_.secret.empty()) {
        throw AuthenticationError("API Secret is required for private endpoints");
    }
}

std::string CryptoCom::parseOrderStatus(const std::string& status) const {
    static const std::map<std::string, std::string> statuses = {
        {"ACTIVE", "open"},
        {"CANCELED", "canceled"},
        {"FILLED", "closed"},
        {"REJECTED", "rejected"},
        {"EXPIRED", "expired"}
    };
    return statuses.contains(status) ? statuses.at(status) : status;
}

std::string CryptoCom::getTimeInForce(const Params& params) const {
    static const std::vector<std::string> validTimeInForce = {
        "GOOD_TILL_CANCEL",
        "FILL_OR_KILL",
        "IMMEDIATE_OR_CANCEL",
        "DAY"
    };
    
    auto timeInForce = this->safeString(params, "timeInForce", "");
    if (!timeInForce.empty() && 
        std::find(validTimeInForce.begin(), validTimeInForce.end(), timeInForce) == validTimeInForce.end()) {
        throw BadRequest("Invalid timeInForce: " + timeInForce);
    }
    return timeInForce;
}

std::vector<Trade> CryptoCom::fetchTrades(const std::string& symbol, int since, int limit, const Params& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    
    auto request = {
        {"instrument_name", market["id"].get<std::string>()}
    };
    
    if (limit != 0) {
        request["count"] = std::to_string(limit);
    }
    
    auto response = this->publicGetPublicGetTrades(this->extend(request, params));
    return this->parseTrades(response["result"]["data"], market, since, limit);
}

Ticker CryptoCom::fetchTicker(const std::string& symbol, const Params& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    
    auto request = {
        {"instrument_name", market["id"].get<std::string>()}
    };
    
    auto response = this->publicGetPublicGetTicker(this->extend(request, params));
    return this->parseTicker(response["result"]["data"], market);
}

std::map<std::string, Ticker> CryptoCom::fetchTickers(const std::vector<std::string>& symbols, const Params& params) {
    this->loadMarkets();
    auto response = this->publicGetPublicGetTicker(params);
    auto tickers = response["result"]["data"];
    std::map<std::string, Ticker> result;
    
    for (const auto& ticker : tickers) {
        auto marketId = this->safeString(ticker, "i");
        if (!this->markets_by_id.contains(marketId)) {
            continue;
        }
        auto market = this->markets_by_id[marketId];
        auto symbol = market["symbol"].get<std::string>();
        if (!symbols.empty() && 
            std::find(symbols.begin(), symbols.end(), symbol) == symbols.end()) {
            continue;
        }
        result[symbol] = this->parseTicker(ticker, market);
    }
    
    return result;
}

Trade CryptoCom::parseTrade(const json& trade, const json& market) {
    auto timestamp = this->safeInteger(trade, "t");
    auto price = this->safeFloat(trade, "p");
    auto amount = this->safeFloat(trade, "q");
    auto side = this->safeString(trade, "s");
    auto id = this->safeString(trade, "d");
    
    return {
        {"id", id},
        {"info", trade},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"symbol", market["symbol"]},
        {"type", nullptr},
        {"side", side.empty() ? nullptr : side.toLower()},
        {"order", nullptr},
        {"takerOrMaker", nullptr},
        {"price", price},
        {"amount", amount},
        {"cost", price * amount},
        {"fee", nullptr}
    };
}

Ticker CryptoCom::parseTicker(const json& ticker, const json& market) {
    auto timestamp = this->safeInteger(ticker, "t");
    auto last = this->safeFloat(ticker, "a");
    auto bid = this->safeFloat(ticker, "b");
    auto ask = this->safeFloat(ticker, "k");
    auto high = this->safeFloat(ticker, "h");
    auto low = this->safeFloat(ticker, "l");
    auto volume = this->safeFloat(ticker, "v");
    auto change = this->safeFloat(ticker, "c");
    auto percentage = this->safeFloat(ticker, "ch");
    
    return {
        {"symbol", market["symbol"]},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"high", high},
        {"low", low},
        {"bid", bid},
        {"bidVolume", nullptr},
        {"ask", ask},
        {"askVolume", nullptr},
        {"vwap", nullptr},
        {"open", nullptr},
        {"close", last},
        {"last", last},
        {"previousClose", nullptr},
        {"change", change},
        {"percentage", percentage},
        {"average", nullptr},
        {"baseVolume", volume},
        {"quoteVolume", nullptr},
        {"info", ticker}
    };
}

Order CryptoCom::cancelOrder(const std::string& id, const std::string& symbol, const Params& params) {
    this->loadMarkets();
    
    auto request = {
        {"order_id", id}
    };
    
    if (!symbol.empty()) {
        auto market = this->market(symbol);
        request["instrument_name"] = market["id"].get<std::string>();
    }
    
    auto response = this->privatePostPrivateCancelOrder(this->extend(request, params));
    return this->parseOrder(response["result"]["data"]);
}

bool CryptoCom::cancelAllOrders(const std::vector<std::string>& symbols, const Params& params) {
    this->loadMarkets();
    
    auto request = Params();
    if (!symbols.empty()) {
        std::vector<std::string> instrumentNames;
        for (const auto& symbol : symbols) {
            auto market = this->market(symbol);
            instrumentNames.push_back(market["id"].get<std::string>());
        }
        request["instrument_name"] = instrumentNames;
    }
    
    auto response = this->privatePostPrivateCancelAllOrders(this->extend(request, params));
    return response["result"]["data"]["status"] == "OK";
}

std::vector<Order> CryptoCom::fetchOrders(const std::string& symbol, int since, int limit, const Params& params) {
    this->loadMarkets();
    
    auto request = Params();
    if (!symbol.empty()) {
        auto market = this->market(symbol);
        request["instrument_name"] = market["id"].get<std::string>();
    }
    
    if (since != 0) {
        request["start_ts"] = since;
    }
    
    if (limit != 0) {
        request["page_size"] = limit;
    }
    
    auto response = this->privatePostPrivateGetOrderHistory(this->extend(request, params));
    return this->parseOrders(response["result"]["data"]["order_list"], nullptr, since, limit);
}

Order CryptoCom::parseOrder(const json& order, const json& market) {
    auto id = this->safeString(order, "order_id");
    auto status = this->parseOrderStatus(this->safeString(order, "status"));
    auto symbol = market ? market["symbol"].get<std::string>() : 
                         this->markets_by_id[order["instrument_name"].get<std::string>()]["symbol"].get<std::string>();
    auto type = this->safeStringLower(order, "type");
    auto side = this->safeStringLower(order, "side");
    auto price = this->safeFloat(order, "price");
    auto amount = this->safeFloat(order, "quantity");
    auto filled = this->safeFloat(order, "cumulative_quantity");
    auto remaining = amount - filled;
    auto timestamp = this->safeInteger(order, "create_time");
    
    return {
        {"id", id},
        {"info", order},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"symbol", symbol},
        {"type", type},
        {"side", side},
        {"price", price},
        {"amount", amount},
        {"cost", price * filled},
        {"average", nullptr},
        {"filled", filled},
        {"remaining", remaining},
        {"status", status},
        {"fee", nullptr},
        {"trades", nullptr}
    };
}

std::vector<Account> CryptoCom::fetchAccounts(const Params& params) {
    auto response = this->privatePostPrivateGetAccountSummary(params);
    auto accounts = response["result"]["accounts"];
    std::vector<Account> result;
    
    for (const auto& account : accounts) {
        result.push_back({
            {"id", this->safeString(account, "account_id")},
            {"type", this->safeString(account, "account_type")},
            {"currency", this->safeString(account, "currency")},
            {"info", account}
        });
    }
    
    return result;
}

TradingFees CryptoCom::fetchTradingFees(const Params& params) {
    this->loadMarkets();
    auto response = this->privatePostPrivateGetAccountSummary(params);
    auto fees = response["result"]["fees"];
    
    return {
        {"info", fees},
        {"maker", this->safeFloat(fees, "maker_fee")},
        {"taker", this->safeFloat(fees, "taker_fee")}
    };
}

std::vector<Transaction> CryptoCom::fetchDeposits(const std::string& code, int since, int limit, const Params& params) {
    if (code.empty()) {
        throw ArgumentsRequired("fetchDeposits requires a currency code argument");
    }
    
    this->loadMarkets();
    auto currency = this->currency(code);
    auto request = {
        {"currency", currency["id"].get<std::string>()}
    };
    
    if (since != 0) {
        request["start_ts"] = since;
    }
    
    if (limit != 0) {
        request["page_size"] = limit;
    }
    
    auto response = this->privatePostPrivateGetDepositHistory(this->extend(request, params));
    return this->parseTransactions(response["result"]["data"]["deposit_list"], currency, since, limit, "deposit");
}

std::vector<Transaction> CryptoCom::fetchWithdrawals(const std::string& code, int since, int limit, const Params& params) {
    if (code.empty()) {
        throw ArgumentsRequired("fetchWithdrawals requires a currency code argument");
    }
    
    this->loadMarkets();
    auto currency = this->currency(code);
    auto request = {
        {"currency", currency["id"].get<std::string>()}
    };
    
    if (since != 0) {
        request["start_ts"] = since;
    }
    
    if (limit != 0) {
        request["page_size"] = limit;
    }
    
    auto response = this->privatePostPrivateGetWithdrawalHistory(this->extend(request, params));
    return this->parseTransactions(response["result"]["data"]["withdrawal_list"], currency, since, limit, "withdrawal");
}

DepositAddress CryptoCom::fetchDepositAddress(const std::string& code, const Params& params) {
    this->loadMarkets();
    auto currency = this->currency(code);
    auto request = {
        {"currency", currency["id"].get<std::string>()}
    };
    
    auto response = this->privatePostPrivateGetDepositAddress(this->extend(request, params));
    auto data = response["result"]["data"];
    
    return {
        {"currency", code},
        {"address", this->safeString(data, "address")},
        {"tag", this->safeString(data, "tag")},
        {"network", this->safeString(data, "network")},
        {"info", data}
    };
}

std::vector<LedgerEntry> CryptoCom::fetchLedger(const std::string& code, int since, int limit, const Params& params) {
    this->loadMarkets();
    auto request = Params();
    
    if (!code.empty()) {
        auto currency = this->currency(code);
        request["currency"] = currency["id"].get<std::string>();
    }
    
    if (since != 0) {
        request["start_ts"] = since;
    }
    
    if (limit != 0) {
        request["page_size"] = limit;
    }
    
    auto response = this->privatePostPrivateGetAccountSummary(this->extend(request, params));
    return this->parseLedger(response["result"]["data"]["ledger_list"], nullptr, since, limit);
}

Transaction CryptoCom::parseTransaction(const json& transaction, const json& currency, const std::string& type) {
    auto id = this->safeString(transaction, "transaction_id");
    auto amount = this->safeFloat(transaction, "amount");
    auto timestamp = this->safeInteger(transaction, "create_time");
    auto currencyId = this->safeString(transaction, "currency");
    auto code = this->safeCurrencyCode(currencyId);
    auto status = this->parseTransactionStatus(this->safeString(transaction, "status"));
    auto fee = {
        {"cost", this->safeFloat(transaction, "fee")},
        {"currency", code}
    };
    
    return {
        {"info", transaction},
        {"id", id},
        {"txid", this->safeString(transaction, "tx_id")},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"network", this->safeString(transaction, "network")},
        {"address", this->safeString(transaction, "address")},
        {"addressTo", this->safeString(transaction, "address")},
        {"addressFrom", nullptr},
        {"tag", this->safeString(transaction, "tag")},
        {"tagTo", this->safeString(transaction, "tag")},
        {"tagFrom", nullptr},
        {"type", type},
        {"amount", amount},
        {"currency", code},
        {"status", status},
        {"updated", this->safeInteger(transaction, "update_time")},
        {"fee", fee}
    };
}

std::string CryptoCom::parseTransactionStatus(const std::string& status) {
    static const std::map<std::string, std::string> statuses = {
        {"0", "pending"},
        {"1", "ok"},
        {"2", "failed"},
        {"3", "canceled"}
    };
    return statuses.contains(status) ? statuses.at(status) : status;
}

LedgerEntry CryptoCom::parseLedgerEntry(const json& item, const json& currency) {
    auto id = this->safeString(item, "transaction_id");
    auto direction = this->safeString(item, "direction");
    auto account = this->safeString(item, "account_type");
    auto amount = this->safeFloat(item, "amount");
    if (direction == "OUT") {
        amount = -amount;
    }
    auto timestamp = this->safeInteger(item, "create_time");
    auto currencyId = this->safeString(item, "currency");
    auto code = this->safeCurrencyCode(currencyId);
    
    return {
        {"id", id},
        {"info", item},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"direction", direction == "IN" ? "in" : "out"},
        {"account", account},
        {"referenceId", this->safeString(item, "reference_id")},
        {"referenceAccount", nullptr},
        {"type", this->safeString(item, "type")},
        {"currency", code},
        {"amount", amount},
        {"before", nullptr},
        {"after", nullptr},
        {"status", "ok"},
        {"fee", nullptr}
    };
}

std::vector<Order> CryptoCom::fetchOpenOrders(const std::string& symbol, int since, int limit, const Params& params) {
    this->loadMarkets();
    Json request = Json::object();
    Json market;

    if (!symbol.empty()) {
        market = this->market(symbol);
        request["instrument_name"] = market["id"];
    }

    if (since != 0) {
        request["start_ts"] = since;
    }

    if (limit > 0) {
        request["page_size"] = limit;
    }

    Json response = this->privatePostGetOpenOrders(this->extend(request, params));
    return this->parseOrders(response["result"]["order_list"], market, since, limit);
}

std::vector<Trade> CryptoCom::fetchMyTrades(const std::string& symbol, int since, int limit, const Params& params) {
    this->loadMarkets();
    Json request = Json::object();
    Json market;

    if (!symbol.empty()) {
        market = this->market(symbol);
        request["instrument_name"] = market["id"];
    }

    if (since != 0) {
        request["start_ts"] = since;
    }

    if (limit > 0) {
        request["page_size"] = limit;
    }

    Json response = this->privatePostGetTrades(this->extend(request, params));
    return this->parseTrades(response["result"]["trade_list"], market, since, limit);
}

std::vector<Position> CryptoCom::fetchPositions(const std::vector<std::string>& symbols, const Params& params) {
    this->loadMarkets();
    Json request = Json::object();
    
    if (!symbols.empty()) {
        std::vector<std::string> marketIds;
        for (const auto& symbol : symbols) {
            Json market = this->market(symbol);
            marketIds.push_back(market["id"].get<std::string>());
        }
        request["instrument_name"] = marketIds;
    }

    Json response = this->privatePostGetPositions(this->extend(request, params));
    return this->parsePositions(response["result"]["data"], nullptr);
}

Position CryptoCom::fetchPosition(const std::string& symbol, const Params& params) {
    std::vector<Position> positions = this->fetchPositions({symbol}, params);
    for (const auto& position : positions) {
        if (position.symbol == symbol) {
            return position;
        }
    }
    throw ExchangeError("Position not found");
}

std::vector<Transaction> CryptoCom::fetchDeposits(const std::string& code, int since, int limit, const Params& params) {
    this->loadMarkets();
    Json request = Json::object();
    Json currency;

    if (!code.empty()) {
        currency = this->currency(code);
        request["currency"] = currency["id"];
    }

    if (since != 0) {
        request["start_ts"] = since;
    }

    if (limit > 0) {
        request["page_size"] = limit;
    }

    Json response = this->privatePostGetDepositHistory(this->extend(request, params));
    return this->parseTransactions(response["result"]["data"], currency, "deposit");
}

std::vector<Transaction> CryptoCom::fetchWithdrawals(const std::string& code, int since, int limit, const Params& params) {
    this->loadMarkets();
    Json request = Json::object();
    Json currency;

    if (!code.empty()) {
        currency = this->currency(code);
        request["currency"] = currency["id"];
    }

    if (since != 0) {
        request["start_ts"] = since;
    }

    if (limit > 0) {
        request["page_size"] = limit;
    }

    Json response = this->privatePostGetWithdrawalHistory(this->extend(request, params));
    return this->parseTransactions(response["result"]["data"], currency, "withdrawal");
}

std::vector<LedgerEntry> CryptoCom::fetchLedger(const std::string& code, int since, int limit, const Params& params) {
    this->loadMarkets();
    Json request = Json::object();
    Json currency;

    if (!code.empty()) {
        currency = this->currency(code);
        request["currency"] = currency["id"];
    }

    if (since != 0) {
        request["start_ts"] = since;
    }

    if (limit > 0) {
        request["page_size"] = limit;
    }

    Json response = this->privatePostGetTransactions(this->extend(request, params));
    return this->parseLedger(response["result"]["data"], currency, since, limit);
}

std::vector<Account> CryptoCom::fetchAccounts(const Params& params) {
    Json response = this->privatePostGetAccounts(params);
    return this->parseAccounts(response["result"]["accounts"]);
}

TradingFees CryptoCom::fetchTradingFees(const Params& params) {
    this->loadMarkets();
    Json response = this->privatePostGetFeeRate(params);
    return this->parseTradingFees(response["result"]);
}

void CryptoCom::handleErrors(const Json& response) {
    if (response.contains("code") && response["code"].get<int>() != 0) {
        std::string message = response.value("message", "Unknown error");
        std::string errorCode = std::to_string(response["code"].get<int>());
        
        if (errorCode == "10001") {
            throw InvalidOrder(message);
        } else if (errorCode == "10002") {
            throw OrderNotFound(message);
        } else if (errorCode == "10003") {
            throw InsufficientFunds(message);
        } else if (errorCode == "10004") {
            throw AuthenticationError(message);
        } else if (errorCode == "10005") {
            throw PermissionDenied(message);
        } else if (errorCode == "10006") {
            throw BadRequest(message);
        } else if (errorCode == "10007") {
            throw RateLimitExceeded(message);
        } else {
            throw ExchangeError(message);
        }
    }
}

} // namespace ccxt
