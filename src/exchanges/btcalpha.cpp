#include "ccxt/exchanges/btcalpha.h"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

BTCAlpha::BTCAlpha() {
    this->id = "btcalpha";
    this->name = "BTC-Alpha";
    this->countries = {"US"};
    this->rateLimit = 1000;
    this->version = "v1";
    this->has = {
        {"cancelOrder", true},
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
        {"fetchTickers", true},
        {"fetchTrades", true},
        {"fetchWithdrawals", true},
        {"withdraw", true},
        {"fetchPositions", true},
        {"fetchPosition", true},
        {"setLeverage", true},
        {"setMarginMode", true},
        {"fetchFundingRate", true},
        {"fetchFundingRateHistory", true},
        {"fetchFundingHistory", true}
    };

    this->timeframes = {
        {"1m", "1"},
        {"5m", "5"},
        {"15m", "15"},
        {"30m", "30"},
        {"1h", "60"},
        {"4h", "240"},
        {"1d", "D"}
    };

    this->urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/42625213-dabaa5da-85cf-11e8-8f99-aa8f8f7699f0.jpg"},
        {"api", "https://btc-alpha.com/api"},
        {"www", "https://btc-alpha.com"},
        {"doc", "https://btc-alpha.github.io/api-docs"},
        {"fees", "https://btc-alpha.com/fees/"}
    };

    this->api = {
        {"public", {
            {"GET", {
                "currencies/",
                "pairs/",
                "orderbook/{pair}/",
                "exchanges/",
                "charts/{pair}/{type}/chart/",
                "ticker/",
                "ticker/{pair}/",
                "funding_rate/",
                "funding_rate_history/"
            }}
        }},
        {"private", {
            {"GET", {
                "wallets/",
                "orders/own/",
                "orders/own/{id}/",
                "deposits/",
                "withdraws/",
                "orders/",
                "orders/{id}/",
                "positions/",
                "positions/{id}/",
                "funding_history/"
            }},
            {"POST", {
                "order/",
                "orders/{id}/cancel/",
                "withdraws/payment/",
                "leverage/",
                "margin_mode/"
            }}
        }}
    };

    this->fees = {
        {"trading", {
            {"maker", 0.002},
            {"taker", 0.002}
        }},
        {"funding", {
            {"withdraw", {
                "BTC", 0.00135,
                "LTC", 0.0035,
                "XMR", 0.018,
                "ZEC", 0.002,
                "ETH", 0.01,
                "ETC", 0.01,
                "SIB", 1.5,
                "CCRB", 4,
                "PZM", 0.05,
                "ITI", 0.05,
                "DCY", 5,
                "R", 5,
                "ATB", 0.05,
                "BRIA", 0.05,
                "KZC", 0.05,
                "HWC", 1,
                "SPA", 1,
                "SMS", 0.05,
                "REC", 0.05,
                "SUP", 1,
                "BQ", 100,
                "GDS", 0.05,
                "EVN", 300,
                "TRKC", 0.01,
                "UNI", 1,
                "STN", 1,
                "BCH", 0.001,
                "QBIC", 0.05
            }}
        }}
    };
}

