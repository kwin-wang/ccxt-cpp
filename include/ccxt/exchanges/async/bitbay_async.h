#ifndef CCXT_EXCHANGES_ASYNC_BITBAY_ASYNC_H
#define CCXT_EXCHANGES_ASYNC_BITBAY_ASYNC_H

#include "ccxt/exchanges/async/zonda_async.h"
#include "ccxt/exchanges/bitbay.h"
#include <boost/asio.hpp>

namespace ccxt {

class BitBayAsync : public ZondaAsync, public BitBay {
public:
    explicit BitBayAsync(const boost::asio::io_context& context);
    ~BitBayAsync() override = default;
};

} // namespace ccxt

#endif // CCXT_EXCHANGES_ASYNC_BITBAY_ASYNC_H
