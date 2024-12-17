#include "ccxt/exchanges/woofipro.h"
#include "ccxt/error.h"
#include <algorithm>

namespace ccxt {

WooFiPro::WooFiPro() {
    // Basic exchange properties
    id = "woofipro";
    name = "WOOFI PRO";
    countries = {"KY"}; // Cayman Islands
    version = "v1";
    certified = true;
    pro = true;
    dex = true;
    hostname = "dex.woo.org";
    rateLimit = 100;

    // API endpoint versions
    publicApiVersion = "v1";
    privateApiVersion = "v1";
    v1 = "v1";
    v2 = "v2";

    // Exchange capabilities
    has = {
        {"CORS", false},
        {"spot", false},
        {"margin", false},
        {"swap", true},
        {"future", false},
        {"option", false},
        {"addMargin", false},
        {"cancelAllOrders", true},
        {"cancelOrder", true},
        {"cancelOrders", true},
        {"cancelWithdraw", false},
        {"closeAllPositions", false},
        {"closePosition", false},
        {"createConvertTrade", false},
        {"createDepositAddress", false},
        {"createMarketBuyOrderWithCost", false},
        {"createMarketOrder", false},
        {"createMarketOrderWithCost", false},
        {"createMarketSellOrderWithCost", false},
        {"createOrder", true},
        {"createOrderWithTakeProfitAndStopLoss", true},
        {"createReduceOnlyOrder", true},
        {"createStopLimitOrder", false},
        {"createStopLossOrder", true},
        {"createStopMarketOrder", false},
        {"createStopOrder", false},
        {"createTakeProfitOrder", true},
        {"createTrailingAmountOrder", false},
        {"createTrailingPercentOrder", false},
        {"createTriggerOrder", true},
        {"fetchAccounts", false},
        {"fetchBalance", true},
        {"fetchCanceledOrders", false},
        {"fetchClosedOrder", false},
        {"fetchClosedOrders", true},
        {"fetchConvertCurrencies", false},
        {"fetchConvertQuote", false},
        {"fetchCurrencies", true},
        {"fetchDepositAddress", false},
        {"fetchDeposits", true},
        {"fetchDepositsWithdrawals", true},
        {"fetchFundingHistory", true},
        {"fetchFundingInterval", true},
        {"fetchFundingIntervals", false},
        {"fetchFundingRate", true},
        {"fetchFundingRateHistory", true},
        {"fetchFundingRates", true},
        {"fetchIndexOHLCV", false},
        {"fetchLedger", true},
        {"fetchLeverage", true},
        {"fetchMarginAdjustmentHistory", false},
        {"fetchMarginMode", false},
        {"fetchMarkets", true},
        {"fetchMarkOHLCV", false},
        {"fetchMyTrades", true},
        {"fetchOHLCV", true},
        {"fetchOpenInterestHistory", false},
        {"fetchOpenOrder", false},
        {"fetchOpenOrders", true},
        {"fetchOrder", true},
        {"fetchOrderBook", true},
        {"fetchOrders", true},
        {"fetchOrderTrades", true},
        {"fetchPosition", true},
        {"fetchPositions", true},
        {"fetchStatus", true},
        {"fetchTicker", true},
        {"fetchTickers", true},
        {"fetchTime", true},
        {"fetchTrades", true},
        {"fetchTradingFee", true},
        {"fetchTradingFees", true},
        {"fetchTransactions", true},
        {"fetchTransfer", true},
        {"fetchTransfers", true},
        {"fetchWithdrawal", true},
        {"fetchWithdrawals", true},
        {"setLeverage", true},
        {"setMarginMode", false},
        {"setPositionMode", false},
        {"transfer", true},
        {"withdraw", true}
    };

    initializeApiEndpoints();
}

void WooFiPro::initializeApiEndpoints() {
    // URLs
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/150730761-1a00e5e0-d28c-480f-9e65-089ce3e6ef3b.jpg"},
        {"api", {
            {"public", "https://api.{hostname}"},
            {"private", "https://api.{hostname}"}
        }},
        {"test", {
            {"public", "https://api.staging.dex.woo.org"},
            {"private", "https://api.staging.dex.woo.org"}
        }},
        {"www", "https://woo.org"},
        {"doc", {
            "https://docs.woo.org/",
            "https://support.woo.network/hc/en-001/articles/4404611795353",
        }},
        {"fees", "https://support.woo.network/hc/en-001/articles/4404611795353"}
    };

