#include "ccxt/exchanges/binanceusdm.h"
#include <ctime>
#include <sstream>
#include <iomanip>

namespace ccxt {

BinanceUsdm::BinanceUsdm() {
    this->id = "binanceusdm";
    this->name = "Binance USD-M";
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
        {"fetchLeverageBrackets", true},
        {"fetchMarkets", true},
        {"fetchMarkOHLCV", true},
        {"fetchMyTrades", true},
        {"fetchOHLCV", true},
        {"fetchOpenInterest", true},
        {"fetchOpenInterestHistory", true},
        {"fetchOpenOrders", true},
        {"fetchOrder", true},
        {"fetchOrderBook", true},
        {"fetchPositionRisk", true},
        {"fetchPositions", true},
        {"fetchTicker", true},
        {"fetchTickers", true},
        {"fetchTrades", true},
        {"reduceMargin", true},
        {"setLeverage", true},
        {"setMarginMode", true},
        {"setPositionMode", true}
    };

    this->urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/117738721-668c8d80-b205-11eb-8c49-3fad84c4a07f.jpg"},
        {"api", {
            {"public", "https://fapi.binance.com"},
            {"private", "https://fapi.binance.com"},
            {"v2", "https://fapi.binance.com"}
        }},
        {"www", "https://www.binance.com"},
        {"doc", {
            "https://binance-docs.github.io/apidocs/futures/en/",
            "https://binance-docs.github.io/apidocs/spot/en"
        }}
    };

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
                "premiumIndex",
                "fundingRate",
                "klines",
                "continuousKlines",
                "indexPriceKlines",
                "markPriceKlines",
                "ticker/24hr",
                "ticker/price",
                "ticker/bookTicker",
                "openInterest",
                "openInterestHist",
                "topLongShortAccountRatio",
                "topLongShortPositionRatio",
                "globalLongShortAccountRatio"
            }}
        }},
        {"private", {
            {"GET", {
                "positionRisk",
                "account",
                "balance",
                "positionSide/dual",
                "leverageBracket",
                "openOrders",
                "orders",
                "allOrders",
                "myTrades",
                "income",
                "commissionRate"
            }},
            {"POST", {
                "order",
                "order/test",
                "batchOrders",
                "positionSide/dual",
                "marginType",
                "leverage",
                "listenKey",
                "countdownCancelAll"
            }},
            {"DELETE", {
                "order",
                "allOpenOrders",
                "batchOrders",
                "listenKey"
            }},
            {"PUT", {
                "listenKey"
            }}
        }}
    };
}

nlohmann::json BinanceUsdm::fetch_markets() {
    auto response = this->fetch("exchangeInfo", "public");
    auto markets = response["symbols"];
    auto result = nlohmann::json::array();

    for (const auto& market : markets) {
        auto id = market["symbol"].get<std::string>();
        auto baseId = market["baseAsset"].get<std::string>();
        auto quoteId = market["quoteAsset"].get<std::string>();
        auto base = this->safe_currency_code(baseId);
        auto quote = this->safe_currency_code(quoteId);
        auto symbol = base + "/" + quote;
        
        result.push_back({
            {"id", id},
            {"symbol", symbol},
            {"base", base},
            {"quote", quote},
            {"settle", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"settleId", quoteId},
            {"type", "future"},
            {"spot", false},
            {"margin", false},
            {"swap", true},
            {"future", true},
            {"delivery", false},
            {"linear", true},
            {"inverse", false},
            {"contract", true},
            {"contractSize", this->safe_number(market, "contractSize", 1.0)},
            {"active", market["status"] == "TRADING"},
            {"expiry", 0},  // Perpetual
            {"expiryDatetime", nullptr},
            {"strike", nullptr},
            {"optionType", nullptr}
        });
    }
    return result;
}

nlohmann::json BinanceUsdm::fetch_funding_rate(const std::string& symbol) {
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"].get<std::string>()}
    };
    auto response = this->fetch("premiumIndex", "public", "GET", request);
    return this->parse_funding_rate(response);
}

nlohmann::json BinanceUsdm::fetch_open_interest(const std::string& symbol) {
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"].get<std::string>()}
    };
    return this->fetch("openInterest", "public", "GET", request);
}

nlohmann::json BinanceUsdm::set_leverage(int leverage, const std::string& symbol) {
    this->check_required_credentials();
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"].get<std::string>()},
        {"leverage", leverage}
    };
    return this->fetch("leverage", "private", "POST", request);
}

nlohmann::json BinanceUsdm::fetch_position_risk(const std::vector<std::string>& symbols) {
    this->check_required_credentials();
    auto response = this->fetch("positionRisk", "private", "GET");
    return this->parse_positions(response, symbols);
}

std::string BinanceUsdm::sign(const std::string& path, const std::string& api,
                             const std::string& method, const nlohmann::json& params,
                             const std::map<std::string, std::string>& headers) {
    auto url = this->urls["api"][api].get<std::string>() + "/fapi/v1/" + path;
    auto query = this->omit(params, this->extract_params(path));

    if (api == "public") {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->check_required_credentials();
        query["timestamp"] = std::to_string(this->nonce());
        auto queryString = this->urlencode(this->keysort(query));
        auto signature = this->hmac(queryString, this->secret);
        url += "?" + queryString + "&signature=" + signature;
        
        auto new_headers = headers;
        new_headers["X-MBX-APIKEY"] = this->apiKey;
    }

    return url;
}

std::string BinanceUsdm::get_settlement_currency() {
    return "USDT";  // USD-M futures always settle in USDT
}

} // namespace ccxt
