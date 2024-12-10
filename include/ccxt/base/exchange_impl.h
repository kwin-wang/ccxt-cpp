#pragma once

#include "ccxt/base/exchange.h"

namespace ccxt {

template<typename T>
class ExchangeImpl : public Exchange {
public:
    ExchangeImpl() : Exchange() {}
    ExchangeImpl(const Config& config) : Exchange(config) {}
    virtual ~ExchangeImpl() {}

protected:
    T* derived() {
        return static_cast<T*>(this);
    }

    const T* derived() const {
        return static_cast<const T*>(this);
    }
};

} // namespace ccxt
