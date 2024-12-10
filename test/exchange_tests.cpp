#include <gtest/gtest.h>
#include "../include/ccxt.h"

class ExchangeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code that will be called before each test
    }

    void TearDown() override {
        // Cleanup code that will be called after each test
    }
};

// Test Ace exchange implementation
TEST_F(ExchangeTest, AceExchangeCreation) {
    ccxt::Config config;
    ccxt::ace exchange(config);
    EXPECT_EQ(exchange.id, "ace");
    EXPECT_FALSE(exchange.urls["api"].empty());
}

TEST_F(ExchangeTest, AceExchangeProperties) {
    ccxt::Config config;
    ccxt::ace exchange(config);
    EXPECT_TRUE(exchange.has["fetchTicker"]);
    EXPECT_TRUE(exchange.has["fetchTickers"]);
    EXPECT_TRUE(exchange.has["fetchOrderBook"]);
    EXPECT_TRUE(exchange.has["fetchOHLCV"]);
}

TEST_F(ExchangeTest, AceRateLimits) {
    ccxt::Config config;
    ccxt::ace exchange(config);
    EXPECT_EQ(exchange.rateLimit, 100);
    EXPECT_FALSE(exchange.pro);
}

// Test Alpaca exchange implementation
TEST_F(ExchangeTest, AlpacaExchangeCreation) {
    ccxt::Config config;
    ccxt::alpaca exchange(config);
    EXPECT_EQ(exchange.id, "alpaca");
    EXPECT_FALSE(exchange.urls["api"].empty());
}

TEST_F(ExchangeTest, AlpacaExchangeProperties) {
    ccxt::Config config;
    ccxt::alpaca exchange(config);
    EXPECT_TRUE(exchange.has["fetchTicker"]);
    EXPECT_TRUE(exchange.has["fetchTickers"]);
    EXPECT_TRUE(exchange.has["fetchOrderBook"]);
    EXPECT_TRUE(exchange.has["fetchOHLCV"]);
}

TEST_F(ExchangeTest, AlpacaRateLimits) {
    ccxt::Config config;
    ccxt::alpaca exchange(config);
    EXPECT_EQ(exchange.rateLimit, 333);
    EXPECT_TRUE(exchange.pro);
}
