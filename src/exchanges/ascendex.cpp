#include "ascendex.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

const std::string AscendEX::defaultHostname = "ascendex.com";
const int AscendEX::defaultRateLimit = 400;
const bool AscendEX::defaultPro = true;

AscendEX::AscendEX(const Config& config) : Exchange(config) {
    this->init();
}

void AscendEX::init() {
    

    this->id = "ascendex";
    this->name = "AscendEX";
    this->countries = {"SG"};  // Singapore
    this->version = "v2";
    this->rateLimit = defaultRateLimit;
    this->certified = false;
    this->pro = defaultPro;

    this->has = {
        {"CORS", nullptr},
        {"spot", true},
        {"margin", true},
        {"swap", true},
        {"future", false},
        {"option", false},
        {"addMargin", true},
        {"cancelAllOrders", true},
        {"cancelOrder", true},
        {"createOrder", true},
        {"createOrders", true},
        {"createPostOnlyOrder", true},
        {"createReduceOnlyOrder", true},
        {"createStopLimitOrder", true},
        {"createStopMarketOrder", true},
        {"createStopOrder", true},
        {"fetchAccounts", true},
        {"fetchBalance", true},
        {"fetchClosedOrders", true},
        {"fetchCurrencies", true},
        {"fetchDepositAddress", true},
        {"fetchDepositAddresses", false},
        {"fetchDepositAddressesByNetwork", false},
        {"fetchDeposits", true},
        {"fetchDepositsWithdrawals", true},
        {"fetchDepositWithdrawFee", "emulated"},
        {"fetchDepositWithdrawFees", true},
        {"fetchFundingHistory", true},
        {"fetchFundingRate", "emulated"},
        {"fetchFundingRateHistory", false},
        {"fetchFundingRates", true},
        {"fetchIndexOHLCV", false},
        {"fetchLeverage", "emulated"},
        {"fetchLeverages", true},
        {"fetchLeverageTiers", true},
        {"fetchMarginMode", "emulated"},
        {"fetchMarginModes", true},
        {"fetchMarketLeverageTiers", "emulated"},
        {"fetchMarkets", true},
        {"fetchMarkOHLCV", false},
        {"fetchOHLCV", true},
        {"fetchOpenInterest", false},
        {"fetchOpenInterestHistory", false},
        {"fetchOpenOrders", true},
        {"fetchOrder", true},
        {"fetchOrderBook", true},
        {"fetchOrders", false},
        {"fetchPosition", false},
        {"fetchPositionMode", false},
        {"fetchPositions", true},
        {"fetchPositionsRisk", false},
        {"fetchPremiumIndexOHLCV", false},
        {"fetchTicker", true},
        {"fetchTickers", true},
        {"fetchTime", true},
        {"fetchTrades", true},
        {"fetchTradingFee", false},
        {"fetchTradingFees", true},
        {"fetchTransactionFee", false},
        {"fetchTransactionFees", false},
        {"fetchTransactions", "emulated"},
        {"fetchTransfer", false},
        {"fetchTransfers", false},
        {"fetchWithdrawal", false},
        {"fetchWithdrawals", true},
        {"reduceMargin", true},
        {"sandbox", true},
        {"setLeverage", true},
        {"setMarginMode", true},
        {"setPositionMode", false},
        {"transfer", true}
    };

    this->timeframes = {
        {"1m", "1"},
        {"5m", "5"},
        {"15m", "15"},
        {"30m", "30"},
        {"1h", "60"},
        {"2h", "120"},
        {"4h", "240"},
        {"6h", "360"},
        {"12h", "720"},
        {"1d", "1d"},
        {"1w", "1w"},
        {"1M", "1m"}
    };

    this->urls = {
        {"logo", "https://github.com/user-attachments/assets/55bab6b9-d4ca-42a8-a0e6-fac81ae557f1"},
        {"api", {
            {"rest", "https://ascendex.com"}
        }},
        {"test", {
            {"rest", "https://api-test.ascendex-sandbox.com"}
        }},
        {"www", "https://ascendex.com"},
        {"doc", {
            "https://ascendex.github.io/ascendex-pro-api/#ascendex-pro-api-documentation"
        }},
        {"fees", "https://ascendex.com/en/feerate/transactionfee-traderate"},
        {"referral", {
            {"url", "https://ascendex.com/en-us/register?inviteCode=EL6BXBQM"},
            {"discount", 0.25}
        }}
    };

    this->api = {
        {"v1", {
            {"public", {
                {"get", {
                    "assets",
                    "products",
                    "ticker",
                    "barhist/info",
                    "barhist",
                    "depth",
                    "trades",
                    "cash/assets",     // not documented
                    "cash/products",    // not documented
                    "margin/assets",    // not documented
                    "margin/products",  // not documented
                    "futures/collateral",
                    "futures/contracts",
                    "futures/ref-px",
                    "futures/market-data",
                    "futures/funding-rates",
                    "risk-limit-info",
                    "exchange-info"
                }}
            }},
            {"private", {
                {"get", {
                    "info",
                    "wallet/transactions",
                    "wallet/deposit/address",
                    "data/balance/snapshot",
                    "data/balance/history"
                }},
                {"accountCategory", {
                    {"get", {
                        "balance",
                        "order/open",
                        "order/status",
                        "order/hist/current",
                        "risk"
                    }},
                    {"post", {
                        "order",
                        "order/batch"
                    }},
                    {"delete", {
                        "order",
                        "order/all",
                        "order/batch"
                    }}
                }},
                {"accountGroup", {
                    {"get", {
                        "cash/balance",
                        "margin/balance",
                        "margin/risk",
                        "futures/collateral-balance",
                        "futures/position",
                        "futures/risk"
                    }},
                    {"post", {
                        "futures/transfer/deposit",
                        "futures/transfer/withdraw"
                    }}
                }}
            }}
        }},
        {"v2", {
            {"public", {
                {"get", {
                    "assets",
                    "products",
                    "ticker",
                    "trades",
                    "depth"
                }}
            }},
            {"private", {
                {"get", {
                    "account/info",
                    "order/hist/current",
                    "order/open",
                    "risk",
                    "spot/fee"
                }},
                {"post", {
                    "order",
                    "order/batch"
                }},
                {"delete", {
                    "order",
                    "order/all",
                    "order/batch"
                }}
            }}
        }}
    };
}

