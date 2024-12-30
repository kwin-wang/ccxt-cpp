#include "bitfinex.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

namespace ccxt {

Bitfinex::Bitfinex() {
    id = "bitfinex";
    name = "Bitfinex";
    version = "v2";
    isV1 = false;
    isV2 = true;
    rateLimit = 1500;

    // Initialize API endpoints
    baseUrl = "https://api.bitfinex.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/27766244-e328a50c-5ed2-11e7-947b-041416579bb3.jpg"},
        {"api", {
            {"public", "https://api.bitfinex.com"},
            {"private", "https://api.bitfinex.com"}
        }},
        {"www", "https://www.bitfinex.com"},
        {"doc", {
            "https://docs.bitfinex.com/v2/docs/",
            "https://github.com/bitfinexcom/bitfinex-api-node"
        }},
        {"fees", "https://www.bitfinex.com/fees"}
    };

    timeframes = {
        {"1m", "1m"},
        {"5m", "5m"},
        {"15m", "15m"},
        {"30m", "30m"},
        {"1h", "1h"},
        {"3h", "3h"},
        {"6h", "6h"},
        {"12h", "12h"},
        {"1d", "1D"},
        {"1w", "7D"},
        {"2w", "14D"},
        {"1M", "1M"}
    };

    initializeApiEndpoints();
}

void Bitfinex::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "v2/platform/status",
                "v2/tickers",
                "v2/ticker/{symbol}",
                "v2/trades/{symbol}/hist",
                "v2/book/{symbol}/{precision}",
                "v2/candles/trade:{timeframe}:{symbol}/{section}",
                "v2/conf/pub:map:currency:label",
                "v2/conf/pub:list:currency",
                "v2/conf/pub:info:pair",
                "v2/conf/pub:info:pair:futures",
                "v2/conf/pub:info:tx:status",
                "v2/status/{type}"
            }}
        }},
        {"private", {
            {"POST", {
                "v2/auth/r/orders/{symbol}/new",
                "v2/auth/r/orders/{symbol}/cancel",
                "v2/auth/r/orders/{symbol}/cancel/all",
                "v2/auth/r/orders/{symbol}",
                "v2/auth/r/orders/{symbol}/hist",
                "v2/auth/r/trades/{symbol}/hist",
                "v2/auth/r/positions",
                "v2/auth/r/positions/hist",
                "v2/auth/r/funding/offers/{symbol}",
                "v2/auth/r/funding/offers/{symbol}/hist",
                "v2/auth/r/funding/loans/{symbol}",
                "v2/auth/r/funding/loans/{symbol}/hist",
                "v2/auth/r/funding/credits/{symbol}",
                "v2/auth/r/funding/credits/{symbol}/hist",
                "v2/auth/r/funding/trades/{symbol}/hist",
                "v2/auth/r/info/margin/{key}",
                "v2/auth/r/info/funding/{key}",
                "v2/auth/r/movements/{symbol}/hist",
                "v2/auth/r/ledgers/{symbol}/hist",
                "v2/auth/r/wallets",
                "v2/auth/w/deposit/address",
                "v2/auth/w/withdraw"
            }}
        }}
    };
}

