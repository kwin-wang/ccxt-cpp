#include "ccxt/exchanges/currencycom.h"
#include "ccxt/base/json_helper.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <openssl/hmac.h>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace ccxt {

const std::string currencycom::defaultBaseURL = "https://api-adapter.backend.currency.com/api/v2";
const std::string currencycom::defaultVersion = "v2";
const int currencycom::defaultRateLimit = 100;
const bool currencycom::defaultPro = false;

ExchangeRegistry::Factory currencycom::factory(currencycom::createInstance);

currencycom::currencycom(const Config& config) : ExchangeImpl(config) {
    init();
}

void currencycom::init() {
    name = "Currency.com";
    id = "currencycom";
    version = defaultVersion;
    rateLimit = defaultRateLimit;
    pro = defaultPro;
    baseURL = defaultBaseURL;

    // Initialize URLs
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/83718672-36745c00-a63e-11ea-81a9-677b1f789a4d.jpg"},
        {"api", {
            {"public", "https://api-adapter.backend.currency.com/api/v2"},
            {"private", "https://api-adapter.backend.currency.com/api/v2"}
        }},
        {"www", "https://currency.com"},
        {"doc", {
            "https://currency.com/api",
            "https://currency.com/api-v2"
        }},
        {"fees", "https://currency.com/fees-charges"}
    };

    // Initialize API endpoints
    api = {
        {"public", {
            {"get", {
                "time",
                "exchangeInfo",
                "ticker/24hr",
                "ticker/price",
                "ticker/bookTicker",
                "depth",
                "trades",
                "klines"
            }}
        }},
        {"private", {
            {"get", {
                "account",
                "myTrades",
                "openOrders",
                "allOrders",
                "depositAddress",
                "depositHistory",
                "withdrawHistory"
            }},
            {"post", {
                "order",
                "order/test"
            }},
            {"delete", {
                "order",
                "openOrders"
            }}
        }}
    };

    // Initialize fees
    fees = {
        {"trading", {
            {"maker", 0.002},  // 0.2%
            {"taker", 0.002}   // 0.2%
        }}
    };

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

    precisionMode = DECIMAL_PLACES;
    
    requiredCredentials = {
        {"apiKey", true},
        {"secret", true}
    };
}

Json currencycom::describeImpl() const {
    return {
        {"id", id},
        {"name", name},
        {"countries", Json::array({"GB", "EU"})},
        {"rateLimit", rateLimit},
        {"pro", pro},
        {"has", {
            {"fetchMarkets", true},
            {"fetchCurrencies", true},
            {"fetchTicker", true},
            {"fetchTickers", true},
            {"fetchOrderBook", true},
            {"fetchOHLCV", true},
            {"fetchBalance", true},
            {"createOrder", true},
            {"cancelOrder", true},
            {"fetchOrder", true},
            {"fetchOpenOrders", true},
            {"fetchClosedOrders", true},
            {"fetchMyTrades", true},
            {"fetchLedger", true},
            {"fetchDepositAddress", true},
            {"fetchDeposits", true}
        }},
        {"timeframes", timeframes},
        {"urls", urls},
        {"api", api},
        {"fees", fees},
        {"precisionMode", precisionMode}
    };
}

