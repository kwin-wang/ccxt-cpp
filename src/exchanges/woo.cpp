#include "ccxt/exchanges/woo.h"
#include "ccxt/error.h"
#include <algorithm>

namespace ccxt {

Woo::Woo() {
    // Basic exchange properties
    id = "woo";
    name = "WOO X";
    countries = {"KY"}; // Cayman Islands
    version = "v1";
    certified = true;
    pro = true;
    hostname = "woox.io";
    rateLimit = 100;

    // API endpoint versions
    publicApiVersion = "v1";
    privateApiVersion = "v1";
    v1 = "v1";
    v2 = "v2";

    // Exchange capabilities
    has = {
        {"CORS", false},
        {"spot", true},
        {"margin", true},
        {"swap", true},
        {"future", false},
        {"option", false},
        {"addMargin", true},
        {"cancelAllOrders", true},
        {"cancelAllOrdersAfter", true},
        {"cancelOrder", true},
        {"cancelWithdraw", false},
        {"closeAllPositions", false},
        {"closePosition", false},
        {"createConvertTrade", true},
        {"createDepositAddress", false},
        {"createMarketBuyOrderWithCost", true},
        {"createMarketOrder", false},
        {"createMarketOrderWithCost", false},
        {"createMarketSellOrderWithCost", true},
        {"createOrder", true},
        {"createOrderWithTakeProfitAndStopLoss", true},
        {"createReduceOnlyOrder", true},
        {"createStopLimitOrder", false},
        {"createStopLossOrder", true},
        {"createStopMarketOrder", false},
        {"createStopOrder", false},
        {"createTakeProfitOrder", true},
        {"createTrailingAmountOrder", true},
        {"createTrailingPercentOrder", true},
        {"createTriggerOrder", true},
        {"fetchAccounts", true},
        {"fetchBalance", true},
        {"fetchCanceledOrders", false},
        {"fetchClosedOrder", false},
        {"fetchClosedOrders", true},
        {"fetchConvertCurrencies", true},
        {"fetchConvertQuote", true},
        {"fetchConvertTrade", true},
        {"fetchConvertTradeHistory", true},
        {"fetchCurrencies", true},
        {"fetchDepositAddress", true},
        {"fetchDepositAddresses", false},
        {"fetchDepositAddressesByNetwork", false},
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

void Woo::initializeApiEndpoints() {
    // URLs
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/150730761-1a00e5e0-d28c-480f-9e65-089ce3e6ef3b.jpg"},
        {"api", {
            {"public", "https://api.{hostname}"},
            {"private", "https://api.{hostname}"}
        }},
        {"test", {
            {"public", "https://api.staging.woo.org"},
            {"private", "https://api.staging.woo.org"}
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
                {v1 + "/deposit_address"},
                {v1 + "/deposit_history"},
                {v1 + "/withdraw_history"},
                {v1 + "/direct_borrow"},
                {v1 + "/direct_repay"},
                {v1 + "/direct_repay_history"},
                {v1 + "/direct_borrow_limit"},
                {v1 + "/convert_quote"},
                {v1 + "/convert_check_risk"},
                {v1 + "/convert_trade"},
                {v1 + "/convert_trades"},
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
                {v1 + "/asset/convert"},
                {v1 + "/asset/convert/v2"},
                {v1 + "/asset/convert/v3"},
                {v1 + "/asset/convert/batch"},
                {v1 + "/asset/convert/batch/v2"},
                {v1 + "/asset/convert/batch/v3"},
                {v1 + "/asset/convert/estimate"},
                {v1 + "/asset/convert/estimate/v2"},
                {v1 + "/asset/convert/estimate/v3"},
                {v1 + "/asset/convert/estimate/batch"},
                {v1 + "/asset/convert/estimate/batch/v2"},
                {v1 + "/asset/convert/estimate/batch/v3"},
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
json Woo::fetchMarkets(const json& params) {
    auto response = this->publicGetV1PublicInfo(params);
    auto data = this->safeValue(response, "data", json::array());
    auto rows = this->safeValue(data, "rows", json::array());
    return this->parseMarkets(rows);
}

json Woo::fetchCurrencies(const json& params) {
    auto response = this->publicGetV1PublicToken(params);
    auto data = this->safeValue(response, "data", json::array());
    auto rows = this->safeValue(data, "rows", json::array());
    return this->parseCurrencies(rows);
}

json Woo::fetchTime(const json& params) {
    auto response = this->publicGetV1PublicSystemStatus(params);
    return this->safeInteger(response, "timestamp");
}

json Woo::fetchTicker(const String& symbol, const json& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]}
    };
    auto response = this->publicGetV1PublicTickerSymbol(this->extend(request, params));
    return this->parseTicker(response, market);
}

json Woo::fetchTickers(const std::vector<String>& symbols, const json& params) {
    this->loadMarkets();
    auto response = this->publicGetV1PublicTicker(params);
    auto data = this->safeValue(response, "data", json::array());
    auto rows = this->safeValue(data, "rows", json::array());
    return this->parseTickers(rows, symbols);
}

json Woo::fetchOrderBook(const String& symbol, int limit, const json& params) {
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

json Woo::fetchTrades(const String& symbol, int since, int limit, const json& params) {
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
std::future<json> Woo::fetchMarketsAsync(const json& params) {
    return async(&Woo::fetchMarkets, params);
}

std::future<json> Woo::fetchCurrenciesAsync(const json& params) {
    return async(&Woo::fetchCurrencies, params);
}

std::future<json> Woo::fetchTimeAsync(const json& params) {
    return async(&Woo::fetchTime, params);
}

std::future<json> Woo::fetchTickerAsync(const String& symbol, const json& params) {
    return async(&Woo::fetchTicker, symbol, params);
}

std::future<json> Woo::fetchTickersAsync(const std::vector<String>& symbols, const json& params) {
    return async(&Woo::fetchTickers, symbols, params);
}

std::future<json> Woo::fetchOrderBookAsync(const String& symbol, int limit, const json& params) {
    return async(&Woo::fetchOrderBook, symbol, limit, params);
}

std::future<json> Woo::fetchTradesAsync(const String& symbol, int since, int limit, const json& params) {
    return async(&Woo::fetchTrades, symbol, since, limit, params);
}

// Account Methods - Sync
json Woo::fetchAccounts(const json& params) {
    auto response = this->privateGetV1SubAccountAll(params);
    auto data = this->safeValue(response, "data", json::array());
    auto rows = this->safeValue(data, "rows", json::array());
    return this->parseAccounts(rows);
}

json Woo::fetchBalance(const json& params) {
    this->loadMarkets();
    auto response = this->privateGetV1ClientHolding(params);
    auto data = this->safeValue(response, "data", json::object());
    auto holding = this->safeValue(data, "holding", json::array());
    return this->parseBalance(holding);
}

json Woo::fetchLedger(const String& code, int since, int limit, const json& params) {
    this->loadMarkets();
    auto request = json::object();
    Currency currency;
    if (!code.empty()) {
        currency = this->currency(code);
        request["token"] = currency.id;
    }
    if (since != 0) {
        request["start_t"] = since;
    }
    if (limit != 0) {
        request["limit"] = limit;
    }
    auto response = this->privateGetV1AssetHistory(this->extend(request, params));
    auto data = this->safeValue(response, "data", json::array());
    auto rows = this->safeValue(data, "rows", json::array());
    return this->parseLedger(rows, currency, since, limit);
}

json Woo::fetchDepositAddress(const String& code, const json& params) {
    this->loadMarkets();
    auto currency = this->currency(code);
    auto request = {
        {"token", currency["id"]}
    };
    auto response = this->privateGetV1DepositAddress(this->extend(request, params));
    auto data = this->safeValue(response, "data", json::object());
    return this->parseDepositAddress(data, currency);
}

json Woo::fetchDeposits(const String& code, int since, int limit, const json& params) {
    this->loadMarkets();
    auto request = json::object();
    Currency currency;
    if (!code.empty()) {
        currency = this->currency(code);
        request["token"] = currency.id;
    }
    if (since != 0) {
        request["start_t"] = since;
    }
    if (limit != 0) {
        request["limit"] = limit;
    }
    auto response = this->privateGetV1DepositHistory(this->extend(request, params));
    auto data = this->safeValue(response, "data", json::array());
    auto rows = this->safeValue(data, "rows", json::array());
    return this->parseTransactions(rows, currency, since, limit, "deposit");
}

json Woo::fetchWithdrawals(const String& code, int since, int limit, const json& params) {
    this->loadMarkets();
    auto request = json::object();
    Currency currency;
    if (!code.empty()) {
        currency = this->currency(code);
        request["token"] = currency.id;
    }
    if (since != 0) {
        request["start_t"] = since;
    }
    if (limit != 0) {
        request["limit"] = limit;
    }
    auto response = this->privateGetV1WithdrawHistory(this->extend(request, params));
    auto data = this->safeValue(response, "data", json::array());
    auto rows = this->safeValue(data, "rows", json::array());
    return this->parseTransactions(rows, currency, since, limit, "withdrawal");
}

// Account Methods - Async
std::future<json> Woo::fetchAccountsAsync(const json& params) {
    return async(&Woo::fetchAccounts, params);
}

std::future<json> Woo::fetchBalanceAsync(const json& params) {
    return async(&Woo::fetchBalance, params);
}

std::future<json> Woo::fetchLedgerAsync(const String& code, int since, int limit,
                                      const json& params) {
    return async(&Woo::fetchLedger, code, since, limit, params);
}

std::future<json> Woo::fetchDepositAddressAsync(const String& code, const json& params) {
    return async(&Woo::fetchDepositAddress, code, params);
}

std::future<json> Woo::fetchDepositsAsync(const String& code, int since, int limit,
                                        const json& params) {
    return async(&Woo::fetchDeposits, code, since, limit, params);
}

std::future<json> Woo::fetchWithdrawalsAsync(const String& code, int since, int limit,
                                           const json& params) {
    return async(&Woo::fetchWithdrawals, code, since, limit, params);
}

// Helper Methods
json Woo::parseAccounts(const json& accounts) {
    auto result = json::array();
    for (const auto& account : accounts) {
        result.push_back({
            {"id", this->safeString(account, "sub_account_id")},
            {"type", this->safeString(account, "account_type")},
            {"code", this->safeString(account, "token")},
            {"info", account}
        });
    }
    return result;
}

json Woo::parseBalance(const json& balance) {
    auto result = {
        {"info", balance},
        {"timestamp", nullptr},
        {"datetime", nullptr}
    };

    for (const auto& entry : balance) {
        auto currencyId = this->safeString(entry, "token");
        auto code = this->safeCurrencyCode(currencyId);
        auto account = this->account();
        account["free"] = this->safeString(entry, "available");
        account["used"] = this->safeString(entry, "holding");
        account["total"] = this->safeString(entry, "total");
        result[code] = account;
    }

    return result;
}

json Woo::parseLedgerEntry(const json& item, const Currency& currency) {
    auto id = this->safeString(item, "id");
    auto direction = this->safeString(item, "side");
    auto account = this->safeString(item, "account_id");
    auto referenceId = this->safeString(item, "reference_id");
    auto referenceAccount = this->safeString(item, "reference_account");
    auto type = this->safeString(item, "type");
    auto currencyId = this->safeString(item, "token");
    auto code = this->safeCurrencyCode(currencyId);
    auto amount = this->safeString(item, "amount");
    auto timestamp = this->safeInteger(item, "created_time");
    auto fee = {
        {"cost", this->safeString(item, "fee")},
        {"currency", code}
    };

    return {
        {"id", id},
        {"direction", direction},
        {"account", account},
        {"referenceId", referenceId},
        {"referenceAccount", referenceAccount},
        {"type", type},
        {"currency", code},
        {"amount", amount},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"fee", fee},
        {"info", item}
    };
}

json Woo::parseDepositAddress(const json& depositAddress, const Currency& currency) {
    auto address = this->safeString(depositAddress, "address");
    auto tag = this->safeString(depositAddress, "tag");
    return {
        {"currency", currency.code},
        {"address", address},
        {"tag", tag},
        {"network", this->safeString(depositAddress, "network")},
        {"info", depositAddress}
    };
}

json Woo::parseTransaction(const json& transaction, const String& type) {
    auto id = this->safeString(transaction, "id");
    auto timestamp = this->safeInteger(transaction, "created_time");
    auto currencyId = this->safeString(transaction, "token");
    auto code = this->safeCurrencyCode(currencyId);
    auto amount = this->safeString(transaction, "amount");
    auto status = this->parseTransactionStatus(this->safeString(transaction, "status"));
    auto fee = {
        {"currency", code},
        {"cost", this->safeString(transaction, "fee")}
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
        {"updated", this->safeInteger(transaction, "updated_time")},
        {"fee", fee}
    };
}

String Woo::parseTransactionStatus(const String& status) {
    auto statuses = {
        {"NEW", "pending"},
        {"PROCESSING", "pending"},
        {"COMPLETED", "ok"},
        {"CANCELLED", "canceled"},
        {"FAILED", "failed"}
    };
    return this->safeString(statuses, status, status);
}

// Helper Methods
json Woo::parseTicker(const json& ticker, const Market& market) {
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

String Woo::sign(const String& path, const String& api, const String& method,
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

} // namespace ccxt
