#include "ccxt/exchanges/alpaca.h"

namespace ccxt {

const std::string alpaca::defaultHostname = "https://paper-api.alpaca.markets";
const int alpaca::defaultRateLimit = 333;  // 3 req/s for free tier
const bool alpaca::defaultPro = true;

alpaca::alpaca(const Config& config) : Exchange(config) {
    init();
}

void alpaca::init() {
    this->id = "alpaca";
    this->name = "Alpaca";
    this->countries = {"US"};
    this->rateLimit = defaultRateLimit;
    this->pro = defaultPro;
    this->hostname = "alpaca.markets";

    this->urls = {
        {"logo", "https://github.com/user-attachments/assets/e9476df8-a450-4c3e-ab9a-1a7794219e1b"},
        {"www", "https://alpaca.markets"},
        {"api", {
            {"broker", "https://broker-api." + this->hostname},
            {"trader", "https://api." + this->hostname},
            {"market", "https://data." + this->hostname}
        }},
        {"test", {
            {"broker", "https://broker-api.sandbox." + this->hostname},
            {"trader", "https://paper-api." + this->hostname},
            {"market", "https://data.sandbox." + this->hostname}
        }},
        {"doc", "https://alpaca.markets/docs/"},
        {"fees", "https://docs.alpaca.markets/docs/crypto-fees"}
    };

    this->has = {
        {"CORS", false},
        {"spot", true},
        {"margin", false},
        {"swap", false},
        {"future", false},
        {"option", false},
        {"cancelAllOrders", true},
        {"cancelOrder", true},
        {"closeAllPositions", false},
        {"closePosition", false},
        {"createOrder", true},
        {"createStopOrder", true},
        {"createTriggerOrder", true},
        {"editOrder", true},
        {"fetchBalance", false},
        {"fetchBidsAsks", false},
        {"fetchClosedOrders", true},
        {"fetchCurrencies", false},
        {"fetchDepositAddress", true},
        {"fetchDepositAddressesByNetwork", false},
        {"fetchDeposits", true},
        {"fetchDepositsWithdrawals", true},
        {"fetchFundingHistory", false},
        {"fetchFundingRate", false},
        {"fetchFundingRateHistory", false},
        {"fetchFundingRates", false},
        {"fetchL1OrderBook", true},
        {"fetchL2OrderBook", false},
        {"fetchMarkets", true},
        {"fetchMyTrades", true},
        {"fetchOHLCV", true},
        {"fetchOpenOrder", false},
        {"fetchOpenOrders", true},
        {"fetchOrder", true},
        {"fetchOrderBook", true},
        {"fetchOrders", true},
        {"fetchPosition", false},
        {"fetchPositionHistory", false},
        {"fetchPositionMode", false},
        {"fetchPositions", false},
        {"fetchPositionsForSymbol", false},
        {"fetchPositionsHistory", false},
        {"fetchPositionsRisk", false},
        {"fetchStatus", false},
        {"fetchTicker", true},
        {"fetchTickers", true},
        {"fetchTime", true},
        {"fetchTrades", true},
        {"fetchTradingFee", false},
        {"fetchTradingFees", false},
        {"fetchTransactionFees", false},
        {"fetchTransactions", false},
        {"fetchTransfers", false},
        {"fetchWithdrawals", true},
        {"sandbox", true},
        {"setLeverage", false},
        {"setMarginMode", false},
        {"transfer", false},
        {"withdraw", true}
    };

    this->timeframes = {
        {"1m", "1Min"},
        {"5m", "5Min"},
        {"15m", "15Min"},
        {"1h", "1Hour"},
        {"1d", "1Day"}
    };

    // API Endpoints
    this->api = {
        {"broker", {}},
        {"trader", {
            {"private", {
                {"get", {
                    "v2/account",
                    "v2/orders",
                    "v2/orders/{order_id}",
                    "v2/positions",
                    "v2/positions/{symbol_or_asset_id}",
                    "v2/account/portfolio/history",
                    "v2/watchlists",
                    "v2/watchlists/{watchlist_id}",
                    "v2/watchlists:by_name",
                    "v2/account/configurations",
                    "v2/account/activities",
                    "v2/account/activities/{activity_type}",
                    "v2/calendar",
                    "v2/clock",
                    "v2/assets",
                    "v2/assets/{symbol_or_asset_id}",
                    "v2/corporate_actions/announcements/{id}",
                    "v2/corporate_actions/announcements",
                    "v2/wallets",
                    "v2/wallets/transfers"
                }},
                {"post", {
                    "v2/orders",
                    "v2/watchlists",
                    "v2/watchlists/{watchlist_id}",
                    "v2/watchlists:by_name",
                    "v2/wallets/transfers"
                }},
                {"put", {
                    "v2/orders/{order_id}",
                    "v2/watchlists/{watchlist_id}",
                    "v2/watchlists:by_name"
                }},
                {"patch", {
                    "v2/orders/{order_id}",
                    "v2/account/configurations"
                }},
                {"delete", {
                    "v2/orders",
                    "v2/orders/{order_id}",
                    "v2/positions",
                    "v2/positions/{symbol_or_asset_id}",
                    "v2/watchlists/{watchlist_id}",
                    "v2/watchlists:by_name",
                    "v2/watchlists/{watchlist_id}/{symbol}"
                }}
            }}
        }},
        {"market", {
            {"public", {
                {"get", {
                    "v1beta3/crypto/{loc}/bars",
                    "v1beta3/crypto/{loc}/latest/bars",
                    "v1beta3/crypto/{loc}/latest/orderbooks",
                    "v1beta3/crypto/{loc}/latest/quotes",
                    "v1beta3/crypto/{loc}/latest/trades",
                    "v1beta3/crypto/{loc}/quotes",
                    "v1beta3/crypto/{loc}/snapshots",
                    "v1beta3/crypto/{loc}/trades"
                }}
            }},
            {"private", {
                {"get", {
                    "v1beta1/corporate-actions",
                    "v1beta1/forex/latest/rates",
                    "v1beta1/forex/rates",
                    "v1beta1/logos/{symbol}",
                    "v1beta1/news",
                    "v1beta1/screener/stocks/most-actives",
                    "v1beta1/screener/{market_type}/movers",
                    "v2/stocks/auctions",
                    "v2/stocks/bars",
                    "v2/stocks/bars/latest",
                    "v2/stocks/meta/conditions/{ticktype}",
                    "v2/stocks/meta/exchanges",
                    "v2/stocks/quotes",
                    "v2/stocks/quotes/latest",
                    "v2/stocks/snapshots",
                    "v2/stocks/trades",
                    "v2/stocks/trades/latest",
                    "v2/stocks/{symbol}/auctions",
                    "v2/stocks/{symbol}/bars",
                    "v2/stocks/{symbol}/bars/latest"
                }}
            }}
        }}
    };
}

json alpaca::describeImpl() const {
    json result = ExchangeImpl::describeImpl();
    result["id"] = id;
    result["name"] = name;
    result["countries"] = countries;
    result["rateLimit"] = rateLimit;
    result["pro"] = pro;
    result["urls"] = urls;
    
    json hasJson;
    for (const auto& [key, value] : this->has) {
        if (value) {
            hasJson[key] = *value;
        }
    }
    result["has"] = hasJson;
    
    result["timeframes"] = timeframes;
    return result;
}

json alpaca::fetchMarketsImpl() const {
    // https://docs.alpaca.markets/reference/get-v2-assets
    json response = this->request("v2/assets", "trader", "private", "GET");
    return this->parseMarkets(response);
}

json alpaca::parseMarkets(const json& assets) const {
    json result = json::array();
    for (const auto& asset : assets) {
        if (asset["status"] == "active") {
            json market = {
                {"id", asset["id"]},
                {"symbol", asset["symbol"]},
                {"base", asset["base_currency"]},
                {"quote", asset["quote_currency"]},
                {"baseId", asset["base_currency"]},
                {"quoteId", asset["quote_currency"]},
                {"active", true},
                {"type", "spot"},
                {"spot", true},
                {"margin", false},
                {"swap", false},
                {"future", false},
                {"option", false},
                {"contract", false},
                {"settle", nullptr},
                {"settleId", nullptr},
                {"contractSize", nullptr},
                {"linear", nullptr},
                {"inverse", nullptr},
                {"expiry", nullptr},
                {"expiryDatetime", nullptr},
                {"strike", nullptr},
                {"optionType", nullptr},
                {"taker", std::stod(asset["taker_fee"])},
                {"maker", std::stod(asset["maker_fee"])},
                {"percentage", true},
                {"tierBased", false},
                {"feeSide", "quote"},
                {"precision", {
                    {"amount", asset["min_trade_increment"].get<double>()},
                    {"price", asset["min_price_increment"].get<double>()}
                }},
                {"limits", {
                    {"amount", {
                        {"min", asset["min_order_size"].get<double>()},
                        {"max", asset["max_order_size"].get<double>()}
                    }},
                    {"price", {
                        {"min", asset["min_price_increment"].get<double>()},
                        {"max", nullptr}
                    }},
                    {"cost", {
                        {"min", nullptr},
                        {"max", nullptr}
                    }}
                }},
                {"info", asset}
            };
            result.push_back(market);
        }
    }
    return result;
}

json alpaca::fetchTickerImpl(const std::string& symbol) const {
    // https://docs.alpaca.markets/reference/cryptosnapshots-1
    std::string loc = "us";  // default location
    json params = {{"loc", loc}};
    json response = this->request("v1beta3/crypto/{loc}/snapshots", "market", "public", "GET", params);
    if (response.empty()) {
        throw ExchangeError("No response from server");
    }
    return this->parseTicker(response[0]);
}

json alpaca::parseTicker(const json& ticker, const std::optional<json>& market) const {
    std::string timestamp = ticker["t"].get<std::string>();
    json result = {
        {"symbol", ticker["S"]},
        {"timestamp", parse8601(timestamp)},
        {"datetime", iso8601(parse8601(timestamp))},
        {"high", ticker["h"].get<double>()},
        {"low", ticker["l"].get<double>()},
        {"bid", ticker["bp"].get<double>()},
        {"bidVolume", ticker["bs"].get<double>()},
        {"ask", ticker["ap"].get<double>()},
        {"askVolume", ticker["as"].get<double>()},
        {"vwap", ticker["vw"].get<double>()},
        {"open", ticker["o"].get<double>()},
        {"close", ticker["c"].get<double>()},
        {"last", ticker["c"].get<double>()},
        {"previousClose", nullptr},
        {"change", nullptr},
        {"percentage", nullptr},
        {"average", nullptr},
        {"baseVolume", ticker["v"].get<double>()},
        {"quoteVolume", nullptr},
        {"info", ticker}
    };
    return result;
}

json alpaca::fetchTickersImpl(const std::vector<std::string>& symbols) const {
    // https://docs.alpaca.markets/reference/cryptosnapshots-1
    std::string loc = "us";  // default location
    json params = {{"loc", loc}};
    if (!symbols.empty()) {
        std::string symbolsStr;
        for (size_t i = 0; i < symbols.size(); ++i) {
            if (i > 0) symbolsStr += ",";
            symbolsStr += symbols[i];
        }
        params["symbols"] = symbolsStr;
    }
    
    json response = this->request("v1beta3/crypto/{loc}/snapshots", "market", "public", "GET", params);
    json result = {};
    for (const auto& ticker : response) {
        std::string symbol = ticker["S"].get<std::string>();
        result[symbol] = this->parseTicker(ticker);
    }
    return result;
}

json alpaca::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
    // https://docs.alpaca.markets/reference/cryptolatestorderbooks
    std::string loc = "us";  // default location
    json params = {{"loc", loc}, {"symbols", symbol}};
    json response = this->request("v1beta3/crypto/{loc}/latest/orderbooks", "market", "public", "GET", params);
    if (response.empty()) {
        throw ExchangeError("No response from server");
    }
    return this->parseOrderBook(response[0], symbol, limit);
}

