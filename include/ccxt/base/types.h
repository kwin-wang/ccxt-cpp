#pragma once

#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

namespace ccxt {

using json = nlohmann::json;
using String = std::string;

struct Fee {
    String type;
    String currency;
    double rate;
    double cost;
};

struct Market {
    String id;
    String symbol;
    String base;
    String quote;
    String baseId;
    String quoteId;
    String active;
    String type;
    String spot;
    String margin;
    String swap;
    String future;
    String option;
    int precision;
    int pricePrecision;
    int amountPrecision;
    double limits_amount_min;
    double limits_amount_max;
    double limits_price_min;
    double limits_price_max;
    double limits_cost_min;
    double limits_cost_max;
    std::map<String, String> info;

    Market& operator=(const json& j) {
        if (j.contains("id")) id = j["id"].get<String>();
        if (j.contains("symbol")) symbol = j["symbol"].get<String>();
        if (j.contains("base")) base = j["base"].get<String>();
        if (j.contains("quote")) quote = j["quote"].get<String>();
        if (j.contains("type")) type = j["type"].get<String>();
        if (j.contains("spot")) spot = j["spot"].get<String>();
        if (j.contains("margin")) margin = j["margin"].get<String>();
        if (j.contains("swap")) swap = j["swap"].get<String>();
        if (j.contains("future")) future = j["future"].get<String>();
        if (j.contains("option")) option = j["option"].get<String>();
        return *this;
    }
};

struct Currency {
    String id;
    String code;
    String name;
    int precision;
    bool active;
    Fee fee;
    std::map<String, String> info;
    std::vector<String> networks;
};

struct Balance {
    double free;
    double used;
    double total;
};

struct Order {
    String id;
    String clientOrderId;
    String datetime;
    long long timestamp;
    String lastTradeTimestamp;
    String status;
    String symbol;
    String type;
    String timeInForce;
    String side;
    double price;
    double average;
    double amount;
    double filled;
    double remaining;
    double cost;
    Fee fee;
    std::vector<json> trades;
    std::map<String, String> info;
};

struct Trade {
    String id;
    String order;
    String info;
    long long timestamp;
    String datetime;
    String symbol;
    String type;
    String side;
    String takerOrMaker;
    double price;
    double amount;
    double cost;
    Fee fee;
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
    String symbol;
    long long timestamp;
    String datetime;
    double high;
    double low;
    double bid;
    double bidVolume;
    double ask;
    double askVolume;
    double vwap;
    double open;
    double close;
    double last;
    double previousClose;
    double change;
    double percentage;
    double average;
    double baseVolume;
    double quoteVolume;
    std::map<String, String> info;
};

struct OrderBook {
    long long timestamp;
    String datetime;
    String symbol;
    int nonce;
    std::vector<std::vector<double>> bids;
    std::vector<std::vector<double>> asks;
};

} // namespace ccxt