    // API endpoints
    api = {
        {"public", {
            {"get", {
                {v1 + "/public/info/{symbol}"},
                {v1 + "/public/info"},
                {v1 + "/public/market_trades/{symbol}"},
                {v1 + "/public/orderbook/{symbol}"},
                {v1 + "/public/ticker/{symbol}"},
                {v1 + "/public/ticker"},
                {v1 + "/public/token"},
                {v1 + "/public/token_network"},
                {v1 + "/public/funding_rates"},
                {v1 + "/public/funding_rate/{symbol}"},
                {v1 + "/public/funding_rate_history/{symbol}"},
                {v1 + "/public/kline"},
                {v1 + "/public/exchange_rate"},
                {v1 + "/public/token_conversion_rate"},
                {v1 + "/public/system_status"},
                {v2 + "/public/futures/funding_rate_history"},
                {v2 + "/public/futures/settlement_history"},
                {v2 + "/public/info"},
                {v2 + "/public/kline"}
            }}
        }},
        {"private", {
            {"get", {
                {v1 + "/client/token"},
                {v1 + "/order/{oid}"},
                {v1 + "/client/order/{client_order_id}"},
                {v1 + "/orders"},
                {v1 + "/order_trades/{oid}"},
                {v1 + "/client/trades"},
                {v1 + "/accountinfo"},
                {v1 + "/positions"},
                {v1 + "/position/{symbol}"},
                {v1 + "/funding_fee_history"},
                {v1 + "/client/holding"},
                {v1 + "/asset/main"},
                {v1 + "/asset/sub"},
                {v1 + "/asset/direct_borrowable"},
                {v1 + "/asset/deduction"},
                {v1 + "/asset/interest"},
                {v1 + "/asset/contract"},
                {v1 + "/asset/convert"},
                {v1 + "/asset/history"},
                {v1 + "/deposit_history"},
                {v1 + "/withdraw_history"},
                {v1 + "/direct_borrow"},
                {v1 + "/direct_repay"},
                {v1 + "/direct_repay_history"},
                {v1 + "/direct_borrow_limit"},
                {v1 + "/sub_account/all"},
                {v1 + "/sub_account/assets"},
                {v1 + "/token_interest"},
                {v1 + "/token_interest_history"},
                {v1 + "/transfer_history"},
                {v1 + "/interest_history"},
                {v2 + "/client/holding"},
                {v2 + "/client/history"},
                {v2 + "/client/holding/broker_otc"},
                {v2 + "/client/holding/broker_otc/details"}
            }},
            {"post", {
                {v1 + "/order"},
                {v1 + "/order/batch"},
                {v1 + "/order/algo"},
                {v1 + "/order/cancel"},
                {v1 + "/order/cancel/batch"},
                {v1 + "/order/cancel/all"},
                {v1 + "/order/cancel/by_client_order_id"},
                {v1 + "/order/cancel/algo"},
                {v1 + "/order/cancel/algo/batch"},
                {v1 + "/order/cancel/algo/all"},
                {v1 + "/asset/main_sub_transfer"},
                {v1 + "/asset/withdraw"},
                {v1 + "/asset/internal_withdraw"},
                {v1 + "/sub_account/transfer"},
                {v1 + "/token_interest/repay"},
                {v1 + "/token_interest/repay/all"}
            }}
        }}
    };

    // Timeframes
    timeframes = {
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
        {"1w", "1w"},
        {"1M", "1M"}
    };
}