json alpaca::parseOrderBook(const json& orderbook, const std::string& symbol, const std::optional<int>& limit) const {
    long long timestamp = parse8601(orderbook["t"].get<std::string>());
    json asks = json::array();
    json bids = json::array();
    
    // Parse asks
    for (const auto& ask : orderbook["a"]) {
        asks.push_back({
            ask["p"].get<double>(),  // price
            ask["s"].get<double>()   // size
        });
    }
    
    // Parse bids
    for (const auto& bid : orderbook["b"]) {
        bids.push_back({
            bid["p"].get<double>(),  // price
            bid["s"].get<double>()   // size
        });
    }
    
    // Sort asks ascending and bids descending
    std::sort(asks.begin(), asks.end(), [](const json& a, const json& b) {
        return a[0].get<double>() < b[0].get<double>();
    });
    std::sort(bids.begin(), bids.end(), [](const json& a, const json& b) {
        return a[0].get<double>() > b[0].get<double>();
    });
    
    // Apply limit if specified
    if (limit.has_value()) {
        if (asks.size() > limit.value()) {
            asks.erase(asks.begin() + limit.value(), asks.end());
        }
        if (bids.size() > limit.value()) {
            bids.erase(bids.begin() + limit.value(), bids.end());
        }
    }
    
    return {
        {"symbol", symbol},
        {"bids", bids},
        {"asks", asks},
        {"timestamp", timestamp},
        {"datetime", iso8601(timestamp)},
        {"nonce", nullptr}
    };
}

