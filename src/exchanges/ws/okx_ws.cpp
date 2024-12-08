#include "../../../include/ccxt/exchanges/ws/okx_ws.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <boost/crc.hpp>

namespace ccxt {

OKXWS::OKXWS(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, Okx& exchange)
    : WebSocketClient(ioc, ctx), exchange_(exchange), checksumEnabled_(true) {
    // Initialize connection settings specific to OKX
}

std::string OKXWS::getEndpoint() {
    return exchange_.getTestMode() ? "wss://wspap.okx.com:8443/ws/v5" : "wss://ws.okx.com:8443/ws/v5";
}

std::string OKXWS::sign(const std::string& timestamp, const std::string& method,
                       const std::string& path, const std::string& body) {
    std::string message = timestamp + method + path + body;
    return exchange_.hmac(message, exchange_.getSecret(), "sha256", "hex");
}

void OKXWS::authenticate() {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    std::string timestampStr = std::to_string(timestamp / 1000);

    std::string sign = this->sign(timestampStr, "GET", "/users/self/verify", "");

    nlohmann::json request = {
        {"op", "login"},
        {"args", {{
            {"apiKey", exchange_.getApiKey()},
            {"passphrase", exchange_.getPassword()},
            {"timestamp", timestampStr},
            {"sign", sign}
        }}}
    };

    send(request.dump());
}

void OKXWS::watchTicker(const std::string& symbol) {
    nlohmann::json request = {
        {"op", "subscribe"},
        {"args", {{
            {"channel", "tickers"},
            {"instId", symbol}
        }}}
    };
    send(request.dump());
}

void OKXWS::watchTickers(const std::vector<std::string>& symbols) {
    nlohmann::json args = nlohmann::json::array();
    for (const auto& symbol : symbols) {
        args.push_back({
            {"channel", "tickers"},
            {"instId", symbol}
        });
    }
    nlohmann::json request = {
        {"op", "subscribe"},
        {"args", args}
    };
    send(request.dump());
}

void OKXWS::watchOrderBook(const std::string& symbol, const std::string& depth) {
    nlohmann::json request = {
        {"op", "subscribe"},
        {"args", {{
            {"channel", depth},
            {"instId", symbol}
        }}}
    };
    send(request.dump());
}

void OKXWS::watchTrades(const std::string& symbol) {
    nlohmann::json request = {
        {"op", "subscribe"},
        {"args", {{
            {"channel", "trades"},
            {"instId", symbol}
        }}}
    };
    send(request.dump());
}

void OKXWS::watchOHLCV(const std::string& symbol, const std::string& timeframe) {
    nlohmann::json request = {
        {"op", "subscribe"},
        {"args", {{
            {"channel", "candle" + timeframe},
            {"instId", symbol}
        }}}
    };
    send(request.dump());
}

void OKXWS::watchMarkPrice(const std::string& symbol) {
    nlohmann::json request = {
        {"op", "subscribe"},
        {"args", {{
            {"channel", "mark-price"},
            {"instId", symbol}
        }}}
    };
    send(request.dump());
}

void OKXWS::watchMarkPrices(const std::vector<std::string>& symbols) {
    nlohmann::json args = nlohmann::json::array();
    for (const auto& symbol : symbols) {
        args.push_back({
            {"channel", "mark-price"},
            {"instId", symbol}
        });
    }
    nlohmann::json request = {
        {"op", "subscribe"},
        {"args", args}
    };
    send(request.dump());
}

void OKXWS::watchFundingRate(const std::string& symbol) {
    nlohmann::json request = {
        {"op", "subscribe"},
        {"args", {{
            {"channel", "funding-rate"},
            {"instId", symbol}
        }}}
    };
    send(request.dump());
}

void OKXWS::watchFundingRates(const std::vector<std::string>& symbols) {
    nlohmann::json args = nlohmann::json::array();
    for (const auto& symbol : symbols) {
        args.push_back({
            {"channel", "funding-rate"},
            {"instId", symbol}
        });
    }
    nlohmann::json request = {
        {"op", "subscribe"},
        {"args", args}
    };
    send(request.dump());
}

void OKXWS::watchBalance(const std::string& type) {
    authenticate();
    nlohmann::json request = {
        {"op", "subscribe"},
        {"args", {{
            {"channel", "account"},
            {"ccy", type}
        }}}
    };
    send(request.dump());
}

void OKXWS::watchOrders(const std::string& type) {
    authenticate();
    nlohmann::json request = {
        {"op", "subscribe"},
        {"args", {{
            {"channel", "orders"},
            {"instType", type}
        }}}
    };
    send(request.dump());
}

void OKXWS::watchMyTrades(const std::string& type) {
    authenticate();
    nlohmann::json request = {
        {"op", "subscribe"},
        {"args", {{
            {"channel", "trades"},
            {"instType", type}
        }}}
    };
    send(request.dump());
}

void OKXWS::watchPositions() {
    authenticate();
    nlohmann::json request = {
        {"op", "subscribe"},
        {"args", {{
            {"channel", "positions"}
        }}}
    };
    send(request.dump());
}

void OKXWS::watchLiquidations(const std::string& symbol) {
    authenticate();
    nlohmann::json request = {
        {"op", "subscribe"},
        {"args", {{
            {"channel", "liquidation-warning"},
            {"instId", symbol}
        }}}
    };
    send(request.dump());
}

void OKXWS::createOrder(const std::string& symbol, const std::string& type, const std::string& side,
                       double amount, double price) {
    authenticate();
    nlohmann::json request = {
        {"op", "order"},
        {"args", {{
            {"instId", symbol},
            {"tdMode", "cash"},
            {"side", side},
            {"ordType", type},
            {"sz", std::to_string(amount)}
        }}}
    };
    
    if (price > 0) {
        request["args"][0]["px"] = std::to_string(price);
    }
    
    send(request.dump());
}

void OKXWS::editOrder(const std::string& id, const std::string& symbol, const std::string& type,
                     const std::string& side, double amount, double price) {
    authenticate();
    nlohmann::json request = {
        {"op", "amend-order"},
        {"args", {{
            {"instId", symbol},
            {"ordId", id},
            {"newSz", std::to_string(amount)}
        }}}
    };
    
    if (price > 0) {
        request["args"][0]["newPx"] = std::to_string(price);
    }
    
    send(request.dump());
}

void OKXWS::cancelOrder(const std::string& id, const std::string& symbol) {
    authenticate();
    nlohmann::json request = {
        {"op", "cancel-order"},
        {"args", {{
            {"instId", symbol},
            {"ordId", id}
        }}}
    };
    send(request.dump());
}

void OKXWS::cancelOrders(const std::vector<std::string>& ids, const std::string& symbol) {
    authenticate();
    nlohmann::json args = nlohmann::json::array();
    for (const auto& id : ids) {
        args.push_back({
            {"instId", symbol},
            {"ordId", id}
        });
    }
    nlohmann::json request = {
        {"op", "batch-cancel-orders"},
        {"args", args}
    };
    send(request.dump());
}

void OKXWS::cancelAllOrders(const std::string& symbol) {
    authenticate();
    nlohmann::json request = {
        {"op", "cancel-all-orders"},
        {"args", {{
            {"instId", symbol}
        }}}
    };
    send(request.dump());
}

void OKXWS::handleMessage(const std::string& message) {
    try {
        auto j = nlohmann::json::parse(message);
        
        if (j.contains("event")) {
            std::string event = j["event"];
            
            if (event == "login") {
                if (j["code"] == "0") {
                    authenticated_ = true;
                    emit("authenticated", j);
                } else {
                    emit("error", j);
                }
                return;
            }
            
            if (event == "subscribe" || event == "unsubscribe") {
                emit(event, j);
                return;
            }
            
            if (event == "error") {
                handleError(j);
                return;
            }
        }
        
        if (j.contains("data") && j.contains("arg")) {
            const auto& data = j["data"];
            const auto& arg = j["arg"];
            std::string channel = arg["channel"];
            std::string instId = arg["instId"];
            
            if (channel == "tickers") {
                handleTicker(data);
            } else if (channel == "books" || channel == "books5" || channel == "books-l2-tbt") {
                handleOrderBook(data, instId, channel);
            } else if (channel == "trades") {
                handleTrade(data);
            } else if (channel == "candle1m" || channel == "candle5m" || 
                       channel == "candle15m" || channel == "candle30m" ||
                       channel == "candle1H" || channel == "candle4H" ||
                       channel == "candle12H" || channel == "candle1D" ||
                       channel == "candle1W" || channel == "candle1M") {
                handleOHLCV(data);
            } else if (channel == "mark-price") {
                handleMarkPrice(data);
            } else if (channel == "funding-rate") {
                handleFundingRate(data);
            } else if (channel == "liquidations") {
                handleLiquidation(data);
            } else if (channel == "balance") {
                handleBalance(data);
            } else if (channel == "orders") {
                handleOrder(data);
            } else if (channel == "trades") {
                handleMyTrade(data);
            } else if (channel == "positions") {
                handlePosition(data);
            } else if (channel == "liquidation-warning") {
                handleMyLiquidation(data);
            }
        }
    } catch (const std::exception& e) {
        emit("error", {{"message", e.what()}});
    }
}

void OKXWS::handleError(const nlohmann::json& data) {
    emit("error", data);
}

void OKXWS::handleTrade(const nlohmann::json& data) {
    for (const auto& trade : data) {
        nlohmann::json parsedTrade = {
            {"id", trade["tradeId"]},
            {"order", trade["ordId"]},
            {"info", trade},
            {"timestamp", std::stoll(trade["ts"])},
            {"datetime", exchange_.iso8601(std::stoll(trade["ts"]))},
            {"symbol", trade["instId"]},
            {"type", trade["ordType"]},
            {"side", trade["side"]},
            {"takerOrMaker", trade["execType"]},
            {"price", std::stod(trade["px"])},
            {"amount", std::stod(trade["sz"])},
            {"cost", std::stod(trade["px"]) * std::stod(trade["sz"])},
            {"fee", nullptr},  // Fee information not provided in trade update
        };
        emit("trade", parsedTrade);
    }
}

void OKXWS::handleOrder(const nlohmann::json& data) {
    for (const auto& order : data) {
        std::string status;
        if (order["state"] == "live") {
            status = "open";
        } else if (order["state"] == "filled") {
            status = "closed";
        } else if (order["state"] == "canceled") {
            status = "canceled";
        } else if (order["state"] == "partially_filled") {
            status = "open";
        } else {
            status = "unknown";
        }

        nlohmann::json parsedOrder = {
            {"id", order["ordId"]},
            {"clientOrderId", order.value("clOrdId", "")},
            {"info", order},
            {"timestamp", std::stoll(order["cTime"])},
            {"datetime", exchange_.iso8601(std::stoll(order["cTime"]))},
            {"lastTradeTimestamp", std::stoll(order["uTime"])},
            {"status", status},
            {"symbol", order["instId"]},
            {"type", order["ordType"]},
            {"side", order["side"]},
            {"price", std::stod(order["px"])},
            {"amount", std::stod(order["sz"])},
            {"filled", std::stod(order["accFillSz"])},
            {"remaining", std::stod(order["sz"]) - std::stod(order["accFillSz"])},
            {"cost", std::stod(order["px"]) * std::stod(order["accFillSz"])},
            {"average", order.value("avgPx", "0") == "0" ? 0.0 : std::stod(order["avgPx"])},
            {"fee", nullptr},  // Fee information not provided in order update
        };
        emit("order", parsedOrder);
    }
}

void OKXWS::handleMyTrade(const nlohmann::json& data) {
    for (const auto& trade : data) {
        nlohmann::json parsedTrade = {
            {"id", trade["tradeId"]},
            {"order", trade["ordId"]},
            {"info", trade},
            {"timestamp", std::stoll(trade["ts"])},
            {"datetime", exchange_.iso8601(std::stoll(trade["ts"]))},
            {"symbol", trade["instId"]},
            {"type", trade["ordType"]},
            {"side", trade["side"]},
            {"takerOrMaker", trade["execType"]},
            {"price", std::stod(trade["fillPx"])},
            {"amount", std::stod(trade["fillSz"])},
            {"cost", std::stod(trade["fillPx"]) * std::stod(trade["fillSz"])},
            {"fee", {
                {"cost", std::stod(trade["fee"])},
                {"currency", trade["feeCcy"]},
            }},
        };
        emit("trade", parsedTrade);
    }
}

void OKXWS::handlePosition(const nlohmann::json& data) {
    for (const auto& position : data) {
        std::string side = position["posSide"];
        if (side == "net") {
            double pos = std::stod(position["pos"]);
            side = (pos > 0) ? "long" : (pos < 0) ? "short" : "closed";
        }

        nlohmann::json parsedPosition = {
            {"info", position},
            {"symbol", position["instId"]},
            {"timestamp", std::stoll(position["uTime"])},
            {"datetime", exchange_.iso8601(std::stoll(position["uTime"]))},
            {"side", side},
            {"contracts", std::abs(std::stod(position["pos"]))},
            {"contractSize", std::stod(position["ctVal"])},
            {"entryPrice", std::stod(position["avgPx"])},
            {"markPrice", std::stod(position["markPx"])},
            {"notional", std::stod(position["notionalUsd"])},
            {"leverage", std::stod(position["lever"])},
            {"collateral", std::stod(position["margin"])},
            {"initialMargin", std::stod(position["imr"])},
            {"maintenanceMargin", std::stod(position["mmr"])},
            {"marginRatio", std::stod(position["mgnRatio"])},
            {"percentage", std::stod(position["upl"])},
            {"marginMode", position["mgnMode"]},
            {"liquidationPrice", std::stod(position["liqPx"])},
        };
        emit("position", parsedPosition);
    }
}

void OKXWS::handleMyLiquidation(const nlohmann::json& data) {
    for (const auto& liquidation : data) {
        nlohmann::json parsedLiquidation = {
            {"info", liquidation},
            {"symbol", liquidation["instId"]},
            {"timestamp", std::stoll(liquidation["ts"])},
            {"datetime", exchange_.iso8601(std::stoll(liquidation["ts"]))},
            {"type", "margin"},  // OKX only supports margin liquidations
            {"side", liquidation["posSide"]},
            {"price", std::stod(liquidation["markPx"])},
            {"amount", std::stod(liquidation["pos"])},
            {"cost", std::stod(liquidation["notionalUsd"])},
            {"marginMode", liquidation["mgnMode"]},
            {"marginRatio", std::stod(liquidation["mgnRatio"])},
            {"liquidationPrice", std::stod(liquidation["liqPx"])},
            {"warning", true}  // This is a liquidation warning
        };
        emit("liquidation", parsedLiquidation);
    }
}

void OKXWS::handleTicker(const nlohmann::json& data) {
    for (const auto& ticker : data) {
        nlohmann::json parsedTicker = {
            {"symbol", ticker["instId"]},
            {"timestamp", ticker["ts"]},
            {"datetime", exchange_.iso8601(ticker["ts"].get<long long>())},
            {"high", exchange_.parseNumber(ticker["high24h"])},
            {"low", exchange_.parseNumber(ticker["low24h"])},
            {"bid", exchange_.parseNumber(ticker["bidPx"])},
            {"bidVolume", exchange_.parseNumber(ticker["bidSz"])},
            {"ask", exchange_.parseNumber(ticker["askPx"])},
            {"askVolume", exchange_.parseNumber(ticker["askSz"])},
            {"vwap", exchange_.parseNumber(ticker["sodUtc0"])},
            {"open", exchange_.parseNumber(ticker["open24h"])},
            {"close", exchange_.parseNumber(ticker["last"])},
            {"last", exchange_.parseNumber(ticker["last"])},
            {"previousClose", exchange_.parseNumber(ticker["sodUtc0"])},
            {"change", exchange_.parseNumber(ticker["priceChange24h"])},
            {"percentage", exchange_.parseNumber(ticker["priceChangePercent24h"])},
            {"average", nullptr},
            {"baseVolume", exchange_.parseNumber(ticker["vol24h"])},
            {"quoteVolume", exchange_.parseNumber(ticker["volCcy24h"])},
            {"info", ticker}
        };
        exchange_.emit("ticker", parsedTicker["symbol"], parsedTicker);
    }
}

void OKXWS::handleOrderBook(const nlohmann::json& data, const std::string& symbol, const std::string& channel) {
    for (const auto& book : data) {
        bool isSnapshot = book.contains("action") && book["action"] == "snapshot";
        
        nlohmann::json orderBook = {
            {"symbol", symbol},
            {"timestamp", book["ts"]},
            {"datetime", exchange_.iso8601(book["ts"].get<long long>())},
            {"nonce", book["seqId"]},
            {"bids", nlohmann::json::array()},
            {"asks", nlohmann::json::array()}
        };
        
        // Process bids
        for (const auto& bid : book["bids"]) {
            orderBook["bids"].push_back({
                exchange_.parseNumber(bid[0]),  // price
                exchange_.parseNumber(bid[1]),  // amount
                bid.size() > 2 ? bid[2] : nullptr  // count/orders
            });
        }
        
        // Process asks
        for (const auto& ask : book["asks"]) {
            orderBook["asks"].push_back({
                exchange_.parseNumber(ask[0]),  // price
                exchange_.parseNumber(ask[1]),  // amount
                ask.size() > 2 ? ask[2] : nullptr  // count/orders
            });
        }

        // Handle checksum if enabled
        if (checksumEnabled_ && book.contains("checksum")) {
            uint32_t calculatedChecksum = calculateOrderBookChecksum(orderBook);
            uint32_t receivedChecksum = book["checksum"].get<uint32_t>();
            
            if (calculatedChecksum != receivedChecksum) {
                std::cerr << "Orderbook checksum mismatch for " << symbol << ". Expected: " 
                         << receivedChecksum << ", Got: " << calculatedChecksum << std::endl;
                // Resubscribe to get a fresh snapshot
                watchOrderBook(symbol, channel);
                return;
            }
        }
        
        // Emit appropriate event based on whether it's a snapshot or update
        if (isSnapshot) {
            exchange_.emit("orderBook", symbol, orderBook);
        } else {
            exchange_.emit("orderBookUpdate", symbol, orderBook);
        }
    }
}

uint32_t OKXWS::calculateOrderBookChecksum(const nlohmann::json& orderBook) {
    std::stringstream ss;
    
    // Include top 25 bids and asks in checksum calculation
    size_t depth = 25;
    
    // Process bids
    size_t bidCount = std::min(orderBook["bids"].size(), depth);
    for (size_t i = 0; i < bidCount; ++i) {
        const auto& bid = orderBook["bids"][i];
        ss << bid[0].get<std::string>() << ":" << bid[1].get<std::string>() << ":";
    }
    
    // Process asks
    size_t askCount = std::min(orderBook["asks"].size(), depth);
    for (size_t i = 0; i < askCount; ++i) {
        const auto& ask = orderBook["asks"][i];
        ss << ask[0].get<std::string>() << ":" << ask[1].get<std::string>() << ":";
    }
    
    // Calculate CRC32 checksum
    std::string data = ss.str();
    boost::crc_32_type result;
    result.process_bytes(data.data(), data.length());
    return result.checksum();
}

void OKXWS::handleTrades(const nlohmann::json& data) {
    for (const auto& trade : data) {
        nlohmann::json parsedTrade = {
            {"id", trade["tradeId"]},
            {"order", nullptr},  // Not provided in public trades
            {"info", trade},
            {"timestamp", trade["ts"]},
            {"datetime", exchange_.iso8601(trade["ts"].get<long long>())},
            {"symbol", trade["instId"]},
            {"type", nullptr},  // Not provided in public trades
            {"side", trade["side"]},
            {"takerOrMaker", "taker"},  // OKX only provides taker trades in public feed
            {"price", exchange_.parseNumber(trade["px"])},
            {"amount", exchange_.parseNumber(trade["sz"])},
            {"cost", nullptr},  // Will be calculated below
            {"fee", nullptr}    // Not provided in public trades
        };
        
        // Calculate cost
        double price = parsedTrade["price"].get<double>();
        double amount = parsedTrade["amount"].get<double>();
        parsedTrade["cost"] = price * amount;
        
        exchange_.emit("trade", parsedTrade["symbol"], parsedTrade);
    }
}

void OKXWS::handleOHLCV(const nlohmann::json& data) {
    for (const auto& candle : data) {
        nlohmann::json parsedCandle = {
            {"timestamp", candle["ts"]},
            {"open", exchange_.parseNumber(candle["o"])},
            {"high", exchange_.parseNumber(candle["h"])},
            {"low", exchange_.parseNumber(candle["l"])},
            {"close", exchange_.parseNumber(candle["c"])},
            {"volume", exchange_.parseNumber(candle["vol"])},
            {"symbol", candle["instId"]},
            {"timeframe", candle.value("bar", "1m")},  // Default to 1m if not provided
            {"info", candle}
        };

        exchange_.emit("ohlcv", parsedCandle["symbol"], parsedCandle["timeframe"], parsedCandle);
    }
}

void OKXWS::handleMarkPrice(const nlohmann::json& data) {
    for (const auto& price : data) {
        nlohmann::json parsedPrice = {
            {"symbol", price["instId"]},
            {"timestamp", price["ts"]},
            {"datetime", exchange_.iso8601(price["ts"].get<long long>())},
            {"markPrice", exchange_.parseNumber(price["markPx"])},
            {"indexPrice", exchange_.parseNumber(price["idxPx"])},
            {"lastFundingRate", exchange_.parseNumber(price["fundingRate"])},
            {"nextFundingTime", exchange_.parseNumber(price["nextFundingTime"])},
            {"info", price}
        };

        exchange_.emit("markPrice", parsedPrice["symbol"], parsedPrice);
    }
}

void OKXWS::handleFundingRate(const nlohmann::json& data) {
    for (const auto& rate : data) {
        nlohmann::json parsedRate = {
            {"symbol", rate["instId"]},
            {"timestamp", rate["ts"]},
            {"datetime", exchange_.iso8601(rate["ts"].get<long long>())},
            {"fundingRate", exchange_.parseNumber(rate["fundingRate"])},
            {"fundingTimestamp", exchange_.parseNumber(rate["fundingTime"])},
            {"fundingDatetime", exchange_.iso8601(rate["fundingTime"].get<long long>())},
            {"nextFundingRate", exchange_.parseNumber(rate["nextFundingRate"])},
            {"nextFundingTimestamp", exchange_.parseNumber(rate["nextFundingTime"])},
            {"nextFundingDatetime", exchange_.iso8601(rate["nextFundingTime"].get<long long>())},
            {"previousFundingRate", exchange_.parseNumber(rate["lastFundingRate"])},
            {"info", rate}
        };

        exchange_.emit("fundingRate", parsedRate["symbol"], parsedRate);
    }
}

void OKXWS::handleBalance(const nlohmann::json& data) {
    for (const auto& balance : data) {
        nlohmann::json parsedBalance = {
            {"info", balance},
            {"timestamp", balance["ts"]},
            {"datetime", exchange_.iso8601(balance["ts"].get<long long>())},
            {"currency", balance["ccy"]},
            {"total", exchange_.parseNumber(balance["totalEq"])},
            {"free", exchange_.parseNumber(balance["availEq"])},
            {"used", exchange_.parseNumber(balance["frozenBal"])},
            {"debt", exchange_.parseNumber(balance["liab"])},
            {"collateral", exchange_.parseNumber(balance["collateral"])},
            {"marginRatio", exchange_.parseNumber(balance["mgnRatio"])}
        };

        exchange_.emit("balance", parsedBalance["currency"], parsedBalance);
    }
}

void OKXWS::handleOrders(const nlohmann::json& data) {
    for (const auto& order : data) {
        std::string status;
        if (order["state"] == "live") {
            status = "open";
        } else if (order["state"] == "filled") {
            status = "closed";
        } else if (order["state"] == "canceled") {
            status = "canceled";
        } else if (order["state"] == "partially_filled") {
            status = "open";
        } else {
            status = "unknown";
        }

        nlohmann::json parsedOrder = {
            {"id", order["ordId"]},
            {"clientOrderId", order.value("clOrdId", nullptr)},
            {"timestamp", order["uTime"]},
            {"datetime", exchange_.iso8601(order["uTime"].get<long long>())},
            {"lastTradeTimestamp", order["uTime"]},
            {"status", status},
            {"symbol", order["instId"]},
            {"type", order["ordType"]},
            {"timeInForce", order.value("timeInForce", "GTC")},
            {"side", order["side"]},
            {"price", exchange_.parseNumber(order["px"])},
            {"average", exchange_.parseNumber(order["avgPx"])},
            {"amount", exchange_.parseNumber(order["sz"])},
            {"filled", exchange_.parseNumber(order["accFillSz"])},
            {"remaining", exchange_.parseNumber(order["sz"].get<double>() - order["accFillSz"].get<double>())},
            {"cost", exchange_.parseNumber(order["avgPx"].get<double>() * order["accFillSz"].get<double>())},
            {"trades", nullptr},
            {"fee", {
                {"cost", exchange_.parseNumber(order["fee"])},
                {"currency", order["feeCcy"]}
            }},
            {"info", order}
        };

        exchange_.emit("order", parsedOrder["symbol"], parsedOrder);
    }
}

void OKXWS::handleMyTrades(const nlohmann::json& data) {
    for (const auto& trade : data) {
        nlohmann::json parsedTrade = {
            {"id", trade["tradeId"]},
            {"order", trade["ordId"]},
            {"info", trade},
            {"timestamp", trade["ts"]},
            {"datetime", exchange_.iso8601(trade["ts"].get<long long>())},
            {"symbol", trade["instId"]},
            {"type", trade["ordType"]},
            {"side", trade["side"]},
            {"takerOrMaker", trade["execType"] == "T" ? "taker" : "maker"},
            {"price", exchange_.parseNumber(trade["fillPx"])},
            {"amount", exchange_.parseNumber(trade["fillSz"])},
            {"cost", exchange_.parseNumber(trade["fillPx"].get<double>() * trade["fillSz"].get<double>())},
            {"fee", {
                {"cost", exchange_.parseNumber(trade["fee"])},
                {"currency", trade["feeCcy"]}
            }}
        };

        exchange_.emit("trade", parsedTrade["symbol"], parsedTrade);
    }
}

void OKXWS::handlePositions(const nlohmann::json& data) {
    for (const auto& position : data) {
        std::string side = "long";
        if (position["posSide"] == "short" || 
            (position["posSide"] == "net" && exchange_.parseNumber(position["pos"]).get<double>() < 0)) {
            side = "short";
        }

        nlohmann::json parsedPosition = {
            {"info", position},
            {"symbol", position["instId"]},
            {"timestamp", position["uTime"]},
            {"datetime", exchange_.iso8601(position["uTime"].get<long long>())},
            {"contracts", std::abs(exchange_.parseNumber(position["pos"]).get<double>())},
            {"contractSize", exchange_.parseNumber(position["lotSz"])},
            {"side", side},
            {"notional", exchange_.parseNumber(position["notionalUsd"])},
            {"leverage", exchange_.parseNumber(position["lever"])},
            {"collateral", exchange_.parseNumber(position["margin"])},
            {"initialMargin", exchange_.parseNumber(position["imr"])},
            {"maintenanceMargin", exchange_.parseNumber(position["mmr"])},
            {"entryPrice", exchange_.parseNumber(position["avgPx"])},
            {"markPrice", exchange_.parseNumber(position["markPx"])},
            {"liquidationPrice", exchange_.parseNumber(position["liqPx"])},
            {"unrealizedPnl", exchange_.parseNumber(position["upl"])},
            {"realizedPnl", exchange_.parseNumber(position["realizedPnl"])},
            {"percentage", exchange_.parseNumber(position["uplRatio"])},
            {"marginRatio", exchange_.parseNumber(position["mgnRatio"])},
            {"marginMode", position["mgnMode"]},
            {"hedged", position["hedged"].get<bool>()}
        };

        exchange_.emit("position", parsedPosition["symbol"], parsedPosition);
    }
}

void OKXWS::handleLiquidations(const nlohmann::json& data) {
    for (const auto& liquidation : data) {
        nlohmann::json parsedLiquidation = {
            {"info", liquidation},
            {"symbol", liquidation["instId"]},
            {"timestamp", liquidation["ts"]},
            {"datetime", exchange_.iso8601(liquidation["ts"].get<long long>())},
            {"type", liquidation["type"]},
            {"side", liquidation["side"]},
            {"price", std::stod(liquidation["price"])},
            {"amount", std::stod(liquidation["size"])},
            {"marginMode", liquidation["mgnMode"]},
            {"marginRatio", exchange_.parseNumber(liquidation["mgnRatio"])},
            {"liquidationPrice", exchange_.parseNumber(liquidation["liqPx"])},
            {"status", liquidation["state"]},
            {"warning", true}  // This is a liquidation warning
        };

        exchange_.emit("liquidation", parsedLiquidation);
    }
}

void OKXWS::subscribe(const std::string& channel, const std::string& instId,
                     const nlohmann::json& args) {
    nlohmann::json request = {
        {"op", "subscribe"},
        {"args", {{
            {"channel", channel},
            {"instId", instId}
        }}}
    };

    // Add any additional arguments
    if (!args.empty()) {
        for (auto& [key, value] : args.items()) {
            request["args"][0][key] = value;
        }
    }

    // Store subscription for later use
    std::string key = channel + ":" + instId;
    subscriptions_[key] = request.dump();

    send(request.dump());
}

void OKXWS::unsubscribe(const std::string& channel, const std::string& instId) {
    nlohmann::json request = {
        {"op", "unsubscribe"},
        {"args", {{
            {"channel", channel},
            {"instId", instId}
        }}}
    };

    // Remove subscription from storage
    std::string key = channel + ":" + instId;
    subscriptions_.erase(key);

    send(request.dump());
}

void OKXWS::watchMyLiquidations() {
    authenticate();
    nlohmann::json request = {
        {"op", "subscribe"},
        {"args", {{
            {"channel", "liquidation-warning"}
        }}}
    };
    send(request.dump());
}

} // namespace ccxt
