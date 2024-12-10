#include "ccxt/exchanges/bitfinex2.h"
#include "ccxt/base/json_helper.h"
#include <openssl/hmac.h>
#include <sstream>
#include <iomanip>

namespace ccxt {

const std::string bitfinex2::defaultBaseURL = "https://api-pub.bitfinex.com";
const std::string bitfinex2::defaultVersion = "v2";
const int bitfinex2::defaultRateLimit = 1500;
const bool bitfinex2::defaultPro = true;

ExchangeRegistry::Factory bitfinex2::factory("bitfinex2", bitfinex2::createInstance);

bitfinex2::bitfinex2(const Config& config) : ExchangeImpl(config) {
    init();
}

void bitfinex2::init() {
    ExchangeImpl::init();
    
    // Set exchange properties
    this->id = "bitfinex2";
    this->name = "Bitfinex";
    this->countries = {"VG"};  // British Virgin Islands
    this->version = defaultVersion;
    this->rateLimit = defaultRateLimit;
    this->pro = defaultPro;
    this->certified = false;
    
    if (this->urls.empty()) {
        this->urls = {
            {"logo", "https://user-images.githubusercontent.com/1294454/27766244-e328a50c-5ed2-11e7-947b-041416579bb3.jpg"},
            {"api", {
                {"public", "https://api-pub.bitfinex.com"},
                {"private", "https://api.bitfinex.com"}
            }},
            {"www", "https://www.bitfinex.com"},
            {"doc", "https://docs.bitfinex.com/docs"},
            {"fees", "https://www.bitfinex.com/fees"}
        };
    }

    // Set capabilities
    this->has = {
        {"CORS", std::nullopt},
        {"spot", true},
        {"margin", true},
        {"swap", true},
        {"future", false},
        {"option", false},
        {"cancelAllOrders", true},
        {"cancelOrder", true},
        {"cancelOrders", true},
        {"createDepositAddress", true},
        {"createOrder", true},
        {"editOrder", true},
        {"fetchBalance", true},
        {"fetchClosedOrders", true},
        {"fetchCurrencies", true},
        {"fetchDepositAddress", true},
        {"fetchDepositsWithdrawals", true},
        {"fetchFundingRate", true},
        {"fetchFundingRates", true},
        {"fetchFundingRateHistory", true},
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
        {"fetchTradingFees", true}
    };

    // Set timeframes
    this->timeframes = {
        {"1m", "1m"},
        {"5m", "5m"},
        {"15m", "15m"},
        {"30m", "30m"},
        {"1h", "1h"},
        {"3h", "3h"},
        {"6h", "6h"},
        {"12h", "12h"},
        {"1d", "1D"},
        {"1w", "7D"},
        {"2w", "14D"},
        {"1M", "1M"}
    };
}

Json bitfinex2::describeImpl() const {
    return Json::object({
        {"id", this->id},
        {"name", this->name},
        {"countries", this->countries},
        {"version", this->version},
        {"rateLimit", this->rateLimit},
        {"pro", this->pro},
        {"has", this->has},
        {"timeframes", this->timeframes}
    });
}

std::string bitfinex2::sign(const std::string& path, const std::string& api, const std::string& method,
                           const Json& params, const Json& headers, const Json& body) const {
    std::string url = this->urls["api"][api] + "/" + this->version + "/" + this->implodeParams(path, params);
    Json query = this->omit(params, this->extractParams(path));

    if (api == "public") {
        if (!query.empty()) {
            url += "?" + this->urlencode(query);
        }
    } else {
        this->checkRequiredCredentials();
        
        std::string nonce = this->nonce().str();
        std::string auth_body = "";

        if (method == "POST") {
            if (!body.empty()) {
                auth_body = this->json(body);
            } else if (!query.empty()) {
                auth_body = this->json(query);
            }
        }

        std::string auth_path = "/" + this->version + "/" + path;
        std::string signature = nonce + auth_path + auth_body;
        std::string signature_hex = this->hmac(signature, this->secret, "SHA384", "hex");

        headers["bfx-nonce"] = nonce;
        headers["bfx-apikey"] = this->apiKey;
        headers["bfx-signature"] = signature_hex;
        
        if (method == "POST") {
            headers["Content-Type"] = "application/json";
            if (!auth_body.empty()) {
                headers["Content-Length"] = std::to_string(auth_body.length());
            }
        }
    }

    return url;
}

Json bitfinex2::fetchMarketsImpl() const {
    Json response = this->publicGetTradingPairsInfo();
    return this->parseMarkets(response);
}

Json bitfinex2::fetchCurrenciesImpl() const {
    Json response = this->publicGetCurrencies();
    return this->parseCurrencies(response);
}

Json bitfinex2::fetchTickerImpl(const std::string& symbol) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json::object({
        {"symbol", "t" + market["id"].get<std::string>()}
    });
    Json response = this->publicGetTickerSymbol(request);
    return this->parseTicker(response, market);
}

Json bitfinex2::fetchTickersImpl(const std::vector<std::string>& symbols) const {
    this->loadMarkets();
    Json response = this->publicGetTickers();
    return this->parseTickers(response, symbols);
}

Json bitfinex2::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json request = Json::object({
        {"symbol", "t" + this->marketId(symbol)}
    });
    if (limit) {
        request["len"] = std::to_string(*limit);
    }
    Json response = this->publicGetBookSymbolPrecision(request);
    return this->parseOrderBook(response, symbol, limit);
}

Json bitfinex2::fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe,
                              const std::optional<long long>& since, const std::optional<int>& limit) const {
    this->loadMarkets();
    Json market = this->market(symbol);
    Json request = Json::object({
        {"symbol", "t" + market["id"].get<std::string>()},
        {"timeframe", this->timeframes[timeframe]}
    });
    if (since) {
        request["start"] = *since;
    }
    if (limit) {
        request["limit"] = *limit;
    }
    Json response = this->publicGetCandlesTradeTimeframeSymbolHist(request);
    return this->parseOHLCV(response, market, timeframe, since, limit);
}

// ... Implementation of other methods ...
// Note: For brevity, I've omitted the implementation of the remaining methods.
// Each method would follow a similar pattern of making API requests and parsing responses.

} // namespace ccxt
