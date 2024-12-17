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
  json fetchTicker(const String &symbol,
                   const json &params = json::object()) override;
  json fetchTickers(const std::vector<String> &symbols = std::vector<String>(),
                    const json &params = json::object()) override;
  json fetchOrderBook(const String &symbol, const long *limit = nullptr,
                      const json &params = json::object()) override
                      {
                        return json::object();
                      }
  json fetchOHLCV(const String &symbol, const String &timeframe = "1m",
                  const long *since = nullptr, const long *limit = nullptr,
                  const json &params = json::object()) override{
                    return json::object();
                  }

  // Trading API - Sync
  // json createOrder(const String& symbol, const String& type, const String&
  // side,
  //                  const double& amount, const Nullable<Number>& price =
  //                  nullptr, const json& params = json::object()) override;
  json cancelOrder(const String &id, const String &symbol = "",
                   const json &params = json::object()) override;
  json fetchOrder(const String &id, const String &symbol = "",
                  const json &params = json::object()) override;
  // json fetchOpenOrders(const String& symbol = "", const long* since =
  // nullptr,
  //                     const long* limit = nullptr, const json& params =
  //                     json::object()) override;
  // json fetchMyTrades(const String& symbol = "", const long* since = nullptr,
  //                     const json& params = json::object()) override;

  // Account API - Sync
  json fetchBalance(const json &params = json::object()) override;

  // Market Data API - Async
  AsyncPullType fetchMarketsAsync(const json &params = json::object());
  AsyncPullType fetchTickerAsync(const String &symbol,
                                 const json &params = json::object());
  AsyncPullType fetchTickersAsync(const std::vector<String> &symbols = std::vector<String>(),
                    const json &params = json::object());
  // AsyncPullType fetchOrderBookAsync(const String& symbol,
  //                                                                     const
  //                                                                     long*
  //                                                                     limit =
  //                                                                     nullptr,
  //                                                                     const
  //                                                                     json&
  //                                                                     params
  //                                                                     =
  //                                                                     json::object());
  AsyncPullType fetchOHLCVAsync(const String &symbol,
                                const String &timeframe = "1m",
                                const long *since = nullptr,
                                const long *limit = nullptr,
                                const json &params = json::object());

  AsyncPullType withdrawAsync(const String &code, const double &amount,
                              const String &address, const String &tag,
                              const json &params);

  // Trading API - Async
  // AsyncPullType createOrderAsync(const String& symbol,
  //                                                                  const
  //                                                                  String&
  //                                                                  type,
  //                                                                  const
  //                                                                  String&
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
  AsyncPullType cancelOrderAsync(const String &id, const String &symbol = "",
                                 const json &params = json::object());
  AsyncPullType fetchOrderAsync(const String &id, const String &symbol = "",
                                const json &params = json::object());
  // AsyncPullType fetchOpenOrdersAsync(const String& symbol = "",
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
  // AsyncPullType fetchMyTradesAsync(const String& symbol = "",
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
  String parseOrderStatus(const String &status) const;

  // Request Helpers
  String
  sign(const String &path, const String &api = "public",
       const String &method = "GET", const json &params = json::object(),
       const std::map<String, String> &headers = std::map<String, String>(),
       const json &body = json::object()) const override;
};

} // namespace ccxt

#endif // CCXT_ACE_H