Json currencycom::fetchMarketsImpl() const {
    auto response = get("exchangeInfo");
    auto markets = Json::array();
    auto symbols = response["symbols"].array_items();
    
    for (const auto& market : symbols) {
        markets.push_back({
            {"id", market["symbol"].string_value()},
            {"symbol", market["baseAsset"].string_value() + "/" + market["quoteAsset"].string_value()},
            {"base", market["baseAsset"].string_value()},
            {"quote", market["quoteAsset"].string_value()},
            {"baseId", market["baseAsset"].string_value()},
            {"quoteId", market["quoteAsset"].string_value()},
            {"active", market["status"].string_value() == "TRADING"},
            {"precision", {
                {"amount", market["baseAssetPrecision"].int_value()},
                {"price", market["quotePrecision"].int_value()}
            }},
            {"limits", {
                {"amount", {
                    {"min", market["minQty"].number_value()},
                    {"max", market["maxQty"].number_value()}
                }},
                {"price", {
                    {"min", market["minPrice"].number_value()},
                    {"max", market["maxPrice"].number_value()}
                }},
                {"cost", {
                    {"min", market["minNotional"].number_value()},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    
    return markets;
}

Json currencycom::fetchTickerImpl(const std::string& symbol) const {
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"].string_value()}
    };
    auto response = get("ticker/24hr", request);
    return parseTicker(response, market);
}

Json currencycom::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"].string_value()}
    };
    if (limit) {
        request["limit"] = *limit;
    }
    auto response = get("depth", request);
    return parseOrderBook(response, symbol);
}

Json currencycom::createOrderImpl(const std::string& symbol, const std::string& type,
                                const std::string& side, double amount,
                                const std::optional<double>& price) {
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"].string_value()},
        {"side", side},
        {"type", type},
        {"quantity", formatNumber(amount, market["precision"]["amount"].int_value())}
    };
    
    if (type == "LIMIT") {
        if (!price) {
            throw std::runtime_error("Price is required for limit orders");
        }
        request["price"] = formatNumber(*price, market["precision"]["price"].int_value());
        request["timeInForce"] = "GTC";
    }
    
    auto response = privatePost("order", request);
    return parseOrder(response, market);
}

Json currencycom::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"].string_value()},
        {"orderId", id}
    };
    return privateDelete("order", request);
}

Json currencycom::fetchBalanceImpl() const {
    auto response = privateGet("account");
    auto result = {
        {"info", response},
        {"timestamp", nullptr},
        {"datetime", nullptr}
    };
    
    auto balances = response["balances"].array_items();
    for (const auto& balance : balances) {
        auto currencyId = balance["asset"].string_value();
        auto code = this->safeCurrencyCode(currencyId);
        result[code] = {
            {"free", balance["free"].number_value()},
            {"used", balance["locked"].number_value()},
            {"total", balance["free"].number_value() + balance["locked"].number_value()}
        };
    }
    
    return result;
}

Json currencycom::parseTicker(const Json& ticker, const Json& market) const {
    auto timestamp = ticker["closeTime"].int_value();
    auto symbol = market ? market["symbol"].string_value() : "";
    
    return {
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", iso8601(timestamp)},
        {"high", ticker["highPrice"].number_value()},
        {"low", ticker["lowPrice"].number_value()},
        {"bid", ticker["bidPrice"].number_value()},
        {"bidVolume", ticker["bidQty"].number_value()},
        {"ask", ticker["askPrice"].number_value()},
        {"askVolume", ticker["askQty"].number_value()},
        {"vwap", ticker["weightedAvgPrice"].number_value()},
        {"open", ticker["openPrice"].number_value()},
        {"close", ticker["lastPrice"].number_value()},
        {"last", ticker["lastPrice"].number_value()},
        {"previousClose", ticker["prevClosePrice"].number_value()},
        {"change", ticker["priceChange"].number_value()},
        {"percentage", ticker["priceChangePercent"].number_value()},
        {"average", nullptr},
        {"baseVolume", ticker["volume"].number_value()},
        {"quoteVolume", ticker["quoteVolume"].number_value()},
        {"info", ticker}
    };
}

std::string currencycom::sign(const std::string& path, const std::string& api,
                             const std::string& method, const Json& params,
                             const Json& headers, const Json& body) const {
    auto url = urls["api"][api] + "/" + path;
    auto timestamp = std::to_string(milliseconds());
    
    if (api == "private") {
        checkRequiredCredentials();
        auto auth = timestamp + method + path;
        
        if (method == "GET" || method == "DELETE") {
            if (!params.empty()) {
                auto query = urlencode(params);
                url += "?" + query;
                auth += "?" + query;
            }
        } else {
            if (!params.empty()) {
                body = json(params);
                auth += body.dump();
            }
        }
        
        auto signature = hmac(auth, secret, "sha256", "hex");
        headers["X-MBX-APIKEY"] = apiKey;
        headers["X-MBX-TIMESTAMP"] = timestamp;
        headers["X-MBX-SIGNATURE"] = signature;
    } else {
        if (!params.empty()) {
            url += "?" + urlencode(params);
        }
    }
    
    return url;
}

void currencycom::handleErrors(const std::string& code, const std::string& reason,
                             const std::string& url, const std::string& method,
                             const Json& headers, const Json& body,
                             const Json& response,
                             const std::string& requestHeaders,
                             const std::string& requestBody) const {
    if (response.contains("code")) {
        auto errorCode = response["code"].string_value();
        auto message = response.contains("msg") ? response["msg"].string_value() : "";
        
        if (errorCode != "0" && errorCode != "200") {
            throw ExchangeError(id + " " + message);
        }
    }
}

Json currencycom::fetchCurrenciesImpl() const {
    auto response = get("exchangeInfo");
    auto result = Json::object();
    
    for (const auto& symbol : response["symbols"].array_items()) {
        auto baseId = symbol["baseAsset"].string_value();
        auto quoteId = symbol["quoteAsset"].string_value();
        
        if (!result.contains(baseId)) {
            result[baseId] = {
                {"id", baseId},
                {"code", this->safeCurrencyCode(baseId)},
                {"name", nullptr},
                {"active", true},
                {"fee", nullptr},
                {"precision", symbol["baseAssetPrecision"].int_value()},
                {"limits", {
                    {"amount", {
                        {"min", symbol["minQty"].number_value()},
                        {"max", symbol["maxQty"].number_value()}
                    }},
                    {"price", {
                        {"min", symbol["minPrice"].number_value()},
                        {"max", symbol["maxPrice"].number_value()}
                    }},
                    {"cost", {
                        {"min", symbol["minNotional"].number_value()},
                        {"max", nullptr}
                    }}
                }},
                {"info", symbol}
            };
        }
        
        if (!result.contains(quoteId)) {
            result[quoteId] = {
                {"id", quoteId},
                {"code", this->safeCurrencyCode(quoteId)},
                {"name", nullptr},
                {"active", true},
                {"fee", nullptr},
                {"precision", symbol["quotePrecision"].int_value()},
                {"limits", {
                    {"amount", {
                        {"min", nullptr},
                        {"max", nullptr}
                    }},
                    {"price", {
                        {"min", nullptr},
                        {"max", nullptr}
                    }},
                    {"cost", {
                        {"min", nullptr},
                        {"max", nullptr}
                    }}
                }},
                {"info", symbol}
            };
        }
    }
    
    return result;
}

Json currencycom::fetchTickersImpl(const std::vector<std::string>& symbols) const {
    auto response = get("ticker/24hr");
    auto result = Json::object();
    
    for (const auto& ticker : response.array_items()) {
        auto marketId = ticker["symbol"].string_value();
        auto market = this->safeMarket(marketId);
        auto symbol = market["symbol"].string_value();
        result[symbol] = this->parseTicker(ticker, market);
    }
    
    return this->filterByArray(result, "symbol", symbols);
}

Json currencycom::fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe,
                                const std::optional<long long>& since,
                                const std::optional<int>& limit) const {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"symbol", market["id"].string_value()},
        {"interval", this->timeframes[timeframe].string_value()}
    });
    
    if (since.has_value()) {
        request["startTime"] = *since;
    }
    if (limit.has_value()) {
        request["limit"] = *limit;
    }
    
    auto response = get("klines", request);
    return this->parseOHLCVs(response, market, timeframe, since, limit);
}

