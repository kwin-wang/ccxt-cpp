#ifndef CCXT_EXCHANGES_ASYNC_COINBASEADVANCED_ASYNC_H
#define CCXT_EXCHANGES_ASYNC_COINBASEADVANCED_ASYNC_H

#include "ccxt/exchanges/async/coinbase_async.h"
#include "ccxt/exchanges/coinbaseadvanced.h"
#include <boost/asio.hpp>

namespace ccxt {

class CoinbaseAdvancedAsync : public CoinbaseAsync, public CoinbaseAdvanced {
public:
    explicit CoinbaseAdvancedAsync(const boost::asio::io_context& context);
    ~CoinbaseAdvancedAsync() override = default;
};

} // namespace ccxt

#endif // CCXT_EXCHANGES_ASYNC_COINBASEADVANCED_ASYNC_H
