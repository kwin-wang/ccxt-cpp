#include "ccxt/exchanges/cex.h"
#include "../base/json_helper.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <openssl/hmac.h>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace ccxt {

const std::string cex::defaultBaseURL = "https://cex.io/api";
const std::string cex::defaultVersion = "v1";
const int cex::defaultRateLimit = 1000;
const bool cex::defaultPro = false;

ExchangeRegistry::Factory cex::factory(cex::createInstance);

cex::cex(const Config& config) : ExchangeImpl(config) {
    init();
}

void cex::init() {
    name = "CEX";
    id = "cex";
    version = defaultVersion;
    rateLimit = defaultRateLimit;
    pro = defaultPro;
    baseURL = defaultBaseURL;

    // Initialize URLs
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/27766442-8ddc33b0-5ed8-11e7-8b98-f786aef0f3c9.jpg"},
        {"api", {
            {"public", "https://cex.io/api"},
            {"private", "https://cex.io/api"}
        }},
        {"www", "https://cex.io"},
        {"doc", {
            "https://cex.io/cex-api",
            "https://cex.io/websocket-api"
        }},
        {"fees", "https://cex.io/fee-schedule"}
    };

    // Initialize API endpoints
    api = {
        {"public", {
            {"get", {
                "currency_profile",
                "currency_limits",
                "ticker/{symbol}",
                "tickers/{symbols}",
                "order_book/{symbol}",
                "trade_history/{symbol}",
                "ohlcv/hd/{yyyymmdd}/{symbol}",
                "last_price/{symbol}",
                "last_prices/{symbols}",
                "convert/{pair}",
                "price_stats/{symbol}"
            }}
        }},
        {"private", {
            {"post", {
                "place_order/{symbol}",
                "cancel_order",
                "cancel_orders/{symbol}",
                "open_orders/{symbol}",
                "open_orders",
                "active_orders_status",
                "archived_orders/{symbol}",
                "get_order",
                "get_order_tx",
                "get_address",
                "get_myfee",
                "balance/",
                "open_position/{symbol}",
                "close_position/{symbol}",
                "get_position",
                "get_positions",
                "get_marginal_fee",
                "cancel_replace_order/{symbol}"
            }}
        }}
    };

    // Initialize fees
    fees = {
        {"trading", {
            {"maker", 0.0016},  // 0.16%
            {"taker", 0.0025}   // 0.25%
        }}
    };

    // Initialize precision rules
    precisionMode = DECIMAL_PLACES;
    
    requiredCredentials = {
        {"apiKey", true},
        {"secret", true},
        {"uid", true}
    };
}

Json cex::describeImpl() const {
    return {
        {"id", id},
        {"name", name},
        {"countries", Json::array({"UK", "EU", "Cyprus", "RU"})},
        {"rateLimit", rateLimit},
        {"pro", pro},
        {"has", {
            {"fetchMarkets", true},
            {"fetchCurrencies", true},
            {"fetchTicker", true},
            {"fetchTickers", true},
            {"fetchOrderBook", true},
            {"fetchTrades", true},
            {"fetchOHLCV", true},
            {"fetchBalance", true},
            {"createOrder", true},
            {"cancelOrder", true},
            {"fetchOrder", true},
            {"fetchOpenOrders", true},
            {"fetchClosedOrders", true},
            {"fetchDepositAddress", true},
            {"withdraw", true},
            {"fetchMyTrades", true},
            {"fetchLedger", true}
        }},
        {"timeframes", {
            {"1m", "1m"},
            {"3m", "3m"},
            {"5m", "5m"},
            {"15m", "15m"},
            {"30m", "30m"},
            {"1h", "1h"},
            {"2h", "2h"},
            {"4h", "4h"},
            {"6h", "6h"},
            {"12h", "12h"},
            {"1d", "1d"},
            {"1w", "1w"},
            {"1M", "1M"}
        }},
        {"urls", urls},
        {"api", api},
        {"fees", fees},
        {"precisionMode", precisionMode}
    };
}

