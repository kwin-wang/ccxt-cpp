#include "ccxt/exchanges/async/gate_async.h"
#include "ccxt/error.h"
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/http.hpp>
#include <openssl/hmac.h>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace ccxt {

gate_async::gate_async(const Config& config) : async_client(config), gate(config) {}

// Helper function to get current timestamp in milliseconds
static long get_current_timestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

// Helper function for HMAC-SHA512 signing
static std::string hmac_sha512(const std::string& key, const std::string& data) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    
    HMAC_CTX* hmac = HMAC_CTX_new();
    HMAC_Init_ex(hmac, key.c_str(), key.length(), EVP_sha512(), nullptr);
    HMAC_Update(hmac, (unsigned char*)data.c_str(), data.length());
    HMAC_Final(hmac, hash, &hash_len);
    HMAC_CTX_free(hmac);

    std::stringstream ss;
    for(unsigned int i = 0; i < hash_len; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

void gate_async::sign_request(const std::string& method, const std::string& path,
                           std::map<std::string, std::string>& headers,
                           std::string& body) {
    if (!this->api_key.empty() && !this->secret.empty()) {
        long timestamp = get_current_timestamp();
        std::string msg = std::to_string(timestamp) + method + path;
        if (!body.empty()) {
            msg += body;
        }
        
        std::string signature = hmac_sha512(this->secret, msg);
        
        headers["KEY"] = this->api_key;
        headers["Timestamp"] = std::to_string(timestamp);
        headers["SIGN"] = signature;
        headers["Content-Type"] = "application/json";
    }
}

std::string gate_async::sign(const std::string& path, const std::string& method,
                          const std::string& body,
                          const std::map<std::string, std::string>& headers) {
    auto h = headers;
    auto b = body;
    sign_request(method, path, h, b);
    return b;
}

// Public API implementations
std::future<json> gate_async::fetch_time() {
    return public_get("/api/v4/time");
}

std::future<json> gate_async::fetch_markets() {
    return public_get("/api/v4/spot/currency_pairs");
}

std::future<json> gate_async::fetch_currencies() {
    return public_get("/api/v4/spot/currencies");
}

std::future<json> gate_async::fetch_ticker(const std::string& symbol) {
    return public_get("/api/v4/spot/tickers/" + this->market_id(symbol));
}

std::future<json> gate_async::fetch_tickers(const std::vector<std::string>& symbols) {
    std::map<std::string, std::string> params;
    if (!symbols.empty()) {
        std::string symbols_str;
        for (const auto& symbol : symbols) {
            if (!symbols_str.empty()) symbols_str += ",";
            symbols_str += this->market_id(symbol);
        }
        params["currency_pair"] = symbols_str;
    }
    return public_get("/api/v4/spot/tickers", params);
}

std::future<json> gate_async::fetch_order_book(const std::string& symbol, int limit) {
    std::map<std::string, std::string> params;
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    return public_get("/api/v4/spot/order_book/" + this->market_id(symbol), params);
}

std::future<json> gate_async::fetch_trades(const std::string& symbol, int limit) {
    std::map<std::string, std::string> params;
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    return public_get("/api/v4/spot/trades/" + this->market_id(symbol), params);
}

std::future<json> gate_async::fetch_ohlcv(const std::string& symbol,
                                       const std::string& timeframe,
                                       long since, int limit) {
    std::map<std::string, std::string> params;
    if (since > 0) {
        params["from"] = std::to_string(since / 1000);
    }
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    params["interval"] = timeframe;
    return public_get("/api/v4/spot/candlesticks/" + this->market_id(symbol), params);
}

std::future<json> gate_async::fetch_funding_rate(const std::string& symbol) {
    return public_get("/api/v4/futures/funding_rate/" + this->market_id(symbol));
}

std::future<json> gate_async::fetch_funding_rates(const std::vector<std::string>& symbols) {
    std::map<std::string, std::string> params;
    if (!symbols.empty()) {
        std::string symbols_str;
        for (const auto& symbol : symbols) {
            if (!symbols_str.empty()) symbols_str += ",";
            symbols_str += this->market_id(symbol);
        }
        params["contract"] = symbols_str;
    }
    return public_get("/api/v4/futures/funding_rates", params);
}

std::future<json> gate_async::fetch_trading_fees(const std::string& symbol) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["currency_pair"] = this->market_id(symbol);
    }
    return private_get("/api/v4/spot/fee", params);
}

