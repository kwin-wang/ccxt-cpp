#include <ccxt/exchanges/binance.h>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/params.h>
#include <boost/algorithm/string.hpp>

namespace ccxt {
Binance::Binance(boost::asio::io_context& context, const Config& config)
    : Exchange(context, config) {
    config_.loadRest("config/binance_rest.json");
    config_.loadWs("config/binance_ws.json");
  
    // Initialize capabilities
    /*
    this->has.emplace("CORS", true);
    this->has.emplace("spot", true);
    this->has.emplace("margin", true);
    this->has.emplace("swap", true);
    this->has.emplace("future", true);
    this->has.emplace("option", false);
    this->has.emplace("addMargin", true);
    this->has.emplace("borrowCrossMargin", true);
    this->has.emplace("borrowIsolatedMargin", true);
    this->has.emplace("cancelAllOrders", true);
    this->has.emplace("cancelOrder", true);
    this->has.emplace("cancelOrders", true); // contract only
    this->has.emplace("closeAllPositions", false);
    this->has.emplace("closePosition", false); // exchange specific closePosition parameter for binance createOrder is not synonymous with how CCXT uses closePositions
    this->has.emplace("createOrder", true);
    this->has.emplace("createReduceOnlyOrder", true);
    this->has.emplace("fetchBalance", true);
    this->has.emplace("fetchBidsAsks", true);
    this->has.emplace("fetchClosedOrders", true);
    this->has.emplace("fetchCurrencies", true);
    this->has.emplace("fetchDeposits", true);
    this->has.emplace("fetchFundingRate", true);
    this->has.emplace("fetchFundingRateHistory", true);
    this->has.emplace("fetchFundingRates", true);
    this->has.emplace("fetchIndexOHLCV", true);
    this->has.emplace("fetchMarkets", true);
    this->has.emplace("fetchMarkOHLCV", true);
    this->has.emplace("fetchMyTrades", true);
    this->has.emplace("fetchOHLCV", true);
    this->has.emplace("fetchOpenOrders", true);
    this->has.emplace("fetchOrder", true);
    this->has.emplace("fetchOrders", true);
    this->has.emplace("fetchOrderBook", true);
    this->has.emplace("fetchPositions", true);
    this->has.emplace("fetchPremiumIndexOHLCV", true);
    this->has.emplace("fetchStatus", true);
    this->has.emplace("fetchTicker", true);
    this->has.emplace("fetchTickers", true);
    this->has.emplace("fetchTime", true);
    this->has.emplace("fetchTrades", true);
    this->has.emplace("fetchTradingFee", true);
    this->has.emplace("fetchTradingFees", true);
    this->has.emplace("fetchTransactionFees", true);
    this->has.emplace("fetchTransfers", true);
    this->has.emplace("fetchWithdrawals", true);
    this->has.emplace("setLeverage", true);
    this->has.emplace("setMarginMode", true);
    this->has.emplace("transfer", true);
    this->has.emplace("withdraw", true);
    */
}

void Binance::init() {
    std::string defaultHostname = "api.binance.com";
    std::string hostname = config_.hostname.empty() ? defaultHostname : config_.hostname;
    
    
    //this->urls["logo"] = "https://user-images.githubusercontent.com/1294454/29604020-d5483cdc-87ee-11e7-94c7-d1a8d9169293.jpg";
    /*this->urls["api"] = {
        {"public", "https://" + hostname},
        {"private", "https://" + hostname},
        {"v3", "https://" + hostname + "/api/v3"},
        {"web", "https://" + hostname + "/api/web/v3"}
    };*/

    
    // Initialize API endpoints
    /*
    this->api = {
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
    */
}

void Binance::describe() const {
    /*
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
    */
}

std::string Binance::getMarketType(const std::string& symbol) const {
    //auto marketType = this->safeString(this->config_.options, "defaultType", "spot");
    //auto market = this->markets[symbol];
   
    //return market["type"];
    return "spot";
}

std::string Binance::getEndpoint(const std::string& path, const std::string& type) const {
    auto marketType = type.empty() ? this->safeString(this->config_.options, "defaultType", "spot") : type;
    auto& urls = this->config_.json_rest["api"];
    
    if (marketType == "future") {
        return this->safeString(urls, "usdm", "") + path;
    } else if (marketType == "delivery") {
        return this->safeString(urls, "coinm", "") + path;
    } else {
        return this->safeString(urls, "spot", "") + path;
    }
}

// Market Data API
json Binance::fetchMarketsImpl() const {
    json response ;//= this->publicGetExchangeInfo();
    json markets = response["symbols"];
    json result = json::array();
    
    for (const auto& market : markets) {
        result.push_back({
            {"id", market["symbol"]},
            {"symbol", market["baseAsset"].get<std::string>() + "/" + market["quoteAsset"].get<std::string>()},
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
                    {"min", market["filters"][2]["minQty"].get<std::string>()},
                    {"max", market["filters"][2]["maxQty"].get<std::string>()}
                }},
                {"price", {
                    {"min", market["filters"][0]["minPrice"].get<std::string>()},
                    {"max", market["filters"][0]["maxPrice"].get<std::string>()}
                }},
                {"cost", {
                    {"min", market["filters"][3]["minNotional"].get<std::string>()}
                }}
            }},
            {"info", market}
        });
    }
    return result;
}

json Binance::fetchTickerImpl(const std::string& symbol) const {
    //this->loadMarkets();
    Market market;// = markets[symbol];;
    json request = json::object();
    request["symbol"] = market["id"];
    json response ;//= this->publicGetTicker24hr(request);
    return this->parseTicker(response, market);
}