Json cex::fetchMarketsImpl() const {
    auto response = get("currency_limits");
    auto markets = Json::array();
    auto pairs = response["data"]["pairs"].array_items();
    
    for (const auto& pair : pairs) {
        auto symbol = pair["symbol1"].string_value() + "/" + pair["symbol2"].string_value();
        markets.push_back({
            {"id", pair["symbol1"].string_value() + pair["symbol2"].string_value()},
            {"symbol", symbol},
            {"base", pair["symbol1"].string_value()},
            {"quote", pair["symbol2"].string_value()},
            {"baseId", pair["symbol1"].string_value()},
            {"quoteId", pair["symbol2"].string_value()},
            {"active", true},
            {"precision", {
                {"amount", pair["scale1"].int_value()},
                {"price", pair["scale2"].int_value()}
            }},
            {"limits", {
                {"amount", {
                    {"min", pair["minLotSize"].number_value()},
                    {"max", pair["maxLotSize"].number_value()}
                }},
                {"price", {
                    {"min", pair["minPrice"].number_value()},
                    {"max", pair["maxPrice"].number_value()}
                }},
                {"cost", {
                    {"min", pair["minLotSizeS2"].number_value()},
                    {"max", pair["maxLotSizeS2"].number_value()}
                }}
            }},
            {"info", pair}
        });
    }
    
    return markets;
}

Json cex::fetchTickerImpl(const std::string& symbol) const {
    auto market = this->market(symbol);
    auto response = get("ticker/" + market["id"].string_value());
    return parseTicker(response, market);
}

Json cex::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = "order_book/" + market["id"].string_value();
    if (limit) {
        request += "?depth=" + std::to_string(*limit);
    }
    auto response = get(request);
    
    return {
        {"bids", response["bids"]},
        {"asks", response["asks"]},
        {"timestamp", response["timestamp"].int_value() * 1000},
        {"datetime", iso8601(response["timestamp"].int_value() * 1000)},
        {"nonce", response["timestamp"].int_value()}
    };
}

Json cex::createOrderImpl(const std::string& symbol, const std::string& type,
                         const std::string& side, double amount,
                         const std::optional<double>& price) {
    auto market = this->market(symbol);
    auto request = {
        {"type", side},
        {"amount", formatNumber(amount, market["precision"]["amount"].int_value())}
    };
    
    if (type == "limit") {
        if (!price) {
            throw std::runtime_error("Price is required for limit orders");
        }
        request["price"] = formatNumber(*price, market["precision"]["price"].int_value());
    }
    
    auto response = privatePost("place_order/" + market["id"].string_value(), request);
    return parseOrder(response, market);
}

Json cex::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    return privatePost("cancel_order", {{"id", id}});
}

Json cex::fetchBalanceImpl() const {
    auto response = privatePost("balance/");
    auto result = {
        {"info", response},
        {"timestamp", nullptr},
        {"datetime", nullptr}
    };
    
    for (const auto& balance : response.object_items()) {
        if (balance.first.find("available") != std::string::npos) {
            auto currencyId = balance.first.substr(0, balance.first.find("available"));
            auto currency = this->currency(currencyId);
            auto account = {
                {"free", balance.second.number_value()},
                {"used", response[currencyId + "orders"].number_value()},
                {"total", response[currencyId + "orders"].number_value() + balance.second.number_value()}
            };
            result[currency["code"].string_value()] = account;
        }
    }
    
    return result;
}

Json cex::parseTicker(const Json& ticker, const Json& market) const {
    auto timestamp = ticker["timestamp"].int_value() * 1000;
    auto symbol = market ? market["symbol"].string_value() : "";
    
    return {
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", iso8601(timestamp)},
        {"high", ticker["high"]},
        {"low", ticker["low"]},
        {"bid", ticker["bid"]},
        {"bidVolume", nullptr},
        {"ask", ticker["ask"]},
        {"askVolume", nullptr},
        {"vwap", nullptr},
        {"open", nullptr},
        {"close", ticker["last"]},
        {"last", ticker["last"]},
        {"previousClose", nullptr},
        {"change", nullptr},
        {"percentage", nullptr},
        {"average", nullptr},
        {"baseVolume", ticker["volume"]},
        {"quoteVolume", nullptr},
        {"info", ticker}
    };
}

std::string cex::sign(const std::string& path, const std::string& api,
                     const std::string& method, const Json& params,
                     const Json& headers, const Json& body) const {
    auto url = urls["api"][api] + "/" + path;
    
    if (api == "public") {
        if (!params.empty()) {
            url += "?" + urlencode(params);
        }
    } else {
        checkRequiredCredentials();
        auto nonce = std::to_string(milliseconds());
        auto auth = nonce + uid + apiKey;
        auto signature = hmac(auth, secret, "sha256", "hex");
        
        auto request = params;
        request["key"] = apiKey;
        request["signature"] = signature;
        request["nonce"] = nonce;
        
        if (method == "GET") {
            url += "?" + urlencode(request);
        } else {
            headers["Content-Type"] = "application/x-www-form-urlencoded";
            body = urlencode(request);
        }
    }
    
    return url;
}

} // namespace ccxt