json alpaca::fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe,
                         const std::optional<long long>& since,
                         const std::optional<int>& limit) const {
    // https://docs.alpaca.markets/reference/cryptobars
    std::string loc = "us";  // default location
    json params = {{"loc", loc}, {"symbols", symbol}};
    
    // Add timeframe
    auto it = this->timeframes.find(timeframe);
    if (it == this->timeframes.end()) {
        throw BadRequest("Invalid timeframe: " + timeframe);
    }
    params["timeframe"] = it->second;
    
    // Add start time if specified
    if (since.has_value()) {
        params["start"] = iso8601(since.value());
    }
    
    // Add limit if specified
    if (limit.has_value()) {
        params["limit"] = limit.value();
    }
    
    json response = this->request("v1beta3/crypto/{loc}/bars", "market", "public", "GET", params);
    if (response.empty()) {
        return json::array();
    }
    
    json bars = response[0]["bars"];
    json result = json::array();
    for (const auto& bar : bars) {
        result.push_back(this->parseOHLCV(bar));
    }
    return result;
}

json alpaca::parseOHLCV(const json& ohlcv) const {
    return {
        parse8601(ohlcv["t"].get<std::string>()),  // timestamp
        ohlcv["o"].get<double>(),  // open
        ohlcv["h"].get<double>(),  // high
        ohlcv["l"].get<double>(),  // low
        ohlcv["c"].get<double>(),  // close
        ohlcv["v"].get<double>()   // volume
    };
}

