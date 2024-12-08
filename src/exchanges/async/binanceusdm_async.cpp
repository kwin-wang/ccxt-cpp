#include "ccxt/exchanges/async/binanceusdm_async.h"
#include "ccxt/async_base/async_utils.h"

namespace ccxt {

BinanceUSDMAsync::BinanceUSDMAsync(const boost::asio::io_context& context)
    : BinanceAsync(context)
    , BinanceUSDM() {}

async_result<json> BinanceUSDMAsync::transfer_in(const std::string& code, double amount, const json& params) const {
    // Transfer from spot wallet to USDⓈ-M futures wallet (type = 1)
    return futures_transfer(code, amount, 1, params);
}

async_result<json> BinanceUSDMAsync::transfer_out(const std::string& code, double amount, const json& params) const {
    // Transfer from USDⓈ-M futures wallet to spot wallet (type = 2)
    return futures_transfer(code, amount, 2, params);
}

} // namespace ccxt
