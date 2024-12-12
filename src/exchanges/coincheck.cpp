#include "coincheck.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>
#include <boost/thread/future.hpp>

namespace ccxt {

Coincheck::Coincheck() {
    id = "coincheck";
    name = "Coincheck";
    version = "v1";
    rateLimit = 1000;
    certified = false;
    pro = false;

    // Initialize API endpoints
    baseUrl = "https://coincheck.com";
    
    urls = {
        {"logo", "https://user-images.githubusercontent.com/51840849/87182088-1d6d6380-c2ec-11ea-9c64-8ab9f9b289f5.jpg"},
        {"api", {
            {"public", "https://coincheck.com/api"},
            {"private", "https://coincheck.com/api"}
        }},
        {"www", "https://coincheck.com"},
        {"doc", {
            "https://coincheck.com/documents/exchange/api",
            "https://coincheck.com/ja/documents/exchange/api"
        }},
        {"fees", "https://coincheck.com/exchange/fee"}
    };

    timeframes = {
        {"1m", "1"},
        {"5m", "5"},
        {"15m", "15"},
        {"30m", "30"},
        {"1h", "60"},
        {"4h", "240"},
        {"6h", "360"},
        {"12h", "720"},
        {"1d", "1440"},
        {"1w", "10080"}
    };

    options = {
        {"adjustForTimeDifference", true},
        {"defaultMarket", "btc_jpy"}
    };

    errorCodes = {
        {1, "Invalid authentication"},
        {2, "Invalid API key"},
        {3, "Invalid nonce"},
        {4, "Invalid signature"},
        {5, "Rate limit exceeded"},
        {6, "Invalid order type"},
        {7, "Invalid amount"},
        {8, "Invalid price"},
        {9, "Insufficient funds"},
        {10, "Order not found"},
        {11, "Market not found"},
        {12, "Trading temporarily unavailable"},
        {13, "Invalid address"},
        {14, "Invalid currency"},
        {15, "Invalid withdrawal amount"},
        {16, "Address not found"},
        {17, "Withdrawal temporarily unavailable"}
    };

    initializeApiEndpoints();
}

void Coincheck::initializeApiEndpoints() {
    api = {
        {"public", {
            {"GET", {
                "exchange/orders/rate",
                "order_books",
                "trades",
                "ticker",
                "exchange/orders/opens"
            }}
        }},
        {"private", {
            {"GET", {
                "accounts",
                "accounts/balance",
                "accounts/leverage_balance",
                "exchange/orders/opens",
                "exchange/orders/transactions",
                "exchange/orders/transactions_pagination",
                "exchange/leverage/positions",
                "bank_accounts",
                "deposit_money",
                "withdraw_history"
            }},
            {"POST", {
                "exchange/orders",
                "exchange/leverage/positions",
                "bank_accounts",
                "withdraw_money"
            }},
            {"DELETE", {
                "exchange/orders/{id}",
                "bank_accounts/{id}"
            }}
        }}
    };
}

json Coincheck::fetchMarkets(const json& params) {
    json response = fetch("/ticker", "public", "GET", params);
    json result = json::array();
    
    // Coincheck primarily focuses on BTC/JPY pair
    result.push_back({
        {"id", "btc_jpy"},
        {"symbol", "BTC/JPY"},
        {"base", "BTC"},
        {"quote", "JPY"},
        {"baseId", "btc"},
        {"quoteId", "jpy"},
        {"active", true},
        {"type", "spot"},
        {"spot", true},
        {"future", false},
        {"option", false},
        {"contract", false},
        {"precision", {
            {"amount", 8},
            {"price", 0}
        }},
        {"limits", {
            {"amount", {
                {"min", 0.005},
                {"max", nullptr}
            }},
            {"price", {
                {"min", 1},
                {"max", nullptr}
            }},
            {"cost", {
                {"min", nullptr},
                {"max", nullptr}
            }}
        }},
        {"info", response}
    });
    
    return result;
}

json Coincheck::fetchBalance(const json& params) {
    this->loadMarkets();
    json response = fetch("/accounts/balance", "private", "GET", params);
    return parseBalance(response);
}

json Coincheck::parseBalance(const json& response) {
    json result = {{"info", response}};
    
    std::map<String, String> currencyIds = {
        {"jpy", "JPY"},
        {"btc", "BTC"},
        {"etc", "ETC"},
        {"fct", "FCT"},
        {"mona", "MONA"},
        {"plt", "PLT"}
    };
    
    for (const auto& [currencyId, code] : currencyIds) {
        String available = currencyId + "_available";
        String reserved = currencyId + "_reserved";
        
        if (response.contains(available) && response.contains(reserved)) {
            result[code] = {
                {"free", this->safeFloat(response, available)},
                {"used", this->safeFloat(response, reserved)},
                {"total", nullptr}
            };
            result[code]["total"] = this->sum(result[code]["free"], result[code]["used"]);
        }
    }
    
    return result;
}

json Coincheck::createOrder(const String& symbol, const String& type,
                          const String& side, double amount,
                          double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"pair", market["id"]},
        {"order_type", side + "_" + type},
        {"amount", this->amountToPrecision(symbol, amount)}
    };
    
    if (type == "limit") {
        request["rate"] = this->priceToPrecision(symbol, price);
    }
    
    json response = fetch("/exchange/orders", "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response, market);
}