json alpaca::createOrderImpl(const std::string& symbol, const std::string& type,
                         const std::string& side, double amount,
                         const std::optional<double>& price) {
    // https://docs.alpaca.markets/reference/postorder
    json request = {
        {"symbol", symbol},
        {"qty", std::to_string(amount)},
        {"side", side},
        {"type", type}
    };
    
    if (type == "limit") {
        if (!price.has_value()) {
            throw ArgumentsRequired("Limit orders require a price");
        }
        request["limit_price"] = std::to_string(price.value());
    }
    
    // Generate client order id
    request["client_order_id"] = uuid();
    
    // Default time in force to GTC (Good Till Cancelled)
    request["time_in_force"] = "gtc";
    
    json response = this->request("v2/orders", "trader", "private", "POST", {}, request);
    return this->parseOrder(response);
}

json alpaca::parseOrder(const json& order, const std::optional<json>& market) const {
    std::string id = order["id"].get<std::string>();
    std::string clientOrderId = order["client_order_id"].get<std::string>();
    std::string symbol = order["symbol"].get<std::string>();
    std::string type = order["type"].get<std::string>();
    std::string side = order["side"].get<std::string>();
    std::string status = this->parseOrderStatus(order["status"].get<std::string>());
    std::string timeInForce = this->parseTimeInForce(order["time_in_force"].get<std::string>());
    
    long long timestamp = parse8601(order["created_at"].get<std::string>());
    long long lastTradeTimestamp = order.contains("filled_at") ? parse8601(order["filled_at"].get<std::string>()) : 0;
    
    double amount = std::stod(order["qty"].get<std::string>());
    double filled = std::stod(order["filled_qty"].get<std::string>());
    double remaining = amount - filled;
    
    double price = 0;
    if (order.contains("limit_price") && !order["limit_price"].is_null()) {
        price = std::stod(order["limit_price"].get<std::string>());
    }
    
    double cost = 0;
    if (filled > 0 && order.contains("filled_avg_price") && !order["filled_avg_price"].is_null()) {
        cost = filled * std::stod(order["filled_avg_price"].get<std::string>());
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
        {"timeInForce", timeInForce},
        {"side", side},
        {"price", price},
        {"amount", amount},
        {"cost", cost},
        {"average", cost > 0 ? cost / filled : 0},
        {"filled", filled},
        {"remaining", remaining},
        {"status", status},
        {"fee", nullptr},
        {"trades", nullptr}
    };
}

