#include "ccxt/exchanges/bitbank.h"
#include <openssl/hmac.h>
#include <iomanip>
#include <sstream>
#include <boost/thread/future.hpp>

namespace ccxt {

Bitbank::Bitbank() : Exchange() {
    id = "bitbank";
    name = "bitbank";
    countries = {"JP"};
    version = "1";
    rateLimit = 1000;
    has = {
        {"CORS", false},
        {"spot", true},
        {"margin", false},
        {"swap", false},
        {"future", false},
        {"option", false},
        {"fetchMarkets", true},
        {"fetchTicker", true},
        {"fetchOrderBook", true},
        {"fetchTrades", true},
        {"fetchOHLCV", true},
        {"fetchBalance", true},
        {"createOrder", true},
        {"cancelOrder", true},
        {"fetchOrder", true},
        {"fetchOpenOrders", true},
        {"fetchMyTrades", true},
        {"fetchDepositAddress", true},
        {"withdraw", true},
    };

    timeframes = {
        {"1m", "1min"},
        {"5m", "5min"},
        {"15m", "15min"},
        {"30m", "30min"},
        {"1h", "1hour"},
        {"4h", "4hour"},
        {"8h", "8hour"},
        {"12h", "12hour"},
        {"1d", "1day"},
        {"1w", "1week"},
    };

    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/37808081-b87f2d9c-2e59-11e8-894d-c1900b7584fe.jpg"},
        {"api", {
            {"public", "https://public.bitbank.cc"},
            {"private", "https://api.bitbank.cc/v1"},
        }},
        {"www", "https://bitbank.cc/"},
        {"doc", "https://docs.bitbank.cc/"},
    };

    api = {
        {"public", {
            {"GET", {
                "{pair}/ticker",
                "{pair}/depth",
                "{pair}/transactions",
                "{pair}/transactions/{yyyymmdd}",
                "{pair}/candlestick/{candletype}/{yyyymmdd}",
            }},
        }},
        {"private", {
            {"GET", {
                "user/assets",
                "user/spot/order",
                "user/spot/active_orders",
                "user/spot/trade_history",
                "user/withdrawal_account",
                "user/crypto_withdrawal",
            }},
            {"POST", {
                "user/spot/order",
                "user/spot/cancel_order",
                "user/crypto_withdrawal",
            }},
        }},
    };

    options = {
        {"marketsByType", {
            {"spot", {
                "btc_jpy", "xrp_jpy", "ltc_jpy", "eth_jpy", "mona_jpy",
                "bcc_jpy", "xlm_jpy", "qtum_jpy", "bat_jpy", "omg_jpy",
                "xym_jpy", "link_jpy", "mkr_jpy", "boba_jpy", "enj_jpy",
                "matic_jpy", "dot_jpy", "doge_jpy", "astr_jpy", "ada_jpy",
                "avax_jpy", "axs_jpy", "flr_jpy", "sand_jpy",
            }},
        }},
    };

    initializeApiEndpoints();
}

void Bitbank::initializeApiEndpoints() {
    // Initialize API endpoints
    apiEndpoints = {
        {"public", "https://public.bitbank.cc"},
        {"private", "https://api.bitbank.cc/v1"},
    };
}

json Bitbank::fetchMarkets(const json& params) {
    auto response = this->publicGetPairs(params);
    auto markets = response["data"]["pairs"];
    auto result = json::array();

    for (const auto& market : markets) {
        auto id = market["name"].get<String>();
        auto baseId = market["base_asset"].get<String>();
        auto quoteId = market["quote_asset"].get<String>();
        auto base = this->safeCurrencyCode(baseId);
        auto quote = this->safeCurrencyCode(quoteId);
        auto symbol = base + "/" + quote;

        result.push_back({
            {"id", id},
            {"symbol", symbol},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", true},
            {"type", "spot"},
            {"spot", true},
            {"precision", {
                {"amount", market["amount_digits"].get<int>()},
                {"price", market["price_digits"].get<int>()},
            }},
            {"limits", {
                {"amount", {
                    {"min", this->safeNumber(market, "min_amount")},
                    {"max", this->safeNumber(market, "max_amount")},
                }},
                {"price", {
                    {"min", this->safeNumber(market, "min_price")},
                    {"max", this->safeNumber(market, "max_price")},
                }},
                {"cost", {
                    {"min", this->safeNumber(market, "min_order_value")},
                    {"max", this->safeNumber(market, "max_order_value")},
                }},
            }},
            {"info", market},
        });
    }

    return result;
}

json Bitbank::fetchTicker(const String& symbol, const json& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto request = {{"pair", market["id"]}};
    auto response = this->publicGetPairTicker(this->extend(request, params));
    auto ticker = response["data"];
    auto timestamp = this->safeTimestamp(ticker, "timestamp");

    return {
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"high", this->safeString(ticker, "high")},
        {"low", this->safeString(ticker, "low")},
        {"bid", this->safeString(ticker, "buy")},
        {"bidVolume", undefined},
        {"ask", this->safeString(ticker, "sell")},
        {"askVolume", undefined},
        {"vwap", undefined},
        {"open", undefined},
        {"close", this->safeString(ticker, "last")},
        {"last", this->safeString(ticker, "last")},
        {"previousClose", undefined},
        {"change", undefined},
        {"percentage", undefined},
        {"average", undefined},
        {"baseVolume", this->safeString(ticker, "vol")},
        {"quoteVolume", undefined},
        {"info", ticker},
    };
}

