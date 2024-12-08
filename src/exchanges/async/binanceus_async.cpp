#include "ccxt/exchanges/async/binanceus_async.h"
#include "ccxt/async_base/async_utils.h"

namespace ccxt {

BinanceUSAsync::BinanceUSAsync(const boost::asio::io_context& context)
    : BinanceAsync(context)
    , BinanceUS() {}

} // namespace ccxt
