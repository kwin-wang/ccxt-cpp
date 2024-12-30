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
        auto auth = this->config_.apiKey + nonce + this->config_.secret;
        auto signature = this->hmac(auth, this->config_.secret, "sha256", "hex");
        
        auto new_headers = headers;
        new_headers["API_KEY"] = this->config_.apiKey;
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

nlohmann::json NDAX::fetch_ticker(const std::string& symbol) {
    auto market = this->market(symbol);
    auto request = {
        {"InstrumentId", std::stoi(market["id"].get<std::string>())}
    };
    auto response = this->fetch("GetTickerHistory", "public", "GET", request);
    return this->parse_ticker(response[0], market);
}

nlohmann::json NDAX::fetch_order_book(const std::string& symbol, int limit) {
    auto market = this->market(symbol);
    auto request = {
        {"InstrumentId", std::stoi(market["id"].get<std::string>())}
    };
    if (limit) {
        request["Depth"] = limit;
    }
    auto response = this->fetch("GetL2Snapshot", "public", "GET", request);
    auto timestamp = this->safe_integer(response, "OMSId");
    auto orderbook = this->parse_order_book(response, symbol, timestamp);
    orderbook["nonce"] = this->safe_integer(response, "SeqNum");
    return orderbook;
}

nlohmann::json NDAX::fetch_trades(const std::string& symbol, int since, int limit) {
    auto market = this->market(symbol);
    auto request = {
        {"InstrumentId", std::stoi(market["id"].get<std::string>())}
    };
    if (since) {
        request["StartTime"] = since;
    }
    if (limit) {
        request["Count"] = limit;
    }
    auto response = this->fetch("GetTrades", "public", "GET", request);
    return this->parse_trades(response, market, since, limit);
}

nlohmann::json NDAX::fetch_ohlcv(const std::string& symbol, const std::string& timeframe,
                                int since, int limit) {
    auto market = this->market(symbol);
    auto request = {
        {"InstrumentId", std::stoi(market["id"].get<std::string>())},
        {"Interval", this->timeframes[timeframe]}
    };
    if (since) {
        request["FromDate"] = this->iso8601(since);
    }
    if (limit) {
        request["Count"] = limit;
    }
    auto response = this->fetch("GetOHLCV", "public", "GET", request);
    return this->parse_ohlcvs(response, market, timeframe, since, limit);
}

nlohmann::json NDAX::fetch_order(const std::string& id, const std::string& symbol) {
    this->check_required_credentials();
    auto request = {
        {"OrderId", std::stoi(id)}
    };
    auto response = this->fetch("GetOrderStatus", "private", "GET", request);
    return this->parse_order(response);
}

nlohmann::json NDAX::fetch_orders(const std::string& symbol, int since, int limit) {
    this->check_required_credentials();
    auto market = symbol.empty() ? nlohmann::json(nullptr) : this->market(symbol);
    auto request = nlohmann::json::object();
    if (!symbol.empty()) {
        request["InstrumentId"] = std::stoi(market["id"].get<std::string>());
    }
    if (since) {
        request["StartTime"] = since;
    }
    if (limit) {
        request["Count"] = limit;
    }
    auto response = this->fetch("GetOrdersHistory", "private", "GET", request);
    return this->parse_orders(response, market, since, limit);
}

nlohmann::json NDAX::fetch_open_orders(const std::string& symbol, int since, int limit) {
    this->check_required_credentials();
    auto market = symbol.empty() ? nlohmann::json(nullptr) : this->market(symbol);
    auto request = nlohmann::json::object();
    if (!symbol.empty()) {
        request["InstrumentId"] = std::stoi(market["id"].get<std::string>());
    }
    auto response = this->fetch("GetOpenOrders", "private", "GET", request);
    return this->parse_orders(response, market, since, limit);
}

nlohmann::json NDAX::fetch_closed_orders(const std::string& symbol, int since, int limit) {
    auto orders = this->fetch_orders(symbol, since, limit);
    return this->filter_by(orders, "status", "closed");
}

nlohmann::json NDAX::fetch_my_trades(const std::string& symbol, int since, int limit) {
    this->check_required_credentials();
    auto market = symbol.empty() ? nlohmann::json(nullptr) : this->market(symbol);
    auto request = nlohmann::json::object();
    if (!symbol.empty()) {
        request["InstrumentId"] = std::stoi(market["id"].get<std::string>());
    }
    if (since) {
        request["StartTime"] = since;
    }
    if (limit) {
        request["Count"] = limit;
    }
    auto response = this->fetch("GetAccountTrades", "private", "GET", request);
    return this->parse_trades(response, market, since, limit);
}

nlohmann::json NDAX::fetch_deposit_address(const std::string& code) {
    this->check_required_credentials();
    auto currency = this->currency(code);
    auto request = {
        {"ProductId", std::stoi(currency["id"].get<std::string>())},
        {"GenerateNewKey", false}
    };
    auto response = this->fetch("GetDepositInfo", "private", "GET", request);
    return this->parse_deposit_address(response);
}

