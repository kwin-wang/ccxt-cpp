#pragma once

#include <string>
#include <vector>
#include <map>
#include <ccxt/base/config.h>

namespace ccxt {

// 费用结构体，用于表示交易费用信息
struct Fee {
    std::string type;      // 费用类型
    std::string currency;  // 费用币种
    double rate;          // 费率
    double cost;          // 费用金额
};

// 市场结构体，用于表示交易市场信息
struct Market {
    std::string id;           // 市场ID
    std::string symbol;       // 交易对符号
    std::string base;         // 基础货币
    std::string quote;        // 计价货币
    std::string baseId;       // 基础货币ID
    std::string quoteId;      // 计价货币ID
    std::string active;       // 市场是否活跃
    std::string type;         // 市场类型
    std::string spot;         // 现货交易
    std::string margin;       // 保证金交易
    std::string swap;         // 永续合约
    std::string future;       // 期货合约
    std::string option;       // 期权合约
    int precision;            // 精度
    int pricePrecision;       // 价格精度
    int amountPrecision;      // 数量精度
    double limits_amount_min; // 最小交易数量
    double limits_amount_max; // 最大交易数量
    double limits_price_min;  // 最小价格
    double limits_price_max;  // 最大价格
    double limits_cost_min;   // 最小交易成本
    double limits_cost_max;   // 最大交易成本
    std::map<std::string, std::string> info; // 额外信息

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

// 货币结构体，用于表示货币信息
struct Currency {
    std::string id;           // 货币ID
    std::string code;         // 货币代码
    std::string name;         // 货币名称
    int precision;            // 精度
    bool active;              // 是否可用
    Fee fee;                  // 费用信息
    std::map<std::string, std::string> info; // 额外信息
    std::vector<std::string> networks;       // 支持的网络
};

// 余额结构体，用于表示账户余额信息
struct Balance {
    double free;              // 可用余额
    double used;              // 已用余额
    double total;             // 总余额
    uint64_t timestamp;       // 时间戳
    std::string currency;     // 货币
};

// 订单结构体，用于表示交易订单信息
struct Order {
    std::string id;           // 订单ID
    std::string clientOrderId;// 客户端订单ID
    std::string datetime;     // 日期时间
    long long timestamp;      // 时间戳
    std::string lastTradeTimestamp; // 最后交易时间
    std::string status;       // 订单状态
    std::string symbol;       // 交易对
    std::string type;         // 订单类型
    std::string timeInForce;  // 订单时效
    std::string side;         // 买卖方向
    double price;             // 价格
    double average;           // 平均价格
    double amount;            // 数量
    double filled;            // 已成交数量
    double remaining;         // 剩余数量
    double cost;              // 成本
    Fee fee;                  // 费用信息
    std::vector<json> trades; // 成交记录
    std::map<std::string, std::string> info; // 额外信息
};

// 交易结构体，用于表示交易记录信息
struct Trade {
    std::string id;           // 交易ID
    std::string order;        // 订单ID
    std::string info;         // 交易信息
    long long timestamp;      // 时间戳
    std::string datetime;     // 日期时间
    std::string symbol;       // 交易对
    std::string type;         // 交易类型
    std::string side;         // 买卖方向
    std::string takerOrMaker; // 吃单方/挂单方
    double price;             // 价格
    double amount;            // 数量
    double cost;              // 成本
    double fee;               // 费用
    std::string feeCurrency;  // 费用币种
    std::string orderId;      // 订单ID
};

// K线数据结构体，用于表示OHLCV数据
struct OHLCV {
    long long timestamp;      // 时间戳
    double open;              // 开盘价
    double high;              // 最高价
    double low;               // 最低价
    double close;             // 收盘价
    double volume;            // 成交量
};

// 行情结构体，用于表示市场行情信息
struct Ticker {
    std::string symbol;       // 交易对
    long long timestamp;      // 时间戳
    std::string datetime;     // 日期时间
    double high;              // 24小时最高价
    double low;               // 24小时最低价
    double bid;               // 买一价
    double bidVolume;         // 买一量
    double ask;               // 卖一价
    double askVolume;         // 卖一量
    double vwap;              // 成交量加权平均价格
    double volume;            // 24小时成交量
    double open;              // 开盘价
    double close;             // 收盘价
    double last;              // 最新成交价
    double previousClose;     // 前收盘价
    double change;            // 价格变化
    double percentage;        // 价格变化百分比
    double average;           // 平均价格
    double baseVolume;        // 基础货币成交量
    double quoteVolume;       // 计价货币成交量
    double markPrice;         // 标记价格
    double indexPrice;        // 指数价格
    double priceChange;       // 价格变化
    double priceChangePercent;// 价格变化百分比
    double previousClosePrice;// 前收盘价
    double lastPrice;         // 最新价格
    double lastQuantity;      // 最新成交量
    double previousDayClose;  // 前一日收盘价
    double currentDayClose;   // 当日收盘价
    double currentDayCloseChange;      // 当日收盘价变化
    double currentDayCloseChangePercent;// 当日收盘价变化百分比
    double currentDayClosePrice;       // 当日收盘价
    double currentDayCloseQuantity;    // 当日收盘成交量
    double currentDayCloseAverage;     // 当日收盘平均价
    std::map<std::string, std::string> info; // 额外信息
};

// 订单簿结构体，用于表示市场深度信息
struct OrderBook {
    long long timestamp;      // 时间戳
    std::string datetime;     // 日期时间
    std::string symbol;       // 交易对
    int nonce;                // 序号
    std::vector<std::vector<double>> bids; // 买单列表 [价格, 数量]
    std::vector<std::vector<double>> asks; // 卖单列表 [价格, 数量]
};

// 持仓结构体，用于表示合约持仓信息
struct Position {
    std::string symbol;       // 交易对
    std::string type;         // 持仓类型
    std::string side;         // 持仓方向
    std::string marginType;   // 保证金类型
    double notional;          // 名义价值
    double leverage;          // 杠杆倍数
    double unrealizedPnl;     // 未实现盈亏
    double contracts;         // 合约数量
    double contractSize;      // 合约大小
    double entryPrice;        // 开仓价格
    double markPrice;         // 标记价格
    double collateral;        // 保证金
    double initialMargin;     // 初始保证金
    double maintenanceMargin; // 维持保证金
    long long timestamp;      // 时间戳
    std::string datetime;     // 日期时间
    std::map<std::string, std::string> info; // 额外信息
    std::string amount;       // 持仓数量
};

// 标记价格结构体，用于表示合约标记价格信息
struct MarkPrice {
    std::string symbol;       // 交易对
    double markPrice;         // 标记价格
    double indexPrice;        // 指数价格
    double estimatedSettlePrice; // 预估结算价
    double lastFundingRate;   // 最新资金费率
    long long nextFundingTime;// 下次资金费率时间
    long long timestamp;      // 时间戳
    double fundingRate;       // 资金费率
    std::string datetime;     // 日期时间
    std::map<std::string, std::string> info; // 额外信息
};

} // namespace ccxt
