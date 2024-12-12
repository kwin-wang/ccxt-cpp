#include "ccxt/exchanges/async/bitbay_async.h"

namespace ccxt {

BitBayAsync::BitBayAsync(const Config& config) : ZondaAsync(config) {
    // BitBay is an alias for Zonda, so we only need to override the exchange info
    this->id = "bitbay";
    this->name = "BitBay";
}

} // namespace ccxt