// Market Data Methods - Sync
json WooFiPro::fetchMarkets(const json& params) {
    auto response = this->publicGetV1PublicInfo(params);
    auto data = this->safeValue(response, "data", json::array());
    auto rows = this->safeValue(data, "rows", json::array());
    return this->parseMarkets(rows);
}

json WooFiPro::fetchCurrencies(const json& params) {
    auto response = this->publicGetV1PublicToken(params);
    auto data = this->safeValue(response, "data", json::array());
    auto rows = this->safeValue(data, "rows", json::array());
    return this->parseCurrencies(rows);
}

json WooFiPro::fetchTime(const json& params) {
    auto response = this->publicGetV1PublicSystemStatus(params);
    return this->safeInteger(response, "timestamp");
}

json WooFiPro::fetchTicker(const String& symbol, const json& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]}
    };
    auto response = this->publicGetV1PublicTickerSymbol(this->extend(request, params));
    return this->parseTicker(response, market);
}

json WooFiPro::fetchTickers(const std::vector<String>& symbols, const json& params) {
    this->loadMarkets();
    auto response = this->publicGetV1PublicTicker(params);
    auto data = this->safeValue(response, "data", json::array());
    auto rows = this->safeValue(data, "rows", json::array());
    return this->parseTickers(rows, symbols);
}

json WooFiPro::fetchOrderBook(const String& symbol, int limit, const json& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]}
    };
    if (limit != 0) {
        request["max_level"] = limit;
    }
    auto response = this->publicGetV1PublicOrderbookSymbol(this->extend(request, params));
    return this->parseOrderBook(response, market["symbol"]);
}

json WooFiPro::fetchTrades(const String& symbol, int since, int limit, const json& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]}
    };
    if (limit != 0) {
        request["limit"] = limit;
    }
    auto response = this->publicGetV1PublicMarketTradesSymbol(this->extend(request, params));
    auto data = this->safeValue(response, "data", json::array());
    auto rows = this->safeValue(data, "rows", json::array());
    return this->parseTrades(rows, market, since, limit);
}

// Market Data Methods - Async
AsyncPullType WooFiPro::fetchMarketsAsync(const json& params) {
    return async(&WooFiPro::fetchMarkets, params);
}

AsyncPullType WooFiPro::fetchCurrenciesAsync(const json& params) {
    return async(&WooFiPro::fetchCurrencies, params);
}

AsyncPullType WooFiPro::fetchTimeAsync(const json& params) {
    return async(&WooFiPro::fetchTime, params);
}

AsyncPullType WooFiPro::fetchTickerAsync(const String& symbol, const json& params) {
    return async(&WooFiPro::fetchTicker, symbol, params);
}

AsyncPullType WooFiPro::fetchTickersAsync(const std::vector<String>& symbols, const json& params) {
    return async(&WooFiPro::fetchTickers, symbols, params);
}

AsyncPullType WooFiPro::fetchOrderBookAsync(const String& symbol, int limit, const json& params) {
    return async(&WooFiPro::fetchOrderBook, symbol, limit, params);
}

AsyncPullType WooFiPro::fetchTradesAsync(const String& symbol, int since, int limit, const json& params) {
    return async(&WooFiPro::fetchTrades, symbol, since, limit, params);
}

// Trading Methods - Sync
json WooFiPro::createOrder(const String& symbol, const String& type, const String& side,
                          double amount, double price, const json& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto uppercaseType = type.toUpperCase();
    auto uppercaseSide = side.toUpperCase();
    
    auto request = {
        {"symbol", market["id"]},
        {"type", uppercaseType},
        {"side", uppercaseSide},
        {"size", this->amountToPrecision(symbol, amount)}
    };

    if (uppercaseType == "LIMIT") {
        request["price"] = this->priceToPrecision(symbol, price);
    }

    auto response = this->privatePostV1Order(this->extend(request, params));
    return this->parseOrder(response, market);
}

