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
    api = {
        {"public", {
            {"GET", {
                "api/v2/public/auth",
                "api/v2/public/get_time",
                "api/v2/public/hello",
                "api/v2/public/test",
                "api/v2/public/get_announcements",
                "api/v2/public/get_book_summary_by_currency",
                "api/v2/public/get_book_summary_by_instrument",
                "api/v2/public/get_contract_size",
                "api/v2/public/get_currencies",
                "api/v2/public/get_funding_chart_data",
                "api/v2/public/get_funding_rate_history",
                "api/v2/public/get_funding_rate_value",
                "api/v2/public/get_historical_volatility",
                "api/v2/public/get_index",
                "api/v2/public/get_instruments",
                "api/v2/public/get_last_settlements_by_currency",
                "api/v2/public/get_last_settlements_by_instrument",
                "api/v2/public/get_last_trades_by_currency",
                "api/v2/public/get_last_trades_by_currency_and_time",
                "api/v2/public/get_last_trades_by_instrument",
                "api/v2/public/get_last_trades_by_instrument_and_time",
                "api/v2/public/get_order_book",
                "api/v2/public/get_trade_volumes",
                "api/v2/public/get_tradingview_chart_data",
                "api/v2/public/ticker"
            }}
        }},
        {"private", {
            {"GET", {
                "api/v2/private/get_account_summary",
                "api/v2/private/get_position",
                "api/v2/private/get_positions",
                "api/v2/private/get_order_history_by_currency",
                "api/v2/private/get_order_margin_by_ids",
                "api/v2/private/get_order_state",
                "api/v2/private/get_user_trades_by_currency",
                "api/v2/private/get_user_trades_by_instrument",
                "api/v2/private/get_settlement_history_by_instrument",
                "api/v2/private/get_settlement_history_by_currency"
            }},
            {"POST", {
                "api/v2/private/buy",
                "api/v2/private/sell",
                "api/v2/private/edit",
                "api/v2/private/cancel",
                "api/v2/private/cancel_all",
                "api/v2/private/cancel_all_by_currency",
                "api/v2/private/close_position",
                "api/v2/private/get_margins",
                "api/v2/private/set_leverage"
            }}
        }}
    };
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
    this->loadMarkets();
    Market market = this->market(symbol);
    
    String request = {
        {"instrument_name", market.id},
        {"amount", amount},
        {"type", type.upper()}
    };
    
    String method = side == "buy" ? "private/buy" : "private/sell";
    
    if (type == "limit") {
        if (price == 0) {
            throw InvalidOrder("For limit orders, price cannot be zero");
        }
        request["price"] = price;
    }
    
    json response = fetch("/api/v2/" + method, "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response["result"], market);
}

String Deribit::sign(const String& path, const String& api,
                     const String& method, const json& params,
                     const std::map<String, String>& headers,
                     const json& body) {
    String request = "/" + this->version + "/" + this->implodeParams(path, params);
    String url = this->urls["api"][api] + request;
    String query = this->omit(params, this->extractParams(path));
    
    if (api == "public") {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->checkRequiredCredentials();
        String nonce = std::to_string(this->nonce());
        String timestamp = this->milliseconds();
        String requestBody = "";
        
        if (!query.empty()) {
            if (method == "GET") {
                url += "?" + this->urlencode(query);
            } else {
                requestBody = this->json(query);
            }
        }
        
        String auth = timestamp + "\n" + nonce + "\n" + method + "\n" + request;
        if (!requestBody.empty()) {
            auth += "\n" + requestBody;
        }
        
        String signature = this->hmac(auth, this->secret, "sha256", "hex");
        
        const_cast<std::map<String, String>&>(headers)["Authorization"] = "deri-hmac-sha256 " + 
            this->apiKey + ":" + nonce + ":" + timestamp + ":" + signature;
        
        if (method != "GET") {
            const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/json";
        }
    }
    
    return url;
}

json Deribit::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"open", "open"},
        {"filled", "closed"},
        {"rejected", "rejected"},
        {"cancelled", "canceled"},
        {"untriggered", "open"},
        {"triggered", "open"},
        {"closed", "closed"}
    };
    
    return statuses.contains(status) ? statuses.at(status) : status;
}

json Deribit::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, "order_id");
    String timestamp = this->safeInteger(order, "creation_timestamp");
    String lastTradeTimestamp = this->safeInteger(order, "last_update_timestamp");
    String status = this->parseOrderStatus(this->safeString(order, "order_state"));
    
    return {
        {"id", id},
        {"clientOrderId", this->safeString(order, "label")},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", lastTradeTimestamp},
        {"status", status},
        {"symbol", market.symbol},
        {"type", this->safeStringLower(order, "order_type")},
        {"side", this->safeStringLower(order, "direction")},
        {"price", this->safeFloat(order, "price")},
        {"amount", this->safeFloat(order, "amount")},
        {"filled", this->safeFloat(order, "filled_amount")},
        {"remaining", this->safeFloat(order, "amount") - this->safeFloat(order, "filled_amount")},
        {"cost", this->safeFloat(order, "average_price") * this->safeFloat(order, "filled_amount")},
        {"average", this->safeFloat(order, "average_price")},
        {"trades", nullptr},
        {"fee", {
            {"cost", this->safeFloat(order, "commission")},
            {"currency", market.quote}
        }},
        {"info", order}
    };
}

json Deribit::parseOptionContract(const json& contract) {
    return {
        {"symbol", contract["instrument_name"]},
        {"timestamp", contract["creation_timestamp"]},
        {"expiry", contract["expiration_timestamp"]},
        {"strike", contract["strike"]},
        {"optionType", contract["option_type"]},
        {"underlying", contract["base_currency"]},
        {"settlement", contract["settlement_currency"]},
        {"contractSize", contract["contract_size"]},
        {"active", contract["is_active"]},
        {"info", contract}
    };
}

} // namespace ccxt
