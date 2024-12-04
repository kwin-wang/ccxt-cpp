#include "ccxt/exchanges/kucoinfutures.h"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

KuCoinFutures::KuCoinFutures() {
    this->id = "kucoinfutures";
    this->name = "KuCoin Futures";
    this->countries = {"SC"}; // Seychelles
    this->rateLimit = 75;
    this->version = "v1";
    this->has = {
        {"addMargin", true},
        {"cancelAllOrders", true},
        {"cancelOrder", true},
        {"createOrder", true},
        {"fetchBalance", true},
        {"fetchClosedOrders", true},
        {"fetchFundingRate", true},
        {"fetchFundingRateHistory", true},
        {"fetchFundingRates", true},
        {"fetchIndexOHLCV", true},
        {"fetchLeverage", true},
        {"fetchLeverageTiers", true},
        {"fetchMarkets", true},
        {"fetchMarkOHLCV", true},
        {"fetchMyTrades", true},
        {"fetchOHLCV", true},
        {"fetchOpenOrders", true},
        {"fetchOrder", true},
        {"fetchOrderBook", true},
        {"fetchPosition", true},
        {"fetchPositions", true},
        {"fetchPremiumIndexOHLCV", true},
        {"fetchTicker", true},
        {"fetchTickers", true},
        {"fetchTrades", true},
        {"reduceMargin", true},
        {"setLeverage", true},
        {"setMarginMode", true}
    };

    this->timeframes = {
        {"1m", "1min"},
        {"3m", "3min"},
        {"5m", "5min"},
        {"15m", "15min"},
        {"30m", "30min"},
        {"1h", "1hour"},
        {"2h", "2hour"},
        {"4h", "4hour"},
        {"6h", "6hour"},
        {"8h", "8hour"},
        {"12h", "12hour"},
        {"1d", "1day"},
        {"1w", "1week"}
    };

    this->urls = {
        {"logo", "https://user-images.githubusercontent.com/51840849/87295558-132aaf80-c50e-11ea-9801-a2fb0c57c799.jpg"},
        {"api", {
            {"public", "https://api-futures.kucoin.com"},
            {"private", "https://api-futures.kucoin.com"},
            {"contract", "https://api-futures.kucoin.com"}
        }},
        {"www", "https://futures.kucoin.com"},
        {"doc", {
            "https://docs.kucoin.com/futures",
            "https://docs.kucoin.com"
        }},
        {"fees", "https://futures.kucoin.com/contract/detail"}
    };

    this->api = {
        {"public", {
            {"GET", {
                "contracts/active",
                "contracts/{symbol}",
                "ticker",
                "level2/snapshot",
                "level2/depth{limit}",
                "level2/message/query",
                "level3/message/query",
                "trade/history",
                "interest/query",
                "index/query",
                "mark-price/{symbol}/current",
                "premium/query",
                "funding-rate/{symbol}/current",
                "timestamp",
                "status",
                "kline/query"
            }}
        }},
        {"private", {
            {"GET", {
                "account-overview",
                "positions",
                "orders",
                "orders/{orderId}",
                "fills",
                "recentDoneOrders",
                "openOrderStatistics",
                "position/margin/history",
                "funding-history"
            }},
            {"POST", {
                "orders",
                "position/margin/auto-deposit-status",
                "position/margin/deposit-margin",
                "position/risk-limit-level/change"
            }},
            {"DELETE", {
                "orders/{orderId}",
                "orders",
                "stopOrders"
            }}
        }}
    };

    this->fees = {
        {"trading", {
            {"tierBased", true},
            {"percentage", true},
            {"taker", 0.0006},
            {"maker", 0.0002}
        }},
        {"funding", {
            {"tierBased", false},
            {"percentage", false},
            {"withdraw", {}},
            {"deposit", {}}
        }}
    };
}