json WooFiPro::cancelOrder(const String& id, const String& symbol, const json& params) {
    this->loadMarkets();
    auto request = {
        {"order_id", id}
    };
    if (!symbol.empty()) {
        auto market = this->market(symbol);
        request["symbol"] = market["id"];
    }
    auto response = this->privatePostV1OrderCancel(this->extend(request, params));
    return this->parseOrder(response);
}

json WooFiPro::cancelAllOrders(const String& symbol, const json& params) {
    this->loadMarkets();
    auto request = json::object();
    if (!symbol.empty()) {
        auto market = this->market(symbol);
        request["symbol"] = market["id"];
    }
    auto response = this->privatePostV1OrderCancelAll(this->extend(request, params));
    return response;
}

json WooFiPro::fetchOrder(const String& id, const String& symbol, const json& params) {
    this->loadMarkets();
    auto request = {
        {"order_id", id}
    };
    if (!symbol.empty()) {
        auto market = this->market(symbol);
        request["symbol"] = market["id"];
    }
    auto response = this->privateGetV1OrderOrderId(this->extend(request, params));
    return this->parseOrder(response);
}

json WooFiPro::fetchOrders(const String& symbol, int since, int limit, const json& params) {
    this->loadMarkets();
    auto request = json::object();
    auto market = nullptr;
    if (!symbol.empty()) {
        market = this->market(symbol);
        request["symbol"] = market["id"];
    }
    if (since != 0) {
        request["start_t"] = since;
    }
    if (limit != 0) {
        request["limit"] = limit;
    }
    auto response = this->privateGetV1Orders(this->extend(request, params));
    return this->parseOrders(response, market, since, limit);
}

// Trading Methods - Async
AsyncPullType WooFiPro::createOrderAsync(const String& symbol, const String& type,
                                           const String& side, double amount, double price,
                                           const json& params) {
    return async(&WooFiPro::createOrder, symbol, type, side, amount, price, params);
}

AsyncPullType WooFiPro::cancelOrderAsync(const String& id, const String& symbol,
                                           const json& params) {
    return async(&WooFiPro::cancelOrder, id, symbol, params);
}

AsyncPullType WooFiPro::cancelAllOrdersAsync(const String& symbol, const json& params) {
    return async(&WooFiPro::cancelAllOrders, symbol, params);
}

AsyncPullType WooFiPro::fetchOrderAsync(const String& id, const String& symbol,
                                          const json& params) {
    return async(&WooFiPro::fetchOrder, id, symbol, params);
}

AsyncPullType WooFiPro::fetchOrdersAsync(const String& symbol, int since, int limit,
                                           const json& params) {
    return async(&WooFiPro::fetchOrders, symbol, since, limit, params);
}

AsyncPullType WooFiPro::fetchOpenOrdersAsync(const String& symbol, int since, int limit,
                                               const json& params) {
    return async(&WooFiPro::fetchOpenOrders, symbol, since, limit, params);
}

AsyncPullType WooFiPro::fetchClosedOrdersAsync(const String& symbol, int since, int limit,
                                                 const json& params) {
    return async(&WooFiPro::fetchClosedOrders, symbol, since, limit, params);
}

// Account Methods - Sync
json WooFiPro::fetchBalance(const json& params) {
    this->loadMarkets();
    auto response = this->privateGetV1AccountInfo(params);
    return this->parseBalance(response);
}

json WooFiPro::fetchDeposits(const String& code, int since, int limit, const json& params) {
    this->loadMarkets();
    auto request = json::object();
    auto currency = nullptr;
    if (!code.empty()) {
        currency = this->currency(code);
        request["token"] = currency["id"];
    }
    if (since != 0) {
        request["start_t"] = since;
    }
    if (limit != 0) {
        request["limit"] = limit;
    }
    auto response = this->privateGetV1DepositHistory(this->extend(request, params));
    return this->parseTransactions(response, currency, since, limit, {"deposit"});
}

