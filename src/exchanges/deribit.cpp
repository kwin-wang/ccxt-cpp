#include "deribit.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

Deribit::Deribit() {
    id = "deribit";
    name = "Deribit";
    version = "v2";
    rateLimit = 500;
    testnet = false;
    defaultType = "future";
    defaultSettlement = "BTC";

    // Initialize API endpoints
    baseUrl = testnet ? "https://test.deribit.com" : "https://www.deribit.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/41933112-9e2dd65a-798b-11e8-8440-5bab2959fcb8.jpg"},
        {"api", {
            {"public", baseUrl},
            {"private", baseUrl}
        }},
        {"www", "https://www.deribit.com"},
        {"doc", {
            "https://docs.deribit.com/v2",
            "https://github.com/deribit/deribit-api-clients"
        }},
        {"fees", "https://www.deribit.com/pages/information/fees"},
        {"test", "https://test.deribit.com"}
    };

    api = {
        {"public", {
            {"get", {
                "get_time",
                "get_instruments",
                "get_currencies",
                "get_order_book",
                "get_trade_volumes",
                "get_last_trades_by_currency",
                "get_last_trades_by_instrument",
                "get_index",
                "get_funding_rate_value",
                "get_book_summary_by_instrument",
                "get_book_summary_by_currency",
                "get_delivery_prices",
                "get_mark_price_history",
                "get_historical_volatility"
            }},
        }},
        {"private", {
            {"get", {
                "get_positions",
                "get_position",
                "get_account_summary",
                "get_open_orders_by_currency",
                "get_open_orders_by_instrument",
                "get_user_trades_by_currency",
                "get_user_trades_by_instrument",
                "get_order_history_by_currency",
                "get_order_history_by_instrument",
                "get_order_state",
                "get_transaction_log"
            }},
            {"post", {
                "buy",
                "sell",
                "edit",
                "cancel",
                "cancel_all",
                "cancel_all_by_currency",
                "cancel_all_by_instrument",
                "close_position",
                "get_margins",
                "set_leverage"
            }}
        }}
    };

    has = {
        {"CORS", true},
        {"spot", false},
        {"margin", false},
        {"swap", true},
        {"future", true},
        {"option", true},
        {"cancelAllOrders", true},
        {"cancelOrder", true},
        {"createOrder", true},
        {"editOrder", true},
        {"fetchBalance", true},
        {"fetchClosedOrders", true},
        {"fetchDeposits", true},
        {"fetchLedger", true},
        {"fetchMarkets", true},
        {"fetchMyTrades", true},
        {"fetchOHLCV", true},
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
        {"fetchTransactions", true},
        {"fetchWithdrawals", true},
        {"withdraw", true}
    };

    options = {
        {"fetchTickerQuotes", true},
        {"fetchMarkets", {
            {"types", {"spot", "future", "option"}}
        }},
        {"versions", {
            {"public", {"get", "v2"}},
            {"private", {"get", "v2"}},
            {"private", {"post", "v2"}}
        }}
    };

    precisionMode = TICK_SIZE;

    timeframes = {
        {"1m", "1"},
        {"3m", "3"},
        {"5m", "5"},
        {"10m", "10"},
        {"15m", "15"},
        {"30m", "30"},
        {"1h", "60"},
        {"2h", "120"},
        {"3h", "180"},
        {"6h", "360"},
        {"12h", "720"},
        {"1d", "1D"}
    };

    initializeApiEndpoints();
}

void Deribit::initializeApiEndpoints() {
}

