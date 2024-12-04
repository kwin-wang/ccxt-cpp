#include "ccxt/exchanges/ndax.h"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

NDAX::NDAX() {
    this->id = "ndax";
    this->name = "NDAX";
    this->countries = {"CA"};  // Canada
    this->rateLimit = 1000;
    this->has = {
        {"cancelAllOrders", true},
        {"cancelOrder", true},
        {"createOrder", true},
        {"fetchBalance", true},
        {"fetchClosedOrders", true},
        {"fetchDeposits", true},
        {"fetchLedger", true},
        {"fetchMarkets", true},
        {"fetchMyTrades", true},
        {"fetchOHLCV", true},
        {"fetchOpenOrders", true},
        {"fetchOrder", true},
        {"fetchOrderBook", true},
        {"fetchOrders", true},
        {"fetchTicker", true},
        {"fetchTrades", true},
        {"fetchWithdrawals", true}
    };

    this->timeframes = {
        {"1m", "60"},
        {"5m", "300"},
        {"15m", "900"},
        {"30m", "1800"},
        {"1h", "3600"},
        {"2h", "7200"},
        {"4h", "14400"},
        {"6h", "21600"},
        {"12h", "43200"},
        {"1d", "86400"},
        {"1w", "604800"}
    };

    this->urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/108623144-67a3ef00-744e-11eb-8140-75c6b851e945.jpg"},
        {"api", {
            {"public", "https://api.ndax.io:8443/AP"},
            {"private", "https://api.ndax.io:8443/AP"}
        }},
        {"www", "https://ndax.io"},
        {"doc", {
            "https://apidoc.ndax.io/",
        }},
        {"fees", "https://ndax.io/fees"}
    };

    this->api = {
        {"public", {
            {"GET", {
                "Authenticate",
                "GetInstruments",
                "GetProducts",
                "GetL2Snapshot",
                "GetTickerHistory",
                "GetTrades",
                "GetTradesByTicker",
                "GetOHLCV"
            }}
        }},
        {"private", {
            {"GET", {
                "GetUserInfo",
                "GetAccountInfo",
                "GetAccountTrades",
                "GetDepositTickets",
                "GetOrdersHistory",
                "GetOrderStatus",
                "GetLevel2",
                "GetOpenOrders",
                "GetUserTradesByOrder",
                "GetWithdrawTickets"
            }},
            {"POST", {
                "AddOrder",
                "CancelAllOrders",
                "CancelOrder",
                "CreateDepositTicket",
                "CreateWithdrawTicket"
            }}
        }}
    };

    this->fees = {
        {"trading", {
            {"tierBased", false},
            {"percentage", true},
            {"maker", 0.002},
            {"taker", 0.002}
        }}
    };
}

nlohmann::json NDAX::fetch_markets() {
    auto response = this->fetch("GetInstruments", "public");
    auto result = nlohmann::json::array();

    for (const auto& market : response) {
        auto id = market["InstrumentId"].get<std::string>();
        auto baseId = std::to_string(market["Product1"].get<int>());
        auto quoteId = std::to_string(market["Product2"].get<int>());
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
            {"active", market["IsActive"].get<bool>()},
            {"precision", {
                {"price", market["DecimalPlaces"].get<int>()},
                {"amount", market["QuantityIncrement"].get<double>()}
            }},
            {"limits", {
                {"amount", {
                    {"min", market["MinimumQuantity"].get<double>()},
                    {"max", market["MaximumQuantity"].get<double>()}
                }},
                {"price", {
                    {"min", market["MinimumPrice"].get<double>()},
                    {"max", market["MaximumPrice"].get<double>()}
                }},
                {"cost", {
                    {"min", nullptr},
                    {"max", nullptr}
                }}
            }},
            {"info", market}
        });
    }
    return result;
}

