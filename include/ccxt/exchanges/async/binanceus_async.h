#ifndef CCXT_EXCHANGES_ASYNC_BINANCEUS_ASYNC_H
#define CCXT_EXCHANGES_ASYNC_BINANCEUS_ASYNC_H

#include "ccxt/exchanges/async/binance_async.h"
#include "ccxt/exchanges/binanceus.h"
#include <boost/asio.hpp>

namespace ccxt {

class BinanceUSAsync : public BinanceAsync, public BinanceUS {
public:
    explicit BinanceUSAsync(const boost::asio::io_context& context);
    ~BinanceUSAsync() override = default;
};

} // namespace ccxt

#endif // CCXT_EXCHANGES_ASYNC_BINANCEUS_ASYNC_H