json Binance::fetchTickersImpl(const std::vector<std::string>& symbols) const {
    //this->loadMarkets();
    json response ;//= this->publicGetTicker24hr();
    return response;//this->parseTickers(response, symbols);
}

json Binance::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
    //this->loadMarkets();
    Market market;// = markets[symbol];;
    json request = json::object();
    request["symbol"] = market["id"];
    if (limit) {
        request["limit"] = *limit;
    }
    json response ;//= this->publicGetDepth(request);
    return this->parseOrderBook(response, symbol);
}

json Binance::fetchTradesImpl(const std::string& symbol, const std::optional<long long>& since,
                           const std::optional<int>& limit) const {
    //this->loadMarkets();
    Market market;// = markets[symbol];;
    json request = json::object();
    request["symbol"] = market["id"];
    if (limit) {
        request["limit"] = *limit;
    }
    json response ;//= this->publicGetTrades(request);
    return response;//this->parseTrades(response, market, since, limit);
}

json Binance::fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe,
                          const std::optional<long long>& since,
                          const std::optional<int>& limit) const {
    ////this->loadMarkets();
    Market market;// = markets[symbol];;
    
    json request = json::object();
    request["symbol"] = market["id"];
    request["interval"] = "";//timeframes[timeframe];
    
    if (since) {
        request["startTime"] = *since;
    }
    if (limit) {
        request["limit"] = *limit;
    }
    
    json response;// = this->publicGetKlines(request);
    return response;//this->parseOHLCV(response, market, timeframe, since, limit);
}

// Trading API
json Binance::createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side,
                          double amount, const std::optional<double>& price) {
    //this->loadMarkets();
    Market market;// = markets[symbol];;
    
    json request = json::object();
    request["symbol"] = market["id"];
    request["side"] = boost::algorithm::to_upper_copy(side);
    request["type"] = boost::algorithm::to_upper_copy(type);
    request["quantity"] = this->amountToPrecision(symbol, amount);
    
    if (type == "LIMIT" && price) {
        request["price"] = this->priceToPrecision(symbol, *price);
        request["timeInForce"] = "GTC";
    }
    
    json response;// = this->privatePostOrder(request);
    return this->parseOrder(response, market);
}

json Binance::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    //this->loadMarkets();
    Market market;// = markets[symbol];;
    
    json request = json::object();
    request["symbol"] = market["id"];
    request["orderId"] = id;
    
    json response ;//= this->privateDeleteOrder(request);
    return this->parseOrder(response, market);
}

json Binance::fetchOrderImpl(const std::string& id, const std::string& symbol) const {
    ////this->loadMarkets();
    Market market;// = markets[symbol];;
    
    json request = json::object();
    request["symbol"] = market["id"];
    request["orderId"] = id;
    
    json response ;//= this->privateGetOrder(request);
    return this->parseOrder(response, market);
}

json Binance::fetchBalanceImpl() const {
    ////this->loadMarkets();
    json response ;//= this->privateGetAccount();
    return this->parseBalance(response);
}

// Helper methods
std::string Binance::sign(const std::string& path, const std::string& api,
                       const std::string& method, const json& params,
                       const std::map<std::string, std::string>& headers,
                       const json& body) const {
    /*std::string url = this->getEndpoint(path, api);
    
    if (api == "private" || api == "sapi" || api == "fapi") {
        this->checkRequiredCredentials();
        std::string timestamp = this->getTimestamp();
        std::string queryString = this->extend(params, {{"timestamp", timestamp}}).dump();
        
        if (method == "POST" || method == "PUT" || method == "DELETE") {
            if (!params.empty()) {
                //body = params;
            }
        } else {
            if (!params.empty()) {
                //url += "?" + this->urlencode(params);
            }
        }
        
        std::string signature = this->createSignature(queryString);
        
        const_cast<std::map<std::string, std::string>&>(headers)["X-MBX-APIKEY"] = this->config_.apiKey;
        
        if (method == "POST" || method == "PUT" || method == "DELETE") {
            const_cast<std::map<std::string, std::string>&>(headers)["Content-Type"] = "application/json";
        }
        
        if (!params.empty()) {
            url += url.find('?') != std::string::npos ? "&" : "?";
            url += "signature=" + signature;
        }
    } else {
        if (!params.empty()) {
            //url += "?" + this->urlencode(params);
        }
    }
    
    return url;*/
    return "";
}

std::string Binance::getTimestamp() const {
    return std::to_string(milliseconds());
}

std::string Binance::createSignature(const std::string& queryString) const {
    EVP_MAC* mac = EVP_MAC_fetch(nullptr, "HMAC", nullptr);
    EVP_MAC_CTX* ctx = EVP_MAC_CTX_new(mac);
    
    OSSL_PARAM params[] = {
        OSSL_PARAM_construct_utf8_string("digest", (char*)"sha256", 0),
        OSSL_PARAM_construct_end()
    };
    
    EVP_MAC_init(ctx, (const unsigned char*)this->config_.secret.c_str(), this->config_.secret.length(), params);
    EVP_MAC_update(ctx, (const unsigned char*)queryString.c_str(), queryString.length());
    
    unsigned char digest[EVP_MAX_MD_SIZE];
    size_t digestLen = 0;
    EVP_MAC_final(ctx, digest, &digestLen, EVP_MAX_MD_SIZE);
    
    EVP_MAC_CTX_free(ctx);
    EVP_MAC_free(mac);
    
    std::stringstream ss;
    for (size_t i = 0; i < digestLen; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
    }
    return ss.str();
}

} // namespace ccxt
