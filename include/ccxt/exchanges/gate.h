#ifndef CCXT_GATE_H
#define CCXT_GATE_H

#include "ccxt/base/exchange.h"
#include <string>
#include <map>

namespace ccxt {

class gate : public Exchange {
public:
    gate(const Exchange::Config &config);
    ~gate() = default;

    // Market Data
    Json fetchMarkets(const Json &params = Json::object()) override;
    Json fetchTicker(const std::string &symbol, const Json &params = Json::object()) override;
    Json fetchOrderBook(const std::string &symbol, const int limit = 0, const Json &params = Json::object()) override;
    Json fetchTrades(const std::string &symbol, int since = 0, int limit = 0, const Json &params = Json::object()) override;
    Json fetchOHLCV(const std::string &symbol, const std::string &timeframe = "1m", int since = 0, int limit = 0, const Json &params = Json::object()) override;

    // Trading
    Json createOrder(const std::string &symbol, const std::string &type, const std::string &side,
                    double amount, double price = 0, const Json &params = Json::object()) override;
    Json cancelOrder(const std::string &id, const std::string &symbol = "", const Json &params = Json::object()) override;
    Json fetchOrder(const std::string &id, const std::string &symbol = "", const Json &params = Json::object()) override;
    Json fetchOpenOrders(const std::string &symbol = "", int since = 0, int limit = 0, const Json &params = Json::object()) override;
    Json fetchClosedOrders(const std::string &symbol = "", int since = 0, int limit = 0, const Json &params = Json::object()) override;
    Json fetchMyTrades(const std::string &symbol = "", int since = 0, int limit = 0, const Json &params = Json::object()) override;

    // Account
    Json fetchBalance(const Json &params = Json::object()) override;
    Json fetchTradingFees(const Json &params = Json::object()) override;

protected:
    // API Request Helpers
    Json sign(const std::string &path, const std::string &api = "public", const std::string &method = "GET",
              const Json &params = Json::object(), const Json &headers = Json::object(), const Json &body = Json::object()) override;
    void handleErrors(const Json &response) override;

    // Response Parsers
    Json parseOrder(const Json &order, const Json &market = Json::object());
    Json parseTrade(const Json &trade, const Json &market = Json::object());
    Json parseTicker(const Json &ticker, const Json &market = Json::object());
    Json parseOrderBook(const Json &orderbook, const std::string &symbol = "", const int timestamp = 0,
                       const std::string &bidsKey = "bids", const std::string &asksKey = "asks", const Json &market = Json::object());

private:
    // Helper Methods
    std::string getOrderStatus(const std::string &status);
    void initMarkets();
};

} // namespace ccxt

#endif // CCXT_GATE_H
