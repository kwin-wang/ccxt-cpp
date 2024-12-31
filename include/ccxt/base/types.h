#pragma once

#include <string>
#include <vector>
#include <map>
#include <ccxt/base/config.h>

namespace ccxt {
struct Fee {
    std::string type;
    std::string currency;
    double rate;
    double cost;
};

struct Market {
    std::string id;
    std::string symbol;
    std::string base;
    std::string quote;
    std::string baseId;
    std::string quoteId;
    std::string active;
    std::string type;
    std::string spot;
    std::string margin;
    std::string swap;
    std::string future;
    std::string option;
    int precision;
    int pricePrecision;
    int amountPrecision;
    double limits_amount_min;
    double limits_amount_max;
    double limits_price_min;
    double limits_price_max;
    double limits_cost_min;
    double limits_cost_max;
    std::map<std::string, std::string> info;

    Market& operator=(const json& j) {
        if (j.contains("id")) id = j["id"].get<std::string>();
        if (j.contains("symbol")) symbol = j["symbol"].get<std::string>();
        if (j.contains("base")) base = j["base"].get<std::string>();
        if (j.contains("quote")) quote = j["quote"].get<std::string>();
        if (j.contains("type")) type = j["type"].get<std::string>();
        if (j.contains("spot")) spot = j["spot"].get<std::string>();
        if (j.contains("margin")) margin = j["margin"].get<std::string>();
        if (j.contains("swap")) swap = j["swap"].get<std::string>();
        if (j.contains("future")) future = j["future"].get<std::string>();
        if (j.contains("option")) option = j["option"].get<std::string>();
        return *this;
    }

    const std::string& operator[](const std::string& key) const {
        if (key == "id") return id;
        if (key == "symbol") return symbol;
        if (key == "base") return base;
        if (key == "quote") return quote;
        if (key == "baseId") return baseId;
        if (key == "quoteId") return quoteId;
        if (key == "type") return type;
        if (key == "spot") return spot;
        if (key == "margin") return margin;
        if (key == "swap") return swap;
        if (key == "future") return future;
        if (key == "option") return option;
        if (key == "active") return active;
        static const std::string empty;
        return empty;
    }
};

struct Currency {
    std::string id;
    std::string code;
    std::string name;
    int precision;
    bool active;
    Fee fee;
    std::map<std::string, std::string> info;
    std::vector<std::string> networks;
};

struct Balance {
    double free;
    double used;
    double total;
    uint64_t timestamp;
    std::string currency;
};

struct Order {
    std::string id;
    std::string clientOrderId;
    std::string datetime;
    long long timestamp;
    std::string lastTradeTimestamp;
    std::string status;
    std::string symbol;
    std::string type;
    std::string timeInForce;
    std::string side;
    double price;
    double average;
    double amount;
    double filled;
    double remaining;
    double cost;
    Fee fee;
    std::vector<json> trades;
    std::map<std::string, std::string> info;
};

struct Trade {
    std::string id;
    std::string order;
    std::string info;
    long long timestamp;
    std::string datetime;
    std::string symbol;
    std::string type;
    std::string side;
    std::string takerOrMaker;
    double price;
    double amount;
    double cost;
    double fee;
    std::string feeCurrency;
    std::string orderId;
};

struct OHLCV {
    long long timestamp;
    double open;
    double high;
    double low;
    double close;
    double volume;
};

struct Ticker {
    std::string symbol;
    long long timestamp;
    std::string datetime;
    double high;
    double low;
    double bid;
    double bidVolume;
    double ask;
    double askVolume;
    double vwap;
    double volume;
    double open;
    double close;
    double last;
    double previousClose;
    double change;
    double percentage;
    double average;
    double baseVolume;
    double quoteVolume;
    double markPrice;
    double indexPrice;
    double priceChange;
    double priceChangePercent;
    double previousClosePrice;
    double lastPrice;
    double lastQuantity;
    double previousDayClose;
    double currentDayClose;
    double currentDayCloseChange;
    double currentDayCloseChangePercent;
    double currentDayClosePrice;
    double currentDayCloseQuantity;
    double currentDayCloseAverage;
    std::map<std::string, std::string> info;
};

struct OrderBook {
    long long timestamp;
    std::string datetime;
    std::string symbol;
    int nonce;
    std::vector<std::vector<double>> bids;
    std::vector<std::vector<double>> asks;
};

struct Position {
    std::string symbol;
    std::string type;
    std::string side;
    std::string marginType;
    double notional;
    double leverage;
    double unrealizedPnl;
    double contracts;
    double contractSize;
    double entryPrice;
    double markPrice;
    double collateral;
    double initialMargin;
    double maintenanceMargin;
    long long timestamp;
    std::string datetime;
    std::map<std::string, std::string> info;
    std::string amount;
};

struct MarkPrice {
    std::string symbol;
    double markPrice;
    double indexPrice;
    double estimatedSettlePrice;
    double lastFundingRate;
    long long nextFundingTime;
    long long timestamp;
    double fundingRate;
    std::string datetime;
    std::map<std::string, std::string> info;
};

} // namespace ccxt
