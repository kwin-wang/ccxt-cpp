#include "binance.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

const std::string Binance::defaultHostname = "api.binance.com";
const int Binance::defaultRateLimit = 50;
const bool Binance::defaultPro = true;

Binance::Binance(const Config& config) : Exchange(config) {
    id = "binance";
    name = "Binance";
    countries = {"JP", "MT"}; // Japan, Malta
    version = "v3";
    certified = true;
    pro = true;
    
    // Initialize capabilities
    has = {
        {"CORS", nullptr},
        {"spot", true},
        {"margin", true},
        {"swap", true},
        {"future", true},
        {"option", true},
        {"addMargin", true},
        {"borrowCrossMargin", true},
        {"borrowIsolatedMargin", true},
        {"cancelAllOrders", true},
        {"cancelOrder", true},
        {"cancelOrders", true}, // contract only
        {"closeAllPositions", false},
        {"closePosition", false}, // exchange specific closePosition parameter for binance createOrder is not synonymous with how CCXT uses closePositions
        {"createOrder", true},
        {"createReduceOnlyOrder", true},
        {"fetchBalance", true},
        {"fetchBidsAsks", true},
        {"fetchClosedOrders", true},
        {"fetchCurrencies", true},
        {"fetchDeposits", true},
        {"fetchFundingRate", true},
        {"fetchFundingRateHistory", true},
        {"fetchFundingRates", true},
        {"fetchIndexOHLCV", true},
        {"fetchMarkets", true},
        {"fetchMarkOHLCV", true},
        {"fetchMyTrades", true},
        {"fetchOHLCV", true},
        {"fetchOpenOrders", true},
        {"fetchOrder", true},
        {"fetchOrders", true},
        {"fetchOrderBook", true},
        {"fetchPositions", true},
        {"fetchPremiumIndexOHLCV", true},
        {"fetchStatus", true},
        {"fetchTicker", true},
        {"fetchTickers", true},
        {"fetchTime", true},
        {"fetchTrades", true},
        {"fetchTradingFee", true},
        {"fetchTradingFees", true},
        {"fetchTransactionFees", true},
        {"fetchTransfers", true},
        {"fetchWithdrawals", true},
        {"setLeverage", true},
        {"setMarginMode", true},
        {"transfer", true},
        {"withdraw", true}
    };
}

void Binance::init() {
    Exchange::init();
    
    // Initialize timeframes
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
        {"3d", "3d"},
        {"1w", "1w"},
        {"1M", "1M"}
    };

    // Initialize URLs
    hostname = this->extractParam<std::string>(config, "hostname", defaultHostname);
    urls["api"] = {
        {"public", "https://" + hostname},
        {"private", "https://" + hostname},
        {"v3", "https://" + hostname},
        {"v1", "https://" + hostname}
    };
    urls["logo"] = "https://user-images.githubusercontent.com/1294454/29604020-d5483cdc-87ee-11e7-94c7-d1a8d9169293.jpg";
    urls["www"] = "https://www.binance.com";
    urls["doc"] = {
        "https://binance-docs.github.io/apidocs/spot/en"
    };
    urls["fees"] = "https://www.binance.com/en/fee/schedule";
    urls["referral"] = "https://www.binance.com/en/register?ref=D7YA7CLY";

    // Initialize API endpoints
    api = {
        {"public", {
            {"GET", {
                "ping",
                "time",
                "exchangeInfo",
                "depth",
                "trades",
                "historicalTrades",
                "aggTrades",
                "klines",
                "ticker/24hr",
                "ticker/price",
                "ticker/bookTicker"
            }}
        }},
        {"private", {
            {"GET", {
                "account",
                "myTrades",
                "openOrders",
                "allOrders",
                "order"
            }},
            {"POST", {
                "order",
                "order/test"
            }},
            {"DELETE", {
                "order"
            }}
        }},
        {"sapi", {
            {"GET", {
                "margin/transfer",
                "margin/loan",
                "margin/repay",
                "margin/account",
                "margin/order",
                "margin/openOrders",
                "margin/allOrders",
                "margin/myTrades",
                "margin/maxBorrowable",
                "margin/maxTransferable",
                "margin/tradeCoeff",
                "margin/priceIndex",
                "margin/isolated/transfer",
                "margin/isolated/account",
                "margin/isolated/pair",
                "margin/isolated/allPairs",
                "margin/interestRateHistory",
                "margin/forceLiquidationRec",
                "margin/isolatedMarginData",
                "margin/isolatedMarginTier"
            }},
            {"POST", {
                "margin/transfer",
                "margin/loan",
                "margin/repay",
                "margin/order",
                "margin/isolated/transfer",
                "margin/isolated/account"
            }},
            {"DELETE", {
                "margin/order",
                "margin/isolated/account"
            }}
        }},
        {"fapi", {
            {"GET", {
                "ping",
                "time",
                "exchangeInfo",
                "depth",
                "trades",
                "historicalTrades",
                "aggTrades",
                "klines",
                "fundingRate",
                "ticker/24hr",
                "ticker/price",
                "ticker/bookTicker",
                "openInterest",
                "openInterestHist",
                "topLongShortAccountRatio",
                "topLongShortPositionRatio",
                "globalLongShortAccountRatio",
                "takerlongshortRatio",
                "lvtKlines",
                "indexInfo",
                "assetIndex"
            }},
            {"POST", {
                "positionSide/dual",
                "order",
                "batchOrders",
                "countdownCancelAll",
                "leverage",
                "marginType",
                "positionMargin",
                "listenKey"
            }},
            {"DELETE", {
                "order",
                "allOpenOrders",
                "batchOrders",
                "listenKey"
            }}
        }}
    };
}