nlohmann::json BTCAlpha::fetch_markets() {
    auto response = this->fetch("pairs/", "public");
    auto result = nlohmann::json::array();

    for (const auto& market : response) {
        auto id = market["name"].get<std::string>();
        auto baseId = market["currency1"].get<std::string>();
        auto quoteId = market["currency2"].get<std::string>();
        auto base = this->safe_currency_code(baseId);
        auto quote = this->safe_currency_code(quoteId);
        auto symbol = base + "/" + quote;
        auto precision = {
            {"amount", market["price_precision"].get<int>()},
            {"price", market["price_precision"].get<int>()}
        };

        result.push_back({
            {"id", id},
            {"symbol", symbol},
            {"base", base},
            {"quote", quote},
            {"baseId", baseId},
            {"quoteId", quoteId},
            {"active", true},
            {"precision", precision},
            {"limits", {
                {"amount", {
                    {"min", this->safe_number(market, "minimum_order_size")},
                    {"max", this->safe_number(market, "maximum_order_size")}
                }},
                {"price", {
                    {"min", std::pow(10, -precision["price"].get<int>())},
                    {"max", nullptr}
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

nlohmann::json BTCAlpha::create_order(const std::string& symbol, const std::string& type,
                                    const std::string& side, double amount, double price) {
    this->load_markets();
    nlohmann::json market = this->market(symbol);
    nlohmann::json request = {
        {"pair", market["id"]},
        {"type", side},
        {"amount", this->amount_to_precision(symbol, amount)},
        {"price", this->price_to_precision(symbol, price)}
    };
    nlohmann::json response = this->privatePostOrder(request);
    return this->parse_order(response, market);
}

nlohmann::json BTCAlpha::cancel_order(const std::string& id, const std::string& symbol) {
    this->load_markets();
    nlohmann::json request = {
        {"id", id}
    };
    return this->privatePostOrdersIdCancel(request);
}

nlohmann::json BTCAlpha::cancel_all_orders(const std::string& symbol) {
    this->load_markets();
    nlohmann::json market = this->market(symbol);
    nlohmann::json request = {
        {"pair", market["id"]}
    };
    return this->privatePostOrdersCancelAll(request);
}

nlohmann::json BTCAlpha::edit_order(const std::string& id, const std::string& symbol,
                                  const std::string& type, const std::string& side,
                                  double amount, double price) {
    this->load_markets();
    nlohmann::json market = this->market(symbol);
    nlohmann::json request = {
        {"id", id},
        {"pair", market["id"]},
        {"type", side},
        {"amount", this->amount_to_precision(symbol, amount)},
        {"price", this->price_to_precision(symbol, price)}
    };
    nlohmann::json response = this->privatePostOrdersIdEdit(request);
    return this->parse_order(response, market);
}

nlohmann::json BTCAlpha::fetch_balance() {
    this->check_required_credentials();
    auto response = this->fetch("wallets/", "private");
    return this->parse_balance(response);
}

nlohmann::json BTCAlpha::fetch_ticker(const std::string& symbol) {
    this->load_markets();
    nlohmann::json market = this->market(symbol);
    nlohmann::json request = {
        {"pair", market["id"]}
    };
    nlohmann::json response = this->publicGetTickerPair(request);
    return this->parse_ticker(response, market);
}

nlohmann::json BTCAlpha::fetch_order_book(const std::string& symbol, int limit) {
    this->load_markets();
    nlohmann::json market = this->market(symbol);
    nlohmann::json request = {
        {"pair", market["id"]}
    };
    if (limit > 0) {
        request["limit"] = limit;
    }
    nlohmann::json response = this->publicGetOrderbookPair(request);
    return this->parse_order_book(response, symbol);
}

nlohmann::json BTCAlpha::fetch_trades(const std::string& symbol, int since, int limit) {
    this->load_markets();
    nlohmann::json market = this->market(symbol);
    nlohmann::json request = {
        {"pair", market["id"]}
    };
    if (limit > 0) {
        request["limit"] = limit;
    }
    nlohmann::json response = this->publicGetExchanges(request);
    return this->parse_trades(response, market, since, limit);
}

nlohmann::json BTCAlpha::fetch_ohlcv(const std::string& symbol, const std::string& timeframe,
                                    int since, int limit) {
    this->load_markets();
    nlohmann::json market = this->market(symbol);
    nlohmann::json request = {
        {"pair", market["id"]},
        {"type", this->timeframes[timeframe]}
    };
    if (limit > 0) {
        request["limit"] = limit;
    }
    if (since > 0) {
        request["since"] = since;
    }
    nlohmann::json response = this->publicGetChartsPairTypeChart(request);
    return this->parse_ohlcvs(response, market, timeframe, since, limit);
}

nlohmann::json BTCAlpha::fetch_open_orders(const std::string& symbol, int since, int limit) {
    this->load_markets();
    nlohmann::json market = this->market(symbol);
    nlohmann::json request = {
        {"pair", market["id"]}
    };
    if (limit > 0) {
        request["limit"] = limit;
    }
    nlohmann::json response = this->privateGetOrdersOwn(request);
    return this->parse_orders(response, market, since, limit);
}

nlohmann::json BTCAlpha::fetch_closed_orders(const std::string& symbol, int since, int limit) {
    this->load_markets();
    nlohmann::json market = this->market(symbol);
    nlohmann::json request = {
        {"pair", market["id"]},
        {"status", "closed"}
    };
    if (limit > 0) {
        request["limit"] = limit;
    }
    nlohmann::json response = this->privateGetOrdersOwn(request);
    return this->parse_orders(response, market, since, limit);
}

nlohmann::json BTCAlpha::fetch_my_trades(const std::string& symbol, int since, int limit) {
    this->load_markets();
    nlohmann::json market = this->market(symbol);
    nlohmann::json request = {
        {"pair", market["id"]}
    };
    if (limit > 0) {
        request["limit"] = limit;
    }
    if (since > 0) {
        request["since"] = since;
    }
    nlohmann::json response = this->privateGetExchanges(request);
    return this->parse_trades(response, market, since, limit);
}

nlohmann::json BTCAlpha::fetch_order(const std::string& id, const std::string& symbol) {
    this->load_markets();
    nlohmann::json request = {
        {"id", id}
    };
    nlohmann::json response = this->privateGetOrdersOwnId(request);
    return this->parse_order(response);
}

nlohmann::json BTCAlpha::fetch_deposit_address(const std::string& code) {
    this->load_markets();
    std::string currency = this->get_currency_id(code);
    nlohmann::json request = {
        {"currency", currency}
    };
    nlohmann::json response = this->privateGetWallets(request);
    return this->parse_deposit_address(response);
}

nlohmann::json BTCAlpha::fetch_deposits(const std::string& code, int since, int limit) {
    this->load_markets();
    std::string currency = this->get_currency_id(code);
    nlohmann::json request = {
        {"currency", currency}
    };
    if (since > 0) {
        request["since"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    nlohmann::json response = this->privateGetDeposits(request);
    return this->parse_transactions(response, code, since, limit, "deposit");
}

nlohmann::json BTCAlpha::fetch_withdrawals(const std::string& code, int since, int limit) {
    this->load_markets();
    std::string currency = this->get_currency_id(code);
    nlohmann::json request = {
        {"currency", currency}
    };
    if (since > 0) {
        request["since"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    nlohmann::json response = this->privateGetWithdraws(request);
    return this->parse_transactions(response, code, since, limit, "withdrawal");
}

nlohmann::json BTCAlpha::fetch_positions(const std::string& symbols, bool params) {
    this->load_markets();
    nlohmann::json request = {};
    if (!symbols.empty()) {
        nlohmann::json market = this->market(symbols);
        request["pair"] = market["id"];
    }
    nlohmann::json response = this->privateGetPositions(request);
    return this->parse_positions(response, symbols);
}

nlohmann::json BTCAlpha::fetch_position(const std::string& symbol, bool params) {
    this->load_markets();
    nlohmann::json market = this->market(symbol);
    nlohmann::json request = {
        {"pair", market["id"]}
    };
    nlohmann::json response = this->privateGetPosition(request);
    return this->parse_position(response, market);
}

nlohmann::json BTCAlpha::set_leverage(int leverage, const std::string& symbol) {
    this->load_markets();
    nlohmann::json market = this->market(symbol);
    nlohmann::json request = {
        {"pair", market["id"]},
        {"leverage", leverage}
    };
    return this->privatePostLeverage(request);
}

nlohmann::json BTCAlpha::set_margin_mode(const std::string& marginMode, const std::string& symbol) {
    this->load_markets();
    nlohmann::json market = this->market(symbol);
    nlohmann::json request = {
        {"pair", market["id"]},
        {"mode", marginMode}
    };
    return this->privatePostMarginMode(request);
}

nlohmann::json BTCAlpha::fetch_funding_rate(const std::string& symbol) {
    this->load_markets();
    nlohmann::json market = this->market(symbol);
    nlohmann::json request = {
        {"pair", market["id"]}
    };
    nlohmann::json response = this->publicGetFundingRate(request);
    return this->parse_funding_rate(response, market);
}

nlohmann::json BTCAlpha::fetch_funding_rate_history(const std::string& symbol, int since, int limit) {
    this->load_markets();
    nlohmann::json market = this->market(symbol);
    nlohmann::json request = {
        {"pair", market["id"]}
    };
    if (since > 0) {
        request["since"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    nlohmann::json response = this->publicGetFundingRateHistory(request);
    return this->parse_funding_rate_history(response, market, since, limit);
}

nlohmann::json BTCAlpha::fetch_funding_history(const std::string& symbol, int since, int limit) {
    this->load_markets();
    nlohmann::json market = this->market(symbol);
    nlohmann::json request = {
        {"pair", market["id"]}
    };
    if (since > 0) {
        request["since"] = since;
    }
    if (limit > 0) {
        request["limit"] = limit;
    }
    nlohmann::json response = this->privateGetFundingHistory(request);
    return this->parse_funding_history(response, market, since, limit);
}

std::string BTCAlpha::sign(const std::string& path, const std::string& api,
                          const std::string& method, const nlohmann::json& params,
                          const std::map<std::string, std::string>& headers) {
    auto url = this->urls["api"].get<std::string>() + "/" + this->version + "/" + this->implode_params(path, params);
    auto query = this->omit(params, this->extract_params(path));

    if (api == "private") {
        this->check_required_credentials();
        auto nonce = std::to_string(this->nonce());
        auto auth = nonce + this->uid + this->apiKey;
        auto signature = this->hmac(auth, this->secret, "sha256", "hex");
        
        auto new_headers = headers;
        new_headers["Api-Key"] = this->apiKey;
        new_headers["Api-Nonce"] = nonce;
        new_headers["Api-Signature"] = signature;
        
        if (!query.empty()) {
            if (method == "GET") {
                url += "?" + this->urlencode(query);
            } else {
                new_headers["Content-Type"] = "application/json";
            }
        }
    } else {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    }

    return url;
}

nlohmann::json BTCAlpha::parse_ticker(const nlohmann::json& ticker, const nlohmann::json& market) {
    auto timestamp = this->safe_timestamp(ticker, "timestamp");
    auto symbol = market ? market["symbol"].get<std::string>() : "";
    
    return {
        {"symbol", symbol},
        {"timestamp", timestamp},
        {"datetime", this->iso8601(timestamp)},
        {"high", this->safe_number(ticker, "high")},
        {"low", this->safe_number(ticker, "low")},
        {"bid", this->safe_number(ticker, "buy_price")},
        {"ask", this->safe_number(ticker, "sell_price")},
        {"last", this->safe_number(ticker, "last_price")},
        {"close", this->safe_number(ticker, "last_price")},
        {"previousClose", nullptr},
        {"change", nullptr},
        {"percentage", nullptr},
        {"average", nullptr},
        {"baseVolume", this->safe_number(ticker, "vol")},
        {"quoteVolume", nullptr},
        {"info", ticker}
    };
}

nlohmann::json BTCAlpha::parse_balance(const nlohmann::json& response) {
    auto result = {
        {"info", response},
        {"timestamp", nullptr},
        {"datetime", nullptr}
    };

    for (const auto& balance : response) {
        auto currencyId = balance["currency"].get<std::string>();
        auto code = this->safe_currency_code(currencyId);
        auto account = this->account();
        account["free"] = this->safe_string(balance, "balance");
        account["used"] = this->safe_string(balance, "reserve");
        result[code] = account;
    }

    return result;
}

std::string BTCAlpha::get_currency_id(const std::string& code) {
    if (this->currencies.find(code) != this->currencies.end()) {
        return this->currencies[code]["id"].get<std::string>();
    }
    return code;
}

} // namespace ccxt