json Bitfinex::fetchMarkets(const json& params) {
    json response = fetch("/v2/conf/pub:info:pair", "public", "GET", params);
    json markets = json::array();
    
    for (const auto& market : response[0]) {
        String id = market[0];
        String baseId = market[1];
        String quoteId = market[2];
        String base = this->commonCurrencyCode(baseId);
        String quote = this->commonCurrencyCode(quoteId);
        bool active = true;
        
        markets.push_back({
            {"id", id},
            {"symbol", base + "/" + quote},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", active},
            {"type", "spot"},
            {"spot", true},
            {"margin", true},
            {"future", false},
            {"swap", false},
            {"option", false},
            {"precision", {
                {"price", market[3]},
                {"amount", market[4]}
            }},
            {"limits", {
                {"amount", {
                    {"min", market[5]},
                    {"max", market[6]}
                }},
                {"price", {
                    {"min", market[7]},
                    {"max", market[8]}
                }},
                {"cost", {
                    {"min", market[9]},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return markets;
}

json Bitfinex::fetchTicker(const String& symbol, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    String request = {{"symbol", "t" + market.id}};
    
    json response = fetch("/v2/ticker/" + market.id, "public", "GET",
                         this->extend(request, params));
    
    return {
        {"symbol", symbol},
        {"timestamp", this->milliseconds()},
        {"datetime", this->iso8601(this->milliseconds())},
        {"high", response[8]},
        {"low", response[9]},
        {"bid", response[0]},
        {"bidVolume", response[1]},
        {"ask", response[2]},
        {"askVolume", response[3]},
        {"vwap", response[10]},
        {"open", nullptr},
        {"close", response[6]},
        {"last", response[6]},
        {"previousClose", nullptr},
        {"change", response[4]},
        {"percentage", response[5] * 100},
        {"average", nullptr},
        {"baseVolume", response[7]},
        {"quoteVolume", response[7] * response[6]},
        {"info", response}
    };
}

json Bitfinex::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/v2/auth/r/wallets", "private", "POST", params);
    json result = {
        {"info", response},
        {"timestamp", nullptr},
        {"datetime", nullptr}
    };
    
    for (const auto& balance : response) {
        String type = balance[0];
        String currency = this->commonCurrencyCode(balance[1]);
        double total = balance[2];
        double available = type == "exchange" ? balance[4] : balance[2];
        
        if (!result.contains(currency)) {
            result[currency] = {
                {"free", 0.0},
                {"used", 0.0},
                {"total", 0.0}
            };
        }
        
        if (type == "exchange") {
            result[currency]["free"] = available;
            result[currency]["total"] += total;
        } else if (type == "margin") {
            result[currency]["used"] = total;
        }
    }
    
    return result;
}

json Bitfinex::createOrder(const String& symbol, const String& type,
                          const String& side, double amount,
                          double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    String orderType = type;
    if (type == "limit") {
        orderType = "EXCHANGE LIMIT";
    } else if (type == "market") {
        orderType = "EXCHANGE MARKET";
    }
    
    int orderTypes = {
        {"limit", 0},
        {"market", 1}
    };
    
    int orderSides = {
        {"buy", 1},
        {"sell", -1}
    };
    
    String request = {
        {"symbol", "t" + market.id},
        {"type", orderType},
        {"side", orderSides[side]},
        {"amount", this->amountToPrecision(symbol, std::abs(amount))},
        {"flags", 0}
    };
    
    if (type == "limit") {
        request["price"] = this->priceToPrecision(symbol, price);
    }
    
    json response = fetch("/v2/auth/w/order/submit", "private", "POST",
                         this->extend(request, params));
    
    return this->parseOrder(response, market);
}

String Bitfinex::sign(const String& path, const String& api,
                      const String& method, const json& params,
                      const std::map<String, String>& headers,
                      const json& body) {
    String url = this->urls["api"][api] + "/" + this->version + "/" + this->implodeParams(path, params);
    String query = this->omit(params, this->extractParams(path));
    
    if (api == "public") {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->checkRequiredCredentials();
        String nonce = std::to_string(this->nonce());
        String request = "/" + this->version + "/" + path;
        
        body = this->json(this->extend({
            {"request", request},
            {"nonce", nonce}
        }, query));
        
        String signature = this->hmac(body, this->config_.secret, "sha384", "hex");
        
        const_cast<std::map<String, String>&>(headers)["bfx-nonce"] = nonce;
        const_cast<std::map<String, String>&>(headers)["bfx-apikey"] = this->config_.apiKey;
        const_cast<std::map<String, String>&>(headers)["bfx-signature"] = signature;
        const_cast<std::map<String, String>&>(headers)["content-type"] = "application/json";
    }
    
    return url;
}

String Bitfinex::createSignature(const String& path, const String& nonce,
                               const String& body) {
    String message = "/api/" + path + nonce + body;
    
    unsigned char* hmac = nullptr;
    unsigned int hmacLen = 0;
    
    HMAC_CTX* ctx = HMAC_CTX_new();
    HMAC_Init_ex(ctx, secret.c_str(), secret.length(), EVP_sha384(), nullptr);
    HMAC_Update(ctx, (unsigned char*)message.c_str(), message.length());
    HMAC_Final(ctx, hmac, &hmacLen);
    HMAC_CTX_free(ctx);
    
    return this->toHex(hmac, hmacLen);
}

json Bitfinex::parseOrderStatus(const String& status) {
    static const std::map<String, String> statuses = {
        {"ACTIVE", "open"},
        {"PARTIALLY FILLED", "open"},
        {"EXECUTED", "closed"},
        {"CANCELED", "canceled"},
        {"INSUFFICIENT MARGIN", "canceled"},
        {"RSN_DUST", "rejected"},
        {"RSN_PAUSE", "rejected"}
    };
    
    return statuses.contains(status) ? statuses.at(status) : status;
}

json Bitfinex::parseOrder(const json& order, const Market& market) {
    String id = this->safeString(order, 0);
    String symbol = market.symbol;
    String timestamp = this->safeString(order, 4);
    String side = order[6] > 0 ? "buy" : "sell";
    String type = this->safeString(order, 8);
    String status = this->parseOrderStatus(this->safeString(order, 13));
    
    return {
        {"id", id},
        {"clientOrderId", nullptr},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"status", status},
        {"symbol", symbol},
        {"type", type},
        {"side", side},
        {"price", std::abs(this->safeFloat(order, 16))},
        {"amount", std::abs(this->safeFloat(order, 6))},
        {"filled", std::abs(this->safeFloat(order, 7))},
        {"remaining", std::abs(this->safeFloat(order, 6)) - std::abs(this->safeFloat(order, 7))},
        {"cost", nullptr},
        {"trades", nullptr},
        {"fee", nullptr},
        {"info", order}
    };
}

// Market Data API - Async Methods
boost::future<json> Bitfinex::fetchMarketsAsync(const json& params) {
    return boost::async([this, params]() {
        return this->fetchMarkets(params);
    });
}

boost::future<json> Bitfinex::fetchTickerAsync(const String& symbol, const json& params) {
    return boost::async([this, symbol, params]() {
        return this->fetchTicker(symbol, params);
    });
}

boost::future<json> Bitfinex::fetchTickersAsync(const std::vector<String>& symbols, const json& params) {
    return boost::async([this, symbols, params]() {
        return this->fetchTickers(symbols, params);
    });
}

boost::future<json> Bitfinex::fetchOrderBookAsync(const String& symbol, int limit, const json& params) {
    return boost::async([this, symbol, limit, params]() {
        return this->fetchOrderBook(symbol, limit, params);
    });
}

boost::future<json> Bitfinex::fetchTradesAsync(const String& symbol, int since, int limit, const json& params) {
    return boost::async([this, symbol, since, limit, params]() {
        return this->fetchTrades(symbol, since, limit, params);
    });
}

boost::future<json> Bitfinex::fetchOHLCVAsync(const String& symbol, const String& timeframe,
                                           int since, int limit, const json& params) {
    return boost::async([this, symbol, timeframe, since, limit, params]() {
        return this->fetchOHLCV(symbol, timeframe, since, limit, params);
    });
}

// Trading API - Async Methods
boost::future<json> Bitfinex::fetchBalanceAsync(const json& params) {
    return boost::async([this, params]() {
        return this->fetchBalance(params);
    });
}

boost::future<json> Bitfinex::createOrderAsync(const String& symbol, const String& type,
                                           const String& side, double amount,
                                           double price, const json& params) {
    return boost::async([this, symbol, type, side, amount, price, params]() {
        return this->createOrder(symbol, type, side, amount, price, params);
    });
}

boost::future<json> Bitfinex::cancelOrderAsync(const String& id, const String& symbol, const json& params) {
    return boost::async([this, id, symbol, params]() {
        return this->cancelOrder(id, symbol, params);
    });
}

boost::future<json> Bitfinex::fetchOrderAsync(const String& id, const String& symbol, const json& params) {
    return boost::async([this, id, symbol, params]() {
        return this->fetchOrder(id, symbol, params);
    });
}

boost::future<json> Bitfinex::fetchOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    return boost::async([this, symbol, since, limit, params]() {
        return this->fetchOrders(symbol, since, limit, params);
    });
}

boost::future<json> Bitfinex::fetchOpenOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    return boost::async([this, symbol, since, limit, params]() {
        return this->fetchOpenOrders(symbol, since, limit, params);
    });
}

boost::future<json> Bitfinex::fetchClosedOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    return boost::async([this, symbol, since, limit, params]() {
        return this->fetchClosedOrders(symbol, since, limit, params);
    });
}

// Bitfinex specific methods - Async
boost::future<json> Bitfinex::fetchPositionsAsync(const String& symbol, const json& params) {
    return boost::async([this, symbol, params]() {
        return this->fetchPositions(symbol, params);
    });
}

boost::future<json> Bitfinex::fetchMyTradesAsync(const String& symbol, int since, int limit, const json& params) {
    return boost::async([this, symbol, since, limit, params]() {
        return this->fetchMyTrades(symbol, since, limit, params);
    });
}

boost::future<json> Bitfinex::fetchLedgerAsync(const String& code, int since, int limit, const json& params) {
    return boost::async([this, code, since, limit, params]() {
        return this->fetchLedger(code, since, limit, params);
    });
}

boost::future<json> Bitfinex::fetchFundingRatesAsync(const std::vector<String>& symbols, const json& params) {
    return boost::async([this, symbols, params]() {
        return this->fetchFundingRates(symbols, params);
    });
}

boost::future<json> Bitfinex::setLeverageAsync(const String& symbol, double leverage, const json& params) {
    return boost::async([this, symbol, leverage, params]() {
        return this->setLeverage(symbol, leverage, params);
    });
}

boost::future<json> Bitfinex::fetchDepositsAsync(const String& code, int since, int limit, const json& params) {
    return boost::async([this, code, since, limit, params]() {
        return this->fetchDeposits(code, since, limit, params);
    });
}

boost::future<json> Bitfinex::fetchWithdrawalsAsync(const String& code, int since, int limit, const json& params) {
    return boost::async([this, code, since, limit, params]() {
        return this->fetchWithdrawals(code, since, limit, params);
    });
}

} // namespace ccxt
