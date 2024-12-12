#include "ccxt/exchanges/bitcoincom.h"
#include "ccxt/error.h"
#include <openssl/hmac.h>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

namespace ccxt {

bitcoincom::bitcoincom(const Config& config) : exchange(config) {
    this->describe_exchange(R"({
        "id": "bitcoincom",
        "name": "Bitcoin.com",
        "countries": ["JP"],
        "version": "v2",
        "rateLimit": 100,
        "has": {
            "fetchMarkets": true,
            "fetchCurrencies": true,
            "fetchTicker": true,
            "fetchTickers": true,
            "fetchOrderBook": true,
            "fetchTrades": true,
            "fetchOHLCV": true,
            "fetchBalance": true,
            "createOrder": true,
            "cancelOrder": true,
            "cancelAllOrders": true,
            "editOrder": true,
            "fetchOrder": true,
            "fetchOrders": true,
            "fetchOpenOrders": true,
            "fetchClosedOrders": true,
            "fetchMyTrades": true,
            "fetchTradingFees": true,
            "fetchTradingFee": true,
            "fetchDepositAddress": true,
            "fetchDeposits": true,
            "fetchWithdrawals": true,
            "withdraw": true
        },
        "urls": {
            "logo": "https://user-images.githubusercontent.com/1294454/97296144-514fa300-1861-11eb-952b-3d55d492200b.jpg",
            "api": {
                "public": "https://api.exchange.bitcoin.com/api/2",
                "private": "https://api.exchange.bitcoin.com/api/2"
            },
            "www": "https://exchange.bitcoin.com",
            "doc": "https://api.exchange.bitcoin.com/api/2/explore",
            "fees": "https://exchange.bitcoin.com/fees"
        },
        "api": {
            "public": {
                "get": [
                    "public/symbol",
                    "public/ticker",
                    "public/ticker/{symbol}",
                    "public/orderbook/{symbol}",
                    "public/trades/{symbol}",
                    "public/candles/{symbol}"
                ]
            },
            "private": {
                "get": [
                    "spot/balance",
                    "spot/order",
                    "spot/order/{clientOrderId}",
                    "spot/fee/{symbol}",
                    "spot/fee/all",
                    "spot/trades",
                    "spot/trades/history",
                    "spot/orders",
                    "spot/orders/active",
                    "spot/orders/recent",
                    "wallet/crypto/address/{currency}"
                ],
                "post": [
                    "spot/order",
                    "wallet/crypto/withdraw"
                ],
                "put": [
                    "spot/order/{clientOrderId}"
                ],
                "delete": [
                    "spot/order",
                    "spot/order/{clientOrderId}"
                ]
            }
        }
    })"_json);

    init();
}

void bitcoincom::init() {
    exchange::init();
}

// Helper function to get current timestamp in milliseconds
static long get_current_timestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