std::future<json> gate_async::fetch_contract_size(const std::string& symbol) {
    return public_get("/api/v4/futures/contracts/" + this->market_id(symbol));
}

// Private API implementations
std::future<json> gate_async::create_order(const std::string& symbol,
                                        const std::string& type,
                                        const std::string& side,
                                        double amount,
                                        double price,
                                        const std::map<std::string, std::string>& params) {
    std::map<std::string, std::string> request = {
        {"currency_pair", this->market_id(symbol)},
        {"type", type},
        {"side", side},
        {"amount", std::to_string(amount)}
    };
    if (price > 0) {
        request["price"] = std::to_string(price);
    }
    for (const auto& param : params) {
        request[param.first] = param.second;
    }
    return private_post("/api/v4/spot/orders", request);
}

std::future<json> gate_async::create_reduce_only_order(const std::string& symbol,
                                                    const std::string& type,
                                                    const std::string& side,
                                                    double amount,
                                                    double price,
                                                    const std::map<std::string, std::string>& params) {
    auto request = params;
    request["reduce_only"] = "true";
    return create_order(symbol, type, side, amount, price, request);
}

std::future<json> gate_async::cancel_order(const std::string& id,
                                        const std::string& symbol) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["currency_pair"] = this->market_id(symbol);
    }
    return private_delete("/api/v4/spot/orders/" + id, params);
}

std::future<json> gate_async::cancel_all_orders(const std::string& symbol) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["currency_pair"] = this->market_id(symbol);
    }
    return private_delete("/api/v4/spot/orders", params);
}

std::future<json> gate_async::edit_order(const std::string& id,
                                      const std::string& symbol,
                                      const std::string& type,
                                      const std::string& side,
                                      double amount,
                                      double price,
                                      const std::map<std::string, std::string>& params) {
    std::map<std::string, std::string> request = {
        {"currency_pair", this->market_id(symbol)},
        {"type", type},
        {"side", side},
        {"amount", std::to_string(amount)}
    };
    if (price > 0) {
        request["price"] = std::to_string(price);
    }
    for (const auto& param : params) {
        request[param.first] = param.second;
    }
    return private_put("/api/v4/spot/orders/" + id, request);
}

std::future<json> gate_async::fetch_balance() {
    return private_get("/api/v4/spot/accounts");
}

std::future<json> gate_async::fetch_spot_balance() {
    return private_get("/api/v4/spot/accounts");
}

std::future<json> gate_async::fetch_futures_balance() {
    return private_get("/api/v4/futures/accounts");
}

std::future<json> gate_async::fetch_margin_balance() {
    return private_get("/api/v4/margin/accounts");
}

std::future<json> gate_async::fetch_open_orders(const std::string& symbol) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["currency_pair"] = this->market_id(symbol);
    }
    return private_get("/api/v4/spot/open_orders", params);
}

std::future<json> gate_async::fetch_closed_orders(const std::string& symbol,
                                               long since, int limit) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["currency_pair"] = this->market_id(symbol);
    }
    if (since > 0) {
        params["from"] = std::to_string(since / 1000);
    }
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    return private_get("/api/v4/spot/orders", params);
}

std::future<json> gate_async::fetch_order(const std::string& id,
                                       const std::string& symbol) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["currency_pair"] = this->market_id(symbol);
    }
    return private_get("/api/v4/spot/orders/" + id, params);
}

std::future<json> gate_async::fetch_orders(const std::string& symbol,
                                        long since, int limit) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["currency_pair"] = this->market_id(symbol);
    }
    if (since > 0) {
        params["from"] = std::to_string(since / 1000);
    }
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    return private_get("/api/v4/spot/orders", params);
}

std::future<json> gate_async::fetch_my_trades(const std::string& symbol,
                                           long since, int limit) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["currency_pair"] = this->market_id(symbol);
    }
    if (since > 0) {
        params["from"] = std::to_string(since / 1000);
    }
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    return private_get("/api/v4/spot/my_trades", params);
}

std::future<json> gate_async::fetch_positions(const std::string& symbol) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["contract"] = this->market_id(symbol);
    }
    return private_get("/api/v4/futures/positions", params);
}

std::future<json> gate_async::fetch_leverage_tiers() {
    return public_get("/api/v4/futures/tiers");
}

