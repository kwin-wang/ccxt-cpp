#include "bequant.h"

namespace ccxt {

const std::string Bequant::defaultHostname = "api.bequant.io";
const int Bequant::defaultRateLimit = 50;
const bool Bequant::defaultPro = true;

Bequant::Bequant(const Config& config) : HitBTC(config) {
    id = "bequant";
    name = "Bequant";
}

void Bequant::init() {
    HitBTC::init();
    hostname = this->extractParam<std::string>(config, "hostname", defaultHostname);
    urls["api"] = {
        {"public", "https://" + hostname + "/api/3"},
        {"private", "https://" + hostname + "/api/3"}
    };
    urls["logo"] = "https://user-images.githubusercontent.com/1294454/55248342-a75dfe00-525a-11e9-8aa2-05e9dca943c6.jpg";
    urls["www"] = "https://bequant.io";
    urls["doc"] = {
        "https://api.bequant.io/",
        "https://api.bequant.io/api/3/docs",
        "https://api.bequant.io/api/3/docs/websocket"
    };
    urls["fees"] = "https://bequant.io/fees-and-limits";
    urls["referral"] = "https://bequant.io";
}

// Market Data API
json Bequant::fetchMarketsImpl() const {
    return HitBTC::fetchMarketsImpl();
}

json Bequant::fetchTickerImpl(const std::string& symbol) const {
    return HitBTC::fetchTickerImpl(symbol);
}

json Bequant::fetchTickersImpl(const std::vector<std::string>& symbols) const {
    return HitBTC::fetchTickersImpl(symbols);
}

json Bequant::fetchOrderBookImpl(const std::string& symbol, const std::optional<int>& limit) const {
    return HitBTC::fetchOrderBookImpl(symbol, limit);
}

json Bequant::fetchTradesImpl(const std::string& symbol, const std::optional<long long>& since,
                           const std::optional<int>& limit) const {
    return HitBTC::fetchTradesImpl(symbol, since, limit);
}

json Bequant::fetchOHLCVImpl(const std::string& symbol, const std::string& timeframe,
                          const std::optional<long long>& since,
                          const std::optional<int>& limit) const {
    return HitBTC::fetchOHLCVImpl(symbol, timeframe, since, limit);
}

json Bequant::fetchTimeImpl() const {
    return HitBTC::fetchTimeImpl();
}

json Bequant::fetchCurrenciesImpl() const {
    return HitBTC::fetchCurrenciesImpl();
}

json Bequant::fetchTradingFeesImpl() const {
    return HitBTC::fetchTradingFeesImpl();
}

json Bequant::fetchBalanceImpl() const {
    return HitBTC::fetchBalanceImpl();
}

json Bequant::fetchDepositAddressImpl(const std::string& code, const json& params) const {
    return HitBTC::fetchDepositAddressImpl(code, params);
}

json Bequant::fetchDepositsImpl(const std::string& code, const std::optional<long long>& since,
                             const std::optional<int>& limit) const {
    return HitBTC::fetchDepositsImpl(code, since, limit);
}

json Bequant::fetchWithdrawalsImpl(const std::string& code, const std::optional<long long>& since,
                                const std::optional<int>& limit) const {
    return HitBTC::fetchWithdrawalsImpl(code, since, limit);
}

json Bequant::fetchDepositsWithdrawalsImpl(const std::string& code, const std::optional<long long>& since,
                                        const std::optional<int>& limit) const {
    return HitBTC::fetchDepositsWithdrawalsImpl(code, since, limit);
}

json Bequant::fetchDepositWithdrawFeesImpl() const {
    return HitBTC::fetchDepositWithdrawFeesImpl();
}

json Bequant::fetchFundingRatesImpl(const std::vector<std::string>& symbols) const {
    return HitBTC::fetchFundingRatesImpl(symbols);
}

json Bequant::fetchFundingRateHistoryImpl(const std::string& symbol, const std::optional<long long>& since,
                                       const std::optional<int>& limit) const {
    return HitBTC::fetchFundingRateHistoryImpl(symbol, since, limit);
}

json Bequant::fetchLeverageImpl(const std::string& symbol) const {
    return HitBTC::fetchLeverageImpl(symbol);
}

json Bequant::fetchMarginModesImpl(const std::vector<std::string>& symbols) const {
    return HitBTC::fetchMarginModesImpl(symbols);
}

json Bequant::fetchPositionsImpl(const std::vector<std::string>& symbols) const {
    return HitBTC::fetchPositionsImpl(symbols);
}

// Trading API
json Bequant::createOrderImpl(const std::string& symbol, const std::string& type, const std::string& side,
                          double amount, const std::optional<double>& price) {
    return HitBTC::createOrderImpl(symbol, type, side, amount, price);
}

json Bequant::cancelOrderImpl(const std::string& id, const std::string& symbol) {
    return HitBTC::cancelOrderImpl(id, symbol);
}

json Bequant::cancelAllOrdersImpl(const std::string& symbol) {
    return HitBTC::cancelAllOrdersImpl(symbol);
}

json Bequant::fetchOrderImpl(const std::string& id, const std::string& symbol) const {
    return HitBTC::fetchOrderImpl(id, symbol);
}

json Bequant::fetchOrdersImpl(const std::string& symbol, const std::optional<long long>& since,
                          const std::optional<int>& limit) const {
    return HitBTC::fetchOrdersImpl(symbol, since, limit);
}

json Bequant::fetchOpenOrdersImpl(const std::string& symbol, const std::optional<long long>& since,
                              const std::optional<int>& limit) const {
    return HitBTC::fetchOpenOrdersImpl(symbol, since, limit);
}

json Bequant::fetchClosedOrdersImpl(const std::string& symbol, const std::optional<long long>& since,
                                const std::optional<int>& limit) const {
    return HitBTC::fetchClosedOrdersImpl(symbol, since, limit);
}

json Bequant::setLeverageImpl(int leverage, const std::string& symbol) {
    return HitBTC::setLeverageImpl(leverage, symbol);
}

json Bequant::setMarginModeImpl(const std::string& marginMode, const std::string& symbol) {
    return HitBTC::setMarginModeImpl(marginMode, symbol);
}

json Bequant::addMarginImpl(const std::string& symbol, double amount) {
    return HitBTC::addMarginImpl(symbol, amount);
}

json Bequant::reduceMarginImpl(const std::string& symbol, double amount) {
    return HitBTC::reduceMarginImpl(symbol, amount);
}

json Bequant::transferImpl(const std::string& code, double amount, const std::string& fromAccount,
                       const std::string& toAccount) {
    return HitBTC::transferImpl(code, amount, fromAccount, toAccount);
}

// Helper methods
std::string Bequant::sign(const std::string& path, const std::string& api,
                       const std::string& method, const json& params,
                       const std::map<std::string, std::string>& headers,
                       const json& body) const {
    return HitBTC::sign(path, api, method, params, headers, body);
}

} // namespace ccxt
