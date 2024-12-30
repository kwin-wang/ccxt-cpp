#include "ccxt/exchanges/wavesexchange.h"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>

namespace ccxt {

WavesExchange::WavesExchange() {
    this->id = "wavesexchange";
    this->name = "Waves.Exchange";
    this->countries = {"EE"}; // Estonia
    this->rateLimit = 500;
    this->version = "v1";
    this->has = {
        {"cancelOrder", true},
        {"CORS", true},
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
        {"withdraw", true}
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
        {"12h", "12h"},
        {"1d", "1d"},
        {"1w", "1w"},
        {"1M", "1M"}
    };

    this->urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/84547058-5fb27d80-ad0b-11ea-8711-78ac8b3c7f31.jpg"},
        {"api", {
            {"matcher", "https://matcher.waves.exchange"},
            {"node", "https://nodes.waves.exchange"},
            {"public", "https://api.waves.exchange/v1"},
            {"private", "https://api.waves.exchange/v1"},
            {"forward", "https://waves.exchange/api/v1/forward/matcher"}
        }},
        {"www", "https://waves.exchange"},
        {"doc", {
            "https://docs.waves.exchange",
            "https://github.com/wavesplatform/matcher-web-api"
        }}
    };

    this->api = {
        {"matcher", {
            {"GET", {
                "matcher",
                "matcher/settings",
                "matcher/settings/rates",
                "matcher/balance/reserved/{publicKey}",
                "matcher/orderbook/{amountAsset}/{priceAsset}",
                "matcher/orderbook/{baseId}/{quoteId}/publicKey/{publicKey}",
                "matcher/orderbook/{baseId}/{quoteId}/{orderId}",
                "matcher/orderbook/{baseId}/{quoteId}/info",
                "matcher/orderbook/{baseId}/{quoteId}/status",
                "matcher/orders/{address}",
                "matcher/orders/{address}/{orderId}"
            }},
            {"POST", {
                "matcher/orderbook",
                "matcher/orderbook/market",
                "matcher/orderbook/cancel",
                "matcher/orderbook/{baseId}/{quoteId}/cancel",
                "matcher/debug/saveSnapshots",
                "matcher/orders/{address}/cancel",
                "matcher/orders/cancel/{orderId}"
            }}
        }},
        {"node", {
            {"GET", {
                "addresses",
                "addresses/balance/{address}",
                "addresses/balance/{address}/{confirmations}",
                "addresses/balance/details/{address}",
                "addresses/data/{address}",
                "addresses/data/{address}/{key}",
                "addresses/effectiveBalance/{address}",
                "addresses/effectiveBalance/{address}/{confirmations}",
                "addresses/publicKey/{publicKey}",
                "addresses/scriptInfo/{address}",
                "addresses/scriptInfo/{address}/meta",
                "addresses/seed/{address}",
                "addresses/seq/{from}/{to}",
                "addresses/validate/{address}",
                "alias/by-address/{address}",
                "alias/by-alias/{alias}",
                "assets/{assetId}/distribution/{height}/{limit}",
                "assets/balance/{address}",
                "assets/balance/{address}/{assetId}",
                "assets/details/{assetId}",
                "assets/nft/{address}/limit/{limit}",
                "blockchain/rewards",
                "blockchain/rewards/height",
                "blocks/address/{address}/{from}/{to}",
                "blocks/at/{height}",
                "blocks/delay/{signature}/{blockNum}",
                "blocks/first",
                "blocks/headers/last",
                "blocks/headers/seq/{from}/{to}",
                "blocks/height",
                "blocks/height/{signature}",
                "blocks/last",
                "blocks/seq/{from}/{to}",
                "blocks/signature/{signature}",
                "consensus/algo",
                "consensus/basetarget",
                "consensus/basetarget/{blockId}",
                "consensus/generatingbalance/{address}",
                "consensus/generationsignature",
                "consensus/generationsignature/{blockId}",
                "debug/balances/history/{address}",
                "debug/blocks/{howMany}",
                "debug/configInfo",
                "debug/historyInfo",
                "debug/info",
                "debug/minerInfo",
                "debug/portfolios/{address}",
                "debug/state",
                "debug/stateChanges/address/{address}",
                "debug/stateChanges/info/{id}",
                "debug/stateWaves/{height}",
                "node/status",
                "node/version",
                "peers/all",
                "peers/blacklisted",
                "peers/connected",
                "peers/suspended",
                "transactions/address/{address}/limit/{limit}",
                "transactions/info/{id}",
                "transactions/status",
                "transactions/unconfirmed",
                "transactions/unconfirmed/info/{id}",
                "transactions/unconfirmed/size",
                "utils/seed",
                "utils/seed/{length}",
                "utils/time",
                "wallet/seed"
            }},
            {"POST", {
                "addresses",
                "addresses/data/{address}",
                "addresses/sign/{address}",
                "addresses/signText/{address}",
                "addresses/verify/{address}",
                "addresses/verifyText/{address}",
                "debug/blacklist",
                "debug/print",
                "debug/rollback",
                "debug/validate",
                "node/stop",
                "peers/clearblacklist",
                "peers/connect",
                "transactions/broadcast",
                "transactions/calculateFee",
                "tranasctions/sign",
                "transactions/sign/{signerAddress}",
                "transactions/status",
                "utils/hash/fast",
                "utils/hash/secure",
                "utils/script/compileCode",
                "utils/script/compileWithImports",
                "utils/script/decompile",
                "utils/script/estimate",
                "utils/sign/{privateKey}",
                "utils/transactionsSerialize"
            }}
        }},
        {"public", {
            {"GET", {
                "matcher",
                "matcher/orderbook",
                "matcher/orderbook/{amountAsset}/{priceAsset}",
                "matcher/orderbook/{baseId}/{quoteId}/status",
                "matcher/orderbook/{baseId}/{quoteId}/info",
                "trades/effectiveRate",
                "rates",
                "rates/{baseId}/{quoteId}",
                "ticker/{amountAsset}/{priceAsset}",
                "ticker",
                "transactions",
                "transactions/exchange"
            }}
        }},
        {"private", {
            {"POST", {
                "matcher/orderbook/{baseId}/{quoteId}/cancel",
                "matcher/orderbook/cancel",
                "matcher/orderbook",
                "matcher/orderbook/market",
                "matcher/debug/saveSnapshots",
                "matcher/orders/{address}/cancel",
                "matcher/orders/cancel/{orderId}",
                "oauth2/token"
            }}
        }},
        {"forward", {
            {"POST", {
                "order",
                "order/cancel",
                "orders/{orderId}/cancel"
            }}
        }}
    };