std::future<json> gate_async::fetch_funding_history(const std::string& symbol,
                                                 long since, int limit) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["contract"] = this->market_id(symbol);
    }
    if (since > 0) {
        params["from"] = std::to_string(since / 1000);
    }
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    return private_get("/api/v4/futures/funding_history", params);
}

// Margin Trading
std::future<json> gate_async::fetch_borrowing_rate(const std::string& code) {
    return private_get("/api/v4/margin/funding_rates/" + this->currency_id(code));
}

std::future<json> gate_async::fetch_borrowing_rates() {
    return private_get("/api/v4/margin/funding_rates");
}

std::future<json> gate_async::fetch_borrowing_interest(const std::string& code) {
    return private_get("/api/v4/margin/interests/" + this->currency_id(code));
}

std::future<json> gate_async::fetch_leverage(const std::string& symbol) {
    return private_get("/api/v4/futures/positions/" + this->market_id(symbol));
}

std::future<json> gate_async::set_leverage(int leverage, const std::string& symbol) {
    std::map<std::string, std::string> params = {
        {"leverage", std::to_string(leverage)},
        {"contract", this->market_id(symbol)}
    };
    return private_post("/api/v4/futures/positions/leverage", params);
}

std::future<json> gate_async::borrow_margin(const std::string& code, double amount) {
    std::map<std::string, std::string> params = {
        {"currency", this->currency_id(code)},
        {"amount", std::to_string(amount)}
    };
    return private_post("/api/v4/margin/loans", params);
}

std::future<json> gate_async::repay_margin(const std::string& code, double amount) {
    std::map<std::string, std::string> params = {
        {"currency", this->currency_id(code)},
        {"amount", std::to_string(amount)}
    };
    return private_post("/api/v4/margin/repayments", params);
}

// Account Management
std::future<json> gate_async::fetch_deposit_address(const std::string& code,
                                                 const std::map<std::string, std::string>& params) {
    auto request = params;
    request["currency"] = this->currency_id(code);
    return private_get("/api/v4/wallet/deposit_address", request);
}

std::future<json> gate_async::fetch_deposit_addresses(const std::vector<std::string>& codes) {
    std::map<std::string, std::string> params;
    if (!codes.empty()) {
        std::string currencies;
        for (const auto& code : codes) {
            if (!currencies.empty()) currencies += ",";
            currencies += this->currency_id(code);
        }
        params["currency"] = currencies;
    }
    return private_get("/api/v4/wallet/deposit_addresses", params);
}

std::future<json> gate_async::fetch_deposits(const std::string& code,
                                          long since, int limit) {
    std::map<std::string, std::string> params;
    if (!code.empty()) {
        params["currency"] = this->currency_id(code);
    }
    if (since > 0) {
        params["from"] = std::to_string(since / 1000);
    }
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    return private_get("/api/v4/wallet/deposits", params);
}

std::future<json> gate_async::fetch_withdrawals(const std::string& code,
                                             long since, int limit) {
    std::map<std::string, std::string> params;
    if (!code.empty()) {
        params["currency"] = this->currency_id(code);
    }
    if (since > 0) {
        params["from"] = std::to_string(since / 1000);
    }
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    return private_get("/api/v4/wallet/withdrawals", params);
}

std::future<json> gate_async::withdraw(const std::string& code,
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
        request["memo"] = tag;
    }
    for (const auto& param : params) {
        request[param.first] = param.second;
    }
    return private_post("/api/v4/wallet/withdrawals", request);
}

// HTTP request helper methods
std::future<json> gate_async::private_get(const std::string& path,
                                       const std::map<std::string, std::string>& params) {
    return request("GET", path, params, true);
}

std::future<json> gate_async::private_post(const std::string& path,
                                        const std::map<std::string, std::string>& params) {
    return request("POST", path, params, true);
}

std::future<json> gate_async::private_put(const std::string& path,
                                       const std::map<std::string, std::string>& params) {
    return request("PUT", path, params, true);
}

std::future<json> gate_async::private_delete(const std::string& path,
                                          const std::map<std::string, std::string>& params) {
    return request("DELETE", path, params, true);
}

std::future<json> gate_async::public_get(const std::string& path,
                                      const std::map<std::string, std::string>& params) {
    return request("GET", path, params, false);
}

} // namespace ccxt
