#include <gtest/gtest.h>
#include "ccxt/base/exchange.h"
#include "ccxt/base/config.h"
#include "ccxt/exchanges/ace.h"

class BaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        config = ccxt::Config();
        config.apiKey = "test_api_key";
        config.secret = "test_secret";
    }

    ccxt::Config config;
};

TEST_F(BaseTest, ExchangeCreation) {
    ccxt::ace exchange(config);
    EXPECT_EQ(exchange.id, "ace");
    EXPECT_EQ(exchange.name, "ACE");
    EXPECT_FALSE(exchange.countries.empty());
    EXPECT_EQ(exchange.countries[0], "TW");
}

TEST_F(BaseTest, ExchangeConfiguration) {
    ccxt::ace exchange(config);
    EXPECT_EQ(exchange.rateLimit, 2000);
    EXPECT_FALSE(exchange.pro);
    EXPECT_FALSE(exchange.urls.empty());
    EXPECT_FALSE(exchange.has.empty());
    EXPECT_FALSE(exchange.timeframes.empty());
}

TEST_F(BaseTest, MarketMethods) {
    ccxt::ace exchange(config);
    auto markets = exchange.fetchMarkets();
    EXPECT_TRUE(markets.is_array());

    auto ticker = exchange.fetchTicker("BTC/USDT");
    EXPECT_TRUE(ticker.is_object());

    auto tickers = exchange.fetchTickers({"BTC/USDT", "ETH/USDT"});
    EXPECT_TRUE(tickers.is_array());

    auto orderbook = exchange.fetchOrderBook("BTC/USDT");
    EXPECT_TRUE(orderbook.is_object());
}

TEST_F(BaseTest, TradingMethods) {
    ccxt::ace exchange(config);
    auto balance = exchange.fetchBalance();
    EXPECT_TRUE(balance.is_object());

    auto order = exchange.createOrder("BTC/USDT", "limit", "buy", 0.1, 50000.0);
    EXPECT_TRUE(order.is_object());

    auto cancelResult = exchange.cancelOrder(order["id"], "BTC/USDT");
    EXPECT_TRUE(cancelResult.is_object());

    auto fetchedOrder = exchange.fetchOrder(order["id"], "BTC/USDT");
    EXPECT_TRUE(fetchedOrder.is_object());
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
