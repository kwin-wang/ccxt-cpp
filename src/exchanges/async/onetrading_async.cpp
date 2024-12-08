#include "ccxt/exchanges/async/onetrading_async.h"
#include "ccxt/error.h"
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/http.hpp>
#include <openssl/hmac.h>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace ccxt {

onetrading_async::onetrading_async(const Config& config) : async_client(config), onetrading(config) {}

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

void onetrading_async::sign_request(const std::string& method, const std::string& path,
                                 std::map<std::string, std::string>& headers,
                                 std::string& body) {
    if (!this->api_key.empty() && !this->secret.empty()) {
        long timestamp = get_current_timestamp();
        std::string msg = std::to_string(timestamp) + method + path;
        if (!body.empty()) {
            msg += body;
        }
        
        std::string signature = hmac_sha256(this->secret, msg);
        
        headers["ONE-ACCESS-KEY"] = this->api_key;
        headers["ONE-ACCESS-TIMESTAMP"] = std::to_string(timestamp);
        headers["ONE-ACCESS-SIGNATURE"] = signature;
        headers["Content-Type"] = "application/json";
    }
}

std::string onetrading_async::sign(const std::string& path, const std::string& method,
                                const std::string& body,
                                const std::map<std::string, std::string>& headers) {
    auto h = headers;
    auto b = body;
    sign_request(method, path, h, b);
    return b;
}

// Public API implementations
std::future<json> onetrading_async::fetch_time() {
    return public_get("/api/v1/time");
}

std::future<json> onetrading_async::fetch_markets() {
    return public_get("/api/v1/markets");
}

std::future<json> onetrading_async::fetch_currencies() {
    return public_get("/api/v1/currencies");
}

std::future<json> onetrading_async::fetch_ticker(const std::string& symbol) {
    return public_get("/api/v1/ticker/" + this->market_id(symbol));
}

std::future<json> onetrading_async::fetch_tickers(const std::vector<std::string>& symbols) {
    std::map<std::string, std::string> params;
    if (!symbols.empty()) {
        std::string symbols_str;
        for (const auto& symbol : symbols) {
            if (!symbols_str.empty()) symbols_str += ",";
            symbols_str += this->market_id(symbol);
        }
        params["symbols"] = symbols_str;
    }
    return public_get("/api/v1/tickers", params);
}

std::future<json> onetrading_async::fetch_order_book(const std::string& symbol, int limit) {
    std::map<std::string, std::string> params;
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    return public_get("/api/v1/depth/" + this->market_id(symbol), params);
}

std::future<json> onetrading_async::fetch_trades(const std::string& symbol, int limit) {
    std::map<std::string, std::string> params;
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    return public_get("/api/v1/trades/" + this->market_id(symbol), params);
}

std::future<json> onetrading_async::fetch_ohlcv(const std::string& symbol,
                                             const std::string& timeframe,
                                             long since, int limit) {
    std::map<std::string, std::string> params;
    if (since > 0) {
        params["start_time"] = std::to_string(since);
    }
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    params["interval"] = timeframe;
    return public_get("/api/v1/klines/" + this->market_id(symbol), params);
}

std::future<json> onetrading_async::fetch_trading_fees(const std::string& symbol) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["symbol"] = this->market_id(symbol);
    }
    return public_get("/api/v1/trading-fees", params);
}

// Private API implementations
std::future<json> onetrading_async::create_order(const std::string& symbol,
                                              const std::string& type,
                                              const std::string& side,
                                              double amount,
                                              double price,
                                              const std::map<std::string, std::string>& params) {
    std::map<std::string, std::string> request = {
        {"symbol", this->market_id(symbol)},
        {"type", type},
        {"side", side},
        {"quantity", std::to_string(amount)}
    };
    if (price > 0) {
        request["price"] = std::to_string(price);
    }
    for (const auto& param : params) {
        request[param.first] = param.second;
    }
    return private_post("/api/v1/order", request);
}

std::future<json> onetrading_async::cancel_order(const std::string& id,
                                              const std::string& symbol) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["symbol"] = this->market_id(symbol);
    }
    return private_delete("/api/v1/order/" + id, params);
}

std::future<json> onetrading_async::cancel_all_orders(const std::string& symbol) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["symbol"] = this->market_id(symbol);
    }
    return private_delete("/api/v1/orders", params);
}

std::future<json> onetrading_async::edit_order(const std::string& id,
                                            const std::string& symbol,
                                            const std::string& type,
                                            const std::string& side,
                                            double amount,
                                            double price,
                                            const std::map<std::string, std::string>& params) {
    std::map<std::string, std::string> request = {
        {"symbol", this->market_id(symbol)},
        {"type", type},
        {"side", side},
        {"quantity", std::to_string(amount)}
    };
    if (price > 0) {
        request["price"] = std::to_string(price);
    }
    for (const auto& param : params) {
        request[param.first] = param.second;
    }
    return private_put("/api/v1/order/" + id, request);
}

