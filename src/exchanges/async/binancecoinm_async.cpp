#include "ccxt/exchanges/async/binancecoinm_async.h"
#include "ccxt/async_base/async_utils.h"

namespace ccxt {

BinanceCoinMAsync::BinanceCoinMAsync(const boost::asio::io_context& context)
    : BinanceAsync(context)
    , BinanceCoinM() {}

// Transfer Methods
boost::future<TransferEntry> BinanceCoinMAsync::transfer_in_async(const std::string& code,
                                                                double amount,
                                                                const std::map<std::string, std::string>& params) {
    return async_request<TransferEntry>(context_, [this, code, amount, params]() {
        return transfer_in(code, amount, params);
    });
}

boost::future<TransferEntry> BinanceCoinMAsync::transfer_out_async(const std::string& code,
                                                                 double amount,
                                                                 const std::map<std::string, std::string>& params) {
    return async_request<TransferEntry>(context_, [this, code, amount, params]() {
        return transfer_out(code, amount, params);
    });
}

} // namespace ccxt