Json currencycom::fetchOrderImpl(const std::string& id, const std::string& symbol) const {
    auto market = this->market(symbol);
    auto request = Json::object({
        {"symbol", market["id"].string_value()},
        {"orderId", id}
    });
    
    auto response = privateGet("order", request);
    return this->parseOrder(response, market);
}

Json currencycom::fetchOpenOrdersImpl(const std::string& symbol,
                                    const std::optional<long long>& since,
                                    const std::optional<int>& limit) const {
    auto request = Json::object();
    auto market = symbol.empty() ? Json() : this->market(symbol);
    
    if (!symbol.empty()) {
        request["symbol"] = market["id"].string_value();
    }
    if (since.has_value()) {
        request["startTime"] = *since;
    }
    if (limit.has_value()) {
        request["limit"] = *limit;
    }
    
    auto response = privateGet("openOrders", request);
    return this->parseOrders(response, market, since, limit);
}

Json currencycom::fetchClosedOrdersImpl(const std::string& symbol,
                                      const std::optional<long long>& since,
                                      const std::optional<int>& limit) const {
    auto request = Json::object();
    auto market = symbol.empty() ? Json() : this->market(symbol);
    
    if (!symbol.empty()) {
        request["symbol"] = market["id"].string_value();
    }
    if (since.has_value()) {
        request["startTime"] = *since;
    }
    if (limit.has_value()) {
        request["limit"] = *limit;
    }
    
    auto response = privateGet("allOrders", request);
    auto orders = this->parseOrders(response, market, since, limit);
    return this->filterBy(orders, "status", "closed");
}