String Coincheck::sign(const String& path, const String& api,
                      const String& method, const json& params,
                      const std::map<String, String>& headers,
                      const json& body) {
    String url = this->urls["api"][api] + path;
    String nonce = this->createNonce();
    
    if (api == "public") {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    } else {
        this->checkRequiredCredentials();
        
        String queryString = "";
        if (method == "GET") {
            if (!params.empty()) {
                queryString = this->urlencode(params);
                url += "?" + queryString;
            }
        } else {
            if (!params.empty()) {
                body = params;
                queryString = this->json(params);
            }
        }
        
        String auth = nonce + url + (queryString.empty() ? "" : queryString);
        String signature = this->hmac(auth, this->encode(this->secret),
                                    "sha256", "hex");
        
        const_cast<std::map<String, String>&>(headers)["ACCESS-KEY"] = this->apiKey;
        const_cast<std::map<String, String>&>(headers)["ACCESS-NONCE"] = nonce;
        const_cast<std::map<String, String>&>(headers)["ACCESS-SIGNATURE"] = signature;
        
        if (method != "GET") {
            const_cast<std::map<String, String>&>(headers)["Content-Type"] = "application/json";
        }
    }
    
    return url;
}

String Coincheck::createNonce() {
    return std::to_string(this->milliseconds());
}

json Coincheck::parseOrder(const json& order, const Market& market) {
    String timestamp = this->safeString(order, "created_at");
    String id = this->safeString(order, "id");
    String type = this->safeString(order, "order_type");
    String side = nullptr;
    String price = nullptr;
    
    if (type != nullptr) {
        std::vector<String> parts = this->split(type, "_");
        side = parts[0];
        type = parts[1];
    }
    
    return {
        {"id", id},
        {"clientOrderId", nullptr},
        {"timestamp", this->parse8601(timestamp)},
        {"datetime", timestamp},
        {"lastTradeTimestamp", nullptr},
        {"status", "open"},
        {"symbol", market["symbol"]},
        {"type", type},
        {"side", side},
        {"price", this->safeFloat(order, "rate")},
        {"amount", this->safeFloat(order, "amount")},
        {"filled", this->safeFloat(order, "executed_amount")},
        {"remaining", this->safeFloat(order, "remaining_amount")},
        {"trades", nullptr},
        {"fee", nullptr},
        {"info", order}
    };
}

// Async Market Data API
boost::future<json> Coincheck::fetchMarketsAsync(const json& params) {
    return boost::async(boost::launch::async, [this, params]() {
        return this->fetchMarkets(params);
    });
}

boost::future<json> Coincheck::fetchTickerAsync(const String& symbol, const json& params) {
    return boost::async(boost::launch::async, [this, symbol, params]() {
        return this->fetchTicker(symbol, params);
    });
}

boost::future<json> Coincheck::fetchTickersAsync(const std::vector<String>& symbols, const json& params) {
    return boost::async(boost::launch::async, [this, symbols, params]() {
        return this->fetchTickers(symbols, params);
    });
}

boost::future<json> Coincheck::fetchOrderBookAsync(const String& symbol, int limit, const json& params) {
    return boost::async(boost::launch::async, [this, symbol, limit, params]() {
        return this->fetchOrderBook(symbol, limit, params);
    });
}

boost::future<json> Coincheck::fetchTradesAsync(const String& symbol, int since, int limit, const json& params) {
    return boost::async(boost::launch::async, [this, symbol, since, limit, params]() {
        return this->fetchTrades(symbol, since, limit, params);
    });
}

