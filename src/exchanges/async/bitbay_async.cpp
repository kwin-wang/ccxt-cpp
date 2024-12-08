#include "ccxt/exchanges/async/bitbay_async.h"

namespace ccxt {

BitBayAsync::BitBayAsync(const boost::asio::io_context& context)
    : ZondaAsync(context)
    , BitBay() {}

} // namespace ccxt