Json currencycom::fetchMyTradesImpl(const std::string& symbol,
                                  const std::optional<long long>& since,
                                  const std::optional<int>& limit) const {
    auto request = Json::object();
    auto market = symbol.empty() ? Json() : this->market(symbol);
    
    if (!symbol.empty()) {
        request["symbol"] = market["id"].string_value();
    }
    if (since.has_value()) {
        request["startTime"] = *since;
    }
    if (limit.has_value()) {
        request["limit"] = *limit;
    }
    
    auto response = privateGet("myTrades", request);
    return this->parseTrades(response, market, since, limit);
}

Json currencycom::fetchLedgerImpl(const std::optional<std::string>& code,
                                const std::optional<long long>& since,
                                const std::optional<int>& limit) const {
    auto request = Json::object();
    auto currency = code.has_value() ? this->currency(*code) : Json();
    
    if (code.has_value()) {
        request["asset"] = currency["id"].string_value();
    }
    if (since.has_value()) {
        request["startTime"] = *since;
    }
    if (limit.has_value()) {
        request["limit"] = *limit;
    }
    
    auto response = privateGet("account/transactions", request);
    return this->parseLedger(response, currency, since, limit);
}

Json currencycom::fetchDepositAddressImpl(const std::string& code,
                                       const std::optional<std::string>& network) const {
    auto currency = this->currency(code);
    auto request = Json::object({
        {"asset", currency["id"].string_value()}
    });
    
    if (network.has_value()) {
        request["network"] = *network;
    }
    
    auto response = privateGet("depositAddress", request);
    return this->parseDepositAddress(response, currency);
}

Json currencycom::fetchDepositsImpl(const std::optional<std::string>& code,
                                  const std::optional<long long>& since,
                                  const std::optional<int>& limit) const {
    auto request = Json::object();
    auto currency = code.has_value() ? this->currency(*code) : Json();
    
    if (code.has_value()) {
        request["asset"] = currency["id"].string_value();
    }
    if (since.has_value()) {
        request["startTime"] = *since;
    }
    if (limit.has_value()) {
        request["limit"] = *limit;
    }
    
    auto response = privateGet("depositHistory", request);
    return this->parseTransactions(response, currency, "deposit", since, limit);
}

Json currencycom::parseTrade(const Json& trade, const Json& market) const {
    auto timestamp = trade["time"].int_value();
    auto price = trade["price"].number_value();
    auto amount = trade["qty"].number_value();
    auto cost = price * amount;
    
    return {
        {"info", trade},
        {"id", trade["id"].string_value()},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"symbol", market["symbol"].string_value()},
        {"order", trade.contains("orderId") ? trade["orderId"].string_value() : nullptr},
        {"type", trade.contains("type") ? trade["type"].string_value() : nullptr},
        {"side", trade["isBuyer"].bool_value() ? "buy" : "sell"},
        {"takerOrMaker", trade["isMaker"].bool_value() ? "maker" : "taker"},
        {"price", price},
        {"amount", amount},
        {"cost", cost},
        {"fee", trade.contains("commission") ? {
            {"cost", trade["commission"].number_value()},
            {"currency", trade["commissionAsset"].string_value()}
        } : nullptr}
    };
}

