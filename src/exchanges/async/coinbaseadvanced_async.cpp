#include "ccxt/exchanges/async/coinbaseadvanced_async.h"

namespace ccxt {

CoinbaseAdvancedAsync::CoinbaseAdvancedAsync(const boost::asio::io_context& context)
    : CoinbaseAsync(context)
    , CoinbaseAdvanced() {}

} // namespace ccxt