std::string alpaca::parseOrderStatus(const std::string& status) const {
    static const std::map<std::string, std::string> statuses = {
        {"new", "open"},
        {"accepted", "open"},
        {"partially_filled", "open"},
        {"filled", "closed"},
        {"done_for_day", "closed"},
        {"canceled", "canceled"},
        {"expired", "expired"},
        {"replaced", "open"},
        {"pending_cancel", "open"},
        {"pending_replace", "open"},
        {"pending_new", "open"},
        {"accepted_for_bidding", "open"},
        {"stopped", "open"},
        {"rejected", "rejected"},
        {"suspended", "open"},
        {"calculated", "open"}
    };
    
    auto it = statuses.find(status);
    if (it != statuses.end()) {
        return it->second;
    }
    return status;
}

std::string alpaca::parseTimeInForce(const std::string& timeInForce) const {
    static const std::map<std::string, std::string> timeInForces = {
        {"day", "Day"},
        {"gtc", "GTC"},
        {"opg", "OPG"},
        {"cls", "CLS"},
        {"ioc", "IOC"},
        {"fok", "FOK"}
    };
    
    auto it = timeInForces.find(timeInForce);
    if (it != timeInForces.end()) {
        return it->second;
    }
    return timeInForce;
}

json alpaca::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    // https://docs.alpaca.markets/reference/deleteorderbyorderid
    json response = this->request("v2/orders/" + id, "trader", "private", "DELETE");
    return this->parseOrder(response);
}

json alpaca::fetchOrderImpl(const std::string& id, const std::string& symbol) const {
    // https://docs.alpaca.markets/reference/getorderbyorderid
    json response = this->request("v2/orders/" + id, "trader", "private", "GET");
    return this->parseOrder(response);
}

