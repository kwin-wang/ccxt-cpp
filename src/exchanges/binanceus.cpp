#include "ccxt/exchanges/binanceus.h"
#include <chrono>
#include <sstream>
#include <iomanip>

namespace ccxt {

const std::string BinanceUS::defaultHostname = "binance.us";
const int BinanceUS::defaultRateLimit = 50;  // 1200 req per min
const bool BinanceUS::defaultPro = true;


BinanceUS::BinanceUS(const Config& config) : Binance(config) {
    init();
}

void BinanceUS::init() {
    Binance::init();

    id = "binanceus";
    name = "Binance US";
    countries = {"US"};
    hostname = "binance.us";
    rateLimit = 50;  // 1200 req per min
    certified = false;
    pro = true;

    // Update URLs
    urls["logo"] = "https://github.com/user-attachments/assets/a9667919-b632-4d52-a832-df89f8a35e8c";
    urls["api"]["web"] = "https://www.binance.us";
    urls["api"]["public"] = "https://api.binance.us/api/v3";
    urls["api"]["private"] = "https://api.binance.us/api/v3";
    urls["api"]["sapi"] = "https://api.binance.us/sapi/v1";
    urls["api"]["sapiV2"] = "https://api.binance.us/sapi/v2";
    urls["api"]["sapiV3"] = "https://api.binance.us/sapi/v3";
    urls["www"] = "https://www.binance.us";
    urls["referral"] = "https://www.binance.us/?ref=35005074";
    urls["doc"] = "https://github.com/binance-us/binance-official-api-docs";
    urls["fees"] = "https://www.binance.us/en/fee/schedule";

    // Update fees
    fees["trading"] = {
        {"tierBased", true},
        {"percentage", true},
        {"taker", parseNumber("0.001")},  // 0.1% trading fee
        {"maker", parseNumber("0.001")}   // 0.1% trading fee
    };

    // Update options
    options["fetchMarkets"] = {"spot"};
    options["defaultType"] = "spot";
    options["fetchMargins"] = false;
    options["quoteOrderQty"] = false;

    // Update capabilities
    has["CORS"] = std::nullopt;
    has["spot"] = true;
    has["margin"] = false;
    has["swap"] = false;
    has["future"] = std::nullopt;
    has["option"] = false;
    has["addMargin"] = false;
    has["closeAllPositions"] = false;
    has["closePosition"] = false;
    has["createReduceOnlyOrder"] = false;
    has["fetchBorrowInterest"] = false;
    has["fetchBorrowRate"] = false;
    has["fetchBorrowRateHistories"] = false;
    has["fetchBorrowRateHistory"] = false;
    has["fetchBorrowRates"] = false;
    has["fetchBorrowRatesPerSymbol"] = false;
    has["fetchFundingHistory"] = false;
    has["fetchFundingRate"] = false;
    has["fetchFundingRateHistory"] = false;
    has["fetchFundingRates"] = false;
    has["fetchIndexOHLCV"] = false;
    has["fetchIsolatedPositions"] = false;
    has["fetchLeverage"] = false;
    has["fetchLeverageTiers"] = false;
    has["fetchMarketLeverageTiers"] = false;
    has["fetchMarkOHLCV"] = false;
    has["fetchOpenInterestHistory"] = false;
    has["fetchPosition"] = false;
    has["fetchPositions"] = false;
    has["fetchPositionsRisk"] = false;
    has["fetchPremiumIndexOHLCV"] = false;
    has["reduceMargin"] = false;
    has["setLeverage"] = false;
    has["setMargin"] = false;
    has["setMarginMode"] = false;
    has["setPositionMode"] = false;
}

json BinanceUS::fetchMarketsImpl() const {
    json response = Binance::fetchMarketsImpl();
    // Add any BinanceUS specific market processing here
    return response;
}

json BinanceUS::fetchTickerImpl(const std::string& symbol) const {
    return Binance::fetchTickerImpl(symbol);
}

json BinanceUS::fetchTickersImpl(const std::vector<std::string>& symbols) const {
    return Binance::fetchTickersImpl(symbols);
}

json BinanceUS::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
    return Binance::fetchOrderBookImpl(symbol, limit);
}

json BinanceUS::fetchTradesImpl(const std::string& symbol, const std::optional<long long>& since,
                             const std::optional<int>& limit) const {
    return Binance::fetchTradesImpl(symbol, since, limit);
}

json BinanceUS::fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe,
                            const std::optional<long long>& since, const std::optional<int>& limit) const {
    return Binance::fetchOHLCVImpl(symbol, timeframe, since, limit);
}

json BinanceUS::fetchBalanceImpl(const json& params) const {
    return Binance::fetchBalanceImpl(params);
}

json BinanceUS::createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side,
                            double amount, double price, const json& params) {
    return Binance::createOrderImpl(symbol, type, side, amount, price, params);
}

json BinanceUS::cancelOrderImpl(const std::string& id, const std::string& symbol, const json& params) {
    return Binance::cancelOrderImpl(id, symbol, params);
}

json BinanceUS::fetchOrderImpl(const std::string& id, const std::string& symbol, const json& params) const {
    return Binance::fetchOrderImpl(id, symbol, params);
}

json BinanceUS::fetchOrdersImpl(const std::string& symbol, const std::optional<long long>& since,
                            const std::optional<int>& limit, const json& params) const {
    return Binance::fetchOrdersImpl(symbol, since, limit, params);
}

json BinanceUS::fetchOpenOrdersImpl(const std::string& symbol, const std::optional<long long>& since,
                                const std::optional<int>& limit, const json& params) const {
    return Binance::fetchOpenOrdersImpl(symbol, since, limit, params);
}

json BinanceUS::fetchClosedOrdersImpl(const std::string& symbol, const std::optional<long long>& since,
                                  const std::optional<int>& limit, const json& params) const {
    return Binance::fetchClosedOrdersImpl(symbol, since, limit, params);
}

std::string BinanceUS::sign(const std::string& path, const std::string& api,
                         const std::string& method, const json& params,
                         const std::map<std::string, std::string>& headers,
                         const json& body) const {
    return Binance::sign(path, api, method, params, headers, body);
}

json BinanceUS::parseTrade(const json& trade, const Market& market) const {
    return Binance::parseTrade(trade, market);
}

json BinanceUS::parseOrder(const json& order, const Market& market) const {
    return Binance::parseOrder(order, market);
}

json BinanceUS::parseTicker(const json& ticker, const Market& market) const {
    return Binance::parseTicker(ticker, market);
}

json BinanceUS::parseOHLCV(const json& ohlcv, const Market& market, const std::string& timeframe) const {
    return Binance::parseOHLCV(ohlcv, market, timeframe);
}

} // namespace ccxt
