#include "ccxt/exchanges/binanceusdm.h"
#include <chrono>
#include <sstream>
#include <iomanip>

namespace ccxt {

const std::string BinanceUSDM::defaultHostname = "fapi.binance.com";
const int BinanceUSDM::defaultRateLimit = 50;
const bool BinanceUSDM::defaultPro = true;

BinanceUSDM::BinanceUSDM(const Config& config) : Binance(config) {
    init();
}

void BinanceUSDM::init() {
    Binance::init();

    id = "binanceusdm";
    name = "Binance USDⓈ-M";

    // Update URLs
    urls["logo"] = "https://github.com/user-attachments/assets/871cbea7-eebb-4b28-b260-c1c91df0487a";
    urls["api"]["public"] = "https://fapi.binance.com";
    urls["api"]["private"] = "https://fapi.binance.com";
    urls["api"]["v2"] = "https://fapi.binance.com";
    urls["doc"] = {
        "https://binance-docs.github.io/apidocs/futures/en/",
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
    options["fetchMarkets"] = {"linear"};
    options["defaultSubType"] = "linear";
    options["leverageBrackets"] = std::nullopt;
    options["marginTypes"] = json::object();
    options["marginModes"] = json::object();

    // Add error codes
    exceptions["exact"]["-5021"] = InvalidOrder;  // {"code":-5021,"msg":"Due to the order could not be filled immediately, the FOK order has been rejected."}
    exceptions["exact"]["-5022"] = InvalidOrder;  // {"code":-5022,"msg":"Due to the order could not be executed, the Post Only order will be rejected."}
    exceptions["exact"]["-5028"] = InvalidOrder;  // {"code":-5028,"msg":"Timestamp for self request is outside of the ME recvWindow."}
}

json BinanceUSDM::transferIn(const std::string& code, double amount, const json& params) {
    // transfer from spot wallet to usdm futures wallet
    return futuresTransfer(code, amount, 1, params);
}

json BinanceUSDM::transferOut(const std::string& code, double amount, const json& params) {
    // transfer from usdm futures wallet to spot wallet
    return futuresTransfer(code, amount, 2, params);
}

json BinanceUSDM::fetchMarketsImpl() const {
    json response = Binance::fetchMarketsImpl();
    // Add any USDⓈ-M specific market processing here
    return response;
}

json BinanceUSDM::fetchTickerImpl(const std::string& symbol) const {
    return Binance::fetchTickerImpl(symbol);
}

json BinanceUSDM::fetchTickersImpl(const std::vector<std::string>& symbols) const {
    return Binance::fetchTickersImpl(symbols);
}

json BinanceUSDM::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
    return Binance::fetchOrderBookImpl(symbol, limit);
}

json BinanceUSDM::fetchTradesImpl(const std::string& symbol, const std::optional<long long>& since,
                               const std::optional<int>& limit) const {
    return Binance::fetchTradesImpl(symbol, since, limit);
}

json BinanceUSDM::fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe,
                              const std::optional<long long>& since, const std::optional<int>& limit) const {
    return Binance::fetchOHLCVImpl(symbol, timeframe, since, limit);
}

json BinanceUSDM::fetchFundingRateImpl(const std::string& symbol) const {
    Market market = this->market(symbol);
    json request = {
        {"symbol", market.id}
    };
    json response = this->publicGetPremiumIndex(request);
    return this->parseFundingRate(response);
}

json BinanceUSDM::fetchFundingRatesImpl(const std::vector<std::string>& symbols) const {
    json response = this->publicGetPremiumIndex({});
    json rates;
    for (const auto& entry : response) {
        rates.push_back(this->parseFundingRate(entry));
    }
    return rates;
}

json BinanceUSDM::fetchFundingRateHistoryImpl(const std::string& symbol, const std::optional<long long>& since,
                                           const std::optional<int>& limit) const {
    return Binance::fetchFundingRateHistoryImpl(symbol, since, limit);
}

json BinanceUSDM::fetchBalanceImpl(const json& params) const {
    return Binance::fetchBalanceImpl(params);
}

json BinanceUSDM::createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side,
                               double amount, double price, const json& params) {
    return Binance::createOrderImpl(symbol, type, side, amount, price, params);
}

json BinanceUSDM::cancelOrderImpl(const std::string& id, const std::string& symbol, const json& params) {
    return Binance::cancelOrderImpl(id, symbol, params);
}

json BinanceUSDM::fetchOrderImpl(const std::string& id, const std::string& symbol, const json& params) const {
    return Binance::fetchOrderImpl(id, symbol, params);
}

json BinanceUSDM::fetchOrdersImpl(const std::string& symbol, const std::optional<long long>& since,
                               const std::optional<int>& limit, const json& params) const {
    return Binance::fetchOrdersImpl(symbol, since, limit, params);
}

json BinanceUSDM::fetchOpenOrdersImpl(const std::string& symbol, const std::optional<long long>& since,
                                  const std::optional<int>& limit, const json& params) const {
    return Binance::fetchOpenOrdersImpl(symbol, since, limit, params);
}

json BinanceUSDM::fetchClosedOrdersImpl(const std::string& symbol, const std::optional<long long>& since,
                                    const std::optional<int>& limit, const json& params) const {
    return Binance::fetchClosedOrdersImpl(symbol, since, limit, params);
}

json BinanceUSDM::fetchPositionsImpl(const std::vector<std::string>& symbols) const {
    return Binance::fetchPositionsImpl(symbols);
}

json BinanceUSDM::fetchPositionRiskImpl(const std::vector<std::string>& symbols) const {
    return Binance::fetchPositionRiskImpl(symbols);
}

json BinanceUSDM::setLeverageImpl(int leverage, const std::string& symbol) {
    return Binance::setLeverageImpl(leverage, symbol);
}

json BinanceUSDM::setMarginModeImpl(const std::string& marginMode, const std::string& symbol) {
    return Binance::setMarginModeImpl(marginMode, symbol);
}

json BinanceUSDM::setPositionModeImpl(bool hedged) {
    return Binance::setPositionModeImpl(hedged);
}

std::string BinanceUSDM::sign(const std::string& path, const std::string& api,
                           const std::string& method, const json& params,
                           const std::map<std::string, std::string>& headers,
                           const json& body) const {
    return Binance::sign(path, api, method, params, headers, body);
}

json BinanceUSDM::parseTrade(const json& trade, const Market& market) const {
    return Binance::parseTrade(trade, market);
}

json BinanceUSDM::parseOrder(const json& order, const Market& market) const {
    return Binance::parseOrder(order, market);
}

json BinanceUSDM::parseTicker(const json& ticker, const Market& market) const {
    return Binance::parseTicker(ticker, market);
}

json BinanceUSDM::parseOHLCV(const json& ohlcv, const Market& market, const std::string& timeframe) const {
    return Binance::parseOHLCV(ohlcv, market, timeframe);
}

json BinanceUSDM::parseFundingRate(const json& fundingRate, const Market& market) const {
    return Binance::parseFundingRate(fundingRate, market);
}

json BinanceUSDM::parsePosition(const json& position, const Market& market) const {
    return Binance::parsePosition(position, market);
}

json BinanceUSDM::parseLeverageBrackets(const json& response, const Market& market) const {
    return Binance::parseLeverageBrackets(response, market);
}

} // namespace ccxt