json WooFiPro::fetchWithdrawals(const String& code, int since, int limit, const json& params) {
    this->loadMarkets();
    auto request = json::object();
    auto currency = nullptr;
    if (!code.empty()) {
        currency = this->currency(code);
        request["token"] = currency["id"];
    }
    if (since != 0) {
        request["start_t"] = since;
    }
    if (limit != 0) {
        request["limit"] = limit;
    }
    auto response = this->privateGetV1WithdrawHistory(this->extend(request, params));
    return this->parseTransactions(response, currency, since, limit, {"withdrawal"});
}

// Account Methods - Async
AsyncPullType WooFiPro::fetchBalanceAsync(const json& params) {
    return async(&WooFiPro::fetchBalance, params);
}

AsyncPullType WooFiPro::fetchDepositsAsync(const String& code, int since, int limit,
                                             const json& params) {
    return async(&WooFiPro::fetchDeposits, code, since, limit, params);
}

AsyncPullType WooFiPro::fetchWithdrawalsAsync(const String& code, int since, int limit,
                                                const json& params) {
    return async(&WooFiPro::fetchWithdrawals, code, since, limit, params);
}

// Margin Trading Methods - Sync
json WooFiPro::setLeverage(int leverage, const String& symbol, const json& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]},
        {"leverage", leverage}
    };
    return this->privatePostV1Order(this->extend(request, params));
}

// Margin Trading Methods - Async
AsyncPullType WooFiPro::setLeverageAsync(int leverage, const String& symbol,
                                           const json& params) {
    return async(&WooFiPro::setLeverage, leverage, symbol, params);
}

// Parse Methods
json WooFiPro::parseOrder(const json& order, const Market& market) {
    auto id = this->safeString(order, "order_id");
    auto clientOrderId = this->safeString(order, "client_order_id");
    auto timestamp = this->safeInteger(order, "created_time");
    auto symbol = this->safeString(market, "symbol");
    auto type = this->safeStringLower(order, "type");
    auto side = this->safeStringLower(order, "side");
    auto price = this->safeString(order, "price");
    auto amount = this->safeString(order, "size");
    auto filled = this->safeString(order, "executed");
    auto status = this->parseOrderStatus(this->safeString(order, "status"));
    
    return {
        {"id", id},
        {"clientOrderId", clientOrderId},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"symbol", symbol},
        {"type", type},
        {"timeInForce", this->parseTimeInForce(this->safeString(order, "time_in_force"))},
        {"side", side},
        {"price", price},
        {"amount", amount},
        {"cost", nullptr},
        {"average", this->safeString(order, "average_executed_price")},
        {"filled", filled},
        {"remaining", nullptr},
        {"status", status},
        {"fee", nullptr},
        {"trades", nullptr},
        {"info", order}
    };
}

String WooFiPro::parseOrderStatus(const String& status) {
    auto statuses = {
        {"NEW", "open"},
        {"FILLED", "closed"},
        {"CANCELED", "canceled"},
        {"REJECTED", "rejected"},
        {"PARTIAL_FILLED", "open"},
        {"PENDING_CANCEL", "canceling"}
    };
    return this->safeString(statuses, status, status);
}

String WooFiPro::parseTimeInForce(const String& timeInForce) {
    auto timeInForces = {
        {"GTC", "GTC"},
        {"IOC", "IOC"},
        {"FOK", "FOK"},
        {"POST_ONLY", "PO"}
    };
    return this->safeString(timeInForces, timeInForce, timeInForce);
}

json WooFiPro::parseTicker(const json& ticker, const Market& market) {
    auto timestamp = this->safeInteger(ticker, "timestamp");
    auto symbol = market.symbol;
    return {
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"high", this->safeString(ticker, "high")},
        {"low", this->safeString(ticker, "low")},
        {"bid", this->safeString(ticker, "bid")},
        {"bidVolume", this->safeString(ticker, "bidSize")},
        {"ask", this->safeString(ticker, "ask")},
        {"askVolume", this->safeString(ticker, "askSize")},
        {"vwap", this->safeString(ticker, "vwap")},
        {"open", this->safeString(ticker, "open")},
        {"close", this->safeString(ticker, "close")},
        {"last", this->safeString(ticker, "close")},
        {"previousClose", nullptr},
        {"change", nullptr},
        {"percentage", nullptr},
        {"average", nullptr},
        {"baseVolume", this->safeString(ticker, "volume")},
        {"quoteVolume", this->safeString(ticker, "quoteVolume")},
        {"info", ticker}
    };
}

