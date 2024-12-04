#include "ccxt/exchanges/bitbns.h"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

BitBNS::BitBNS() {
    this->id = "bitbns";
    this->name = "BitBNS";
    this->countries = {"IN"}; // India
    this->rateLimit = 1000;
    this->version = "v2";
    this->has = {
        {"cancelOrder", true},
        {"CORS", false},
        {"createOrder", true},
        {"fetchBalance", true},
        {"fetchClosedOrders", true},
        {"fetchDeposits", true},
        {"fetchMarkets", true},
        {"fetchMyTrades", true},
        {"fetchOHLCV", true},
        {"fetchOpenOrders", true},
        {"fetchOrder", true},
        {"fetchOrderBook", true},
        {"fetchTicker", true},
        {"fetchTickers", false},
        {"fetchTrades", true},
        {"fetchWithdrawals", true}
    };

    this->timeframes = {
        {"1m", "1m"},
        {"5m", "5m"},
        {"15m", "15m"},
        {"30m", "30m"},
        {"1h", "1h"},
        {"2h", "2h"},
        {"4h", "4h"},
        {"6h", "6h"},
        {"1d", "1d"},
        {"1w", "1w"}
    };

    this->urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/117201933-e7a6e780-adf5-11eb-9d80-98fc2a21c3d6.jpg"},
        {"api", {
            {"public", "https://api.bitbns.com/api/trade/v1"},
            {"private", "https://api.bitbns.com/api/trade/v1"}
        }},
        {"www", "https://bitbns.com"},
        {"doc", {
            "https://github.com/bitbns-official/node-bitbns-api",
            "https://bitbns.com/trade/#/api-trading/"
        }},
        {"fees", "https://bitbns.com/fees"}
    };

    this->api = {
        {"public", {
            {"GET", {
                "tickers",
                "orderbook/sell/{symbol}",
                "orderbook/buy/{symbol}",
                "trades/{symbol}",
                "ticker/24hr",
                "ticker/24hr/{symbol}"
            }}
        }},
        {"private", {
            {"POST", {
                "currentCoinBalance/{symbol}",
                "listOpenOrders/{symbol}",
                "listExecutedOrders/{symbol}",
                "placeOrder/{symbol}",
                "cancelOrder/{symbol}",
                "getOrderStatus/{symbol}",
                "depositHistory/{symbol}",
                "withdrawHistory/{symbol}",
                "withdrawConfirmation/{symbol}"
            }}
        }}
    };

    this->fees = {
        {"trading", {
            {"maker", 0.0025},
            {"taker", 0.0025}
        }}
    };
}

nlohmann::json BitBNS::fetch_markets() {
    auto response = this->fetch("tickers", "public");
    auto result = nlohmann::json::array();

    for (const auto& market : response.items()) {
        auto id = market.key();
        auto baseId = id;
        auto quoteId = "INR";  // BitBNS primarily deals in INR pairs
        auto base = this->safe_currency_code(baseId);
        auto quote = this->safe_currency_code(quoteId);
        auto symbol = base + "/" + quote;

        result.push_back({
            {"id", id},
            {"symbol", symbol},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", true},
            {"type", "spot"},
            {"spot", true},
            {"margin", false},
            {"future", false}
        });
    }
    return result;
}

nlohmann::json BitBNS::fetch_ticker(const std::string& symbol) {
    auto market = this->market(symbol);
    auto request = {
        {"symbol", market["id"].get<std::string>()}
    };
    auto response = this->fetch("ticker/24hr/" + market["id"].get<std::string>(), "public");
    return this->parse_ticker(response, market);
}

nlohmann::json BitBNS::fetch_order_book(const std::string& symbol, int limit) {
    auto market = this->market(symbol);
    auto response = nlohmann::json::object();
    
    // BitBNS splits orderbook into buy and sell endpoints
    auto buybook = this->fetch("orderbook/buy/" + market["id"].get<std::string>(), "public");
    auto sellbook = this->fetch("orderbook/sell/" + market["id"].get<std::string>(), "public");
    
    response["bids"] = buybook;
    response["asks"] = sellbook;
    
    return this->parse_order_book(response, symbol);
}

