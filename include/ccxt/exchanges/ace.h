#ifndef CCXT_ACE_H
#define CCXT_ACE_H

#include "ccxt/base/exchange.h"
#include <boost/asio/io_context.hpp>
#include <boost/coroutine2/coroutine.hpp>
// #include <nulltype> // Commenting out the nulltype include as it is not
// found.

namespace ccxt {

class Ace : public Exchange {
public:
  Ace(boost::asio::io_context &context, const Config &config = Config());
  ~Ace() = default;

  // Basic exchange info
  //void describe()override{};

  // Market Data API - Sync
  json fetchMarkets(const json &params = json::object()) override;
  json fetchTicker(const std::string &symbol,
                   const json &params = json::object()) override;
  json fetchTickers(const std::vector<std::string> &symbols = std::vector<std::string>(),
                    const json &params = json::object()) override;
  json fetchOrderBook(const std::string &symbol, const long *limit = nullptr,
                      const json &params = json::object()) override
                      {
                        return json::object();
                      }
  json fetchOHLCV(const std::string &symbol, const std::string &timeframe = "1m",
                  const long *since = nullptr, const long *limit = nullptr,
                  const json &params = json::object()) override{
                    return json::object();
                  }

  // Trading API - Sync
  // json createOrder(const std::string& symbol, const std::string& type, const std::string&
  // side,
  //                  const double& amount, const Nullable<Number>& price =
  //                  nullptr, const json& params = json::object()) override;
  json cancelOrder(const std::string &id, const std::string &symbol = "",
                   const json &params = json::object()) override;
  json fetchOrder(const std::string &id, const std::string &symbol = "",
                  const json &params = json::object()) override;
  // json fetchOpenOrders(const std::string& symbol = "", const long* since =
  // nullptr,
  //                     const long* limit = nullptr, const json& params =
  //                     json::object()) override;
  // json fetchMyTrades(const std::string& symbol = "", const long* since = nullptr,
  //                     const json& params = json::object()) override;

  // Account API - Sync
  json fetchBalance(const json &params = json::object()) override;

  // Market Data API - Async
  AsyncPullType fetchMarketsAsync(const json &params = json::object());
  AsyncPullType fetchTickerAsync(const std::string &symbol,
                                 const json &params = json::object());
  AsyncPullType fetchTickersAsync(const std::vector<std::string> &symbols = std::vector<std::string>(),
                    const json &params = json::object());
  // AsyncPullType fetchOrderBookAsync(const std::string& symbol,
  //                                                                     const
  //                                                                     long*
  //                                                                     limit =
  //                                                                     nullptr,
  //                                                                     const
  //                                                                     json&
  //                                                                     params
  //                                                                     =
  //                                                                     json::object());
  AsyncPullType fetchOHLCVAsync(const std::string &symbol,
                                const std::string &timeframe = "1m",
                                const long *since = nullptr,
                                const long *limit = nullptr,
                                const json &params = json::object());

  AsyncPullType withdrawAsync(const std::string &code, const double &amount,
                              const std::string &address, const std::string &tag,
                              const json &params);

  // Trading API - Async
  // AsyncPullType createOrderAsync(const std::string& symbol,
  //                                                                  const
  //                                                                  std::string&
  //                                                                  type,
  //                                                                  const
  //                                                                  std::string&
  //                                                                  side,
  //                                                                  const
  //                                                                  double&
  //                                                                  amount,
  //                                                                  const
  //                                                                  Nullable<Number>&
  //                                                                  price =
  //                                                                  nullptr,
  //                                                                  const
  //                                                                  json&
  //                                                                  params =
  //                                                                  json::object());
  AsyncPullType cancelOrderAsync(const std::string &id, const std::string &symbol = "",
                                 const json &params = json::object());
  AsyncPullType fetchOrderAsync(const std::string &id, const std::string &symbol = "",
                                const json &params = json::object());
  // AsyncPullType fetchOpenOrdersAsync(const std::string& symbol = "",
  //                                                                      const
  //                                                                      long*
  //                                                                      since
  //                                                                      =
  //                                                                      nullptr,
  //                                                                      const
  //                                                                      long*
  //                                                                      limit
  //                                                                      =
  //                                                                      nullptr,
  //                                                                      const
  //                                                                      json&
  //                                                                      params
  //                                                                      =
  //                                                                      json::object());
  // AsyncPullType fetchMyTradesAsync(const std::string& symbol = "",
  //                                                                  const
  //                                                                  long*
  //                                                                  since =
  //                                                                  nullptr,
  //                                                                  const
  //                                                                  long*
  //                                                                  limit =
  //                                                                  nullptr,
  //                                                                  const
  //                                                                  json&
  //                                                                  params =
  //                                                                  json::object());

  // Account API - Async
  AsyncPullType fetchBalanceAsync(const json &params = json::object());

protected:
  // Helper Methods
  json parseMarket(const json &market) const;
  json parseTicker(const json &ticker, const Market &market = Market()) const;
  json parseOrder(const json &order, const Market &market = Market()) const;
  json parseTrade(const json &trade, const Market &market = Market()) const;
  json parseOHLCV(const json &ohlcv, const Market &market = Market()) const {
    return json::object();
  }
  std::string parseOrderStatus(const std::string &status) const;

  // Request Helpers
  std::string
  sign(const std::string &path, const std::string &api = "public",
       const std::string &method = "GET", const json &params = json::object(),
       const std::map<std::string, std::string> &headers = std::map<std::string, std::string>(),
       const json &body = json::object()) const override;
};

} // namespace ccxt

#endif // CCXT_ACE_H