void Binance::describe() {
    this->set("id", "binance");
    this->set("name", "Binance");
    
    // Add market type options
    this->set("options", {
        {"defaultType", "spot"}, // spot, margin, future, delivery
        {"types", {"spot", "margin", "future", "delivery"}},
        {"accountsByType", {
            {"spot", "SPOT"},
            {"margin", "MARGIN"},
            {"future", "USDM"},
            {"delivery", "COINM"}
        }}
    });

    // Add all endpoints for different market types
    this->set("urls", {
        {"logo", "https://user-images.githubusercontent.com/1294454/29604020-d5483cdc-87ee-11e7-94c7-d1a8d9169293.jpg"},
        {"api", {
            {"spot", "https://api.binance.com"},
            {"usdm", "https://fapi.binance.com"},
            {"coinm", "https://dapi.binance.com"},
            {"margin", "https://api.binance.com"}
        }},
        {"www", "https://www.binance.com"},
        {"doc", {
            "https://binance-docs.github.io/apidocs/spot/en",
            "https://binance-docs.github.io/apidocs/futures/en",
            "https://binance-docs.github.io/apidocs/delivery/en"
        }},
        {"api_management", "https://www.binance.com/en/usercenter/settings/api-management"},
        {"referral", {
            "url": "https://www.binance.com/en/register?ref=D7YA7CLY",
            "discount": 0.1
        }}
    });
}

std::string Binance::getMarketType(const std::string& symbol) {
    auto marketType = this->safeString(this->options, "defaultType", "spot");
    auto market = this->market(symbol);
    if (market.has("type")) {
        marketType = market["type"];
    }
    return marketType;
}

std::string Binance::getEndpoint(const std::string& path, const std::string& type) {
    auto marketType = type.empty() ? this->safeString(this->options, "defaultType", "spot") : type;
    auto urls = this->urls["api"];
    
    if (marketType == "future") {
        return urls["usdm"] + path;
    } else if (marketType == "delivery") {
        return urls["coinm"] + path;
    } else {
        return urls["spot"] + path;
    }
}

// Market Data API
json Binance::fetchMarketsImpl() const {
    json response = this->publicGetExchangeInfo();
    json markets = json::array();
    
    for (const auto& market : response["symbols"]) {
        if (market["status"] == "TRADING") {
            markets.push_back({
                {"id", market["symbol"]},
                {"symbol", market["baseAsset"] + "/" + market["quoteAsset"]},
                {"base", market["baseAsset"]},
                {"quote", market["quoteAsset"]},
                {"baseId", market["baseAsset"]},
                {"quoteId", market["quoteAsset"]},
                {"active", true},
                {"precision", {
                    {"amount", market["baseAssetPrecision"]},
                    {"price", market["quotePrecision"]}
                }},
                {"limits", {
                    {"amount", {
                        {"min", std::stod(market["filters"][2]["minQty"])},
                        {"max", std::stod(market["filters"][2]["maxQty"])}
                    }},
                    {"price", {
                        {"min", std::stod(market["filters"][0]["minPrice"])},
                        {"max", std::stod(market["filters"][0]["maxPrice"])}
                    }},
                    {"cost", {
                        {"min", std::stod(market["filters"][3]["minNotional"])}
                    }}
                }},
                {"info", market}
            });
        }
    }
    
    return markets;
}