// Helper function for HMAC-SHA256 signing
static std::string hmac_sha256(const std::string& key, const std::string& data) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    
    HMAC_CTX* hmac = HMAC_CTX_new();
    HMAC_Init_ex(hmac, key.c_str(), key.length(), EVP_sha256(), nullptr);
    HMAC_Update(hmac, (unsigned char*)data.c_str(), data.length());
    HMAC_Final(hmac, hash, &hash_len);
    HMAC_CTX_free(hmac);

    std::stringstream ss;
    for(unsigned int i = 0; i < hash_len; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

std::string bitcoincom::sign(const std::string& path, const std::string& method,
                          const std::string& body,
                          const std::map<std::string, std::string>& headers) {
    if (this->api_key.empty() || this->secret.empty()) {
        return body;
    }

    long timestamp = get_current_timestamp();
    std::string data = std::to_string(timestamp) + method + path;
    if (!body.empty()) {
        data += body;
    }

    std::string signature = hmac_sha256(this->secret, data);
    auto h = headers;
    h["AUTH-API-KEY"] = this->api_key;
    h["AUTH-API-TIMESTAMP"] = std::to_string(timestamp);
    h["AUTH-API-SIGNATURE"] = signature;

    return body;
}

// Public API implementations
json bitcoincom::fetch_markets() {
    return public_get("/public/symbol");
}

json bitcoincom::fetch_currencies() {
    return public_get("/public/currency");
}

json bitcoincom::fetch_ticker(const std::string& symbol) {
    return public_get("/public/ticker/" + this->market_id(symbol));
}

json bitcoincom::fetch_tickers(const std::vector<std::string>& symbols) {
    json response = public_get("/public/ticker");
    if (!symbols.empty()) {
        json result = json::array();
        for (const auto& ticker : response) {
            std::string symbol = ticker["symbol"].get<std::string>();
            if (std::find(symbols.begin(), symbols.end(), symbol) != symbols.end()) {
                result.push_back(ticker);
            }
        }
        return result;
    }
    return response;
}

json bitcoincom::fetch_order_book(const std::string& symbol, int limit) {
    std::map<std::string, std::string> params;
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    return public_get("/public/orderbook/" + this->market_id(symbol), params);
}

json bitcoincom::fetch_trades(const std::string& symbol, int limit) {
    std::map<std::string, std::string> params;
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    return public_get("/public/trades/" + this->market_id(symbol), params);
}

json bitcoincom::fetch_ohlcv(const std::string& symbol, const std::string& timeframe,
                          long since, int limit) {
    std::map<std::string, std::string> params;
    if (since > 0) {
        params["from"] = std::to_string(since);
    }
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    params["period"] = timeframe;
    return public_get("/public/candles/" + this->market_id(symbol), params);
}

json bitcoincom::fetch_trading_fees(const std::string& symbol) {
    if (!symbol.empty()) {
        return private_get("/spot/fee/" + this->market_id(symbol));
    }
    return private_get("/spot/fee/all");
}

// Private API implementations
json bitcoincom::create_order(const std::string& symbol, const std::string& type,
                           const std::string& side, double amount, double price,
                           const std::map<std::string, std::string>& params) {
    std::map<std::string, std::string> request = {
        {"symbol", this->market_id(symbol)},
        {"side", side},
        {"type", type},
        {"quantity", std::to_string(amount)}
    };
    if (price > 0) {
        request["price"] = std::to_string(price);
    }
    for (const auto& param : params) {
        request[param.first] = param.second;
    }
    return private_post("/spot/order", request);
}

json bitcoincom::cancel_order(const std::string& id, const std::string& symbol) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["symbol"] = this->market_id(symbol);
    }
    return private_delete("/spot/order/" + id, params);
}

json bitcoincom::cancel_all_orders(const std::string& symbol) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["symbol"] = this->market_id(symbol);
    }
    return private_delete("/spot/order", params);
}

json bitcoincom::edit_order(const std::string& id, const std::string& symbol,
                         const std::string& type, const std::string& side,
                         double amount, double price,
                         const std::map<std::string, std::string>& params) {
    std::map<std::string, std::string> request = {
        {"symbol", this->market_id(symbol)},
        {"side", side},
        {"type", type},
        {"quantity", std::to_string(amount)}
    };
    if (price > 0) {
        request["price"] = std::to_string(price);
    }
    for (const auto& param : params) {
        request[param.first] = param.second;
    }
    return private_put("/spot/order/" + id, request);
}

json bitcoincom::fetch_balance() {
    return private_get("/spot/balance");
}

json bitcoincom::fetch_open_orders(const std::string& symbol) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["symbol"] = this->market_id(symbol);
    }
    return private_get("/spot/orders/active", params);
}

json bitcoincom::fetch_closed_orders(const std::string& symbol, long since, int limit) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["symbol"] = this->market_id(symbol);
    }
    if (since > 0) {
        params["from"] = std::to_string(since);
    }
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    return private_get("/spot/orders/recent", params);
}

json bitcoincom::fetch_order(const std::string& id, const std::string& symbol) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["symbol"] = this->market_id(symbol);
    }
    return private_get("/spot/order/" + id, params);
}

