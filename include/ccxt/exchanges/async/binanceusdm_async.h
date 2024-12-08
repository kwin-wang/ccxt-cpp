#ifndef CCXT_EXCHANGES_ASYNC_BINANCEUSDM_ASYNC_H
#define CCXT_EXCHANGES_ASYNC_BINANCEUSDM_ASYNC_H

#include "ccxt/exchanges/async/binance_async.h"
#include "ccxt/exchanges/binanceusdm.h"
#include <boost/asio.hpp>

namespace ccxt {

class BinanceUSDMAsync : public BinanceAsync, public BinanceUSDM {
public:
    explicit BinanceUSDMAsync(const boost::asio::io_context& context);
    ~BinanceUSDMAsync() override = default;

    // Transfer methods specific to USDⓈ-M Futures
    async_result<json> transfer_in(const std::string& code, double amount, const json& params = json::object()) const;
    async_result<json> transfer_out(const std::string& code, double amount, const json& params = json::object()) const;
};

} // namespace ccxt

#endif // CCXT_EXCHANGES_ASYNC_BINANCEUSDM_ASYNC_H