json AscendEX::fetchMarketsImpl() const {
    // https://ascendex.github.io/ascendex-pro-api/#get-all-asset-products
    json cashResponse = this->request("v1/cash/products", "public", "GET");
    json marginResponse = this->request("v1/margin/products", "public", "GET");
    json futuresResponse = this->request("v1/futures/contracts", "public", "GET");
    
    json cashMarkets = this->parseMarkets(cashResponse, "spot");
    json marginMarkets = this->parseMarkets(marginResponse, "margin");
    json futuresMarkets = this->parseMarkets(futuresResponse, "swap");
    
    json markets;
    markets.insert(markets.end(), cashMarkets.begin(), cashMarkets.end());
    markets.insert(markets.end(), marginMarkets.begin(), marginMarkets.end());
    markets.insert(markets.end(), futuresMarkets.begin(), futuresMarkets.end());
    
    return markets;
}

json AscendEX::parseMarkets(const json& response, const std::string& type) const {
    json data = response["data"];
    json result = json::array();
    
    for (const auto& market : data) {
        json entry = {
            {"id", market["symbol"]},
            {"symbol", market["symbol"]},
            {"base", market["baseAsset"]},
            {"quote", market["quoteAsset"]},
            {"settle", nullptr},
            {"baseId", market["baseAsset"]},
            {"quoteId", market["quoteAsset"]},
            {"settleId", nullptr},
            {"type", type},
            {"spot", type == "spot"},
            {"margin", type == "margin"},
            {"swap", type == "swap"},
            {"future", false},
            {"option", false},
            {"active", market["status"] == "Normal"},
            {"contract", type == "swap"},
            {"linear", type == "swap" ? true : nullptr},
            {"inverse", type == "swap" ? false : nullptr},
            {"taker", std::stod(market["commissionReserveRate"].get<std::string>())},
            {"maker", std::stod(market["commissionReserveRate"].get<std::string>())},
            {"contractSize", type == "swap" ? std::stod(market["contractSize"].get<std::string>()) : nullptr},
            {"expiry", nullptr},
            {"expiryDatetime", nullptr},
            {"strike", nullptr},
            {"optionType", nullptr},
            {"precision", {
                {"amount", std::stod(market["lotSize"].get<std::string>())},
                {"price", std::stod(market["tickSize"].get<std::string>())}
            }},
            {"limits", {
                {"leverage", {
                    {"min", type == "swap" ? std::stod(market["minLeverage"].get<std::string>()) : nullptr},
                    {"max", type == "swap" ? std::stod(market["maxLeverage"].get<std::string>()) : nullptr}
                }},
                {"amount", {
                    {"min", std::stod(market["minQty"].get<std::string>())},
                    {"max", std::stod(market["maxQty"].get<std::string>())}
                }},
                {"price", {
                    {"min", std::stod(market["tickSize"].get<std::string>())},
                    {"max", nullptr}
                }},
                {"cost", {
                    {"min", std::stod(market["minNotional"].get<std::string>())},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        };
        
        result.push_back(entry);
    }
    
    return result;
}

json AscendEX::fetchTicker(const String& symbol, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    
    json request = {{"symbol", market.id}};
    json response = fetch("/api/pro/v1/ticker", "public", "GET",
                         this->extend(request, params));
    json ticker = response["data"];
    
    return {
        {"symbol", symbol},
        {"timestamp", ticker["timestamp"]},
        {"datetime", this->iso8601(ticker["timestamp"])},
        {"high", ticker["high"]},
        {"low", ticker["low"]},
        {"bid", ticker["bid"][0]},
        {"bidVolume", ticker["bid"][1]},
        {"ask", ticker["ask"][0]},
        {"askVolume", ticker["ask"][1]},
        {"vwap", nullptr},
        {"open", ticker["open"]},
        {"close", ticker["close"]},
        {"last", ticker["close"]},
        {"previousClose", nullptr},
        {"change", nullptr},
        {"percentage", nullptr},
        {"average", nullptr},
        {"baseVolume", ticker["volume"]},
        {"quoteVolume", ticker["volume"] * ticker["close"]},
        {"info", ticker}
    };
}

json AscendEX::fetchTickerImpl(const std::string& symbol) const {
    // https://ascendex.github.io/ascendex-pro-api/#ticker
    json request = {{"symbol", symbol}};
    json response = this->request("v1/ticker", "public", "GET", request);
    
    if (response["data"].empty()) {
        throw ExchangeError("No ticker data found for symbol " + symbol);
    }
    
    return this->parseTicker(response["data"][0]);
}

json AscendEX::parseTicker(const json& ticker, const std::optional<json>& market) const {
    long long timestamp = parse8601(ticker["ts"].get<std::string>());
    
    return {
        {"symbol", ticker["symbol"]},
        {"timestamp", timestamp},
        {"datetime", iso8601(timestamp)},
        {"high", ticker.contains("h") ? std::stod(ticker["h"].get<std::string>()) : nullptr},
        {"low", ticker.contains("l") ? std::stod(ticker["l"].get<std::string>()) : nullptr},
        {"bid", ticker.contains("b") ? std::stod(ticker["b"].get<std::string>()) : nullptr},
        {"bidVolume", ticker.contains("bs") ? std::stod(ticker["bs"].get<std::string>()) : nullptr},
        {"ask", ticker.contains("a") ? std::stod(ticker["a"].get<std::string>()) : nullptr},
        {"askVolume", ticker.contains("as") ? std::stod(ticker["as"].get<std::string>()) : nullptr},
        {"vwap", nullptr},
        {"open", ticker.contains("o") ? std::stod(ticker["o"].get<std::string>()) : nullptr},
        {"close", ticker.contains("c") ? std::stod(ticker["c"].get<std::string>()) : nullptr},
        {"last", ticker.contains("c") ? std::stod(ticker["c"].get<std::string>()) : nullptr},
        {"previousClose", nullptr},
        {"change", nullptr},
        {"percentage", nullptr},
        {"average", nullptr},
        {"baseVolume", ticker.contains("v") ? std::stod(ticker["v"].get<std::string>()) : nullptr},
        {"quoteVolume", nullptr},
        {"info", ticker}
    };
}

json AscendEX::fetchBalanceImpl() const {
    // https://ascendex.github.io/ascendex-pro-api/#cash-account-balance
    // https://ascendex.github.io/ascendex-pro-api/#margin-account-balance
    // https://ascendex.github.io/ascendex-futures-pro-api-v2/#position
    std::string accountCategory = this->getAccountCategory();
    std::string accountGroup = this->getAccountGroup();
    
    json response;
    if (accountCategory == "cash") {
        response = this->request("v1/cash/balance", "private", "GET");
        return this->parseBalance(response);
    } else if (accountCategory == "margin") {
        response = this->request("v1/margin/balance", "private", "GET");
        return this->parseMarginBalance(response);
    } else if (accountCategory == "futures") {
        response = this->request("v1/futures/position", "private", "GET");
        return this->parseSwapBalance(response);
    }
    
    throw ExchangeError("Invalid account category: " + accountCategory);
}

json AscendEX::parseBalance(const json& response) const {
    json data = response["data"];
    json result = {
        {"info", response},
        {"timestamp", nullptr},
        {"datetime", nullptr}
    };
    
    for (const auto& balance : data) {
        std::string currencyId = balance["asset"].get<std::string>();
        std::string code = this->safeCurrencyCode(currencyId);
        std::string account = {
            {"free", std::stod(balance["availableBalance"].get<std::string>())},
            {"used", std::stod(balance["totalOrderBalance"].get<std::string>())},
            {"total", std::stod(balance["totalBalance"].get<std::string>())}
        };
        result[code] = account;
    }
    
    return result;
}

json AscendEX::parseMarginBalance(const json& response) const {
    json data = response["data"];
    json result = {
        {"info", response},
        {"timestamp", nullptr},
        {"datetime", nullptr}
    };
    
    for (const auto& balance : data) {
        std::string currencyId = balance["asset"].get<std::string>();
        std::string code = this->safeCurrencyCode(currencyId);
        std::string account = {
            {"free", std::stod(balance["availableBalance"].get<std::string>())},
            {"used", std::stod(balance["totalOrderBalance"].get<std::string>())},
            {"total", std::stod(balance["totalBalance"].get<std::string>())},
            {"debt", std::stod(balance["borrowed"].get<std::string>())}
        };
        result[code] = account;
    }
    
    return result;
}

json AscendEX::parseSwapBalance(const json& response) const {
    json data = response["data"];
    json result = {
        {"info", response},
        {"timestamp", nullptr},
        {"datetime", nullptr}
    };
    
    for (const auto& balance : data) {
        std::string currencyId = balance["collateralCurrency"].get<std::string>();
        std::string code = this->safeCurrencyCode(currencyId);
        std::string account = {
            {"free", std::stod(balance["freeCollateral"].get<std::string>())},
            {"used", std::stod(balance["totalCollateral"].get<std::string>()) - std::stod(balance["freeCollateral"].get<std::string>())},
            {"total", std::stod(balance["totalCollateral"].get<std::string>())}
        };
        result[code] = account;
    }
    
    return result;
}

std::string AscendEX::getAccountCategory(const json& params) const {
    std::string accountCategory = "cash";  // default to cash account
    if (params.contains("type")) {
        std::string type = params["type"].get<std::string>();
        if (type == "margin") {
            accountCategory = "margin";
        } else if (type == "swap") {
            accountCategory = "futures";
        }
    }
    return accountCategory;
}

json AscendEX::createOrder(const String& symbol, const String& type,
                          const String& side, double amount,
                          double price, const json& params) {
    this->loadMarkets();
    Market market = this->market(symbol);
    String accountCategory = this->getAccountCategory(params);
    
    json request = {
        {"symbol", market.id},
        {"orderQty", this->amountToPrecision(symbol, amount)},
        {"side", side.upper()},
        {"orderType", type.upper()}
    };
    
    if (type == "limit") {
        if (price == 0) {
            throw InvalidOrder("For limit orders, price cannot be zero");
        }
        request["orderPrice"] = this->priceToPrecision(symbol, price);
    }
    
    String path = accountCategory == "futures" ? 
        "/api/pro/v1/futures/order" : "/api/pro/v1/order";
    
    json response = fetch(path, "private", "POST",
                         this->extend(request, params));
    return this->parseOrder(response["data"], market);
}

json AscendEX::createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side,
                           double amount, const std::optional<double>& price) {
    // https://ascendex.github.io/ascendex-pro-api/#place-order
    // https://ascendex.github.io/ascendex-futures-pro-api-v2/#new-order
    std::string accountCategory = this->getAccountCategory();
    
    json request = {
        {"symbol", symbol},
        {"orderQty", std::to_string(amount)},
        {"side", side},
        {"orderType", type}
    };
    
    if (type == "limit") {
        if (!price.has_value()) {
            throw ArgumentsRequired("Limit orders require a price");
        }
        request["orderPrice"] = std::to_string(price.value());
    }
    
    // Generate client order id
    request["id"] = uuid();
    
    // Default time in force to GTC (Good Till Cancelled)
    request["timeInForce"] = "GTC";
    
    std::string path = accountCategory == "futures" ? "v2/futures/order" : "v2/order";
    json response = this->request(path, "private", "POST", {}, request);
    
    return this->parseOrder(response["data"]);
}

json AscendEX::parseOrder(const json& order, const std::optional<json>& market) const {
    std::string id = order["orderId"].get<std::string>();
    std::string clientOrderId = order["clientOrderId"].get<std::string>();
    long long timestamp = parse8601(order["createTime"].get<std::string>());
    long long lastTradeTimestamp = order.contains("lastExecTime") ? parse8601(order["lastExecTime"].get<std::string>()) : 0;
    
    std::string symbol = order["symbol"].get<std::string>();
    std::string type = order["orderType"].get<std::string>();
    std::string side = order["side"].get<std::string>();
    std::string status = this->parseOrderStatus(order["status"].get<std::string>());
    
    double amount = std::stod(order["orderQty"].get<std::string>());
    double filled = std::stod(order["cumFilledQty"].get<std::string>());
    double remaining = amount - filled;
    
    double price = 0;
    if (order.contains("orderPrice") && !order["orderPrice"].is_null()) {
        price = std::stod(order["orderPrice"].get<std::string>());
    }
    
    double cost = 0;
    if (filled > 0 && order.contains("avgPx") && !order["avgPx"].is_null()) {
        cost = filled * std::stod(order["avgPx"].get<std::string>());
    }
    
    json fee = nullptr;
    if (order.contains("cumFee") && !order["cumFee"].is_null()) {
        fee = {
            {"cost", std::stod(order["cumFee"].get<std::string>())},
            {"currency", order["feeAsset"].get<std::string>()}
        };
    }
    
    return {
        {"id", id},
        {"clientOrderId", clientOrderId},
        {"info", order},
        {"timestamp", timestamp},
        {"datetime", iso8601(timestamp)},
        {"lastTradeTimestamp", lastTradeTimestamp},
        {"symbol", symbol},
        {"type", type},
        {"timeInForce", order.contains("timeInForce") ? order["timeInForce"].get<std::string>() : nullptr},
        {"side", side},
        {"price", price},
        {"amount", amount},
        {"cost", cost},
        {"average", cost > 0 ? cost / filled : 0},
        {"filled", filled},
        {"remaining", remaining},
        {"status", status},
        {"fee", fee},
        {"trades", nullptr}
    };
}

std::string AscendEX::parseOrderStatus(const std::string& status) const {
    static const std::map<std::string, std::string> statuses = {
        {"PendingNew", "open"},
        {"New", "open"},
        {"PartiallyFilled", "open"},
        {"Filled", "closed"},
        {"Canceled", "canceled"},
        {"Rejected", "rejected"}
    };
    
    auto it = statuses.find(status);
    if (it != statuses.end()) {
        return it->second;
    }
    return status;
}

json AscendEX::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    // https://ascendex.github.io/ascendex-pro-api/#cancel-order
    // https://ascendex.github.io/ascendex-futures-pro-api-v2/#cancel-order
    if (symbol.empty()) {
        throw ArgumentsRequired("Symbol is required for canceling an order");
    }
    
    std::string accountCategory = this->getAccountCategory();
    std::string path = accountCategory == "futures" ? "v2/futures/order" : "v2/order";
    
    json request = {
        {"symbol", symbol},
        {"orderId", id},
        {"time", std::to_string(currentTimestamp())}
    };
    
    json response = this->request(path, "private", "DELETE", request);
    return this->parseOrder(response["data"]);
}

json AscendEX::cancelAllOrdersImpl(const std::string& symbol) {
    // https://ascendex.github.io/ascendex-pro-api/#cancel-all-orders
    // https://ascendex.github.io/ascendex-futures-pro-api-v2/#cancel-all-open-orders
    std::string accountCategory = this->getAccountCategory();
    std::string path = accountCategory == "futures" ? "v2/futures/order/all" : "v2/order/all";
    
    json request = {
        {"time", std::to_string(currentTimestamp())}
    };
    
    if (!symbol.empty()) {
        request["symbol"] = symbol;
    }
    
    json response = this->request(path, "private", "DELETE", request);
    return response;
}

json AscendEX::fetchOrderImpl(const std::string& id, const std::string& symbol) const {
    // https://ascendex.github.io/ascendex-pro-api/#query-order
    // https://ascendex.github.io/ascendex-futures-pro-api-v2/#query-order-by-id
    if (symbol.empty()) {
        throw ArgumentsRequired("Symbol is required for fetching an order");
    }
    
    std::string accountCategory = this->getAccountCategory();
    std::string path = accountCategory == "futures" ? "v2/futures/order/status" : "v2/order/status";
    
    json request = {
        {"symbol", symbol},
        {"orderId", id},
        {"time", std::to_string(currentTimestamp())}
    };
    
    json response = this->request(path, "private", "GET", request);
    return this->parseOrder(response["data"]);
}

json AscendEX::fetchOpenOrdersImpl(const std::string& symbol,
                                const std::optional<long long>& since,
                                const std::optional<int>& limit) const {
    // https://ascendex.github.io/ascendex-pro-api/#list-open-orders
    // https://ascendex.github.io/ascendex-futures-pro-api-v2/#list-open-orders
    std::string accountCategory = this->getAccountCategory();
    std::string path = accountCategory == "futures" ? "v2/futures/order/open" : "v2/order/open";
    
    json request;
    if (!symbol.empty()) {
        request["symbol"] = symbol;
    }
    
    if (since.has_value()) {
        request["startTime"] = std::to_string(since.value());
    }
    
    if (limit.has_value()) {
        request["limit"] = std::to_string(limit.value());
    }
    
    json response = this->request(path, "private", "GET", request);
    return this->parseOrders(response["data"], symbol, since, limit);
}

json AscendEX::fetchClosedOrdersImpl(const std::string& symbol,
                                  const std::optional<long long>& since,
                                  const std::optional<int>& limit) const {
    // https://ascendex.github.io/ascendex-pro-api/#list-history-orders-v2
    // https://ascendex.github.io/ascendex-futures-pro-api-v2/#list-current-history-orders
    std::string accountCategory = this->getAccountCategory();
    std::string path = accountCategory == "futures" ? "v2/futures/order/hist/current" : "v2/order/hist/current";
    
    json request;
    if (!symbol.empty()) {
        request["symbol"] = symbol;
    }
    
    if (since.has_value()) {
        request["startTime"] = std::to_string(since.value());
    }
    
    if (limit.has_value()) {
        request["limit"] = std::to_string(limit.value());
    }
    
    json response = this->request(path, "private", "GET", request);
    return this->parseOrders(response["data"], symbol, since, limit);
}

json AscendEX::parseOrders(const json& orders, const std::string& symbol,
                        const std::optional<long long>& since,
                        const std::optional<int>& limit) const {
    json result = json::array();
    
    for (const auto& order : orders) {
        if (symbol.empty() || order["symbol"] == symbol) {
            result.push_back(this->parseOrder(order));
        }
    }
    
    return result;
}

json AscendEX::fetchTradesImpl(const std::string& symbol, const std::optional<long long>& since,
                            const std::optional<int>& limit) const {
    // https://ascendex.github.io/ascendex-pro-api/#market-trades
    if (symbol.empty()) {
        throw ArgumentsRequired("Symbol is required for fetching trades");
    }
    
    json request = {{"symbol", symbol}};
    
    if (since.has_value()) {
        request["startTime"] = std::to_string(since.value());
    }
    
    if (limit.has_value()) {
        request["limit"] = std::to_string(limit.value());
    }
    
    json response = this->request("v1/trades", "public", "GET", request);
    return this->parseTrades(response["data"], symbol, since, limit);
}

json AscendEX::parseTrade(const json& trade, const std::optional<json>& market) const {
    long long timestamp = parse8601(trade["timestamp"].get<std::string>());
    std::string id = trade["seqnum"].get<std::string>();
    std::string orderId = trade.contains("orderId") ? trade["orderId"].get<std::string>() : "";
    std::string symbol = trade["symbol"].get<std::string>();
    std::string side = trade["side"].get<std::string>();
    double price = std::stod(trade["price"].get<std::string>());
    double amount = std::stod(trade["size"].get<std::string>());
    double cost = price * amount;
    
    json fee = nullptr;
    if (trade.contains("fee")) {
        fee = {
            {"cost", std::stod(trade["fee"].get<std::string>())},
            {"currency", trade["feeAsset"].get<std::string>()}
        };
    }
    
    return {
        {"id", id},
        {"info", trade},
        {"timestamp", timestamp},
        {"datetime", iso8601(timestamp)},
        {"symbol", symbol},
        {"order", orderId},
        {"type", "limit"},  // AscendEX doesn't provide this info in trades
        {"side", side},
        {"takerOrMaker", trade.contains("isBuyerMaker") ? (trade["isBuyerMaker"].get<bool>() ? "maker" : "taker") : nullptr},
        {"price", price},
        {"amount", amount},
        {"cost", cost},
        {"fee", fee}
    };
}

json AscendEX::parseTrades(const json& trades, const std::string& symbol,
                        const std::optional<long long>& since,
                        const std::optional<int>& limit) const {
    json result = json::array();
    
    for (const auto& trade : trades) {
        if (symbol.empty() || trade["symbol"] == symbol) {
            result.push_back(this->parseTrade(trade));
        }
    }
    
    return result;
}

json AscendEX::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
    // https://ascendex.github.io/ascendex-pro-api/#depth
    if (symbol.empty()) {
        throw ArgumentsRequired("Symbol is required for fetching order book");
    }
    
    json request = {{"symbol", symbol}};
    
    if (limit.has_value()) {
        request["limit"] = std::to_string(limit.value());
    }
    
    json response = this->request("v1/depth", "public", "GET", request);
    return this->parseOrderBook(response["data"], symbol, limit);
}

json AscendEX::parseOrderBook(const json& orderbook, const std::string& symbol,
                          const std::optional<int>& limit) const {
    long long timestamp = parse8601(orderbook["ts"].get<std::string>());
    
    json result = {
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", iso8601(timestamp)},
        {"nonce", orderbook["seqnum"].get<long long>()},
        {"bids", json::array()},
        {"asks", json::array()}
    };
    
    for (const auto& bid : orderbook["bids"]) {
        result["bids"].push_back({
            std::stod(bid[0].get<std::string>()),  // price
            std::stod(bid[1].get<std::string>())   // amount
        });
    }
    
    for (const auto& ask : orderbook["asks"]) {
        result["asks"].push_back({
            std::stod(ask[0].get<std::string>()),  // price
            std::stod(ask[1].get<std::string>())   // amount
        });
    }
    
    return result;
}

json AscendEX::fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe,
                           const std::optional<long long>& since,
                           const std::optional<int>& limit) const {
    // https://ascendex.github.io/ascendex-pro-api/#bar-history
    if (symbol.empty()) {
        throw ArgumentsRequired("Symbol is required for fetching OHLCV data");
    }
    
    if (timeframes.find(timeframe) == timeframes.end()) {
        throw BadRequest("Timeframe " + timeframe + " is not supported");
    }
    
    json request = {
        {"symbol", symbol},
        {"interval", timeframes[timeframe]}
    };
    
    if (since.has_value()) {
        request["from"] = std::to_string(since.value());
    }
    
    if (limit.has_value()) {
        request["limit"] = std::to_string(limit.value());
    }
    
    json response = this->request("v1/barhist", "public", "GET", request);
    return this->parseOHLCVs(response["data"], symbol, timeframe, since, limit);
}