nlohmann::json BitBNS::create_order(const std::string& symbol, const std::string& type,
                                  const std::string& side, double amount, double price) {
    this->check_required_credentials();
    auto market = this->market(symbol);
    
    auto request = {
        {"symbol", market["id"].get<std::string>()},
        {"side", side},
        {"quantity", this->amount_to_precision(symbol, amount)},
        {"rate", this->price_to_precision(symbol, price)},
        {"type", type}
    };

    auto response = this->fetch("placeOrder/" + market["id"].get<std::string>(), "private", "POST", request);
    return this->parse_order(response);
}

nlohmann::json BitBNS::fetch_balance() {
    this->check_required_credentials();
    auto response = this->fetch("listOpenOrders/EVERYTHING", "private", "POST");
    return this->parse_balance(response);
}

std::string BitBNS::sign(const std::string& path, const std::string& api,
                        const std::string& method, const nlohmann::json& params,
                        const std::map<std::string, std::string>& headers) {
    auto url = this->urls["api"][api].get<std::string>() + "/" + this->implode_params(path, params);
    auto query = this->omit(params, this->extract_params(path));

    if (api == "private") {
        this->check_required_credentials();
        auto timeStamp = std::to_string(this->nonce());
        auto payload = timeStamp + "\n" + method + "\n" + path;
        
        if (!query.empty()) {
            payload += "\n" + this->json(this->keysort(query));
        }

        auto signature = this->hmac(payload, this->secret, "sha512", "hex");
        
        auto new_headers = headers;
        new_headers["X-BITBNS-APIKEY"] = this->apiKey;
        new_headers["X-BITBNS-PAYLOAD"] = this->string_to_base64(payload);
        new_headers["X-BITBNS-SIGNATURE"] = signature;
        new_headers["Accept"] = "application/json";
        new_headers["Content-Type"] = "application/x-www-form-urlencoded";
        
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    }

    return url;
}

nlohmann::json BitBNS::parse_ticker(const nlohmann::json& ticker, const nlohmann::json& market) {
    auto timestamp = this->safe_integer(ticker, "timestamp", this->milliseconds());
    auto symbol = market ? market["symbol"].get<std::string>() : "";
    
    return {
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"high", this->safe_number(ticker, "high")},
        {"low", this->safe_number(ticker, "low")},
        {"bid", this->safe_number(ticker, "highestBid")},
        {"ask", this->safe_number(ticker, "lowestAsk")},
        {"last", this->safe_number(ticker, "last_traded_price")},
        {"close", this->safe_number(ticker, "last_traded_price")},
        {"previousClose", nullptr},
        {"change", nullptr},
        {"percentage", this->safe_number(ticker, "yes_price_change")},
        {"average", nullptr},
        {"baseVolume", this->safe_number(ticker, "volume")},
        {"quoteVolume", nullptr},
        {"info", ticker}
    };
}

nlohmann::json BitBNS::parse_balance(const nlohmann::json& response) {
    auto result = {
        {"info", response},
        {"timestamp", nullptr},
        {"datetime", nullptr}
    };

    for (const auto& balance : response.items()) {
        auto currencyId = balance.key();
        auto code = this->safe_currency_code(currencyId);
        auto account = this->account();
        account["free"] = this->safe_string(balance.value(), "availableOrderBalance");
        account["used"] = this->safe_string(balance.value(), "inOrderBalance");
        account["total"] = this->safe_string(balance.value(), "totalBalance");
        result[code] = account;
    }

    return result;
}

std::string BitBNS::get_version_string() {
    return "v" + this->version;
}

std::string BitBNS::get_payload_hash(const std::string& payload) {
    return this->hmac(payload, this->secret, "sha512", "hex");
}

} // namespace ccxt