json bitcoincom::fetch_orders(const std::string& symbol, long since, int limit) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["symbol"] = this->market_id(symbol);
    }
    if (since > 0) {
        params["from"] = std::to_string(since);
    }
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    return private_get("/spot/orders", params);
}

json bitcoincom::fetch_my_trades(const std::string& symbol, long since, int limit) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["symbol"] = this->market_id(symbol);
    }
    if (since > 0) {
        params["from"] = std::to_string(since);
    }
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    return private_get("/spot/trades/history", params);
}

json bitcoincom::fetch_trading_fee(const std::string& symbol) {
    return private_get("/spot/fee/" + this->market_id(symbol));
}

json bitcoincom::fetch_deposit_address(const std::string& code,
                                   const std::map<std::string, std::string>& params) {
    return private_get("/wallet/crypto/address/" + this->currency_id(code), params);
}

json bitcoincom::fetch_deposits(const std::string& code, long since, int limit) {
    std::map<std::string, std::string> params;
    if (!code.empty()) {
        params["currency"] = this->currency_id(code);
    }
    if (since > 0) {
        params["from"] = std::to_string(since);
    }
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    return private_get("/wallet/crypto/deposits", params);
}

json bitcoincom::fetch_withdrawals(const std::string& code, long since, int limit) {
    std::map<std::string, std::string> params;
    if (!code.empty()) {
        params["currency"] = this->currency_id(code);
    }
    if (since > 0) {
        params["from"] = std::to_string(since);
    }
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    return private_get("/wallet/crypto/withdrawals", params);
}

json bitcoincom::withdraw(const std::string& code, double amount,
                       const std::string& address, const std::string& tag,
                       const std::map<std::string, std::string>& params) {
    std::map<std::string, std::string> request = {
        {"currency", this->currency_id(code)},
        {"amount", std::to_string(amount)},
        {"address", address}
    };
    if (!tag.empty()) {
        request["paymentId"] = tag;
    }
    for (const auto& param : params) {
        request[param.first] = param.second;
    }
    return private_post("/wallet/crypto/withdraw", request);
}

// Async Market Data Methods
boost::future<json> bitcoincom::fetch_markets_async(const json& params) {
    return boost::async([this, params]() {
        return this->fetch_markets();
    });
}

boost::future<json> bitcoincom::fetch_currencies_async(const json& params) {
    return boost::async([this, params]() {
        return this->fetch_currencies();
    });
}

boost::future<json> bitcoincom::fetch_ticker_async(const std::string& symbol, const json& params) {
    return boost::async([this, symbol]() {
        return this->fetch_ticker(symbol);
    });
}

boost::future<json> bitcoincom::fetch_tickers_async(const std::vector<std::string>& symbols, const json& params) {
    return boost::async([this, symbols]() {
        return this->fetch_tickers(symbols);
    });
}

boost::future<json> bitcoincom::fetch_order_book_async(const std::string& symbol, int limit, const json& params) {
    return boost::async([this, symbol, limit]() {
        return this->fetch_order_book(symbol, limit);
    });
}

boost::future<json> bitcoincom::fetch_trades_async(const std::string& symbol, int limit, const json& params) {
    return boost::async([this, symbol, limit]() {
        return this->fetch_trades(symbol, limit);
    });
}

boost::future<json> bitcoincom::fetch_ohlcv_async(const std::string& symbol, const std::string& timeframe,
                                               long since, int limit, const json& params) {
    return boost::async([this, symbol, timeframe, since, limit]() {
        return this->fetch_ohlcv(symbol, timeframe, since, limit);
    });
}

boost::future<json> bitcoincom::fetch_trading_fees_async(const std::string& symbol, const json& params) {
    return boost::async([this, symbol]() {
        return this->fetch_trading_fees(symbol);
    });
}

// Async Trading Methods
boost::future<json> bitcoincom::create_order_async(const std::string& symbol, const std::string& type,
                                               const std::string& side, double amount, double price,
                                               const json& params) {
    return boost::async([this, symbol, type, side, amount, price]() {
        return this->create_order(symbol, type, side, amount, price);
    });
}