json WooFiPro::parseTrade(const json& trade, const Market& market) {
    auto id = this->safeString(trade, "tid");
    auto timestamp = this->safeInteger(trade, "timestamp");
    auto side = this->safeStringLower(trade, "side");
    auto price = this->safeString(trade, "price");
    auto amount = this->safeString(trade, "size");
    auto cost = this->safeString(trade, "value");
    auto orderId = this->safeString(trade, "order_id");
    
    return {
        {"id", id},
        {"info", trade},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"symbol", market.symbol},
        {"order", orderId},
        {"type", nullptr},
        {"side", side},
        {"takerOrMaker", nullptr},
        {"price", price},
        {"amount", amount},
        {"cost", cost},
        {"fee", nullptr}
    };
}

json WooFiPro::parseBalance(const json& response) {
    auto result = {
        {"info", response}
    };
    auto balances = this->safeValue(response, "data", json::array());
    
    for (const auto& balance : balances) {
        auto currencyId = this->safeString(balance, "token");
        auto code = this->safeCurrencyCode(currencyId);
        auto account = this->account();
        
        account["free"] = this->safeString(balance, "available");
        account["used"] = this->safeString(balance, "frozen");
        account["total"] = this->safeString(balance, "total");
        
        result[code] = account;
    }
    
    return result;
}

String WooFiPro::sign(const String& path, const String& api, const String& method,
                     const json& params, const json& headers, const String& body) {
    auto url = this->urls["api"][api];
    url = this->implodeParams(url, {{"hostname", this->hostname}});
    auto query = this->omit(params, this->extractParams(path));
    url += "/" + this->implodeParams(path, params);

    if (api == "public") {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->checkRequiredCredentials();
        auto timestamp = std::to_string(this->milliseconds());
        auto auth = timestamp + method + "/api/" + path;
        if (method == "POST") {
            if (!query.empty()) {
                body = this->json(query);
                auth += body;
            }
        } else {
            if (!query.empty()) {
                auto queryString = this->urlencode(query);
                auth += "?" + queryString;
                url += "?" + queryString;
            }
        }
        auto signature = this->hmac(this->encode(auth), this->encode(this->secret));
        auto newHeaders = {
            {"x-api-key", this->apiKey},
            {"x-api-signature", signature},
            {"x-api-timestamp", timestamp},
            {"Content-Type", "application/json"}
        };
        headers = this->extend(headers, newHeaders);
    }
    return url;
}

// Market Data Methods
json WooFiPro::parseMarkets(const json& markets) {
    auto result = json::array();
    for (const auto& market : markets) {
        result.push_back(this->parseMarket(market));
    }
    return result;
}