nlohmann::json NDAX::create_order(const std::string& symbol, const std::string& type,
                                const std::string& side, double amount, double price) {
    this->check_required_credentials();
    auto market = this->market(symbol);
    auto orderSide = (side == "buy") ? 0 : 1;
    auto orderType = (type == "limit") ? 2 : 1;  // 1 for market, 2 for limit

    auto request = {
        {"InstrumentId", std::stoi(market["id"].get<std::string>())},
        {"OrderType", orderType},
        {"Side", orderSide},
        {"Quantity", this->amount_to_precision(symbol, amount)},
        {"ClientOrderId", this->get_client_order_id()}
    };

    if (type == "limit") {
        request["Price"] = this->price_to_precision(symbol, price);
    }

    auto response = this->fetch("AddOrder", "private", "POST", request);
    return this->parse_order(response);
}

nlohmann::json NDAX::cancel_order(const std::string& id, const std::string& symbol) {
    this->check_required_credentials();
    auto request = {
        {"OrderId", std::stoi(id)}
    };
    return this->fetch("CancelOrder", "private", "POST", request);
}

nlohmann::json NDAX::fetch_balance() {
    this->check_required_credentials();
    auto response = this->fetch("GetAccountInfo", "private");
    return this->parse_balance(response);
}

std::string NDAX::sign(const std::string& path, const std::string& api,
                      const std::string& method, const nlohmann::json& params,
                      const std::map<std::string, std::string>& headers) {
    auto url = this->urls["api"][api].get<std::string>() + "/" + path;
    
    if (api == "private") {
        this->check_required_credentials();
        auto nonce = std::to_string(this->nonce());
        auto auth = this->apiKey + nonce + this->secret;
        auto signature = this->hmac(auth, this->secret, "sha256", "hex");
        
        auto new_headers = headers;
        new_headers["API_KEY"] = this->apiKey;
        new_headers["API_NONCE"] = nonce;
        new_headers["API_SIGNATURE"] = signature;
        
        if (!params.empty()) {
            if (method == "GET") {
                url += "?" + this->urlencode(params);
            } else {
                new_headers["Content-Type"] = "application/json";
            }
        }
    } else {
        if (!params.empty()) {
            url += "?" + this->urlencode(params);
        }
    }

    return url;
}

nlohmann::json NDAX::parse_ticker(const nlohmann::json& ticker, const nlohmann::json& market) {
    auto timestamp = this->safe_timestamp(ticker, "Timestamp");
    auto symbol = market ? market["symbol"].get<std::string>() : "";
    
    return {
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"high", this->safe_number(ticker, "High")},
        {"low", this->safe_number(ticker, "Low")},
        {"bid", this->safe_number(ticker, "BestBid")},
        {"ask", this->safe_number(ticker, "BestAsk")},
        {"last", this->safe_number(ticker, "LastTradedPx")},
        {"close", this->safe_number(ticker, "LastTradedPx")},
        {"baseVolume", this->safe_number(ticker, "Rolling24HrVolume")},
        {"quoteVolume", nullptr},
        {"info", ticker}
    };
}

nlohmann::json NDAX::parse_balance(const nlohmann::json& response) {
    auto result = {
        {"info", response},
        {"timestamp", nullptr},
        {"datetime", nullptr}
    };

    for (const auto& balance : response) {
        auto currencyId = std::to_string(balance["ProductId"].get<int>());
        auto code = this->safe_currency_code(currencyId);
        auto account = this->account();
        account["free"] = this->safe_string(balance, "Available");
        account["used"] = this->safe_string(balance, "Hold");
        account["total"] = this->safe_string(balance, "Total");
        result[code] = account;
    }

    return result;
}

std::string NDAX::get_client_order_id() {
    return std::to_string(this->nonce());
}

int NDAX::get_instrument_id(const std::string& symbol) {
    auto market = this->market(symbol);
    return std::stoi(market["id"].get<std::string>());
}

std::string NDAX::get_currency_id(const std::string& code) {
    if (this->currencies.find(code) != this->currencies.end()) {
        return this->currencies[code]["id"].get<std::string>();
    }
    return code;
}

} // namespace ccxt
