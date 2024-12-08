#ifndef CCXT_EXCHANGE_COINBASEADVANCED_H
#define CCXT_EXCHANGE_COINBASEADVANCED_H

#include "coinbase.h"
#include "../exchange_impl.h"

namespace ccxt {

class coinbaseadvanced : public coinbase {
public:
    coinbaseadvanced(const Config& config = Config());
    ~coinbaseadvanced() = default;

    static Exchange* create(const Config& config = Config()) {
        return new coinbaseadvanced(config);
    }

protected:
    void init() override;
    Json describeImpl() const override;

private:
    static Exchange* createInstance(const Config& config) {
        return new coinbaseadvanced(config);
    }

    static ExchangeRegistry::Factory factory;
};

} // namespace ccxt

#endif // CCXT_EXCHANGE_COINBASEADVANCED_H
