#include "ccxt/exchanges/bit2c.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>
#include <boost/thread/future.hpp>

namespace ccxt {

Bit2c::Bit2c() {
    id = "bit2c";
    name = "Bit2C";
    countries = {"IL"}; // Israel
    rateLimit = 3000;
    pro = false;
    has = {
        {"CORS", nullptr},
        {"spot", true},
        {"margin", false},
        {"swap", false},
        {"future", false},
        {"option", false},
        {"addMargin", false},
        {"cancelAllOrders", false},
        {"cancelOrder", true},
        {"closeAllPositions", false},
        {"closePosition", false},
        {"createOrder", true},
        {"createReduceOnlyOrder", false},
        {"fetchBalance", true},
        {"fetchDepositAddress", true},
        {"fetchDepositAddresses", false},
        {"fetchDepositAddressesByNetwork", false},
        {"fetchMyTrades", true},
        {"fetchOpenOrders", true},
        {"fetchOrder", true},
        {"fetchOrderBook", true},
        {"fetchPosition", false},
        {"fetchPositionMode", false},
        {"fetchPositions", false},
        {"fetchTicker", true},
        {"fetchTrades", true},
        {"fetchTradingFee", false},
        {"fetchTradingFees", true},
        {"transfer", false},
        {"ws", false}
    };

    urls = {
        {"logo", "https://github.com/user-attachments/assets/db0bce50-6842-4c09-a1d5-0c87d22118aa"},
        {"api", {
            {"rest", "https://bit2c.co.il"}
        }},
        {"www", "https://www.bit2c.co.il"},
        {"referral", "https://bit2c.co.il/Aff/63bfed10-e359-420c-ab5a-ad368dab0baf"},
        {"doc", {
            "https://www.bit2c.co.il/home/api",
            "https://github.com/OferE/bit2c"
        }}
    };

    requiredCredentials = {
        {"apiKey", true},
        {"secret", true}
    };

    initializeApiEndpoints();
}

void Bit2c::initializeApiEndpoints() {
    api = {
        {"public", {
            {"get", {
                "Exchanges/{pair}/Ticker",
                "Exchanges/{pair}/orderbook",
                "Exchanges/{pair}/trades",
                "Exchanges/pairs"
            }}
        }},
        {"private", {
            {"post", {
                "Account/Balance",
                "Account/Balance/v2",
                "Merchant/CreateCheckout",
                "Order/AccountHistory",
                "Order/AddCoinFundsRequest",
                "Order/AddFund",
                "Order/AddOrder",
                "Order/AddOrderMarketPriceBuy",
                "Order/AddOrderMarketPriceSell",
                "Order/CancelOrder",
                "Order/MyOrders",
                "Payment/GetMyId",
                "Payment/Send",
                "Order/AddOrderMarketPrice"
            }}
        }}
    };
}

nlohmann::json Bit2c::fetchMarkets(const nlohmann::json& params) {
    nlohmann::json response = request("Exchanges/pairs", "public", "GET", params);
    nlohmann::json result = nlohmann::json::array();
    for (const auto& market : response.items()) {
        std::string id = market.key();
        result.push_back({
            {"id", id},
            {"symbol", getCommonSymbol(id)},
            {"base", market.value()["base"].get<std::string>()},
            {"quote", market.value()["quote"].get<std::string>()},
            {"baseId", market.value()["base"].get<std::string>()},
            {"quoteId", market.value()["quote"].get<std::string>()},
            {"active", true},
            {"type", "spot"},
            {"spot", true},
            {"margin", false},
            {"swap", false},
            {"future", false}
        });
    }
    return result;
}

nlohmann::json Bit2c::fetchTicker(const std::string& symbol, const nlohmann::json& params) {
    loadMarkets();
    Market market = getMarket(symbol);
    nlohmann::json response = request("Exchanges/" + market.id + "/Ticker", "public", "GET", params);
    return parseTicker(response, market);
}

nlohmann::json Bit2c::fetchOrderBook(const std::string& symbol, int limit, const nlohmann::json& params) {
    loadMarkets();
    Market market = getMarket(symbol);
    nlohmann::json response = request("Exchanges/" + market.id + "/orderbook", "public", "GET", params);
    return parseOrderBook(response, symbol);
}

nlohmann::json Bit2c::fetchTrades(const std::string& symbol, int since, int limit, const nlohmann::json& params) {
    loadMarkets();
    Market market = getMarket(symbol);
    nlohmann::json response = request("Exchanges/" + market.id + "/trades", "public", "GET", params);
    return parseTrades(response, market, since, limit);
}

nlohmann::json Bit2c::fetchTradingFees(const nlohmann::json& params) {
    return {
        {"maker", 0.005},
        {"taker", 0.005}
    };
}

nlohmann::json Bit2c::fetchBalance(const nlohmann::json& params) {
    loadMarkets();
    nlohmann::json response = request("Account/Balance", "private", "POST", params);
    return parseBalance(response);
}

nlohmann::json Bit2c::createOrder(const std::string& symbol, const std::string& type, const std::string& side,
                      double amount, double price, const nlohmann::json& params) {
    loadMarkets();
    Market market = getMarket(symbol);
    std::string method = "Order/AddOrder";
    if (type == "market") {
        method = side == "buy" ? "Order/AddOrderMarketPriceBuy" : "Order/AddOrderMarketPriceSell";
    }
    nlohmann::json request = {
        {"Amount", amount},
        {"Pair", market.id}
    };
    if (type == "limit") {
        request["Price"] = price;
        request["Total"] = amount * price;
    }
    nlohmann::json response = request(method, "private", "POST", extend(request, params));
    return parseOrder(response, market);
}

nlohmann::json Bit2c::cancelOrder(const std::string& id, const std::string& symbol, const nlohmann::json& params) {
    if (symbol.empty()) {
        throw std::runtime_error("cancelOrder requires a symbol argument");
    }
    loadMarkets();
    Market market = getMarket(symbol);
    nlohmann::json request = {
        {"id", id},
        {"Pair", market.id}
    };
    return request("Order/CancelOrder", "private", "POST", extend(request, params));
}

std::string Bit2c::createSignature(const std::string& timestamp, const std::string& method,
                           const std::string& path, const std::string& queryString) {
    std::string signString = timestamp + method + path;
    if (!queryString.empty()) {
        signString += "?" + queryString;
    }
    return hmac(signString, secret, "sha512", "hex");
}

// Async Market Data API
boost::future<nlohmann::json> Bit2c::fetchMarketsAsync(const nlohmann::json& params) {
    return requestAsync("Exchanges/pairs", "public", "GET", params);
}

boost::future<nlohmann::json> Bit2c::fetchTickerAsync(const std::string& symbol, const nlohmann::json& params) {
    std::string market = getBit2cSymbol(symbol);
    std::string path = "Exchanges/" + market + "/Ticker";
    return requestAsync(path, "public", "GET", params);
}

boost::future<nlohmann::json> Bit2c::fetchTickersAsync(const std::vector<std::string>& symbols, const nlohmann::json& params) {
    return boost::async(boost::launch::async, [this, symbols, params]() {
        nlohmann::json result = nlohmann::json::object();
        for (const auto& symbol : symbols) {
            result[symbol] = this->fetchTicker(symbol, params);
        }
        return result;
    });
}

boost::future<nlohmann::json> Bit2c::fetchOrderBookAsync(const std::string& symbol, int limit, const nlohmann::json& params) {
    std::string market = getBit2cSymbol(symbol);
    std::string path = "Exchanges/" + market + "/orderbook";
    return requestAsync(path, "public", "GET", params);
}

boost::future<nlohmann::json> Bit2c::fetchTradesAsync(const std::string& symbol, int since, int limit, const nlohmann::json& params) {
    std::string market = getBit2cSymbol(symbol);
    std::string path = "Exchanges/" + market + "/trades";
    return requestAsync(path, "public", "GET", params);
}

boost::future<nlohmann::json> Bit2c::fetchTradingFeesAsync(const nlohmann::json& params) {
    return requestAsync("Account/Balance", "private", "GET", params);
}

// Async Trading API
boost::future<nlohmann::json> Bit2c::fetchBalanceAsync(const nlohmann::json& params) {
    return requestAsync("Account/Balance", "private", "GET", params);
}

boost::future<nlohmann::json> Bit2c::createOrderAsync(const std::string& symbol, const std::string& type, const std::string& side,
                                                   double amount, double price, const nlohmann::json& params) {
    nlohmann::json request = {
        {"Amount", amount},
        {"Pair", getBit2cSymbol(symbol)},
        {"Total", amount * price},
        {"IsBid", side == "buy"}
    };
    
    std::string path = (type == "market") ? 
        (side == "buy" ? "Order/AddOrderMarketPriceBuy" : "Order/AddOrderMarketPriceSell") :
        "Order/AddOrder";
        
    return requestAsync(path, "private", "POST", request);
}

boost::future<nlohmann::json> Bit2c::cancelOrderAsync(const std::string& id, const std::string& symbol, const nlohmann::json& params) {
    nlohmann::json request = {{"id", id}};
    return requestAsync("Order/CancelOrder", "private", "POST", request);
}

boost::future<nlohmann::json> Bit2c::fetchOrderAsync(const std::string& id, const std::string& symbol, const nlohmann::json& params) {
    nlohmann::json request = {{"id", id}};
    return requestAsync("Order/GetById", "private", "GET", request);
}

boost::future<nlohmann::json> Bit2c::fetchOpenOrdersAsync(const std::string& symbol, int since, int limit, const nlohmann::json& params) {
    return requestAsync("Order/MyOrders", "private", "GET", params);
}

boost::future<nlohmann::json> Bit2c::fetchMyTradesAsync(const std::string& symbol, int since, int limit, const nlohmann::json& params) {
    return requestAsync("Order/OrderHistory", "private", "GET", params);
}

// Async Account API
boost::future<nlohmann::json> Bit2c::fetchDepositAddressAsync(const std::string& code, const nlohmann::json& params) {
    return requestAsync("Funds/AddCoinFundsRequest", "private", "POST", params);
}

} // namespace ccxt