json alpaca::fetchOpenOrdersImpl(const std::string& symbol,
                             const std::optional<long long>& since,
                             const std::optional<int>& limit) const {
    // https://docs.alpaca.markets/reference/getallorders
    json params = {{"status", "open"}};
    if (!symbol.empty()) {
        params["symbol"] = symbol;
    }
    if (since.has_value()) {
        params["after"] = iso8601(since.value());
    }
    if (limit.has_value()) {
        params["limit"] = limit.value();
    }
    
    json response = this->request("v2/orders", "trader", "private", "GET", params);
    return this->parseOrders(response, symbol, since, limit);
}

json alpaca::parseOrders(const json& orders, const std::string& symbol,
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

json alpaca::fetchMyTradesImpl(const std::string& symbol,
                           const std::optional<long long>& since,
                           const std::optional<int>& limit) const {
    // https://docs.alpaca.markets/reference/getaccountactivitiesbyactivitytype-1
    json params = {{"activity_type", "FILL"}};
    if (!symbol.empty()) {
        params["symbol"] = symbol;
    }
    if (since.has_value()) {
        params["after"] = iso8601(since.value());
    }
    if (limit.has_value()) {
        params["limit"] = limit.value();
    }
    
    json response = this->request("v2/account/activities/{activity_type}", "trader", "private", "GET", params);
    return this->parseTrades(response, symbol, since, limit);
}

json alpaca::parseTrade(const json& trade, const std::optional<json>& market) const {
    std::string id = trade["id"].get<std::string>();
    std::string orderId = trade["order_id"].get<std::string>();
    std::string symbol = trade["symbol"].get<std::string>();
    std::string side = trade["side"].get<std::string>();
    long long timestamp = parse8601(trade["transaction_time"].get<std::string>());
    
    double price = std::stod(trade["price"].get<std::string>());
    double amount = std::stod(trade["qty"].get<std::string>());
    double cost = price * amount;
    
    json fee = nullptr;
    if (trade.contains("commission")) {
        fee = {
            {"cost", std::stod(trade["commission"].get<std::string>())},
            {"currency", "USD"}  // Alpaca uses USD for commissions
        };
    }
    
    return {
        {"id", id},
        {"info", trade},
        {"timestamp", timestamp},
        {"datetime", iso8601(timestamp)},
        {"symbol", symbol},
        {"order", orderId},
        {"type", "limit"},  // Alpaca doesn't provide this info in trades
        {"side", side},
        {"takerOrMaker", "taker"},  // Alpaca doesn't provide this info
        {"price", price},
        {"amount", amount},
        {"cost", cost},
        {"fee", fee}
    };
}

json alpaca::parseTrades(const json& trades, const std::string& symbol,
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

json alpaca::fetchOrderTradesImpl(const std::string& id, const std::string& symbol) const {
    // https://docs.alpaca.markets/reference/getaccountactivitiesbyactivitytype-1
    json params = {{"activity_type", "FILL"}, {"order_id", id}};
    json response = this->request("v2/account/activities/{activity_type}", "trader", "private", "GET", params);
    return this->parseTrades(response, symbol);
}

json alpaca::fetchBalanceImpl() const {
    // https://docs.alpaca.markets/reference/getaccount
    json response = this->request("v2/account", "trader", "private", "GET");
    
    json result = {
        {"info", response},
        {"timestamp", nullptr},
        {"datetime", nullptr}
    };
    
    // Parse account balance
    if (response.contains("cash") && !response["cash"].is_null()) {
        double cash = std::stod(response["cash"].get<std::string>());
        result["USD"] = {
            {"free", cash},
            {"used", 0.0},
            {"total", cash}
        };
    }
    
    // Parse portfolio value
    if (response.contains("portfolio_value") && !response["portfolio_value"].is_null()) {
        double portfolioValue = std::stod(response["portfolio_value"].get<std::string>());
        result["total"] = {
            {"USD", portfolioValue}
        };
    }
    
    return result;
}

} // namespace ccxt