json AscendEX::parseOHLCV(const json& ohlcv, const std::optional<json>& market) const {
    return {
        parse8601(ohlcv["t"].get<std::string>()),                 // timestamp
        std::stod(ohlcv["o"].get<std::string>()),                // open
        std::stod(ohlcv["h"].get<std::string>()),                // high
        std::stod(ohlcv["l"].get<std::string>()),                // low
        std::stod(ohlcv["c"].get<std::string>()),                // close
        std::stod(ohlcv["v"].get<std::string>())                 // volume
    };
}

json AscendEX::parseOHLCVs(const json& ohlcvs, const std::string& symbol,
                        const std::string& timeframe,
                        const std::optional<long long>& since,
                        const std::optional<int>& limit) const {
    json result = json::array();
    
    for (const auto& ohlcv : ohlcvs) {
        result.push_back(this->parseOHLCV(ohlcv));
    }
    
    return result;
}

std::string AscendEX::sign(const std::string& path, const std::string& api,
                        const std::string& method, const json& params,
                        const std::map<std::string, std::string>& headers,
                        const json& body) const {
    std::string url = this->urls["api"]["rest"] + "/" + this->version + "/" + path;
    std::string query = "";
    
    if (method == "GET") {
        if (!params.empty()) {
            query = "?" + this->urlencode(params);
            url += query;
        }
    }
    
    std::map<std::string, std::string> auth_headers = headers;
    
    if (api == "private") {
        if (this->config_.apiKey.empty() || this->config_.secret.empty()) {
            throw AuthenticationError("API key and secret are required for private endpoints");
        }
        
        std::string timestamp = std::to_string(currentTimestamp());
        std::string prehash = timestamp + " " + method + " /" + this->version + "/" + path;
        
        if (method == "GET") {
            if (!params.empty()) {
                prehash += query;
            }
        } else {
            if (!body.empty()) {
                prehash += this->json_encode(body);
            }
        }
        
        std::string signature = this->hmac(prehash, this->config_.secret, "sha256", true);
        
        auth_headers["x-auth-key"] = this->config_.apiKey;
        auth_headers["x-auth-signature"] = signature;
        auth_headers["x-auth-timestamp"] = timestamp;
        
        if (!this->uid.empty()) {
            auth_headers["x-auth-coid"] = this->uid;
        }
    }
    
    return url;
}

} // namespace ccxt
