#include "ccxt/exchanges/coinbaseadvanced.h"

namespace ccxt {

ExchangeRegistry::Factory coinbaseadvanced::factory("coinbaseadvanced", &coinbaseadvanced::createInstance);

coinbaseadvanced::coinbaseadvanced(const Config& config)
    : coinbase(config) {
    init();
}

void coinbaseadvanced::init() {
    coinbase::init();
}

Json coinbaseadvanced::describeImpl() const {
    return this->deepExtend(coinbase::describeImpl(), {
        {"id", "coinbaseadvanced"},
        {"name", "Coinbase Advanced"},
        {"alias", true}
    });
}

} // namespace ccxt