boost::future<json> Coincheck::fetchOHLCVAsync(const String& symbol, const String& timeframe,
                                             int since, int limit, const json& params) {
    return boost::async(boost::launch::async, [this, symbol, timeframe, since, limit, params]() {
        return this->fetchOHLCV(symbol, timeframe, since, limit, params);
    });
}

// Async Trading API
boost::future<json> Coincheck::fetchBalanceAsync(const json& params) {
    return boost::async(boost::launch::async, [this, params]() {
        return this->fetchBalance(params);
    });
}

boost::future<json> Coincheck::createOrderAsync(const String& symbol, const String& type, const String& side,
                                              double amount, double price, const json& params) {
    return boost::async(boost::launch::async, [this, symbol, type, side, amount, price, params]() {
        return this->createOrder(symbol, type, side, amount, price, params);
    });
}

boost::future<json> Coincheck::cancelOrderAsync(const String& id, const String& symbol, const json& params) {
    return boost::async(boost::launch::async, [this, id, symbol, params]() {
        return this->cancelOrder(id, symbol, params);
    });
}

boost::future<json> Coincheck::fetchOrderAsync(const String& id, const String& symbol, const json& params) {
    return boost::async(boost::launch::async, [this, id, symbol, params]() {
        return this->fetchOrder(id, symbol, params);
    });
}

boost::future<json> Coincheck::fetchOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    return boost::async(boost::launch::async, [this, symbol, since, limit, params]() {
        return this->fetchOrders(symbol, since, limit, params);
    });
}

boost::future<json> Coincheck::fetchOpenOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    return boost::async(boost::launch::async, [this, symbol, since, limit, params]() {
        return this->fetchOpenOrders(symbol, since, limit, params);
    });
}

boost::future<json> Coincheck::fetchClosedOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    return boost::async(boost::launch::async, [this, symbol, since, limit, params]() {
        return this->fetchClosedOrders(symbol, since, limit, params);
    });
}

// Async Account API
boost::future<json> Coincheck::fetchMyTradesAsync(const String& symbol, int since, int limit, const json& params) {
    return boost::async(boost::launch::async, [this, symbol, since, limit, params]() {
        return this->fetchMyTrades(symbol, since, limit, params);
    });
}

boost::future<json> Coincheck::fetchDepositsAsync(const String& code, int since, int limit, const json& params) {
    return boost::async(boost::launch::async, [this, code, since, limit, params]() {
        return this->fetchDeposits(code, since, limit, params);
    });
}

boost::future<json> Coincheck::fetchWithdrawalsAsync(const String& code, int since, int limit, const json& params) {
    return boost::async(boost::launch::async, [this, code, since, limit, params]() {
        return this->fetchWithdrawals(code, since, limit, params);
    });
}

boost::future<json> Coincheck::fetchDepositAddressAsync(const String& code, const json& params) {
    return boost::async(boost::launch::async, [this, code, params]() {
        return this->fetchDepositAddress(code, params);
    });
}

boost::future<json> Coincheck::withdrawAsync(const String& code, double amount, const String& address, const String& tag, const json& params) {
    return boost::async(boost::launch::async, [this, code, amount, address, tag, params]() {
        return this->withdraw(code, amount, address, tag, params);
    });
}

// Async Leverage Trading API
boost::future<json> Coincheck::fetchLeverageBalanceAsync(const json& params) {
    return boost::async(boost::launch::async, [this, params]() {
        return this->fetchLeverageBalance(params);
    });
}

boost::future<json> Coincheck::createLeverageOrderAsync(const String& symbol, const String& type, const String& side,
                                                      double amount, double price, const json& params) {
    return boost::async(boost::launch::async, [this, symbol, type, side, amount, price, params]() {
        return this->createLeverageOrder(symbol, type, side, amount, price, params);
    });
}

boost::future<json> Coincheck::cancelLeverageOrderAsync(const String& id, const String& symbol, const json& params) {
    return boost::async(boost::launch::async, [this, id, symbol, params]() {
        return this->cancelLeverageOrder(id, symbol, params);
    });
}

boost::future<json> Coincheck::fetchLeveragePositionsAsync(const json& params) {
    return boost::async(boost::launch::async, [this, params]() {
        return this->fetchLeveragePositions(params);
    });
}

boost::future<json> Coincheck::fetchLeverageOrdersAsync(const String& symbol, int since, int limit, const json& params) {
    return boost::async(boost::launch::async, [this, symbol, since, limit, params]() {
        return this->fetchLeverageOrders(symbol, since, limit, params);
    });
}

} // namespace ccxt
