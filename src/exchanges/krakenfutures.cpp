#include "ccxt/exchanges/krakenfutures.h"
#include "../base/crypto.h"
#include "../base/error.h"
#include <sstream>
#include <iomanip>
#include <chrono>

namespace ccxt {

KrakenFutures::KrakenFutures(const ExchangeConfig& config) : Exchange(config) {
    // Initialize exchange-specific configurations
    this->id = "krakenfutures";
    this->name = "Kraken Futures";
    this->countries = {"US"};
    this->version = "v3";
    this->rateLimit = 600;
    this->pro = true;
    
    this->has = {
        {"CORS", nullptr},
        {"spot", false},
        {"margin", false},
        {"swap", true},
        {"future", true},
        {"option", false},
        {"cancelAllOrders", true},
        {"cancelAllOrdersAfter", true},
        {"cancelOrder", true},
        {"cancelOrders", true},
        {"createMarketOrder", false},
        {"createOrder", true},
        {"createStopOrder", true},
        {"createTriggerOrder", true},
        {"editOrder", true},
        {"fetchBalance", true},
        {"fetchCanceledOrders", true},
        {"fetchClosedOrders", true},
        {"fetchFundingRate", "emulated"},
        {"fetchFundingRateHistory", true},
        {"fetchFundingRates", true},
        {"fetchLeverage", true},
        {"fetchLeverages", true},
        {"fetchLeverageTiers", true},
        {"fetchMarketLeverageTiers", "emulated"},
        {"fetchMarkets", true},
        {"fetchMarkOHLCV", true},
        {"fetchMyTrades", true},
        {"fetchOHLCV", true},
        {"fetchOpenOrders", true},
        {"fetchOrder", false},
        {"fetchOrderBook", true},
        {"fetchOrders", false},
        {"fetchPositions", true},
        {"fetchTickers", true},
        {"fetchTrades", true},
        {"sandbox", true},
        {"setLeverage", true},
        {"setMarginMode", false},
        {"transfer", true}
    };

    this->urls = {
        {"logo", "https://user-images.githubusercontent.com/51840849/76173629-fc67fb00-61b1-11ea-84fe-f2de582f58a3.jpg"},
        {"www", "https://futures.kraken.com/"},
        {"doc", "https://support.kraken.com/hc/en-us/categories/360001806372-Futures-API"},
        {"fees", "https://support.kraken.com/hc/en-us/articles/360022835771-Transaction-fees-and-rebates"}
    };

    this->api = {
        {"public", {
            {"get", {
                "instruments",
                "instruments/{instrument_name}",
                "tickers",
                "tickers/{symbol}",
                "orderbook/{symbol}",
                "history",
                "history/{symbol}",
                "charts",
                "charts/{symbol}",
                "markets",
                "markets/{symbol}",
                "derivatives/api/v3/openapi.json"
            }}
        }},
        {"private", {
            {"get", {
                "accounts",
                "accounts/balances",
                "accounts/positions",
                "accounts/margins",
                "accounts/notifications",
                "accounts/overview",
                "accounts/pnl",
                "accounts/pnlhistory",
                "accounts/transfers",
                "accounts/withdrawals",
                "accounts/deposits",
                "wallets/accounts",
                "wallets/history",
                "orders",
                "orders/history",
                "fills",
                "fills/history",
                "sendorder",
                "cancelorder",
                "cancelallorders",
                "cancelallordersafter"
            }},
            {"post", {
                "sendorder",
                "withdrawal",
                "transfer",
                "cancelorder",
                "cancelallorders",
                "cancelallordersafter",
                "batchorder",
                "accounts/leverage"
            }}
        }}
    };
}

std::string KrakenFutures::getNonce() const {
    return std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

std::string KrakenFutures::getSignature(const std::string& path, const std::string& method,
                                      const std::string& nonce, const std::string& body) const {
    std::string message = nonce + method + path;
    if (!body.empty()) {
        message += body;
    }
    return hmac_sha512(message, this->config_.secret);
}

void KrakenFutures::handleErrors(const json& response) {
    if (!response.contains("result") || response["result"] != "success") {
        std::string error = response.contains("error") ? response["error"].get<std::string>() : "Unknown error";
        if (error.find("Invalid API key") != std::string::npos) {
            throw AuthenticationError(error);
        } else if (error.find("Insufficient funds") != std::string::npos) {
            throw InsufficientFunds(error);
        } else if (error.find("Order not found") != std::string::npos) {
            throw OrderNotFound(error);
        } else if (error.find("Rate limit exceeded") != std::string::npos) {
            throw RateLimitExceeded(error);
        } else if (error.find("Invalid nonce") != std::string::npos) {
            throw InvalidNonce(error);
        } else {
            throw ExchangeError(error);
        }
    }
}

// Async implementations
std::future<std::vector<Market>> KrakenFutures::fetchMarketsAsync(const Params& params) {
    return std::async(std::launch::async, [this, params]() {
        return this->fetchMarkets(params);
    });
}

std::future<OrderBook> KrakenFutures::fetchOrderBookAsync(const std::string& symbol, int limit, const Params& params) {
    return std::async(std::launch::async, [this, symbol, limit, params]() {
        return this->fetchOrderBook(symbol, limit, params);
    });
}

std::future<Ticker> KrakenFutures::fetchTickerAsync(const std::string& symbol, const Params& params) {
    return std::async(std::launch::async, [this, symbol, params]() {
        return this->fetchTicker(symbol, params);
    });
}

std::future<std::map<std::string, Ticker>> KrakenFutures::fetchTickersAsync(const std::vector<std::string>& symbols, const Params& params) {
    return std::async(std::launch::async, [this, symbols, params]() {
        return this->fetchTickers(symbols, params);
    });
}

std::future<std::vector<Trade>> KrakenFutures::fetchTradesAsync(const std::string& symbol, int since, int limit, const Params& params) {
    return std::async(std::launch::async, [this, symbol, since, limit, params]() {
        return this->fetchTrades(symbol, since, limit, params);
    });
}

std::future<std::vector<OHLCV>> KrakenFutures::fetchOHLCVAsync(const std::string& symbol, const std::string& timeframe, int since, int limit, const Params& params) {
    return std::async(std::launch::async, [this, symbol, timeframe, since, limit, params]() {
        return this->fetchOHLCV(symbol, timeframe, since, limit, params);
    });
}

std::future<Order> KrakenFutures::createOrderAsync(const std::string& symbol, const std::string& type, const std::string& side,
                                                 double amount, double price, const Params& params) {
    return std::async(std::launch::async, [this, symbol, type, side, amount, price, params]() {
        return this->createOrder(symbol, type, side, amount, price, params);
    });
}

std::future<Order> KrakenFutures::cancelOrderAsync(const std::string& id, const std::string& symbol, const Params& params) {
    return std::async(std::launch::async, [this, id, symbol, params]() {
        return this->cancelOrder(id, symbol, params);
    });
}

std::future<std::vector<Order>> KrakenFutures::fetchOpenOrdersAsync(const std::string& symbol, int since, int limit, const Params& params) {
    return std::async(std::launch::async, [this, symbol, since, limit, params]() {
        return this->fetchOpenOrders(symbol, since, limit, params);
    });
}

std::future<std::vector<Order>> KrakenFutures::fetchClosedOrdersAsync(const std::string& symbol, int since, int limit, const Params& params) {
    return std::async(std::launch::async, [this, symbol, since, limit, params]() {
        return this->fetchClosedOrders(symbol, since, limit, params);
    });
}

std::future<std::vector<Order>> KrakenFutures::fetchCanceledOrdersAsync(const std::string& symbol, int since, int limit, const Params& params) {
    return std::async(std::launch::async, [this, symbol, since, limit, params]() {
        return this->fetchCanceledOrders(symbol, since, limit, params);
    });
}

std::future<Balance> KrakenFutures::fetchBalanceAsync(const Params& params) {
    return std::async(std::launch::async, [this, params]() {
        return this->fetchBalance(params);
    });
}

std::future<std::vector<Position>> KrakenFutures::fetchPositionsAsync(const std::vector<std::string>& symbols, const Params& params) {
    return std::async(std::launch::async, [this, symbols, params]() {
        return this->fetchPositions(symbols, params);
    });
}

std::future<Leverage> KrakenFutures::setLeverageAsync(int leverage, const std::string& symbol, const Params& params) {
    return std::async(std::launch::async, [this, leverage, symbol, params]() {
        return this->setLeverage(leverage, symbol, params);
    });
}

std::future<std::vector<LeverageTier>> KrakenFutures::fetchLeverageTiersAsync(const std::vector<std::string>& symbols, const Params& params) {
    return std::async(std::launch::async, [this, symbols, params]() {
        return this->fetchLeverageTiers(symbols, params);
    });
}

std::future<std::vector<FundingRate>> KrakenFutures::fetchFundingRatesAsync(const std::vector<std::string>& symbols, const Params& params) {
    return std::async(std::launch::async, [this, symbols, params]() {
        return this->fetchFundingRates(symbols, params);
    });
}

std::future<FundingRate> KrakenFutures::fetchFundingRateAsync(const std::string& symbol, const Params& params) {
    return std::async(std::launch::async, [this, symbol, params]() {
        return this->fetchFundingRate(symbol, params);
    });
}

std::future<std::vector<FundingRateHistory>> KrakenFutures::fetchFundingRateHistoryAsync(const std::string& symbol, int since, int limit, const Params& params) {
    return std::async(std::launch::async, [this, symbol, since, limit, params]() {
        return this->fetchFundingRateHistory(symbol, since, limit, params);
    });
}

// Additional sync implementations
OrderBook KrakenFutures::fetchOrderBook(const std::string& symbol, int limit, const Params& params) {
    this->checkRequiredSymbol(symbol);
    auto request = params;
    request["symbol"] = symbol;
    if (limit > 0) {
        request["depth"] = std::min(limit, 100);
    }
    
    auto response = this->publicGetOrderbook(request);
    this->handleErrors(response);
    
    return this->parseOrderBook(response["orderBook"], symbol);
}

Ticker KrakenFutures::fetchTicker(const std::string& symbol, const Params& params) {
    this->checkRequiredSymbol(symbol);
    auto request = params;
    request["symbol"] = symbol;
    
    auto response = this->publicGetTickers(request);
    this->handleErrors(response);
    
    return this->parseTicker(response["tickers"][0], this->market(symbol));
}

std::map<std::string, Ticker> KrakenFutures::fetchTickers(const std::vector<std::string>& symbols, const Params& params) {
    auto response = this->publicGetTickers(params);
    this->handleErrors(response);
    
    std::map<std::string, Ticker> result;
    auto tickers = response["tickers"].get<std::vector<json>>();
    
    for (const auto& ticker : tickers) {
        auto parsedTicker = this->parseTicker(ticker);
        std::string symbol = parsedTicker.symbol;
        if (symbols.empty() || std::find(symbols.begin(), symbols.end(), symbol) != symbols.end()) {
            result[symbol] = parsedTicker;
        }
    }
    
    return result;
}

std::vector<Trade> KrakenFutures::fetchTrades(const std::string& symbol, int since, int limit, const Params& params) {
    this->checkRequiredSymbol(symbol);
    auto request = params;
    request["symbol"] = symbol;
    if (since > 0) {
        request["since"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    
    auto response = this->publicGetHistory(request);
    this->handleErrors(response);
    
    std::vector<Trade> trades;
    auto history = response["history"].get<std::vector<json>>();
    auto market = this->market(symbol);
    
    for (const auto& trade : history) {
        trades.push_back(this->parseTrade(trade, market));
    }
    
    return trades;
}

std::vector<OHLCV> KrakenFutures::fetchOHLCV(const std::string& symbol, const std::string& timeframe, int since, int limit, const Params& params) {
    this->checkRequiredSymbol(symbol);
    auto request = params;
    request["symbol"] = symbol;
    request["interval"] = timeframe;
    if (since > 0) {
        request["since"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    
    auto response = this->publicGetCharts(request);
    this->handleErrors(response);
    
    std::vector<OHLCV> result;
    auto candles = response["candles"].get<std::vector<json>>();
    
    for (const auto& candle : candles) {
        result.push_back(this->parseOHLCV(candle, this->market(symbol)));
    }
    
    return result;
}

// Trading Methods
Order KrakenFutures::createOrder(const std::string& symbol, const std::string& type, const std::string& side,
                               double amount, double price, const Params& params) {
    this->checkRequiredSymbol(symbol);
    auto market = this->market(symbol);
    
    auto request = Params{
        {"orderType", type},
        {"symbol", market.id},
        {"side", side},
        {"size", this->amountToPrecision(symbol, amount)}
    };

    if (type == "limit") {
        if (price <= 0) {
            throw InvalidOrder("Limit orders require a price");
        }
        request["limitPrice"] = this->priceToPrecision(symbol, price);
    }

    auto response = this->privatePostSendorder(this->extend(request, params));
    this->handleErrors(response);
    return this->parseOrder(response["sendStatus"], market);
}

Order KrakenFutures::cancelOrder(const std::string& id, const std::string& symbol, const Params& params) {
    this->checkRequiredSymbol(symbol);
    auto request = Params{
        {"order_id", id},
        {"symbol", this->market(symbol).id}
    };

    auto response = this->privatePostCancelorder(this->extend(request, params));
    this->handleErrors(response);
    return this->parseOrder(response["cancelStatus"], this->market(symbol));
}

std::vector<Order> KrakenFutures::fetchOpenOrders(const std::string& symbol, int since, int limit, const Params& params) {
    auto request = params;
    if (!symbol.empty()) {
        request["symbol"] = this->market(symbol).id;
    }
    if (since > 0) {
        request["start_time"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }

    auto response = this->privateGetOrders(request);
    this->handleErrors(response);
    
    std::vector<Order> orders;
    auto market = symbol.empty() ? nullptr : this->market(symbol);
    
    for (const auto& order : response["orders"].get<std::vector<json>>()) {
        if (order["status"].get<std::string>() == "open") {
            orders.push_back(this->parseOrder(order, market));
        }
    }
    
    return orders;
}

std::vector<Order> KrakenFutures::fetchClosedOrders(const std::string& symbol, int since, int limit, const Params& params) {
    auto request = params;
    if (!symbol.empty()) {
        request["symbol"] = this->market(symbol).id;
    }
    if (since > 0) {
        request["start_time"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }

    auto response = this->privateGetOrdersHistory(request);
    this->handleErrors(response);
    
    std::vector<Order> orders;
    auto market = symbol.empty() ? nullptr : this->market(symbol);
    
    for (const auto& order : response["orders"].get<std::vector<json>>()) {
        if (order["status"].get<std::string>() == "filled") {
            orders.push_back(this->parseOrder(order, market));
        }
    }
    
    return orders;
}

std::vector<Order> KrakenFutures::fetchCanceledOrders(const std::string& symbol, int since, int limit, const Params& params) {
    auto request = params;
    if (!symbol.empty()) {
        request["symbol"] = this->market(symbol).id;
    }
    if (since > 0) {
        request["start_time"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }

    auto response = this->privateGetOrdersHistory(request);
    this->handleErrors(response);
    
    std::vector<Order> orders;
    auto market = symbol.empty() ? nullptr : this->market(symbol);
    
    for (const auto& order : response["orders"].get<std::vector<json>>()) {
        if (order["status"].get<std::string>() == "cancelled") {
            orders.push_back(this->parseOrder(order, market));
        }
    }
    
    return orders;
}

// Account Methods
Balance KrakenFutures::fetchBalance(const Params& params) {
    auto response = this->privateGetAccountsBalances(params);
    this->handleErrors(response);
    return this->parseBalance(response["accounts"]);
}

std::vector<Position> KrakenFutures::fetchPositions(const std::vector<std::string>& symbols, const Params& params) {
    auto request = params;
    if (!symbols.empty()) {
        std::vector<std::string> marketIds;
        for (const auto& symbol : symbols) {
            marketIds.push_back(this->market(symbol).id);
        }
        request["symbols"] = marketIds;
    }

    auto response = this->privateGetAccountsPositions(request);
    this->handleErrors(response);
    
    std::vector<Position> positions;
    for (const auto& position : response["positions"].get<std::vector<json>>()) {
        positions.push_back(this->parsePosition(position));
    }
    
    return positions;
}

Leverage KrakenFutures::setLeverage(int leverage, const std::string& symbol, const Params& params) {
    this->checkRequiredSymbol(symbol);
    this->validateLeverageInput(leverage, symbol);
    
    auto request = Params{
        {"symbol", this->market(symbol).id},
        {"leverage", leverage}
    };

    auto response = this->privatePostAccountsLeverage(this->extend(request, params));
    this->handleErrors(response);
    
    return Leverage{
        .leverage = leverage,
        .symbol = symbol,
        .info = response
    };
}

std::vector<LeverageTier> KrakenFutures::fetchLeverageTiers(const std::vector<std::string>& symbols, const Params& params) {
    auto response = this->publicGetInstruments(params);
    this->handleErrors(response);
    
    std::vector<LeverageTier> tiers;
    auto instruments = response["instruments"].get<std::vector<json>>();
    
    for (const auto& instrument : instruments) {
        std::string symbol = instrument["symbol"].get<std::string>();
        if (symbols.empty() || std::find(symbols.begin(), symbols.end(), symbol) != symbols.end()) {
            tiers.push_back(this->parseLeverageTier(instrument));
        }
    }
    
    return tiers;
}

// Funding Methods
std::vector<FundingRate> KrakenFutures::fetchFundingRates(const std::vector<std::string>& symbols, const Params& params) {
    auto response = this->publicGetTickers(params);
    this->handleErrors(response);
    
    std::vector<FundingRate> rates;
    auto tickers = response["tickers"].get<std::vector<json>>();
    
    for (const auto& ticker : tickers) {
        std::string symbol = ticker["symbol"].get<std::string>();
        if (symbols.empty() || std::find(symbols.begin(), symbols.end(), symbol) != symbols.end()) {
            rates.push_back(this->parseFundingRate(ticker));
        }
    }
    
    return rates;
}

FundingRate KrakenFutures::fetchFundingRate(const std::string& symbol, const Params& params) {
    this->checkRequiredSymbol(symbol);
    auto rates = this->fetchFundingRates({symbol}, params);
    return rates[0];
}

std::vector<FundingRateHistory> KrakenFutures::fetchFundingRateHistory(const std::string& symbol, int since, int limit, const Params& params) {
    auto request = params;
    if (!symbol.empty()) {
        request["symbol"] = this->market(symbol).id;
    }
    if (since > 0) {
        request["start_time"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }

    auto response = this->publicGetFundingRateHistory(request);
    this->handleErrors(response);
    
    std::vector<FundingRateHistory> history;
    auto rates = response["rates"].get<std::vector<json>>();
    
    for (const auto& rate : rates) {
        history.push_back(FundingRateHistory{
            .symbol = symbol,
            .timestamp = rate["time"].get<int64_t>(),
            .datetime = this->iso8601(rate["time"].get<int64_t>()),
            .fundingRate = rate["fundingRate"].get<double>(),
            .info = rate
        });
    }
    
    return history;
}

// Parsing Methods
Order KrakenFutures::parseOrder(const json& order, const Market* market) {
    std::string id = order["orderId"].get<std::string>();
    std::string symbol = market ? market->symbol : order["symbol"].get<std::string>();
    int64_t timestamp = order["receivedTime"].get<int64_t>();
    std::string type = order["orderType"].get<std::string>();
    std::string side = order["side"].get<std::string>();
    std::string status = this->parseOrderStatus(order["status"].get<std::string>());
    double price = order.value("limitPrice", 0.0);
    double amount = order["size"].get<double>();
    double filled = order["filledSize"].get<double>();
    double remaining = amount - filled;
    double cost = price * filled;
    
    return Order{
        .id = id,
        .clientOrderId = order.value("cliOrdId", ""),
        .datetime = this->iso8601(timestamp),
        .timestamp = timestamp,
        .lastTradeTimestamp = order.value("lastUpdate", 0),
        .status = status,
        .symbol = symbol,
        .type = type,
        .timeInForce = this->parseTimeInForce(order.value("timeInForce", "")),
        .side = side,
        .price = price,
        .amount = amount,
        .cost = cost,
        .average = order.value("averagePrice", 0.0),
        .filled = filled,
        .remaining = remaining,
        .trades = std::vector<Trade>(),
        .fee = Fee{
            .cost = order.value("fee", 0.0),
            .currency = market ? market->quote : ""
        },
        .info = order
    };
}

Position KrakenFutures::parsePosition(const json& position, const Market* market) {
    std::string symbol = market ? market->symbol : position["symbol"].get<std::string>();
    std::string side = this->parsePositionSide(position["side"].get<std::string>());
    double notional = position["positionValue"].get<double>();
    double size = position["size"].get<double>();
    double collateral = position["collateral"].get<double>();
    double leverage = notional / collateral;
    
    return Position{
        .info = position,
        .symbol = symbol,
        .timestamp = position["timestamp"].get<int64_t>(),
        .datetime = this->iso8601(position["timestamp"].get<int64_t>()),
        .initialMargin = collateral,
        .initialMarginPercentage = 1.0 / leverage * 100,
        .maintenanceMargin = position["maintenanceMargin"].get<double>(),
        .maintenanceMarginPercentage = position["maintenanceMarginRatio"].get<double>() * 100,
        .entryPrice = position["entryPrice"].get<double>(),
        .notional = notional,
        .leverage = leverage,
        .unrealizedPnl = position["unrealizedPnl"].get<double>(),
        .contracts = size,
        .contractSize = market ? market->contractSize : 1,
        .marginRatio = position["marginRatio"].get<double>() * 100,
        .liquidationPrice = position["liquidationPrice"].get<double>(),
        .markPrice = position["markPrice"].get<double>(),
        .collateral = collateral,
        .side = side,
        .percentage = position["unrealizedPnlPercentage"].get<double>()
    };
}

std::string KrakenFutures::parseOrderStatus(const std::string& status) const {
    static const std::map<std::string, std::string> statuses = {
        {"open", "open"},
        {"filled", "closed"},
        {"cancelled", "canceled"},
        {"untriggered", "open"},
        {"triggered", "open"}
    };
    return statuses.count(status) ? statuses.at(status) : status;
}

std::string KrakenFutures::parseTimeInForce(const std::string& timeInForce) const {
    static const std::map<std::string, std::string> timeInForces = {
        {"GTC", "GTC"},
        {"GTD", "GTD"},
        {"IOC", "IOC"},
        {"FOK", "FOK"}
    };
    return timeInForces.count(timeInForce) ? timeInForces.at(timeInForce) : timeInForce;
}

std::string KrakenFutures::parsePositionSide(const std::string& side) const {
    if (side == "long") return "long";
    if (side == "short") return "short";
    return "none";
}

} // namespace ccxt
