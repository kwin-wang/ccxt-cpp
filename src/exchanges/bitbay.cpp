#include "ccxt/exchanges/bitbay.h"
#include "ccxt/error.h"
#include <openssl/hmac.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <boost/thread/future.hpp>

namespace ccxt {

bitbay::bitbay(const Config& config) : exchange(config) {
    this->describe_exchange(R"({
        "id": "bitbay",
        "name": "BitBay",
        "countries": ["MT", "EU"],
        "rateLimit": 1000,
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
            "withdraw": true,
            "transfer": true
        },
        "urls": {
            "logo": "https://user-images.githubusercontent.com/1294454/27766132-978a7bd8-5ece-11e7-9540-bc96d1e9bbb8.jpg",
            "api": {
                "public": "https://api.bitbay.net/rest",
                "private": "https://api.bitbay.net/rest",
                "v1_01Public": "https://api.bitbay.net/rest/trading/ticker",
                "v1_01Private": "https://api.bitbay.net/rest/trading/history/transactions"
            },
            "www": "https://bitbay.net",
            "doc": [
                "https://bitbay.net/public-api",
                "https://bitbay.net/en/private-api"
            ],
            "fees": "https://bitbay.net/en/fees"
        }
    })"_json);

    init();
}

void bitbay::init() {
    exchange::init();
}

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

std::string bitbay::get_uuid() {
    boost::uuids::random_generator gen;
    boost::uuids::uuid uuid = gen();
    return boost::uuids::to_string(uuid);
}

std::string bitbay::sign(const std::string& path, const std::string& method,
                      const std::string& body,
                      const std::map<std::string, std::string>& headers) {
    if (this->api_key.empty() || this->config_.secret.empty()) {
        return body;
    }

    long timestamp = get_current_timestamp();
    std::string data = std::to_string(timestamp) + method + path;
    if (!body.empty()) {
        data += body;
    }

    std::string signature = hmac_sha512(this->config_.secret, data);
    auto h = headers;
    h["API-Key"] = this->api_key;
    h["API-Hash"] = signature;
    h["Request-Timestamp"] = std::to_string(timestamp);
    h["Operation-Id"] = get_uuid();

    return body;
}

// Public API implementations
json bitbay::fetch_markets() {
    return public_get("/trading/ticker");
}

json bitbay::fetch_currencies() {
    return public_get("/currencies");
}

json bitbay::fetch_ticker(const std::string& symbol) {
    return public_get("/trading/ticker/" + this->market_id(symbol));
}

json bitbay::fetch_tickers(const std::vector<std::string>& symbols) {
    json response = public_get("/trading/ticker");
    if (!symbols.empty()) {
        json result = json::array();
        for (const auto& ticker : response) {
            std::string symbol = ticker["market"].get<std::string>();
            if (std::find(symbols.begin(), symbols.end(), symbol) != symbols.end()) {
                result.push_back(ticker);
            }
        }
        return result;
    }
    return response;
}

json bitbay::fetch_order_book(const std::string& symbol, int limit) {
    std::map<std::string, std::string> params;
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    return public_get("/trading/orderbook/" + this->market_id(symbol), params);
}

json bitbay::fetch_trades(const std::string& symbol, int limit) {
    std::map<std::string, std::string> params;
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    return public_get("/trading/transactions/" + this->market_id(symbol), params);
}

json bitbay::fetch_ohlcv(const std::string& symbol, const std::string& timeframe,
                      long since, int limit) {
    std::map<std::string, std::string> params;
    if (since > 0) {
        params["from"] = std::to_string(since);
    }
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    params["resolution"] = timeframe;
    return public_get("/trading/candle/history/" + this->market_id(symbol), params);
}

json bitbay::fetch_trading_fees(const std::string& symbol) {
    if (!symbol.empty()) {
        return private_get("/trading/fee/" + this->market_id(symbol));
    }
    return private_get("/trading/fee");
}

// Private API implementations
json bitbay::create_order(const std::string& symbol, const std::string& type,
                       const std::string& side, double amount, double price,
                       const std::map<std::string, std::string>& params) {
    std::map<std::string, std::string> request = {
        {"market", this->market_id(symbol)},
        {"offerType", side},
        {"mode", type},
        {"amount", std::to_string(amount)}
    };
    if (price > 0) {
        request["rate"] = std::to_string(price);
    }
    for (const auto& param : params) {
        request[param.first] = param.second;
    }
    return private_post("/trading/offer", request);
}