Json currencycom::parseOrder(const Json& order, const Json& market) const {
    auto status = order["status"].string_value();
    auto statusMap = {
        {"NEW", "open"},
        {"PARTIALLY_FILLED", "open"},
        {"FILLED", "closed"},
        {"CANCELED", "canceled"},
        {"PENDING_CANCEL", "canceling"},
        {"REJECTED", "rejected"},
        {"EXPIRED", "expired"}
    };
    
    auto timestamp = order["time"].int_value();
    auto price = order.contains("price") ? order["price"].number_value() : nullptr;
    auto amount = order.contains("origQty") ? order["origQty"].number_value() : nullptr;
    auto filled = order.contains("executedQty") ? order["executedQty"].number_value() : nullptr;
    auto cost = filled && price ? filled * price : nullptr;
    
    return {
        {"id", order["orderId"].string_value()},
        {"clientOrderId", order.contains("clientOrderId") ? order["clientOrderId"].string_value() : nullptr},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"lastTradeTimestamp", nullptr},
        {"status", statusMap[status]},
        {"symbol", market["symbol"].string_value()},
        {"type", order["type"].string_value()},
        {"timeInForce", order.contains("timeInForce") ? order["timeInForce"].string_value() : nullptr},
        {"side", order["side"].string_value()},
        {"price", price},
        {"amount", amount},
        {"filled", filled},
        {"remaining", amount && filled ? amount - filled : nullptr},
        {"cost", cost},
        {"trades", nullptr},
        {"fee", nullptr},
        {"info", order}
    };
}

Json currencycom::parseTransaction(const Json& transaction, const Json& currency) const {
    auto timestamp = transaction["insertTime"].int_value();
    auto currencyId = transaction["asset"].string_value();
    auto code = this->safeCurrencyCode(currencyId);
    
    return {
        {"info", transaction},
        {"id", transaction["txId"].string_value()},
        {"txid", transaction.contains("txHash") ? transaction["txHash"].string_value() : nullptr},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"network", transaction.contains("network") ? transaction["network"].string_value() : nullptr},
        {"address", transaction.contains("address") ? transaction["address"].string_value() : nullptr},
        {"addressTo", transaction.contains("address") ? transaction["address"].string_value() : nullptr},
        {"addressFrom", nullptr},
        {"tag", transaction.contains("addressTag") ? transaction["addressTag"].string_value() : nullptr},
        {"tagTo", transaction.contains("addressTag") ? transaction["addressTag"].string_value() : nullptr},
        {"tagFrom", nullptr},
        {"type", transaction["type"].string_value()},
        {"amount", transaction["amount"].number_value()},
        {"currency", code},
        {"status", transaction["status"].string_value()},
        {"updated", transaction.contains("updateTime") ? transaction["updateTime"].int_value() : nullptr},
        {"internal", false},
        {"fee", transaction.contains("transactionFee") ? {
            {"currency", code},
            {"cost", transaction["transactionFee"].number_value()}
        } : nullptr}
    };
}

Json currencycom::parseLedgerEntry(const Json& item, const Json& currency) const {
    auto timestamp = item["timestamp"].int_value();
    auto direction = item["type"].string_value();
    auto currencyId = item["asset"].string_value();
    auto code = this->safeCurrencyCode(currencyId);
    
    return {
        {"info", item},
        {"id", item.contains("tranId") ? item["tranId"].string_value() : nullptr},
        {"direction", direction == "DEPOSIT" ? "in" : "out"},
        {"account", nullptr},
        {"referenceId", item.contains("referenceId") ? item["referenceId"].string_value() : nullptr},
        {"referenceAccount", nullptr},
        {"type", direction},
        {"currency", code},
        {"amount", item["amount"].number_value()},
        {"before", nullptr},
        {"after", nullptr},
        {"status", item["status"].string_value()},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"fee", item.contains("fee") ? {
            {"currency", code},
            {"cost", item["fee"].number_value()}
        } : nullptr}
    };
}

} // namespace ccxt
