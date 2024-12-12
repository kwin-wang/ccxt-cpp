#include "ccxt/exchanges/async/zonda_async.h"
#include <openssl/hmac.h>
#include <iomanip>
#include <sstream>

namespace ccxt {

ZondaAsync::ZondaAsync(const Config& config) : ExchangeAsync(config) {
    id = "zonda";
    name = "Zonda";
    countries = {"EE"}; // Estonia
    version = "1";
    rateLimit = 1000;
    has = {
        {"CORS", true},
        {"spot", true},
        {"margin", false},
        {"swap", false},
        {"future", false},
        {"option", false},
        {"cancelOrder", true},
        {"createOrder", true},
        {"fetchBalance", true},
        {"fetchDepositAddress", true},
        {"fetchDepositAddresses", true},
        {"fetchLedger", true},
        {"fetchMarkets", true},
        {"fetchMyTrades", true},
        {"fetchOHLCV", true},
        {"fetchOpenOrders", true},
        {"fetchOrder", true},
        {"fetchOrderBook", true},
        {"fetchTicker", true},
        {"fetchTickers", true},
        {"fetchTrades", true},
        {"fetchTradingFees", true},
        {"transfer", true},
        {"withdraw", true}
    };

    timeframes = {
        {"1m", "60"},
        {"3m", "180"},
        {"5m", "300"},
        {"15m", "900"},
        {"30m", "1800"},
        {"1h", "3600"},
        {"2h", "7200"},
        {"4h", "14400"},
        {"6h", "21600"},
        {"12h", "43200"},
        {"1d", "86400"},
        {"3d", "259200"},
        {"1w", "604800"}
    };

    hostname = "zondacrypto.exchange";
    urls = {
        {"referral", "https://auth.zondaglobal.com/ref/jHlbB4mIkdS1"},
        {"logo", "https://user-images.githubusercontent.com/1294454/159202310-a0e38007-5e7c-4ba9-a32f-c8263a0291fe.jpg"},
        {"www", "https://zondaglobal.com"},
        {"api", {
            {"public", "https://{hostname}/API/Public"},
            {"private", "https://{hostname}/API/Trading/tradingApi.php"},
            {"v1_01Public", "https://api.{hostname}/rest"},
            {"v1_01Private", "https://api.{hostname}/rest"}
        }},
        {"doc", {
            "https://docs.zondacrypto.exchange/",
            "https://github.com/BitBayNet/API"
        }},
        {"support", "https://zondaglobal.com/en/helpdesk/zonda-exchange"},
        {"fees", "https://zondaglobal.com/legal/zonda-exchange/fees"}
    };

    api = {
        {"public", {
            {"get", {
                "{id}/all",
                "{id}/market",
                "{id}/orderbook",
                "{id}/ticker",
                "{id}/trades"
            }}
        }},
        {"private", {
            {"post", {
                "info",
                "trade",
                "cancel",
                "orderbook",
                "orders",
                "transfer",
                "withdraw",
                "history",
                "transactions"
            }}
        }},
        {"v1_01Public", {
            {"get", {
                "trading/ticker",
                "trading/ticker/{symbol}",
                "trading/stats",
                "trading/stats/{symbol}",
                "trading/orderbook/{symbol}",
                "trading/transactions/{symbol}",
                "trading/candle/history/{symbol}/{resolution}"
            }}
        }},
        {"v1_01Private", {
            {"get", {
                "api_payments/deposits/crypto/addresses",
                "payments/withdrawal/{detailId}",
                "payments/deposit/history",
                "payments/withdrawal/history",
                "trading/offer",
                "trading/config/{symbol}",
                "trading/history/transactions",
                "balances/BITBAY/history",
                "balances/BITBAY/balance",
                "fiat_cantor/rate/{symbol}",
                "fiat_cantor/history"
            }},
            {"post", {
                "trading/offer/{symbol}",
                "trading/config/{symbol}",
                "trading/withdraw",
                "balances/BITBAY/balance",
                "balances/BITBAY/balance/transfer/{source}/{destination}",
                "fiat_cantor/exchange"
            }},
            {"delete", {
                "trading/offer/{symbol}/{id}/{side}/{price}"
            }},
            {"put", {
                "balances/BITBAY/balance/{id}"
            }}
        }}
    };

    options = {
        {"fiat", {"PLN", "EUR", "USD", "GBP"}}
    };

    initializeApiEndpoints();
}

void ZondaAsync::initializeApiEndpoints() {
    // Initialize API endpoints
    apiEndpoints = {
        {"public", "https://" + hostname + "/API/Public"},
        {"private", "https://" + hostname + "/API/Trading/tradingApi.php"},
        {"v1_01Public", "https://api." + hostname + "/rest"},
        {"v1_01Private", "https://api." + hostname + "/rest"}
    };
}

String ZondaAsync::sign(const String& path, const String& api,
                       const String& method, const json& params,
                       const std::map<String, String>& headers,
                       const json& body) {
    auto query = this->omit(params, this->extractParams(path));
    auto url = this->urls["api"][api];
    url = this->implodeParams(url, {{"hostname", this->hostname}});
    
    if (api == "public" || api == "v1_01Public") {
        url += "/" + this->implodeParams(path, params);
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->checkRequiredCredentials();
        auto nonce = this->nonce().toString();
        auto request = this->extend({
            "tonce", nonce,
            "method", path,
            "currency", "BTC"  // default currency
        }, params);

        auto signature = this->hmac(this->encode(this->json(request)),
                                  this->encode(this->secret),
                                  "sha512");

        auto body = this->urlencode(this->extend(request, {
            "signature", signature
        }));

        headers["API-Key"] = this->apiKey;
        headers["API-Hash"] = signature;
        headers["Content-Type"] = "application/x-www-form-urlencoded";
        headers["Request-Timestamp"] = nonce;
    }
    
    return url;
}

String ZondaAsync::createNonce() {
    return std::to_string(this->milliseconds());
}

String ZondaAsync::createSignature(const String& nonce, const String& method,
                                 const String& path, const String& body) {
    auto message = nonce + method + path + body;
    unsigned char* digest = HMAC(EVP_sha512(), this->secret.c_str(), this->secret.length(),
                                reinterpret_cast<const unsigned char*>(message.c_str()),
                                message.length(), nullptr, nullptr);
    
    std::stringstream ss;
    for(int i = 0; i < 64; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
    }
    return ss.str();
}

// Market Data API
boost::future<std::vector<Market>> ZondaAsync::fetch_markets_async() {
    return async_request<std::vector<Market>>([this]() {
        return this->fetch_markets();
    });
}

boost::future<std::vector<Currency>> ZondaAsync::fetch_currencies_async() {
    return async_request<std::vector<Currency>>([this]() {
        return this->fetch_currencies();
    });
}

boost::future<Ticker> ZondaAsync::fetch_ticker_async(const std::string& symbol) {
    return async_request<Ticker>([this, symbol]() {
        return this->fetch_ticker(symbol);
    });
}

boost::future<std::vector<Ticker>> ZondaAsync::fetch_tickers_async(const std::vector<std::string>& symbols) {
    return async_request<std::vector<Ticker>>([this, symbols]() {
        return this->fetch_tickers(symbols);
    });
}

boost::future<OrderBook> ZondaAsync::fetch_order_book_async(const std::string& symbol,
                                                        const std::optional<int>& limit) {
    return async_request<OrderBook>([this, symbol, limit]() {
        return this->fetch_order_book(symbol, limit);
    });
}

boost::future<std::vector<Trade>> ZondaAsync::fetch_trades_async(const std::string& symbol,
                                                             const std::optional<long long>& since,
                                                             const std::optional<int>& limit) {
    return async_request<std::vector<Trade>>([this, symbol, since, limit]() {
        return this->fetch_trades(symbol, since, limit);
    });
}

boost::future<std::vector<OHLCV>> ZondaAsync::fetch_ohlcv_async(const std::string& symbol,
                                                             const std::string& timeframe,
                                                             const std::optional<long long>& since,
                                                             const std::optional<int>& limit) {
    return async_request<std::vector<OHLCV>>([this, symbol, timeframe, since, limit]() {
        return this->fetch_ohlcv(symbol, timeframe, since, limit);
    });
}

// Trading API
boost::future<Order> ZondaAsync::create_order_async(const std::string& symbol,
                                                const std::string& type,
                                                const std::string& side,
                                                double amount,
                                                const std::optional<double>& price) {
    return async_request<Order>([this, symbol, type, side, amount, price]() {
        return this->create_order(symbol, type, side, amount, price);
    });
}

boost::future<Order> ZondaAsync::cancel_order_async(const std::string& id,
                                                const std::string& symbol) {
    return async_request<Order>([this, id, symbol]() {
        return this->cancel_order(id, symbol);
    });
}

boost::future<Order> ZondaAsync::fetch_order_async(const std::string& id,
                                               const std::string& symbol) {
    return async_request<Order>([this, id, symbol]() {
        return this->fetch_order(id, symbol);
    });
}

boost::future<std::vector<Order>> ZondaAsync::fetch_open_orders_async(const std::string& symbol,
                                                                  const std::optional<long long>& since,
                                                                  const std::optional<int>& limit) {
    return async_request<std::vector<Order>>([this, symbol, since, limit]() {
        return this->fetch_open_orders(symbol, since, limit);
    });
}

boost::future<std::vector<Order>> ZondaAsync::fetch_closed_orders_async(const std::string& symbol,
                                                                    const std::optional<long long>& since,
                                                                    const std::optional<int>& limit) {
    return async_request<std::vector<Order>>([this, symbol, since, limit]() {
        return this->fetch_closed_orders(symbol, since, limit);
    });
}

boost::future<std::vector<Trade>> ZondaAsync::fetch_my_trades_async(const std::string& symbol,
                                                                const std::optional<long long>& since,
                                                                const std::optional<int>& limit) {
    return async_request<std::vector<Trade>>([this, symbol, since, limit]() {
        return this->fetch_my_trades(symbol, since, limit);
    });
}

// Account API
boost::future<Balance> ZondaAsync::fetch_balance_async() {
    return async_request<Balance>([this]() {
        return this->fetch_balance();
    });
}

boost::future<DepositAddress> ZondaAsync::fetch_deposit_address_async(const std::string& code,
                                                                  const std::optional<std::string>& network) {
    return async_request<DepositAddress>([this, code, network]() {
        return this->fetch_deposit_address(code, network);
    });
}

boost::future<std::vector<Transaction>> ZondaAsync::fetch_deposits_async(const std::optional<std::string>& code,
                                                                     const std::optional<long long>& since,
                                                                     const std::optional<int>& limit) {
    return async_request<std::vector<Transaction>>([this, code, since, limit]() {
        return this->fetch_deposits(code, since, limit);
    });
}

boost::future<std::vector<Transaction>> ZondaAsync::fetch_withdrawals_async(const std::optional<std::string>& code,
                                                                        const std::optional<long long>& since,
                                                                        const std::optional<int>& limit) {
    return async_request<std::vector<Transaction>>([this, code, since, limit]() {
        return this->fetch_withdrawals(code, since, limit);
    });
}

} // namespace ccxt