nlohmann::json KuCoinFutures::fetch_markets() {
    auto response = this->fetch("contracts/active", "public");
    auto result = nlohmann::json::array();

    for (const auto& market : response["data"]) {
        auto id = market["symbol"].get<std::string>();
        auto baseId = market["baseCurrency"].get<std::string>();
        auto quoteId = market["quoteCurrency"].get<std::string>();
        auto settleId = market["settleCurrency"].get<std::string>();
        auto base = this->safe_currency_code(baseId);
        auto quote = this->safe_currency_code(quoteId);
        auto settle = this->safe_currency_code(settleId);
        auto symbol = base + "/" + quote + ":" + settle;
        auto type = market["type"].get<std::string>();
        auto inverse = type == "FFWCSX";
        auto linear = type == "FFICSX";
        auto contractSize = this->safe_number(market, "multiplier");
        
        result.push_back({
            {"id", id},
            {"symbol", symbol},
            {"base", base},
            {"quote", quote},
            {"settle", settle},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"settleId", settleId},
            {"type", "future"},
            {"spot", false},
            {"margin", false},
            {"swap", true},
            {"future", true},
            {"option", false},
            {"active", market["status"] == "Open"},
            {"contract", true},
            {"linear", linear},
            {"inverse", inverse},
            {"contractSize", contractSize},
            {"expiry", nullptr},  // Perpetual
            {"expiryDatetime", nullptr},
            {"strike", nullptr},
            {"optionType", nullptr},
            {"precision", {
                {"amount", market["lotSize"]},
                {"price", market["tickSize"]}
            }},
            {"limits", {
                {"leverage", {
                    {"min", market["minLeverage"]},
                    {"max", market["maxLeverage"]}
                }},
                {"amount", {
                    {"min", market["lotSize"]},
                    {"max", market["maxOrderQty"]}
                }},
                {"price", {
                    {"min", market["tickSize"]},
                    {"max", nullptr}
                }},
                {"cost", {
                    {"min", market["minOrderValue"]},
                    {"max", nullptr}
                }}
            }}
        });
    }
    return result;
}

nlohmann::json KuCoinFutures::fetch_funding_rate(const std::string& symbol) {
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"].get<std::string>()}
    };
    auto response = this->fetch("funding-rate/" + market["id"].get<std::string>() + "/current", "public");
    return this->parse_funding_rate(response["data"], market);
}

nlohmann::json KuCoinFutures::create_order(const std::string& symbol, const std::string& type,
                                         const std::string& side, double amount, double price) {
    this->check_required_credentials();
    auto market = this->market(symbol);
    
    auto request = {
        {"clientOid", this->uuid()},
        {"symbol", market["id"].get<std::string>()},
        {"side", side.upper()},
        {"type", type.upper()},
        {"leverage", "1"},  // Default leverage
        {"size", this->amount_to_precision(symbol, amount)}
    };

    if (type == "limit") {
        request["price"] = this->price_to_precision(symbol, price);
    }

    auto response = this->fetch("orders", "private", "POST", request);
    return this->parse_order(response["data"]);
}

nlohmann::json KuCoinFutures::fetch_positions(const std::vector<std::string>& symbols) {
    this->check_required_credentials();
    auto response = this->fetch("positions", "private");
    auto positions = response["data"];
    return this->parse_positions(positions, symbols);
}

nlohmann::json KuCoinFutures::set_leverage(int leverage, const std::string& symbol) {
    this->check_required_credentials();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"].get<std::string>()},
        {"leverage", std::to_string(leverage)}
    };
    return this->fetch("position/risk-limit-level/change", "private", "POST", request);
}

std::string KuCoinFutures::sign(const std::string& path, const std::string& api,
                               const std::string& method, const nlohmann::json& params,
                               const std::map<std::string, std::string>& headers) {
    auto endpoint = "/" + this->version + "/" + this->implode_params(path, params);
    auto url = this->urls["api"][api].get<std::string>() + endpoint;
    auto query = this->omit(params, this->extract_params(path));

    if (api == "public") {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->check_required_credentials();
        auto timestamp = this->get_server_time();
        auto body = "";
        
        if (!query.empty()) {
            if (method == "GET") {
                url += "?" + this->urlencode(query);
            } else {
                body = this->json(query);
            }
        }

        auto auth = timestamp + method + endpoint + body;
        auto signature = this->hmac(auth, this->secret, "sha256", "base64");
        auto passphrase = this->hmac(this->password, this->secret, "sha256", "base64");
        
        auto new_headers = headers;
        new_headers["KC-API-KEY"] = this->apiKey;
        new_headers["KC-API-SIGN"] = signature;
        new_headers["KC-API-TIMESTAMP"] = timestamp;
        new_headers["KC-API-PASSPHRASE"] = passphrase;
        new_headers["KC-API-KEY-VERSION"] = "2";
        
        if (method != "GET") {
            new_headers["Content-Type"] = "application/json";
        }
    }

    return url;
}

std::string KuCoinFutures::get_server_time() {
    auto response = this->fetch("timestamp", "public");
    return response["data"].get<std::string>();
}

std::string KuCoinFutures::get_settlement_currency(const std::string& market) {
    if (this->markets.find(market) != this->markets.end()) {
        return this->markets[market]["settle"].get<std::string>();
    }
    throw std::runtime_error("Market " + market + " not found");
}

} // namespace ccxt