String Bitbank::sign(const String& path, const String& api,
                    const String& method, const json& params,
                    const std::map<String, String>& headers,
                    const json& body) {
    auto query = this->omit(params, this->extractParams(path));
    auto url = this->urls["api"][api];
    
    if (api == "public") {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->checkRequiredCredentials();
        auto nonce = this->nonce().toString();
        auto queryString = this->urlencode(query);
        
        if (method == "GET") {
            if (!query.empty()) {
                url += "?" + queryString;
            }
        } else {
            body = queryString;
        }
        
        auto auth = nonce + url + (body.empty() ? "" : body);
        auto signature = this->hmac(this->encode(auth), this->encode(this->secret),
                                  "sha256", "hex");
        
        headers["API-KEY"] = this->apiKey;
        headers["API-TIMESTAMP"] = nonce;
        headers["API-SIGN"] = signature;
    }
    
    return url;
}

String Bitbank::createNonce() {
    return std::to_string(this->milliseconds());
}

String Bitbank::createSignature(const String& nonce, const String& method,
                              const String& path, const String& body) {
    auto message = nonce + method + path + body;
    unsigned char* digest = HMAC(EVP_sha256(), this->secret.c_str(), this->secret.length(),
                                reinterpret_cast<const unsigned char*>(message.c_str()),
                                message.length(), nullptr, nullptr);
    
    std::stringstream ss;
    for(int i = 0; i < 32; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
    }
    return ss.str();
}

// Async Market Data API
boost::future<json> Bitbank::fetchMarketsAsync(const json& params) {
    return requestAsync("", "public", "GET", params);
}

boost::future<json> Bitbank::fetchTickerAsync(const String& symbol, const json& params) {
    String market = getBitbankSymbol(symbol);
    String path = market + "/ticker";
    return requestAsync(path, "public", "GET", params);
}

boost::future<json> Bitbank::fetchOrderBookAsync(const String& symbol, int limit, const json& params) {
    String market = getBitbankSymbol(symbol);
    String path = market + "/depth";
    return requestAsync(path, "public", "GET", params);
}

boost::future<json> Bitbank::fetchTradesAsync(const String& symbol, int since, int limit, const json& params) {
    String market = getBitbankSymbol(symbol);
    String path = market + "/transactions";
    return requestAsync(path, "public", "GET", params);
}

boost::future<json> Bitbank::fetchOHLCVAsync(const String& symbol, const String& timeframe, int since, int limit, const json& params) {
    String market = getBitbankSymbol(symbol);
    String candleType = timeframes[timeframe];
    String date = getYYYYMMDD(since);
    String path = market + "/candlestick/" + candleType + "/" + date;
    return requestAsync(path, "public", "GET", params);
}

boost::future<json> Bitbank::fetchTradingFeesAsync(const json& params) {
    return requestAsync("user/spot/trade_history", "private", "GET", params);
}

// Async Trading API
boost::future<json> Bitbank::fetchBalanceAsync(const json& params) {
    return requestAsync("user/assets", "private", "GET", params);
}

boost::future<json> Bitbank::createOrderAsync(const String& symbol, const String& type, const String& side,
                                          double amount, double price, const json& params) {
    json request = {
        {"pair", getBitbankSymbol(symbol)},
        {"amount", std::to_string(amount)},
        {"side", side},
        {"type", type}
    };

    if (type == "limit") {
        request["price"] = std::to_string(price);
    }

    return requestAsync("user/spot/order", "private", "POST", request);
}

boost::future<json> Bitbank::cancelOrderAsync(const String& id, const String& symbol, const json& params) {
    json request = {
        {"pair", getBitbankSymbol(symbol)},
        {"order_id", id}
    };
    return requestAsync("user/spot/cancel_order", "private", "POST", request);
}

boost::future<json> Bitbank::fetchOrderAsync(const String& id, const String& symbol, const json& params) {
    json request = {
        {"pair", getBitbankSymbol(symbol)},
        {"order_id", id}
    };
    return requestAsync("user/spot/order", "private", "GET", request);
}

boost::future<json> Bitbank::fetchOpenOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    json request = {{"pair", getBitbankSymbol(symbol)}};
    return requestAsync("user/spot/active_orders", "private", "GET", request);
}

boost::future<json> Bitbank::fetchMyTradesAsync(const String& symbol, int since, int limit, const json& params) {
    json request = {{"pair", getBitbankSymbol(symbol)}};
    return requestAsync("user/spot/trade_history", "private", "GET", request);
}

// Async Account API
boost::future<json> Bitbank::fetchDepositAddressAsync(const String& code, const json& params) {
    return requestAsync("user/withdrawal_account", "private", "GET", params);
}

boost::future<json> Bitbank::withdrawAsync(const String& code, double amount, const String& address,
                                       const String& tag, const json& params) {
    json request = {
        {"asset", code.toLower()},
        {"amount", std::to_string(amount)},
        {"address", address}
    };
    
    if (!tag.empty()) {
        request["memo"] = tag;
    }
    
    return requestAsync("user/crypto_withdrawal", "private", "POST", request);
}

} // namespace ccxt
