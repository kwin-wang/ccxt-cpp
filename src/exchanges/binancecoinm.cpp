#include "ccxt/exchanges/binancecoinm.h"
#include <ctime>
#include <sstream>
#include <iomanip>

namespace ccxt {

const std::string BinanceCoinM::defaultHostname = "dapi.binance.com";
const int BinanceCoinM::defaultRateLimit = 50;
const bool BinanceCoinM::defaultPro = true;

BinanceCoinM::BinanceCoinM(const Config& config) : Binance(config) {
    init();
}

void BinanceCoinM::init() {
    Binance::init();
    
    id = "binancecoinm";
    name = "Binance COIN-M";

    // Update URLs
    urls["logo"] = "https://github.com/user-attachments/assets/387cfc4e-5f33-48cd-8f5c-cd4854dabf0c";
    urls["api"]["public"] = "https://dapi.binance.com";
    urls["api"]["private"] = "https://dapi.binance.com";
    urls["api"]["v2"] = "https://dapi.binance.com";
    urls["doc"] = {
        "https://binance-docs.github.io/apidocs/delivery/en/",
        "https://binance-docs.github.io/apidocs/spot/en",
        "https://developers.binance.com/en"
    };

    // Update capabilities
    has["CORS"] = std::nullopt;
    has["spot"] = false;
    has["margin"] = false;
    has["swap"] = true;
    has["future"] = true;
    has["option"] = std::nullopt;
    has["createStopMarketOrder"] = true;

    // Update options
    options["fetchMarkets"] = {"inverse"};
    options["defaultSubType"] = "inverse";
    options["leverageBrackets"] = std::nullopt;
}

json BinanceCoinM::transferInImpl(const std::string& code, double amount, const json& params) {
    // transfer from spot wallet to coinm futures wallet
    return futuresTransfer(code, amount, 3, params);
}

json BinanceCoinM::transferOutImpl(const std::string& code, double amount, const json& params) {
    // transfer from coinm futures wallet to spot wallet
    return futuresTransfer(code, amount, 4, params);
}

json BinanceCoinM::fetchMarketsImpl() const {
    json response = Binance::fetchMarketsImpl();
    // Add any COIN-M specific market processing here
    return response;
}

json BinanceCoinM::fetchFundingRateImpl(const std::string& symbol) const {
    Market market = this->market(symbol);
    json request = {
        {"symbol", market.id}
    };
    json response = this->publicGetPremiumIndex(request);
    return this->parseFundingRate(response);
}

json BinanceCoinM::fetchFundingRatesImpl(const std::vector<std::string>& symbols) const {
    json response = this->publicGetPremiumIndex({});
    json rates;
    for (const auto& entry : response) {
        rates.push_back(this->parseFundingRate(entry));
    }
    return rates;
}

json BinanceCoinM::parseFundingRate(const json& fundingRate, const Market& market) const {
    // Implement funding rate parsing logic
    return fundingRate;
}

std::string BinanceCoinM::sign(const std::string& path, const std::string& api,
                            const std::string& method, const json& params,
                            const std::map<std::string, std::string>& headers,
                            const json& body) const {
    return Binance::sign(path, api, method, params, headers, body);
}

} // namespace ccxt
