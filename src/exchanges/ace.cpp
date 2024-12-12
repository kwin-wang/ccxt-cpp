#include "ccxt/exchanges/ace.h"
#include "ccxt/base/errors.h"

namespace ccxt {

ace::ace(const Config& config) : Exchange(config) {
    init();
}

void ace::init() {
    this->id = "ace";
    this->name = "ACE";
    this->countries = {"TW"};
    this->version = "v2";
    this->rateLimit = 100;
    this->pro = false;
    this->certified = false;

    this->urls = {
        {"logo", "https://github.com/user-attachments/assets/115f1e4a-0fd0-4b76-85d5-a49ebf64d1c8"},
        {"api", {
            {"public", publicApiUrl},
            {"private", privateApiUrl}
        }},
        {"www", "https://ace.io/"},
        {"doc", {"https://github.com/ace-exchange/ace-offical-api-docs"}},
        {"fees", "https://helpcenter.ace.io/hc/zh-tw/articles/360018609132-%E8%B2%BB%E7%8E%87%E8%AA%AA%E6%98%8E"}
    };

    this->has = {
        {"CORS", nullptr},
        {"spot", true},
        {"margin", false},
        {"swap", false},
        {"future", false},
        {"option", false},
        {"cancelAllOrders", false},
        {"cancelOrder", true},
        {"createOrder", true},
        {"fetchBalance", true},
        {"fetchMarkets", true},
        {"fetchMyTrades", true},
        {"fetchOHLCV", true},
        {"fetchOpenOrders", true},
        {"fetchOrder", true},
        {"fetchOrderBook", true},
        {"fetchTicker", true},
        {"fetchTickers", true}
    };

    this->timeframes = {
        {"1m", "1"},
        {"5m", "5"},
        {"10m", "10"},
        {"30m", "30"},
        {"1h", "60"},
        {"2h", "120"},
        {"4h", "240"},
        {"8h", "480"},
        {"12h", "720"},
        {"1d", "24"},
        {"1w", "70"},
        {"1M", "31"}
    };
}

json ace::describe() const {
    return json({
        {"id", this->id},
        {"name", this->name},
        {"countries", this->countries},
        {"version", this->version},
        {"rateLimit", this->rateLimit},
        {"pro", this->pro},
        {"has", this->has},
        {"timeframes", this->timeframes},
        {"urls", this->urls},
        {"api", {
            {"public", {
                {"get", {
                    "oapi/v2/list/tradePrice",
                    "oapi/v2/list/marketPair",
                    "open/v2/public/getOrderBook"
                }}
            }},
            {"private", {
                {"post", {
                    "v2/coin/customerAccount",
                    "v2/kline/getKline",
                    "v2/order/order",
                    "v2/order/cancel",
                    "v2/order/getOrderList",
                    "v2/order/showOrderStatus",
                    "v2/order/showOrderHistory",
                    "v2/order/getTradeList"
                }}
            }}
        }},
        {"fees", {
            {"trading", {
                {"percentage", true},
                {"maker", makerFee},
                {"taker", takerFee}
            }}
        }}
    });
}

// Synchronous REST API methods
json ace::fetchMarkets(const json& params) {
    json response = this->fetch(publicApiUrl + "/oapi/v2/list/marketPair", "GET", {}, "");
    return response;
}

json ace::fetchTicker(const String& symbol, const json& params) {
    json response = this->fetch(publicApiUrl + "/oapi/v2/list/tradePrice", "GET", {}, "");
    return this->parseTicker(response);
}

json ace::fetchTickers(const std::vector<String>& symbols, const json& params) {
    json response = this->fetch(publicApiUrl + "/oapi/v2/list/tradePrice", "GET", {}, "");
    json result = json::object();
    for (const auto& ticker : response) {
        result[ticker["symbol"].get<String>()] = this->parseTicker(ticker);
    }
    return result;
}

json ace::fetchOrderBook(const String& symbol, int limit, const json& params) {
    json request = {
        {"symbol", symbol}
    };
    if (limit > 0) {
        request["limit"] = limit;
    }
    json response = this->fetch(publicApiUrl + "/open/v2/public/getOrderBook", "GET", {}, "");
    return this->parseOrderBook(response, symbol);
}

json ace::fetchOHLCV(const String& symbol, const String& timeframe, int since, int limit, const json& params) {
    json request = {
        {"symbol", symbol},
        {"interval", this->timeframes[timeframe]}
    };
    if (since > 0) {
        request["since"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    json response = this->fetch(privateApiUrl + "/v2/kline/getKline", "POST", {}, request.dump());
    return this->parseOHLCV(response);
}

json ace::fetchBalance(const json& params) {
    json response = this->fetch(privateApiUrl + "/v2/coin/customerAccount", "POST", {}, "");
    return this->parseBalance(response);
}

json ace::createOrder(const String& symbol, const String& type, const String& side,
                     double amount, double price, const json& params) {
    json request = {
        {"symbol", symbol},
        {"type", type},
        {"side", side},
        {"amount", amount}
    };
    if (price > 0) {
        request["price"] = price;
    }
    json response = this->fetch(privateApiUrl + "/v2/order/order", "POST", {}, request.dump());
    return this->parseOrder(response);
}

json ace::cancelOrder(const String& id, const String& symbol, const json& params) {
    json request = {
        {"orderId", id}
    };
    if (!symbol.empty()) {
        request["symbol"] = symbol;
    }
    json response = this->fetch(privateApiUrl + "/v2/order/cancel", "POST", {}, request.dump());
    return this->parseOrder(response);
}

json ace::fetchOrder(const String& id, const String& symbol, const json& params) {
    json request = {
        {"orderId", id}
    };
    if (!symbol.empty()) {
        request["symbol"] = symbol;
    }
    json response = this->fetch(privateApiUrl + "/v2/order/showOrderStatus", "POST", {}, request.dump());
    return this->parseOrder(response);
}

json ace::fetchOpenOrders(const String& symbol, int since, int limit, const json& params) {
    json request = json::object();
    if (!symbol.empty()) {
        request["symbol"] = symbol;
    }
    if (since > 0) {
        request["since"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    json response = this->fetch(privateApiUrl + "/v2/order/getOrderList", "POST", {}, request.dump());
    return response;
}

json ace::fetchMyTrades(const String& symbol, int since, int limit, const json& params) {
    json request = json::object();
    if (!symbol.empty()) {
        request["symbol"] = symbol;
    }
    if (since > 0) {
        request["since"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    json response = this->fetch(privateApiUrl + "/v2/order/getTradeList", "POST", {}, request.dump());
    return this->parseTrade(response);
}

json ace::fetchOrderTrades(const String& id, const String& symbol, const json& params) {
    json request = {
        {"orderId", id}
    };
    if (!symbol.empty()) {
        request["symbol"] = symbol;
    }
    json response = this->fetch(privateApiUrl + "/v2/order/showOrderHistory", "POST", {}, request.dump());
    return this->parseTrade(response);
}

// HTTP methods
json ace::fetch(const String& url, const String& method,
               const std::map<String, String>& headers,
               const String& body) {
    // TODO: Implement HTTP request logic
    return json::object();
}

// Utility methods
String ace::sign(const String& path, const String& api,
                const String& method, const std::map<String, String>& params,
                const std::map<String, String>& headers) {
    // TODO: Implement signing logic
    return "";
}

// Parsing methods
json ace::parseMarket(const json& market) const {
    return json({
        {"id", market["symbol"]},
        {"symbol", market["symbol"]},
        {"base", market["base"]},
        {"quote", market["quote"]},
        {"baseId", market["baseCurrencyId"]},
        {"active", true},
        {"precision", {
            {"amount", std::stoi(market["basePrecision"].get<String>())},
            {"price", std::stoi(market["quotePrecision"].get<String>())}
        }},
        {"limits", {
            {"amount", {
                {"min", std::stod(market["minLimitBaseAmount"].get<String>())},
                {"max", std::stod(market["maxLimitBaseAmount"].get<String>())}
            }}
        }},
        {"info", market}
    });
}

json ace::parseTicker(const json& ticker, const json& market) const {
    String symbol = ticker["symbol"];
    return json({
        {"symbol", symbol},
        {"timestamp", nullptr},
        {"datetime", nullptr},
        {"high", ticker["high"]},
        {"low", ticker["low"]},
        {"bid", ticker["bid"]},
        {"bidVolume", nullptr},
        {"ask", ticker["ask"]},
        {"askVolume", nullptr},
        {"vwap", nullptr},
        {"open", ticker["open"]},
        {"close", ticker["close"]},
        {"last", ticker["last"]},
        {"previousClose", nullptr},
        {"change", ticker["change"]},
        {"percentage", ticker["percentage"]},
        {"average", nullptr},
        {"baseVolume", ticker["volume"]},
        {"quoteVolume", ticker["quoteVolume"]},
        {"info", ticker}
    });
}

json ace::parseOrderBook(const json& orderBook, const String& symbol, const json& market) const {
    return json({
        {"symbol", symbol},
        {"bids", orderBook["bids"]},
        {"asks", orderBook["asks"]},
        {"timestamp", orderBook["timestamp"]},
        {"datetime", orderBook["datetime"]},
        {"nonce", nullptr}
    });
}

json ace::parseOHLCV(const json& ohlcv, const json& market) const {
    return json({
        {ohlcv["timestamp"]},
        {std::stod(ohlcv["open"].get<String>())},
        {std::stod(ohlcv["high"].get<String>())},
        {std::stod(ohlcv["low"].get<String>())},
        {std::stod(ohlcv["close"].get<String>())},
        {std::stod(ohlcv["volume"].get<String>())}
    });
}

String ace::parseOrderStatus(const String& status) const {
    static const std::map<String, String> statuses = {
        {"NEW", "open"},
        {"PARTIALLY_FILLED", "open"},
        {"FILLED", "closed"},
        {"CANCELED", "canceled"},
        {"REJECTED", "rejected"},
        {"EXPIRED", "expired"}
    };
    return statuses.at(status);
}

json ace::parseOrder(const json& order, const json& market) const {
    String id = order["orderId"];
    String status = this->parseOrderStatus(order["status"]);
    return json({
        {"id", id},
        {"clientOrderId", order["clientOrderId"]},
        {"timestamp", order["time"]},
        {"datetime", order["datetime"]},
        {"lastTradeTimestamp", nullptr},
        {"status", status},
        {"symbol", order["symbol"]},
        {"type", order["type"]},
        {"timeInForce", order["timeInForce"]},
        {"side", order["side"]},
        {"price", order["price"]},
        {"amount", order["origQty"]},
        {"filled", order["executedQty"]},
        {"remaining", std::stod(order["origQty"].get<String>()) - std::stod(order["executedQty"].get<String>())},
        {"cost", order["cummulativeQuoteQty"]},
        {"trades", nullptr},
        {"fee", nullptr},
        {"info", order}
    });
}

json ace::parseTrade(const json& trade, const json& market) const {
    String id = trade["id"];
    String orderId = trade["orderId"];
    return json({
        {"id", id},
        {"info", trade},
        {"timestamp", trade["time"]},
        {"datetime", trade["datetime"]},
        {"symbol", trade["symbol"]},
        {"order", orderId},
        {"type", trade["type"]},
        {"side", trade["side"]},
        {"takerOrMaker", trade["takerOrMaker"]},
        {"price", trade["price"]},
        {"amount", trade["qty"]},
        {"cost", std::stod(trade["price"].get<String>()) * std::stod(trade["qty"].get<String>())},
        {"fee", {
            {"cost", trade["commission"]},
            {"currency", trade["commissionAsset"]}
        }}
    });
}

json ace::parseBalance(const json& balance) const {
    json result = {
        {"info", balance}
    };
    
    for (const auto& entry : balance["balances"]) {
        String currency = entry["asset"];
        result[currency] = {
            {"free", std::stod(entry["free"].get<String>())},
            {"used", std::stod(entry["locked"].get<String>())},
            {"total", std::stod(entry["free"].get<String>()) + std::stod(entry["locked"].get<String>())}
        };
    }
    
    return result;
}

} // namespace ccxt
