#include "ccxt/exchanges/zonda.h"
#include <openssl/hmac.h>
#include <iomanip>
#include <sstream>

namespace ccxt {

Zonda::Zonda(const Config& config) : Exchange(config) {
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

void Zonda::initializeApiEndpoints() {
    // Initialize API endpoints
    apiEndpoints = {
        {"public", "https://" + hostname + "/API/Public"},
        {"private", "https://" + hostname + "/API/Trading/tradingApi.php"},
        {"v1_01Public", "https://api." + hostname + "/rest"},
        {"v1_01Private", "https://api." + hostname + "/rest"}
    };
}

std::string Zonda::sign(const std::string& path, const std::string& api,
                  const std::string& method, const json& params,
                  const std::map<std::string, std::string>& headers,
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
                                  this->encode(this->config_.secret),
                                  "sha512");

        auto body = this->urlencode(this->extend(request, {
            "signature", signature
        }));

        headers["API-Key"] = this->config_.apiKey;
        headers["API-Hash"] = signature;
        headers["Content-Type"] = "application/x-www-form-urlencoded";
        headers["Request-Timestamp"] = nonce;
    }
    
    return url;
}

std::string Zonda::createNonce() {
    return std::to_string(this->milliseconds());
}

std::string Zonda::createSignature(const std::string& nonce, const std::string& method,
                            const std::string& path, const std::string& body) {
    auto message = nonce + method + path + body;
    unsigned char* digest = HMAC(EVP_sha512(), this->config_.secret.c_str(), this->config_.secret.length(),
                                reinterpret_cast<const unsigned char*>(message.c_str()),
                                message.length(), nullptr, nullptr);
    
    std::stringstream ss;
    for(int i = 0; i < 64; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
    }
    return ss.str();
}

} // namespace ccxt
