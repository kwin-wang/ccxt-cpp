#include "ccxt/exchanges/coinbase.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <base64.h>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind/bind.hpp>

namespace ccxt {

Coinbase::Coinbase() {
    id = "coinbase";
    name = "Coinbase Exchange";
    version = "v2";
    rateLimit = 100;
    
    // Initialize API endpoints
    baseUrl = "https://api.exchange.coinbase.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/41764625-63b7ffde-760a-11e8-996d-a6328fa9347a.jpg"},
        {"api", {
            {"public", "https://api.exchange.coinbase.com"},
            {"private", "https://api.exchange.coinbase.com"}
        }},
        {"www", "https://pro.coinbase.com/"},
        {"doc", {
            "https://docs.cloud.coinbase.com/exchange/docs"
        }},
        {"fees", "https://pro.coinbase.com/fees"}
    };

    initializeApiEndpoints();
}

void Coinbase::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "/products",
                "/products/{id}/ticker",
                "/products/{id}/trades",
                "/products/{id}/book",
                "/products/{id}/candles",
                "/currencies",
                "/time"
            }}
        }},
        {"private", {
            {"GET", {
                "/accounts",
                "/orders",
                "/orders/{id}",
                "/fills"
            }},
            {"POST", {
                "/orders"
            }},
            {"DELETE", {
                "/orders",
                "/orders/{id}"
            }}
        }}
    };
}

void Coinbase::describe() {
    this->set("id", "coinbase");
    this->set("name", "Coinbase");
    
    // Add version and region options
    this->set("options", {
        {"version", "advanced"}, // basic, advanced
        {"region", "US"}, // US, international
        {"apiKey", nullptr},
        {"secret", nullptr}
    });

    // Add endpoints for all versions and regions
    this->set("urls", {
        {"logo", "https://user-images.githubusercontent.com/1294454/40811661-b6eceae2-653a-11e8-829e-10bfadb078cf.jpg"},
        {"api", {
            {"basic", {
                {"US", "https://api.coinbase.com"},
                {"international", "https://api.coinbase.com"}
            }},
            {"advanced", {
                {"US", "https://api.exchange.coinbase.com"},
                {"international", "https://api.exchange.coinbase.com"}
            }}
        }},
        {"www", "https://www.coinbase.com"},
        {"doc", {
            "https://docs.cloud.coinbase.com",
            "https://docs.cloud.coinbase.com/advanced-trade-api",
            "https://docs.cloud.coinbase.com/sign-in-with-coinbase"
        }}
    });
}

std::pair<std::string, std::string> Coinbase::getVersionAndRegion() {
    auto version = this->safeString(this->options, "version", "advanced");
    auto region = this->safeString(this->options, "region", "US");
    return {version, region};
}

std::string Coinbase::getEndpoint(const std::string& path) {
    auto [version, region] = this->getVersionAndRegion();
    auto urls = this->urls["api"];
    
    return urls[version][region] + path;
}

json Coinbase::fetchMarkets(const json& params) {
    json response = fetch("/products", "public", "GET", params);
    json markets = json::array();
    
    for (const auto& market : response) {
        markets.push_back({
            {"id", market["id"]},
            {"symbol", market["base_currency"] + "/" + market["quote_currency"]},
            {"base", market["base_currency"]},
            {"quote", market["quote_currency"]},
            {"baseId", market["base_currency"]},
            {"quoteId", market["quote_currency"]},
            {"active", market["status"] == "online"},
            {"precision", {
                {"amount", market["base_increment"].get<std::string>().find('1') - 1},
                {"price", market["quote_increment"].get<std::string>().find('1') - 1}
            }},
            {"limits", {
                {"amount", {
                    {"min", std::stod(market["base_min_size"].get<std::string>())},
                    {"max", std::stod(market["base_max_size"].get<std::string>())}
                }},
                {"price", {
                    {"min", std::stod(market["quote_increment"].get<std::string>())}
                }},
                {"cost", {
                    {"min", std::stod(market["min_market_funds"].get<std::string>())},
                    {"max", std::stod(market["max_market_funds"].get<std::string>())}
                }}
            }},
            {"info", market}
        });
    }
    
    return markets;
}

json Coinbase::fetchBalance(const json& params) {
    json response = fetch("/accounts", "private", "GET", params);
    json result = {
        {"info", response},
        {"timestamp", nullptr},
        {"datetime", nullptr}
    };
    
    for (const auto& balance : response) {
        std::string currency = balance["currency"];
        double available = std::stod(balance["available"].get<std::string>());
        double hold = std::stod(balance["hold"].get<std::string>());
        double total = available + hold;
        
        if (total > 0) {
            result[currency] = {
                {"free", available},
                {"used", hold},
                {"total", total}
            };
        }
    }
    
    return result;
}

json Coinbase::createOrder(const std::string& symbol, const std::string& type,
                         const std::string& side, double amount,
                         double price, const json& params) {
    Market market = this->market(symbol);
    
    json order = {
        {"product_id", market.id},
        {"side", side},
        {"type", type},
        {"size", std::to_string(amount)}
    };
    
    if (type == "limit") {
        if (price == 0) {
            throw InvalidOrder("For limit orders, price cannot be zero");
        }
        order["price"] = std::to_string(price);
    }
    
    return fetch("/orders", "private", "POST", order);
}