nlohmann::json NDAX::fetch_deposits(const std::string& code, int since, int limit) {
    this->check_required_credentials();
    auto currency = code.empty() ? nlohmann::json(nullptr) : this->currency(code);
    auto request = nlohmann::json::object();
    if (!code.empty()) {
        request["ProductId"] = std::stoi(currency["id"].get<std::string>());
    }
    if (since) {
        request["StartTime"] = since;
    }
    if (limit) {
        request["Count"] = limit;
    }
    auto response = this->fetch("GetDepositTickets", "private", "GET", request);
    return this->parse_transactions(response, currency, since, limit, "deposit");
}

nlohmann::json NDAX::fetch_withdrawals(const std::string& code, int since, int limit) {
    this->check_required_credentials();
    auto currency = code.empty() ? nlohmann::json(nullptr) : this->currency(code);
    auto request = nlohmann::json::object();
    if (!code.empty()) {
        request["ProductId"] = std::stoi(currency["id"].get<std::string>());
    }
    if (since) {
        request["StartTime"] = since;
    }
    if (limit) {
        request["Count"] = limit;
    }
    auto response = this->fetch("GetWithdrawTickets", "private", "GET", request);
    return this->parse_transactions(response, currency, since, limit, "withdrawal");
}

nlohmann::json NDAX::fetch_ledger(const std::string& code, int since, int limit) {
    this->check_required_credentials();
    auto currency = code.empty() ? nlohmann::json(nullptr) : this->currency(code);
    auto request = nlohmann::json::object();
    if (!code.empty()) {
        request["ProductId"] = std::stoi(currency["id"].get<std::string>());
    }
    if (since) {
        request["StartTime"] = since;
    }
    if (limit) {
        request["Count"] = limit;
    }
    auto response = this->fetch("GetAccountHistory", "private", "GET", request);
    return this->parse_ledger(response, currency, since, limit);
}

nlohmann::json NDAX::cancel_all_orders(const std::string& symbol) {
    this->check_required_credentials();
    auto request = nlohmann::json::object();
    if (!symbol.empty()) {
        auto market = this->market(symbol);
        request["InstrumentId"] = std::stoi(market["id"].get<std::string>());
    }
    return this->fetch("CancelAllOrders", "private", "POST", request);
}

nlohmann::json NDAX::parse_order_book(const nlohmann::json& orderbook, const std::string& symbol,
                                    int timestamp, double bids_key, double asks_key) {
    auto nonce = this->safe_integer(orderbook, "SeqNum");
    auto result = {
        {"symbol", symbol},
        {"bids", nlohmann::json::array()},
        {"asks", nlohmann::json::array()},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"nonce", nonce}
    };

    for (const auto& bid : orderbook["Bids"]) {
        result["bids"].push_back({
            bid["Price"].get<double>(),
            bid["Quantity"].get<double>()
        });
    }

    for (const auto& ask : orderbook["Asks"]) {
        result["asks"].push_back({
            ask["Price"].get<double>(),
            ask["Quantity"].get<double>()
        });
    }

    return result;
}

nlohmann::json NDAX::parse_trade(const nlohmann::json& trade, const nlohmann::json& market) {
    auto timestamp = this->safe_integer(trade, "TradeTime");
    auto price = this->safe_float(trade, "Price");
    auto amount = this->safe_float(trade, "Quantity");
    auto cost = price * amount;
    auto side = this->safe_integer(trade, "Direction") == 0 ? "buy" : "sell";
    
    return {
        {"info", trade},
        {"id", this->safe_string(trade, "TradeId")},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"symbol", market["symbol"]},
        {"type", nullptr},
        {"side", side},
        {"order", this->safe_string(trade, "OrderId")},
        {"takerOrMaker", nullptr},
        {"price", price},
        {"amount", amount},
        {"cost", cost},
        {"fee", nullptr}
    };
}

nlohmann::json NDAX::parse_ledger_entry(const nlohmann::json& item, const std::string& currency) {
    auto timestamp = this->safe_integer(item, "TimeStamp");
    auto direction = this->safe_float(item, "Amount") >= 0 ? "credit" : "debit";
    
    return {
        {"info", item},
        {"id", this->safe_string(item, "TransactionId")},
        {"direction", direction},
        {"account", this->safe_string(item, "AccountId")},
        {"referenceId", this->safe_string(item, "ReferenceId")},
        {"referenceAccount", this->safe_string(item, "ReferenceAccountId")},
        {"type", this->parse_ledger_entry_type(this->safe_string(item, "TransactionType"))},
        {"currency", currency},
        {"amount", std::abs(this->safe_float(item, "Amount"))},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"fee", {
            {"cost", this->safe_float(item, "Fee")},
            {"currency", currency}
        }}
    };
}

nlohmann::json NDAX::parse_deposit_address(const nlohmann::json& depositAddress) {
    return {
        {"currency", this->safe_string(depositAddress, "ProductSymbol")},
        {"address", this->safe_string(depositAddress, "DepositAddress")},
        {"tag", this->safe_string(depositAddress, "DepositTag")},
        {"network", this->safe_string(depositAddress, "BlockchainNetwork")},
        {"info", depositAddress}
    };
}

std::string NDAX::get_client_order_id() {
    return std::to_string(this->milliseconds());
}

} // namespace ccxt
