#ifndef CCXT_EXCHANGES_ASYNC_BINANCECOINM_ASYNC_H
#define CCXT_EXCHANGES_ASYNC_BINANCECOINM_ASYNC_H

#include "ccxt/exchanges/async/binance_async.h"
#include "ccxt/exchanges/binancecoinm.h"
#include <boost/asio.hpp>
#include <string>
#include <map>

namespace ccxt {

class BinanceCoinMAsync : public BinanceAsync, public BinanceCoinM {
public:
    explicit BinanceCoinMAsync(const boost::asio::io_context& context);
    ~BinanceCoinMAsync() override = default;

    // Transfer Methods
    boost::future<TransferEntry> transfer_in_async(const std::string& code,
                                                 double amount,
                                                 const std::map<std::string, std::string>& params = {});
    boost::future<TransferEntry> transfer_out_async(const std::string& code,
                                                  double amount,
                                                  const std::map<std::string, std::string>& params = {});
};

} // namespace ccxt

#endif // CCXT_EXCHANGES_ASYNC_BINANCECOINM_ASYNC_H