boost::future<json> bitcoincom::cancel_order_async(const std::string& id, const std::string& symbol,
                                               const json& params) {
    return boost::async([this, id, symbol]() {
        return this->cancel_order(id, symbol);
    });
}

boost::future<json> bitcoincom::cancel_all_orders_async(const std::string& symbol, const json& params) {
    return boost::async([this, symbol]() {
        return this->cancel_all_orders(symbol);
    });
}

boost::future<json> bitcoincom::edit_order_async(const std::string& id, const std::string& symbol,
                                             const std::string& type, const std::string& side,
                                             double amount, double price, const json& params) {
    return boost::async([this, id, symbol, type, side, amount, price]() {
        return this->edit_order(id, symbol, type, side, amount, price);
    });
}

// Async Account/Balance Methods
boost::future<json> bitcoincom::fetch_balance_async(const json& params) {
    return boost::async([this]() {
        return this->fetch_balance();
    });
}

boost::future<json> bitcoincom::fetch_open_orders_async(const std::string& symbol, const json& params) {
    return boost::async([this, symbol]() {
        return this->fetch_open_orders(symbol);
    });
}

boost::future<json> bitcoincom::fetch_closed_orders_async(const std::string& symbol, long since,
                                                      int limit, const json& params) {
    return boost::async([this, symbol, since, limit]() {
        return this->fetch_closed_orders(symbol, since, limit);
    });
}

boost::future<json> bitcoincom::fetch_order_async(const std::string& id, const std::string& symbol,
                                              const json& params) {
    return boost::async([this, id, symbol]() {
        return this->fetch_order(id, symbol);
    });
}

boost::future<json> bitcoincom::fetch_orders_async(const std::string& symbol, long since,
                                               int limit, const json& params) {
    return boost::async([this, symbol, since, limit]() {
        return this->fetch_orders(symbol, since, limit);
    });
}

boost::future<json> bitcoincom::fetch_my_trades_async(const std::string& symbol, long since,
                                                  int limit, const json& params) {
    return boost::async([this, symbol, since, limit]() {
        return this->fetch_my_trades(symbol, since, limit);
    });
}

boost::future<json> bitcoincom::fetch_trading_fee_async(const std::string& symbol, const json& params) {
    return boost::async([this, symbol]() {
        return this->fetch_trading_fee(symbol);
    });
}

// Async Account Management Methods
boost::future<json> bitcoincom::fetch_deposit_address_async(const std::string& code, const json& params) {
    return boost::async([this, code]() {
        return this->fetch_deposit_address(code);
    });
}

boost::future<json> bitcoincom::fetch_deposits_async(const std::string& code, long since,
                                                 int limit, const json& params) {
    return boost::async([this, code, since, limit]() {
        return this->fetch_deposits(code, since, limit);
    });
}

boost::future<json> bitcoincom::fetch_withdrawals_async(const std::string& code, long since,
                                                    int limit, const json& params) {
    return boost::async([this, code, since, limit]() {
        return this->fetch_withdrawals(code, since, limit);
    });
}

boost::future<json> bitcoincom::withdraw_async(const std::string& code, double amount,
                                           const std::string& address, const std::string& tag,
                                           const json& params) {
    return boost::async([this, code, amount, address, tag]() {
        return this->withdraw(code, amount, address, tag);
    });
}

// HTTP request helper methods
json bitcoincom::private_get(const std::string& path,
                          const std::map<std::string, std::string>& params) {
    return request("GET", path, params, true);
}

json bitcoincom::private_post(const std::string& path,
                           const std::map<std::string, std::string>& params) {
    return request("POST", path, params, true);
}

json bitcoincom::private_put(const std::string& path,
                          const std::map<std::string, std::string>& params) {
    return request("PUT", path, params, true);
}

json bitcoincom::private_delete(const std::string& path,
                             const std::map<std::string, std::string>& params) {
    return request("DELETE", path, params, true);
}

json bitcoincom::public_get(const std::string& path,
                         const std::map<std::string, std::string>& params) {
    return request("GET", path, params, false);
}

} // namespace ccxt