json bitbay::cancel_order(const std::string& id, const std::string& symbol) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["market"] = this->market_id(symbol);
    }
    return private_delete("/trading/offer/" + id, params);
}

json bitbay::cancel_all_orders(const std::string& symbol) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["market"] = this->market_id(symbol);
    }
    return private_delete("/trading/offer", params);
}

json bitbay::edit_order(const std::string& id, const std::string& symbol,
                     const std::string& type, const std::string& side,
                     double amount, double price,
                     const std::map<std::string, std::string>& params) {
    std::map<std::string, std::string> request = {
        {"market", this->market_id(symbol)},
        {"offerType", side},
        {"mode", type},
        {"amount", std::to_string(amount)}
    };
    if (price > 0) {
        request["rate"] = std::to_string(price);
    }
    for (const auto& param : params) {
        request[param.first] = param.second;
    }
    return private_put("/trading/offer/" + id, request);
}

json bitbay::fetch_balance() {
    return private_get("/balances/BITBAY/balance");
}

json bitbay::fetch_open_orders(const std::string& symbol) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["market"] = this->market_id(symbol);
    }
    return private_get("/trading/offer", params);
}

json bitbay::fetch_closed_orders(const std::string& symbol, long since, int limit) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["market"] = this->market_id(symbol);
    }
    if (since > 0) {
        params["fromTime"] = std::to_string(since);
    }
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    params["status"] = "completed";
    return private_get("/trading/history/orders", params);
}

json bitbay::fetch_order(const std::string& id, const std::string& symbol) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["market"] = this->market_id(symbol);
    }
    return private_get("/trading/offer/" + id, params);
}

json bitbay::fetch_orders(const std::string& symbol, long since, int limit) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["market"] = this->market_id(symbol);
    }
    if (since > 0) {
        params["fromTime"] = std::to_string(since);
    }
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    return private_get("/trading/history/orders", params);
}

json bitbay::fetch_my_trades(const std::string& symbol, long since, int limit) {
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["market"] = this->market_id(symbol);
    }
    if (since > 0) {
        params["fromTime"] = std::to_string(since);
    }
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    return private_get("/trading/history/transactions", params);
}

json bitbay::fetch_trading_fee(const std::string& symbol) {
    return private_get("/trading/fee/" + this->market_id(symbol));
}

// Additional BitBay-specific implementations
json bitbay::fetch_funding_fees() {
    return private_get("/trading/config");
}

json bitbay::fetch_transaction_history(const std::string& code, long since, int limit) {
    std::map<std::string, std::string> params;
    if (!code.empty()) {
        params["currency"] = this->currency_id(code);
    }
    if (since > 0) {
        params["fromTime"] = std::to_string(since);
    }
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    return private_get("/history/transactions", params);
}

json bitbay::fetch_wallets() {
    return private_get("/balances/BITBAY/wallets");
}

json bitbay::transfer(const std::string& code, double amount,
                   const std::string& fromAccount, const std::string& toAccount) {
    std::map<std::string, std::string> params = {
        {"currency", this->currency_id(code)},
        {"amount", std::to_string(amount)},
        {"fromWallet", fromAccount},
        {"toWallet", toAccount}
    };
    return private_post("/balances/BITBAY/transfer", params);
}

json bitbay::fetch_deposit_address(const std::string& code,
                               const std::map<std::string, std::string>& params) {
    return private_get("/deposits/" + this->currency_id(code) + "/address", params);
}

json bitbay::fetch_deposits(const std::string& code, long since, int limit) {
    std::map<std::string, std::string> params;
    if (!code.empty()) {
        params["currency"] = this->currency_id(code);
    }
    if (since > 0) {
        params["fromTime"] = std::to_string(since);
    }
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    return private_get("/history/deposits", params);
}

json bitbay::fetch_withdrawals(const std::string& code, long since, int limit) {
    std::map<std::string, std::string> params;
    if (!code.empty()) {
        params["currency"] = this->currency_id(code);
    }
    if (since > 0) {
        params["fromTime"] = std::to_string(since);
    }
    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }
    return private_get("/history/withdrawals", params);
}

json bitbay::withdraw(const std::string& code, double amount,
                   const std::string& address, const std::string& tag,
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
    return private_post("/withdrawals/crypto", request);
}

// HTTP request helper methods
json bitbay::private_get(const std::string& path,
                      const std::map<std::string, std::string>& params) {
    return request("GET", path, params, true);
}