std::string Coinbase::sign(const std::string& path, const std::string& api,
                     const std::string& method, const json& params,
                     const std::map<std::string, std::string>& headers,
                     const json& body) {
    std::string url = getEndpoint(path);
    
    if (api == "private") {
        std::string timestamp = getTimestamp();
        std::string bodyStr = body.empty() ? "" : body.dump();
        auto authHeaders = getAuthHeaders(method, path, bodyStr);
        
        for (const auto& [key, value] : authHeaders) {
            const_cast<std::map<std::string, std::string>&>(headers)[key] = value;
        }
    }
    
    if (!params.empty()) {
        std::stringstream querystd::string;
        bool first = true;
        for (const auto& [key, value] : params.items()) {
            if (!first) querystd::string << "&";
            querystd::string << key << "=" << value.get<std::string>();
            first = false;
        }
        url += "?" + querystd::string.str();
    }
    
    return url;
}

std::string Coinbase::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    return std::to_string(std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());
}

std::string Coinbase::createSignature(const std::string& timestamp, const std::string& method,
                               const std::string& requestPath, const std::string& body) {
    std::string message = timestamp + method + requestPath + body;
    
    unsigned char* hmac = nullptr;
    unsigned int hmacLen = 0;
    
    HMAC_CTX* ctx = HMAC_CTX_new();
    HMAC_Init_ex(ctx, base64_decode(secret).c_str(), base64_decode(secret).length(), EVP_sha256(), nullptr);
    HMAC_Update(ctx, (unsigned char*)message.c_str(), message.length());
    HMAC_Final(ctx, hmac, &hmacLen);
    HMAC_CTX_free(ctx);
    
    return base64_encode(hmac, hmacLen);
}

std::map<std::string, std::string> Coinbase::getAuthHeaders(const std::string& method,
                                                const std::string& requestPath,
                                                const std::string& body) {
    std::string timestamp = getTimestamp();
    std::string signature = createSignature(timestamp, method, requestPath, body);
    
    return {
        {"CB-ACCESS-KEY", apiKey},
        {"CB-ACCESS-SIGN", signature},
        {"CB-ACCESS-TIMESTAMP", timestamp},
        {"CB-ACCESS-PASSPHRASE", password}
    };
}

// Async Market Data API Implementation
boost::future<json> Coinbase::fetchMarketsAsync(const json& params) const {
    return boost::async(boost::launch::async, [this, params]() {
        return this->fetchMarkets(params);
    });
}

boost::future<json> Coinbase::fetchTickerAsync(const std::string& symbol, const json& params) const {
    return boost::async(boost::launch::async, [this, symbol, params]() {
        return this->fetchTicker(symbol, params);
    });
}

boost::future<json> Coinbase::fetchTickersAsync(const std::vector<std::string>& symbols, const json& params) const {
    return boost::async(boost::launch::async, [this, symbols, params]() {
        return this->fetchTickers(symbols, params);
    });
}

boost::future<json> Coinbase::fetchOrderBookAsync(const std::string& symbol, int limit, const json& params) const {
    return boost::async(boost::launch::async, [this, symbol, limit, params]() {
        return this->fetchOrderBook(symbol, limit, params);
    });
}

boost::future<json> Coinbase::fetchTradesAsync(const std::string& symbol, int since, int limit, const json& params) const {
    return boost::async(boost::launch::async, [this, symbol, since, limit, params]() {
        return this->fetchTrades(symbol, since, limit, params);
    });
}

boost::future<json> Coinbase::fetchOHLCVAsync(const std::string& symbol, const std::string& timeframe,
                                           int since, int limit, const json& params) const {
    return boost::async(boost::launch::async, [this, symbol, timeframe, since, limit, params]() {
        return this->fetchOHLCV(symbol, timeframe, since, limit, params);
    });
}

// Async Trading API Implementation
boost::future<json> Coinbase::fetchBalanceAsync(const json& params) const {
    return boost::async(boost::launch::async, [this, params]() {
        return this->fetchBalance(params);
    });
}

boost::future<json> Coinbase::createOrderAsync(const std::string& symbol, const std::string& type,
                                           const std::string& side, double amount,
                                           double price, const json& params) {
    return boost::async(boost::launch::async, [this, symbol, type, side, amount, price, params]() {
        return this->createOrder(symbol, type, side, amount, price, params);
    });
}

boost::future<json> Coinbase::cancelOrderAsync(const std::string& id, const std::string& symbol, const json& params) {
    return boost::async(boost::launch::async, [this, id, symbol, params]() {
        return this->cancelOrder(id, symbol, params);
    });
}

boost::future<json> Coinbase::fetchOrderAsync(const std::string& id, const std::string& symbol, const json& params) const {
    return boost::async(boost::launch::async, [this, id, symbol, params]() {
        return this->fetchOrder(id, symbol, params);
    });
}

boost::future<json> Coinbase::fetchOrdersAsync(const std::string& symbol, int since, int limit, const json& params) const {
    return boost::async(boost::launch::async, [this, symbol, since, limit, params]() {
        return this->fetchOrders(symbol, since, limit, params);
    });
}

boost::future<json> Coinbase::fetchOpenOrdersAsync(const std::string& symbol, int since, int limit, const json& params) const {
    return boost::async(boost::launch::async, [this, symbol, since, limit, params]() {
        return this->fetchOpenOrders(symbol, since, limit, params);
    });
}

boost::future<json> Coinbase::fetchClosedOrdersAsync(const std::string& symbol, int since, int limit, const json& params) const {
    return boost::async(boost::launch::async, [this, symbol, since, limit, params]() {
        return this->fetchClosedOrders(symbol, since, limit, params);
    });
}

} // namespace ccxt