std::future<json> onetrading_async::fetch_balance() {
    return private_get("/api/v1/account/balances");
}

std::future<json> onetrading_async::fetch_open_orders(const std::string& symbol) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["symbol"] = this->market_id(symbol);
    }
    return private_get("/api/v1/open-orders", params);
}

std::future<json> onetrading_async::fetch_closed_orders(const std::string& symbol,
                                                     long since, int limit) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["symbol"] = this->market_id(symbol);
    }
    if (since > 0) {
        params["start_time"] = std::to_string(since);
    }
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    return private_get("/api/v1/orders", params);
}

std::future<json> onetrading_async::fetch_order(const std::string& id,
                                             const std::string& symbol) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["symbol"] = this->market_id(symbol);
    }
    return private_get("/api/v1/order/" + id, params);
}

std::future<json> onetrading_async::fetch_orders(const std::string& symbol,
                                              long since, int limit) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["symbol"] = this->market_id(symbol);
    }
    if (since > 0) {
        params["start_time"] = std::to_string(since);
    }
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    return private_get("/api/v1/orders", params);
}

std::future<json> onetrading_async::fetch_my_trades(const std::string& symbol,
                                                 long since, int limit) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["symbol"] = this->market_id(symbol);
    }
    if (since > 0) {
        params["start_time"] = std::to_string(since);
    }
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    return private_get("/api/v1/my-trades", params);
}

std::future<json> onetrading_async::fetch_ledger(const std::string& code,
                                              long since, int limit) {
    std::map<std::string, std::string> params;
    if (!code.empty()) {
        params["currency"] = this->currency_id(code);
    }
    if (since > 0) {
        params["start_time"] = std::to_string(since);
    }
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    return private_get("/api/v1/ledger", params);
}

std::future<json> onetrading_async::fetch_deposits(const std::string& code,
                                                long since, int limit) {
    std::map<std::string, std::string> params;
    if (!code.empty()) {
        params["currency"] = this->currency_id(code);
    }
    if (since > 0) {
        params["start_time"] = std::to_string(since);
    }
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    return private_get("/api/v1/deposits", params);
}

std::future<json> onetrading_async::fetch_withdrawals(const std::string& code,
                                                   long since, int limit) {
    std::map<std::string, std::string> params;
    if (!code.empty()) {
        params["currency"] = this->currency_id(code);
    }
    if (since > 0) {
        params["start_time"] = std::to_string(since);
    }
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    return private_get("/api/v1/withdrawals", params);
}

std::future<json> onetrading_async::fetch_accounts() {
    return private_get("/api/v1/accounts");
}

std::future<json> onetrading_async::create_deposit_address(const std::string& code,
                                                        const std::map<std::string, std::string>& params) {
    std::map<std::string, std::string> request = {
        {"currency", this->currency_id(code)}
    };
    for (const auto& param : params) {
        request[param.first] = param.second;
    }
    return private_post("/api/v1/deposit-address", request);
}

std::future<json> onetrading_async::fetch_deposit_address(const std::string& code,
                                                       const std::map<std::string, std::string>& params) {
    std::map<std::string, std::string> request = {
        {"currency", this->currency_id(code)}
    };
    for (const auto& param : params) {
        request[param.first] = param.second;
    }
    return private_get("/api/v1/deposit-address", request);
}

std::future<json> onetrading_async::withdraw(const std::string& code,
                                          double amount,
                                          const std::string& address,
                                          const std::string& tag,
                                          const std::map<std::string, std::string>& params) {
    std::map<std::string, std::string> request = {
        {"currency", this->currency_id(code)},
        {"amount", std::to_string(amount)},
        {"address", address}
    };
    if (!tag.empty()) {
        request["tag"] = tag;
    }
    for (const auto& param : params) {
        request[param.first] = param.second;
    }
    return private_post("/api/v1/withdraw", request);
}

// HTTP request helper methods
std::future<json> onetrading_async::private_get(const std::string& path,
                                             const std::map<std::string, std::string>& params) {
    return request("GET", path, params, true);
}

std::future<json> onetrading_async::private_post(const std::string& path,
                                              const std::map<std::string, std::string>& params) {
    return request("POST", path, params, true);
}

std::future<json> onetrading_async::private_put(const std::string& path,
                                             const std::map<std::string, std::string>& params) {
    return request("PUT", path, params, true);
}

std::future<json> onetrading_async::private_delete(const std::string& path,
                                                const std::map<std::string, std::string>& params) {
    return request("DELETE", path, params, true);
}

std::future<json> onetrading_async::public_get(const std::string& path,
                                            const std::map<std::string, std::string>& params) {
    return request("GET", path, params, false);
}

} // namespace ccxt