json Binance::fetchTickerImpl(const std::string& symbol) const {
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {{"symbol", market["id"]}};
    json response = this->publicGetTicker24hr(request);
    return this->parseTicker(response, market);
}

json Binance::fetchTickersImpl(const std::vector<std::string>& symbols) const {
    this->loadMarkets();
    json response = this->publicGetTicker24hr();
    return this->parseTickers(response, symbols);
}

json Binance::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {{"symbol", market["id"]}};
    if (limit) {
        request["limit"] = *limit;
    }
    json response = this->publicGetDepth(request);
    return this->parseOrderBook(response, symbol);
}

json Binance::fetchTradesImpl(const std::string& symbol, const std::optional<long long>& since,
                           const std::optional<int>& limit) const {
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {{"symbol", market["id"]}};
    if (limit) {
        request["limit"] = *limit;
    }
    json response = this->publicGetTrades(request);
    return this->parseTrades(response, market, since, limit);
}

json Binance::fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe,
                          const std::optional<long long>& since,
                          const std::optional<int>& limit) const {
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {
        {"symbol", market["id"]},
        {"interval", timeframes[timeframe]}
    };
    if (since) {
        request["startTime"] = *since;
    }
    if (limit) {
        request["limit"] = *limit;
    }
    json response = this->publicGetKlines(request);
    return this->parseOHLCVs(response, market, timeframe, since, limit);
}

// Trading API
json Binance::createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side,
                          double amount, const std::optional<double>& price) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {
        {"symbol", market["id"]},
        {"side", side.toUpperCase()},
        {"type", type.toUpperCase()},
        {"quantity", this->amountToPrecision(symbol, amount)}
    };
    
    if (type == "LIMIT" && price) {
        request["price"] = this->priceToPrecision(symbol, *price);
        request["timeInForce"] = "GTC";
    }
    
    json response = this->privatePostOrder(request);
    return this->parseOrder(response, market);
}

json Binance::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {
        {"symbol", market["id"]},
        {"orderId", id}
    };
    json response = this->privateDeleteOrder(request);
    return this->parseOrder(response, market);
}

json Binance::fetchOrderImpl(const std::string& id, const std::string& symbol) const {
    this->loadMarkets();
    Market market = this->market(symbol);
    json request = {
        {"symbol", market["id"]},
        {"orderId", id}
    };
    json response = this->privateGetOrder(request);
    return this->parseOrder(response, market);
}

json Binance::fetchBalanceImpl() const {
    this->loadMarkets();
    json response = this->privateGetAccount();
    return this->parseBalance(response);
}

// Helper methods
std::string Binance::sign(const std::string& path, const std::string& api,
                       const std::string& method, const json& params,
                       const std::map<std::string, std::string>& headers,
                       const json& body) const {
    std::string url = this->getEndpoint(path, api);
    
    if (api == "private" || api == "sapi" || api == "fapi") {
        this->checkRequiredCredentials();
        std::string timestamp = this->getTimestamp();
        std::string queryString = this->extend(params, {{"timestamp", timestamp}}).dump();
        
        if (method == "POST" || method == "PUT" || method == "DELETE") {
            if (!params.empty()) {
                body = params;
            }
        } else {
            if (!params.empty()) {
                url += "?" + this->urlencode(params);
            }
        }
        
        std::string signature = this->hmac(queryString, this->encode(this->secret),
                                       "sha256", "hex");
        
        const_cast<std::map<std::string, std::string>&>(headers)["X-MBX-APIKEY"] = this->apiKey;
        
        if (method == "POST" || method == "PUT" || method == "DELETE") {
            const_cast<std::map<std::string, std::string>&>(headers)["Content-Type"] = "application/json";
        }
        
        url += (url.find('?') != std::string::npos ? "&" : "?") + "signature=" + signature;
    } else {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    }
    
    return url;
}

std::string Binance::getTimestamp() const {
    return std::to_string(this->milliseconds());
}

std::string Binance::createSignature(const std::string& queryString) const {
    unsigned char* digest = nullptr;
    unsigned int digestLen = 0;
    
    HMAC_CTX* ctx = HMAC_CTX_new();
    HMAC_Init_ex(ctx, this->secret.c_str(), this->secret.length(), EVP_sha256(), nullptr);
    HMAC_Update(ctx, (unsigned char*)queryString.c_str(), queryString.length());
    HMAC_Final(ctx, digest, &digestLen);
    HMAC_CTX_free(ctx);
    
    std::stringstream ss;
    for(unsigned int i = 0; i < digestLen; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
    }
    
    return ss.str();
}

} // namespace ccxt