json Deribit::fetchMarkets(const json& params) {
    json response = fetch("/api/v2/public/get_instruments", "public", "GET", params);
    json markets = json::array();
    
    for (const auto& market : response["result"]) {
        String id = market["instrument_name"];
        String baseId = market["base_currency"];
        String quoteId = market["quote_currency"];
        String settleId = market["settlement_currency"];
        String base = this->commonCurrencyCode(baseId);
        String quote = this->commonCurrencyCode(quoteId);
        String settle = this->commonCurrencyCode(settleId);
        String type = market["kind"].get<String>();
        bool future = type == "future";
        bool option = type == "option";
        bool active = market["is_active"];
        
        markets.push_back({
            {"id", id},
            {"symbol", base + "/" + quote + ":" + settle},
            {"base", base},
            {"quote", quote},
            {"settle", settle},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"settleId", settleId},
            {"type", type},
            {"spot", false},
            {"margin", true},
            {"future", future},
            {"option", option},
            {"active", active},
            {"contract", true},
            {"linear", settle == "USDC"},
            {"inverse", settle == base},
            {"contractSize", market["contract_size"]},
            {"expiry", market["expiration_timestamp"]},
            {"expiryDatetime", this->iso8601(market["expiration_timestamp"])},
            {"strike", this->safeFloat(market, "strike")},
            {"optionType", this->safeString(market, "option_type")},
            {"precision", {
                {"amount", market["min_trade_amount"]},
                {"price", market["tick_size"]}
            }},
            {"limits", {
                {"amount", {
                    {"min", market["min_trade_amount"]},
                    {"max", market["max_trade_amount"]}
                }},
                {"price", {
                    {"min", market["tick_size"]},
                    {"max", nullptr}
                }},
                {"cost", {
                    {"min", nullptr},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return markets;
}

json Deribit::fetchTicker(const String& symbol, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {{"instrument_name", market.id}};
    json response = fetch("/api/v2/public/ticker", "public", "GET",
                         this->extend(request, params));
    json ticker = response["result"];
    
    return {
        {"symbol", symbol},
        {"timestamp", ticker["timestamp"]},
        {"datetime", this->iso8601(ticker["timestamp"])},
        {"high", ticker["stats"]["high"]},
        {"low", ticker["stats"]["low"]},
        {"bid", ticker["best_bid_price"]},
        {"bidVolume", ticker["best_bid_amount"]},
        {"ask", ticker["best_ask_price"]},
        {"askVolume", ticker["best_ask_amount"]},
        {"vwap", nullptr},
        {"open", ticker["stats"]["open"]},
        {"close", ticker["last_price"]},
        {"last", ticker["last_price"]},
        {"previousClose", nullptr},
        {"change", nullptr},
        {"percentage", nullptr},
        {"average", nullptr},
        {"baseVolume", ticker["stats"]["volume"]},
        {"quoteVolume", nullptr},
        {"info", ticker}
    };
}

json Deribit::fetchBalance(const json& params) {
    this->loadMarkets();
    String currency = this->safeString(params, "currency", defaultSettlement);
    
    json request = {{"currency", currency}};
    json response = fetch("/api/v2/private/get_account_summary", "private", "GET",
                         this->extend(request, params));
    json result = response["result"];
    
    return {
        {"info", response},
        {"timestamp", this->milliseconds()},
        {"datetime", this->iso8601(this->milliseconds())},
        {currency, {
            {"free", result["available_funds"]},
            {"used", result["maintenance_margin"]},
            {"total", result["equity"]}
        }}
    };
}

json Deribit::createOrder(const String& symbol, const String& type,
                         const String& side, double amount,
                         double price, const json& params) {
    if (symbol.empty()) {
        throw ArgumentsRequired("createOrder() requires a symbol argument");
    }

    auto market = this->market(symbol);
    auto request = json::object();
    request["instrument_name"] = market["id"];
    request["amount"] = this->amountToPrecision(symbol, amount);
    
    if (type == "limit") {
        request["price"] = this->priceToPrecision(symbol, price);
    }

    String method = side;  // "buy" or "sell"
    auto response = this->privatePostUserTrades(this->extend(request, params));
    return this->parseOrder(response, market);
}

json Deribit::cancelOrder(const String& id, const String& symbol, const json& params) {
    if (symbol.empty()) {
        throw ArgumentsRequired("cancelOrder() requires a symbol argument");
    }

    auto market = this->market(symbol);
    auto request = json::object();
    request["order_id"] = id;
    
    auto response = this->privatePostCancel(this->extend(request, params));
    return this->parseOrder(response, market);
}

json Deribit::fetchOrderBook(const String& symbol, int limit, const json& params) {
    auto market = this->market(symbol);
    auto request = json::object();
    request["instrument_name"] = market["id"];
    
    if (limit != 0) {
        request["depth"] = limit;
    }

    auto response = this->publicGetGetOrderBook(this->extend(request, params));
    auto timestamp = this->safeInteger(response, "timestamp");
    return this->parseOrderBook(response, symbol, timestamp, "bids", "asks", "price", "amount");
}

json Deribit::fetchBalance(const json& params) {
    auto response = this->privateGetGetAccountSummary(params);
    auto result = json::object();
    
    for (const auto& balance : response["result"]) {
        auto currencyId = this->safeString(balance, "currency");
        auto code = this->safeCurrencyCode(currencyId);
        
        result[code] = {
            {"free", this->safeString(balance, "available_funds")},
            {"used", this->safeString(balance, "maintenance_margin")},
            {"total", this->safeString(balance, "equity")}
        };
    }
    
    return this->parseBalance(result);
}

json Deribit::fetchPosition(const String& symbol, const json& params) {
    auto market = this->market(symbol);
    auto request = json::object();
    request["instrument_name"] = market["id"];
    
    auto response = this->privateGetPosition(this->extend(request, params));
    return this->parsePosition(response["result"], market);
}

json Deribit::fetchMyTrades(const String& symbol, int since, int limit, const json& params) {
    auto market = this->market(symbol);
    auto request = json::object();
    request["instrument_name"] = market["id"];
    
    if (since != 0) {
        request["start_timestamp"] = since;
    }
    if (limit != 0) {
        request["count"] = limit;
    }
    
    auto response = this->privateGetGetUserTradesByCurrency(this->extend(request, params));
    return this->parseTrades(response["result"]["trades"], market, since, limit);
}

String Deribit::sign(const String& path, const String& api,
                     const String& method, const json& params,
                     const std::map<String, String>& headers,
                     const json& body) {
    auto request = "/" + this->version + "/" + path;
    auto query = this->omit(params, this->extractParams(path));
    auto url = this->urls["api"][api] + request;
    
    if (api == "public") {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->checkRequiredCredentials();
        auto nonce = std::to_string(this->nonce());
        auto timestamp = std::to_string(this->milliseconds());
        auto auth = timestamp + "\n" + nonce + "\n" + method + "\n" + request;
        
        if (method == "GET") {
            if (!query.empty()) {
                url += "?" + this->urlencode(query);
                auth += "?" + this->urlencode(query);
            }
        } else {
            if (!query.empty()) {
                body = this->json(query);
                auth += body;
            }
        }
        
        auto signature = this->hmac(this->encode(auth), this->encode(this->config_.secret),
                                  "sha256", "hex");
        
        headers["Authorization"] = "deri-hmac-sha256 id=" + this->config_.apiKey +
                                 ",ts=" + timestamp +
                                 ",nonce=" + nonce +
                                 ",sig=" + signature;
    }
    
    return url;
}

void Deribit::handleErrors(const json& httpCode, const String& reason, const String& url, const String& method,
                          const std::map<String, String>& headers, const String& body, const json& response,
                          const json& requestHeaders, const json& requestBody) {
    if (response.empty()) {
        return;
    }

    if (!response.contains("error")) {
        return;
    }

    auto error = response["error"];
    auto errorCode = this->safeString(error, "code");
    auto message = this->safeString(error, "message");

    if (errorCode != nullptr) {
        const std::map<String, ExceptionType> exceptions = {
            {"invalid_request", BadRequest},
            {"insufficient_funds", InsufficientFunds},
            {"not_found", OrderNotFound},
            {"not_supported", NotSupported},
            {"overload", DDoSProtection},
            {"server_error", ExchangeError},
            {"requires_authentication", AuthenticationError},
            {"forbidden", PermissionDenied},
            {"rate_limit", DDoSProtection},
            {"under_maintenance", OnMaintenance},
        };

        auto Exception = this->safeValue(exceptions, errorCode, ExchangeError);
        throw Exception(message);
    }
}

json Deribit::parseOrderStatus(const String& status) {
    const std::map<String, String> statuses = {
        {"open", "open"},
        {"filled", "closed"},
        {"rejected", "rejected"},
        {"cancelled", "canceled"},
        {"untriggered", "open"},
        {"triggered", "open"},
        {"closed", "closed"}
    };
    return this->safeString(statuses, status, status);
}

json Deribit::parseOrder(const json& order, const Market& market) {
    auto timestamp = this->safeTimestamp(order, "creation_timestamp");
    auto lastUpdate = this->safeTimestamp(order, "last_update_timestamp");
    auto status = this->parseOrderStatus(this->safeString(order, "order_state"));
    auto marketId = this->safeString(order, "instrument_name");
    auto symbol = this->safeSymbol(marketId, market);
    
    return {
        {"id", this->safeString(order, "order_id")},
        {"clientOrderId", this->safeString(order, "label")},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"lastUpdate", lastUpdate},
        {"lastTradeTimestamp", this->safeTimestamp(order, "last_update_timestamp")},
        {"status", status},
        {"symbol", symbol},
        {"type", this->safeString(order, "order_type")},
        {"side", this->safeString(order, "direction")},
        {"price", this->safeNumber(order, "price")},
        {"amount", this->safeNumber(order, "amount")},
        {"filled", this->safeNumber(order, "filled_amount")},
        {"remaining", this->safeNumber(order, "amount") - this->safeNumber(order, "filled_amount")},
        {"cost", this->safeNumber(order, "filled_amount") * this->safeNumber(order, "average_price")},
        {"average", this->safeNumber(order, "average_price")},
        {"fee", {
            {"cost", this->safeNumber(order, "commission")},
            {"currency", this->safeCurrencyCode(this->safeString(order, "commission_currency"))},
        }},
        {"trades", nullptr},
        {"info", order}
    };
}

json Deribit::parseTrade(const json& trade, const Market& market) {
    auto timestamp = this->safeTimestamp(trade, "timestamp");
    auto id = this->safeString(trade, "trade_id");
    auto orderId = this->safeString(trade, "order_id");
    auto marketId = this->safeString(trade, "instrument_name");
    auto symbol = this->safeSymbol(marketId, market);
    auto side = this->safeString(trade, "direction");
    auto price = this->safeNumber(trade, "price");
    auto amount = this->safeNumber(trade, "amount");
    
    auto fee = json::object();
    if (trade.contains("commission")) {
        fee = {
            {"cost", this->safeNumber(trade, "commission")},
            {"currency", this->safeCurrencyCode(this->safeString(trade, "commission_currency"))}
        };
    }
    
    return {
        {"id", id},
        {"info", trade},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"symbol", symbol},
        {"order", orderId},
        {"type", this->safeString(trade, "order_type")},
        {"side", side},
        {"takerOrMaker", this->safeString(trade, "liquidity")},
        {"price", price},
        {"amount", amount},
        {"cost", price * amount},
        {"fee", fee}
    };
}

json Deribit::parsePosition(const json& position, const Market& market) {
    auto marketId = this->safeString(position, "instrument_name");
    auto symbol = this->safeSymbol(marketId, market);
    auto timestamp = this->safeTimestamp(position, "creation_timestamp");
    auto size = this->safeNumber(position, "size");
    auto side = size < 0 ? "short" : "long";
    auto notional = this->safeNumber(position, "size_currency");
    auto initialMargin = this->safeNumber(position, "initial_margin");
    auto maintenanceMargin = this->safeNumber(position, "maintenance_margin");
    auto unrealizedPnl = this->safeNumber(position, "floating_profit_loss");
    auto contracts = std::abs(size);
    
    return {
        {"info", position},
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"initialMargin", initialMargin},
        {"initialMarginPercentage", initialMargin * 100 / notional},
        {"maintenanceMargin", maintenanceMargin},
        {"maintenanceMarginPercentage", maintenanceMargin * 100 / notional},
        {"entryPrice", this->safeNumber(position, "average_price")},
        {"notional", notional},
        {"leverage", this->safeNumber(position, "leverage")},
        {"unrealizedPnl", unrealizedPnl},
        {"contracts", contracts},
        {"contractSize", this->safeNumber(position, "contract_size")},
        {"side", side},
        {"percentage", unrealizedPnl * 100 / initialMargin}
    };
}

} // namespace ccxt