json WooFiPro::parseMarket(const json& market) {
    auto id = this->safeString(market, "symbol");
    auto baseId = this->safeString(market, "base_token");
    auto quoteId = this->safeString(market, "quote_token");
    auto base = this->safeCurrencyCode(baseId);
    auto quote = this->safeCurrencyCode(quoteId);
    auto symbol = base + "/" + quote;
    auto type = this->safeString(market, "type", "spot").toLower();
    
    auto priceFilter = this->safeValue(market, "price_filter", json::object());
    auto amountFilter = this->safeValue(market, "size_filter", json::object());
    auto costFilter = this->safeValue(market, "notional_filter", json::object());
    
    return {
        {"id", id},
        {"symbol", symbol},
        {"base", base},
        {"quote", quote},
        {"baseId", baseId},
        {"quoteId", quoteId},
        {"type", type},
        {"spot", type == "spot"},
        {"margin", type == "margin"},
        {"swap", type == "swap"},
        {"future", type == "future"},
        {"option", type == "option"},
        {"active", this->safeValue(market, "active", true)},
        {"contract", type != "spot"},
        {"linear", this->safeValue(market, "linear", true)},
        {"inverse", this->safeValue(market, "inverse", false)},
        {"contractSize", this->safeNumber(market, "contract_size", 1.0)},
        {"expiry", nullptr},
        {"expiryDatetime", nullptr},
        {"strike", nullptr},
        {"optionType", nullptr},
        {"precision", {
            {"amount", this->safeInteger(market, "size_decimal")},
            {"price", this->safeInteger(market, "price_decimal")},
            {"cost", this->safeInteger(market, "notional_decimal")}
        }},
        {"limits", {
            {"leverage", {
                {"min", this->safeNumber(market, "min_leverage", 1.0)},
                {"max", this->safeNumber(market, "max_leverage", 1.0)}
            }},
            {"amount", {
                {"min", this->safeNumber(amountFilter, "min_size")},
                {"max", this->safeNumber(amountFilter, "max_size")}
            }},
            {"price", {
                {"min", this->safeNumber(priceFilter, "min_price")},
                {"max", this->safeNumber(priceFilter, "max_price")}
            }},
            {"cost", {
                {"min", this->safeNumber(costFilter, "min_notional")},
                {"max", this->safeNumber(costFilter, "max_notional")}
            }}
        }},
        {"info", market}
    };
}

json WooFiPro::parseCurrencies(const json& currencies) {
    auto result = json::object();
    for (const auto& currency : currencies) {
        auto parsedCurrency = this->parseCurrency(currency);
        auto code = parsedCurrency["code"];
        result[code] = parsedCurrency;
    }
    return result;
}

json WooFiPro::parseCurrency(const json& currency) {
    auto id = this->safeString(currency, "token");
    auto code = this->safeCurrencyCode(id);
    auto name = this->safeString(currency, "name");
    auto depositStatus = this->safeString(currency, "deposit_status");
    auto withdrawalStatus = this->safeString(currency, "withdrawal_status");
    auto active = (depositStatus == "ENABLED") && (withdrawalStatus == "ENABLED");
    auto fee = this->safeNumber(currency, "withdrawal_fee");
    
    return {
        {"id", id},
        {"code", code},
        {"name", name},
        {"active", active},
        {"deposit", depositStatus == "ENABLED"},
        {"withdraw", withdrawalStatus == "ENABLED"},
        {"fee", fee},
        {"precision", this->safeInteger(currency, "decimal")},
        {"limits", {
            {"amount", {
                {"min", nullptr},
                {"max", nullptr}
            }},
            {"withdraw", {
                {"min", this->safeNumber(currency, "minimum_withdrawal")},
                {"max", this->safeNumber(currency, "maximum_withdrawal")}
            }}
        }},
        {"networks", {}},
        {"info", currency}
    };
}

json WooFiPro::fetchOHLCV(const String& symbol, const String& timeframe,
                         int since, int limit, const json& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]},
        {"type", this->timeframes[timeframe]}
    };
    if (since != 0) {
        request["start_t"] = since;
    }
    if (limit != 0) {
        request["limit"] = limit;
    }
    auto response = this->publicGetV1PublicKline(this->extend(request, params));
    return this->parseOHLCVs(response["data"], market, timeframe, since, limit);
}

json WooFiPro::parseOHLCV(const json& ohlcv, const Market& market) {
    return {
        this->safeTimestamp(ohlcv, "t"),      // timestamp
        this->safeNumber(ohlcv, "o"),         // open
        this->safeNumber(ohlcv, "h"),         // high
        this->safeNumber(ohlcv, "l"),         // low
        this->safeNumber(ohlcv, "c"),         // close
        this->safeNumber(ohlcv, "v")          // volume
    };
}

// OHLCV - Async
AsyncPullType WooFiPro::fetchOHLCVAsync(const String& symbol, const String& timeframe,
                                          int since, int limit, const json& params) {
    return async(&WooFiPro::fetchOHLCV, symbol, timeframe, since, limit, params);
}

} // namespace ccxt