json bitbay::private_post(const std::string& path,
                       const std::map<std::string, std::string>& params) {
    return request("POST", path, params, true);
}

json bitbay::private_put(const std::string& path,
                      const std::map<std::string, std::string>& params) {
    return request("PUT", path, params, true);
}

json bitbay::private_delete(const std::string& path,
                         const std::map<std::string, std::string>& params) {
    return request("DELETE", path, params, true);
}

json bitbay::public_get(const std::string& path,
                     const std::map<std::string, std::string>& params) {
    return request("GET", path, params, false);
}

// Async API implementations
boost::future<json> bitbay::fetch_markets_async() {
    return boost::async([this] { return fetch_markets(); });
}

boost::future<json> bitbay::fetch_ticker_async(const std::string& symbol) {
    return boost::async([this, symbol] { return fetch_ticker(symbol); });
}

boost::future<json> bitbay::fetch_tickers_async(const std::vector<std::string>& symbols) {
    return boost::async([this, symbols] { return fetch_tickers(symbols); });
}

boost::future<json> bitbay::fetch_order_book_async(const std::string& symbol, int limit) {
    return boost::async([this, symbol, limit] { return fetch_order_book(symbol, limit); });
}

boost::future<json> bitbay::fetch_trades_async(const std::string& symbol, int limit) {
    return boost::async([this, symbol, limit] { return fetch_trades(symbol, limit); });
}

boost::future<json> bitbay::fetch_ohlcv_async(const std::string& symbol, const std::string& timeframe,
                                         long since, int limit) {
    return boost::async([this, symbol, timeframe, since, limit] { return fetch_ohlcv(symbol, timeframe, since, limit); });
}

boost::future<json> bitbay::fetch_trading_fees_async(const std::string& symbol) {
    return boost::async([this, symbol] { return fetch_trading_fees(symbol); });
}

boost::future<json> bitbay::create_order_async(const std::string& symbol, const std::string& type,
                                         const std::string& side, double amount, double price,
                                         const std::map<std::string, std::string>& params) {
    return boost::async([this, symbol, type, side, amount, price, params] { return create_order(symbol, type, side, amount, price, params); });
}

boost::future<json> bitbay::cancel_order_async(const std::string& id, const std::string& symbol) {
    return boost::async([this, id, symbol] { return cancel_order(id, symbol); });
}

boost::future<json> bitbay::cancel_all_orders_async(const std::string& symbol) {
    return boost::async([this, symbol] { return cancel_all_orders(symbol); });
}

boost::future<json> bitbay::edit_order_async(const std::string& id, const std::string& symbol,
                                         const std::string& type, const std::string& side,
                                         double amount, double price,
                                         const std::map<std::string, std::string>& params) {
    return boost::async([this, id, symbol, type, side, amount, price, params] { return edit_order(id, symbol, type, side, amount, price, params); });
}

boost::future<json> bitbay::fetch_balance_async() {
    return boost::async([this] { return fetch_balance(); });
}

boost::future<json> bitbay::fetch_open_orders_async(const std::string& symbol) {
    return boost::async([this, symbol] { return fetch_open_orders(symbol); });
}

boost::future<json> bitbay::fetch_closed_orders_async(const std::string& symbol, long since, int limit) {
    return boost::async([this, symbol, since, limit] { return fetch_closed_orders(symbol, since, limit); });
}

boost::future<json> bitbay::fetch_order_async(const std::string& id, const std::string& symbol) {
    return boost::async([this, id, symbol] { return fetch_order(id, symbol); });
}

boost::future<json> bitbay::fetch_orders_async(const std::string& symbol, long since, int limit) {
    return boost::async([this, symbol, since, limit] { return fetch_orders(symbol, since, limit); });
}

boost::future<json> bitbay::fetch_my_trades_async(const std::string& symbol, long since, int limit) {
    return boost::async([this, symbol, since, limit] { return fetch_my_trades(symbol, since, limit); });
}

boost::future<json> bitbay::fetch_deposit_address_async(const std::string& code) {
    return boost::async([this, code] { return fetch_deposit_address(code); });
}

boost::future<json> bitbay::withdraw_async(const std::string& code, double amount,
                                      const std::string& address, const std::string& tag) {
    return boost::async([this, code, amount, address, tag] { return withdraw(code, amount, address, tag); });
}

} // namespace ccxt