    this->fees = {
        {"trading", {
            {"maker", 0.0005},
            {"taker", 0.0005}
        }}
    };
}

nlohmann::json WavesExchange::fetch_markets() {
    auto response = this->fetch("matcher/orderbook", "public");
    auto result = nlohmann::json::array();

    for (const auto& market : response) {
        auto id = market["symbol"].get<std::string>();
        auto baseId = market["amountAsset"].get<std::string>();
        auto quoteId = market["priceAsset"].get<std::string>();
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
            {"future", false},
            {"precision", {
                {"amount", market["amountAssetDecimals"]},
                {"price", market["priceAssetDecimals"]}
            }},
            {"limits", {
                {"amount", {
                    {"min", std::pow(10, -market["amountAssetDecimals"].get<int>())},
                    {"max", nullptr}
                }},
                {"price", {
                    {"min", std::pow(10, -market["priceAssetDecimals"].get<int>())},
                    {"max", nullptr}
                }},
                {"cost", {
                    {"min", nullptr},
                    {"max", nullptr}
                }}
            }}
        });
    }
    return result;
}

nlohmann::json WavesExchange::create_order(const std::string& symbol, const std::string& type,
                                         const std::string& side, double amount, double price) {
    this->check_required_credentials();
    auto market = this->market(symbol);
    
    auto order = {
        {"matcherPublicKey", this->matcherPublicKey},
        {"orderType", side},
        {"amount", this->amount_to_precision(symbol, amount)},
        {"price", this->price_to_precision(symbol, price)},
        {"assetPair", {
            {"amountAsset", market["baseId"]},
            {"priceAsset", market["quoteId"]}
        }},
        {"timestamp", this->nonce()},
        {"expiration", this->sum(this->nonce(), 2592000000)},  // 30 days
        {"matcherFee", 300000}  // 0.003 WAVES
    };

    auto signed_order = this->sign_order(order);
    return this->fetch("matcher/orderbook", "private", "POST", signed_order);
}

nlohmann::json WavesExchange::sign_order(const nlohmann::json& order) {
    auto bytes = this->get_order_bytes(order);
    auto signature = this->sign_message(bytes);
    auto signed_order = order;
    signed_order["signature"] = signature;
    return signed_order;
}

std::string WavesExchange::sign(const std::string& path, const std::string& api,
                               const std::string& method, const nlohmann::json& params,
                               const std::map<std::string, std::string>& headers) {
    auto url = this->urls["api"][api].get<std::string>() + "/" + this->implode_params(path, params);
    auto query = this->omit(params, this->extract_params(path));

    if (api == "private" || api == "forward") {
        this->check_required_credentials();
        auto timestamp = std::to_string(this->nonce());
        auto auth = timestamp + this->config_.apiKey;
        auto signature = this->hmac(auth, this->config_.secret, "sha512");
        
        auto new_headers = headers;
        new_headers["X-API-Key"] = this->config_.apiKey;
        new_headers["X-API-Signature"] = signature;
        new_headers["X-API-Timestamp"] = timestamp;
        
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

std::string WavesExchange::get_asset_bytes(const std::string& assetId) {
    if (assetId == "WAVES") {
        return std::string(1, '\0');
    } else {
        return assetId;
    }
}

std::string WavesExchange::get_order_bytes(const nlohmann::json& order) {
    // Implementation of order bytes generation according to Waves protocol
    // This is a placeholder - actual implementation would need to follow Waves protocol
    return "";
}

std::string WavesExchange::sign_message(const std::string& message) {
    return this->hmac(message, this->config_.secret, "sha256");
}

std::string WavesExchange::get_network_byte() {
    return "W";  // Mainnet
}

nlohmann::json WavesExchange::get_matcher_public_key() {
    if (this->matcherPublicKey.empty()) {
        auto response = this->fetch("matcher", "public");
        this->matcherPublicKey = response["matcherPublicKey"].get<std::string>();
    }
    return {{"matcherPublicKey", this->matcherPublicKey}};
}

} // namespace ccxt
