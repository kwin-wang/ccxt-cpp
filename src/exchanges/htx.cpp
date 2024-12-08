#include "ccxt/exchanges/htx.h"
#include "../base/crypto.h"
#include "../base/error.h"
#include <sstream>
#include <iomanip>
#include <chrono>

namespace ccxt {

HTX::HTX(const ExchangeConfig& config) : Exchange(config) {
    // Initialize exchange-specific configurations
    this->has = {
        {"CORS", nullptr},
        {"spot", true},
        {"margin", true},
        {"swap", true},
        {"future", true},
        {"option", nullptr}
    };

    this->urls["api"] = {
        {"market", "https://api.huobi.pro"},
        {"public", "https://api.huobi.pro"},
        {"private", "https://api.huobi.pro"},
        {"v2Public", "https://api.huobi.pro/v2"},
        {"v2Private", "https://api.huobi.pro/v2"}
    };

    this->api = {
        {"public", {
            {"get", {
                "market/history/kline",
                "market/detail/merged",
                "market/tickers",
                "market/depth",
                "market/trade",
                "market/history/trade",
                "common/symbols",
                "common/currencys",
                "common/timestamp",
                "common/exchange",
                "settings/currencys"
            }}
        }},
        {"private", {
            {"get", {
                "account/accounts",
                "account/accounts/{id}/balance",
                "order/orders/{id}",
                "order/orders/{id}/matchresults",
                "order/orders",
                "order/matchresults",
                "dw/deposit-virtual/addresses",
                "query/deposit-withdraw"
            }},
            {"post", {
                "order/orders/place",
                "order/orders/{id}/submitcancel",
                "order/orders/batchcancel",
                "dw/withdraw/api/create"
            }}
        }}
    };
}

OrderBook HTX::fetchOrderBook(const std::string& symbol, int limit, const Params& params) {
    validateSymbol(symbol);
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"]},
        {"type", "step0"}
    };
    if (limit != 0) {
        request["size"] = std::min(limit, 150);
    }
    auto response = this->publicGetMarketDepth(this->extend(request, params));
    auto orderbook = this->parseOrderBook(response["tick"], symbol);
    orderbook.timestamp = this->safeTimestamp(response, "ts");
    return orderbook;
}

std::string HTX::getSignature(const std::string& path, const std::string& method,
                             const std::string& hostname, const Params& params,
                             const std::string& timestamp) const {
    std::stringstream ss;
    ss << method << "\n"
       << hostname << "\n"
       << path << "\n";

    // Sort parameters alphabetically
    std::map<std::string, std::string> sortedParams;
    for (const auto& param : params) {
        sortedParams[param.first] = param.second;
    }

    bool first = true;
    for (const auto& param : sortedParams) {
        if (!first) {
            ss << "&";
        }
        ss << param.first << "=" << param.second;
        first = false;
    }

    auto str = ss.str();
    return hmacSha256(str, this->secret);
}

json HTX::signRequest(const std::string& path, const std::string& api,
                     const std::string& method, const Params& params,
                     const json& headers, const std::string& body) {
    auto timestamp = std::to_string(currentTimestamp());
    auto hostname = this->hostname();
    
    Params signParams = params;
    signParams["AccessKeyId"] = this->apiKey;
    signParams["SignatureMethod"] = "HmacSHA256";
    signParams["SignatureVersion"] = "2";
    signParams["Timestamp"] = timestamp;

    auto signature = this->getSignature(path, method, hostname, signParams, timestamp);
    signParams["Signature"] = signature;

    return {
        {"url", this->urls["api"][api] + path + "?" + buildQuery(signParams)},
        {"method", method},
        {"body", body},
        {"headers", headers}
    };
}

Balance HTX::fetchBalance(const Params& params) {
    this->loadMarkets();
    auto response = this->privateGetAccountAccountsIdBalance(this->extend({
        "id": this->safeString(this->options, "accountId")
    }, params));
    
    return this->parseBalance(response);
}

Order HTX::createOrder(const std::string& symbol, const std::string& type,
                      const std::string& side, double amount, double price,
                      const Params& params) {
    this->loadMarkets();
    auto market = this->market(symbol);
    auto accountId = this->safeString(this->options, "accountId");
    
    auto request = {
        {"account-id", accountId},
        {"symbol", market["id"].get<std::string>()},
        {"type", side + "-" + type},
        {"amount", this->amountToPrecision(symbol, amount)}
    };

    if (type == "limit") {
        request["price"] = this->priceToPrecision(symbol, price);
    }

    auto response = this->privatePostOrderOrdersPlace(this->extend(request, params));
    return this->parseOrder(response);
}

} // namespace ccxt
