#include "ccxt/exchanges/coinbaseadvanced.h"

namespace ccxt {


coinbaseadvanced::coinbaseadvanced(const Config& config)
    : coinbase(config) {
    init();
}

void coinbaseadvanced::init() {
    coinbase::init();

    // Update API endpoints for advanced trading
    baseUrl = "https://api.coinbase.com";
    urls = {
        {"logo", "https://user-images.githubusercontent.com/1294454/108623144-67a3ef00-744e-11eb-8140-75c6b851e945.jpg"},
        {"api", {
            {"public", "https://api.coinbase.com"},
            {"private", "https://api.coinbase.com"}
        }},
        {"www", "https://pro.coinbase.com/"},
        {"doc", {
            "https://docs.cloud.coinbase.com/advanced-trade-api/docs/rest-api-overview",
            "https://docs.cloud.coinbase.com/advanced-trade-api/reference"
        }},
        {"fees", "https://help.coinbase.com/en/advanced-trade/trading-and-funding/trading-fees"}
    };

    // Update API endpoint paths
    api = {
        {"public", {
            {"GET", {
                "/api/v3/brokerage/products",
                "/api/v3/brokerage/products/{product_id}",
                "/api/v3/brokerage/products/{product_id}/book",
                "/api/v3/brokerage/products/{product_id}/candles",
                "/api/v3/brokerage/products/{product_id}/ticker",
                "/api/v3/brokerage/products/{product_id}/trades"
            }}
        }},
        {"private", {
            {"GET", {
                "/api/v3/brokerage/accounts",
                "/api/v3/brokerage/orders/historical/{order_id}",
                "/api/v3/brokerage/orders/historical/batch",
                "/api/v3/brokerage/orders/historical/fills"
            }},
            {"POST", {
                "/api/v3/brokerage/orders"
            }},
            {"DELETE", {
                "/api/v3/brokerage/orders/batch_cancel",
                "/api/v3/brokerage/orders/{order_id}"
            }}
        }}
    };

    // Update rate limits
    rateLimit = 34;  // 30 requests per second for private endpoints, 10 for public
    options = {
        {"versions", {
            {"brokerage", "v3"}
        }},
        {"defaultVersion", "v3"},
        {"defaultType", "spot"},
        {"broker", {
            {"apiKey", ""},
            {"secret", ""},
            {"name", ""}
        }}
    };
}

Json coinbaseadvanced::describeImpl() const {
    return this->deepExtend(coinbase::describeImpl(), {
        {"id", "coinbaseadvanced"},
        {"name", "Coinbase Advanced"},
        {"alias", true},
        {"has", {
            {"CORS", true},
            {"spot", true},
            {"margin", false},
            {"swap", false},
            {"future", false},
            {"option", false},
            {"cancelAllOrders", true},
            {"cancelOrder", true},
            {"createOrder", true},
            {"fetchBalance", true},
            {"fetchClosedOrders", true},
            {"fetchMarkets", true},
            {"fetchMyTrades", true},
            {"fetchOHLCV", true},
            {"fetchOpenOrders", true},
            {"fetchOrder", true},
            {"fetchOrderBook", true},
            {"fetchOrders", true},
            {"fetchTicker", true},
            {"fetchTrades", true}
        }}
    });
}

} // namespace ccxt
