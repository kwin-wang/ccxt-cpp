#include "ccxt/exchanges/async/ascendex_async.h"
#include "ccxt/base/json.hpp"
#include "ccxt/base/errors.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/spawn.hpp>

namespace ccxt {

AscendexAsync::AscendexAsync(const boost::asio::io_context& context)
    : AsyncExchange(context)
    , Ascendex()
    , context_(const_cast<boost::asio::io_context&>(context)) {}

boost::future<std::vector<Market>> AscendexAsync::fetch_markets_async() {
    auto promise = std::make_shared<boost::promise<std::vector<Market>>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("GET", "/api/pro/v1/products", {});
            auto response = send_request_async(request, yield);
            auto json = nlohmann::json::parse(response);
            auto markets = this->parse_markets(json["data"]);
            promise->set_value(markets);
        } catch (const std::exception& e) {
            promise->set_exception(std::current_exception());
        }
    });

    return future;
}

boost::future<std::vector<Currency>> AscendexAsync::fetch_currencies_async() {
    auto promise = std::make_shared<boost::promise<std::vector<Currency>>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("GET", "/api/pro/v1/assets", {});
            auto response = send_request_async(request, yield);
            auto json = nlohmann::json::parse(response);
            auto currencies = this->parse_currencies(json["data"]);
            promise->set_value(currencies);
        } catch (const std::exception& e) {
            promise->set_exception(std::current_exception());
        }
    });

    return future;
}

boost::future<OrderBook> AscendexAsync::fetch_order_book_async(const std::string& symbol, int limit) {
    auto promise = std::make_shared<boost::promise<OrderBook>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, symbol, limit, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("GET", "/api/pro/v1/depth", {
                {"symbol", this->market_id(symbol)},
                {"limit", std::to_string(limit ? limit : 100)}
            });
            auto response = send_request_async(request, yield);
            auto json = nlohmann::json::parse(response);
            auto orderbook = this->parse_order_book(json["data"], symbol);
            promise->set_value(orderbook);
        } catch (const std::exception& e) {
            promise->set_exception(std::current_exception());
        }
    });

    return future;
}

boost::future<std::map<std::string, Ticker>> AscendexAsync::fetch_tickers_async() {
    auto promise = std::make_shared<boost::promise<std::map<std::string, Ticker>>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("GET", "/api/pro/v1/ticker", {});
            auto response = send_request_async(request, yield);
            auto json = nlohmann::json::parse(response);
            auto tickers = this->parse_tickers(json["data"]);
            promise->set_value(tickers);
        } catch (const std::exception& e) {
            promise->set_exception(std::current_exception());
        }
    });

    return future;
}

boost::future<Ticker> AscendexAsync::fetch_ticker_async(const std::string& symbol) {
    auto promise = std::make_shared<boost::promise<Ticker>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, symbol, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("GET", "/api/pro/v1/ticker", {
                {"symbol", this->market_id(symbol)}
            });
            auto response = send_request_async(request, yield);
            auto json = nlohmann::json::parse(response);
            auto ticker = this->parse_ticker(json["data"], symbol);
            promise->set_value(ticker);
        } catch (const std::exception& e) {
            promise->set_exception(std::current_exception());
        }
    });

    return future;
}

boost::future<std::vector<Trade>> AscendexAsync::fetch_trades_async(
    const std::string& symbol, long since, int limit) {
    auto promise = std::make_shared<boost::promise<std::vector<Trade>>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, symbol, since, limit, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("GET", "/api/pro/v1/trades", {
                {"symbol", this->market_id(symbol)},
                {"n", std::to_string(limit ? limit : 100)}
            });
            auto response = send_request_async(request, yield);
            auto json = nlohmann::json::parse(response);
            auto trades = this->parse_trades(json["data"], symbol);
            promise->set_value(trades);
        } catch (const std::exception& e) {
            promise->set_exception(std::current_exception());
        }
    });

    return future;
}

boost::future<std::vector<OHLCV>> AscendexAsync::fetch_ohlcv_async(
    const std::string& symbol, const std::string& timeframe, long since, int limit) {
    auto promise = std::make_shared<boost::promise<std::vector<OHLCV>>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, symbol, timeframe, since, limit, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("GET", "/api/pro/v1/barhist", {
                {"symbol", this->market_id(symbol)},
                {"interval", timeframe},
                {"n", std::to_string(limit ? limit : 100)}
            });
            auto response = send_request_async(request, yield);
            auto json = nlohmann::json::parse(response);
            auto ohlcv = this->parse_ohlcvs(json["data"], symbol, timeframe);
            promise->set_value(ohlcv);
        } catch (const std::exception& e) {
            promise->set_exception(std::current_exception());
        }
    });

    return future;
}

boost::future<long> AscendexAsync::fetch_time_async() {
    auto promise = std::make_shared<boost::promise<long>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("GET", "/api/pro/v1/time", {});
            auto response = send_request_async(request, yield);
            auto json = nlohmann::json::parse(response);
            promise->set_value(json["data"].get<long>());
        } catch (const std::exception& e) {
            promise->set_exception(std::current_exception());
        }
    });

    return future;
}

boost::future<std::map<std::string, TradingFee>> AscendexAsync::fetch_trading_fees_async() {
    auto promise = std::make_shared<boost::promise<std::map<std::string, TradingFee>>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("GET", "/api/pro/v1/fees", {}, true);
            auto response = send_request_async(request, yield);
            auto json = nlohmann::json::parse(response);
            auto fees = this->parse_trading_fees(json["data"]);
            promise->set_value(fees);
        } catch (const std::exception& e) {
            promise->set_exception(std::current_exception());
        }
    });

    return future;
}

boost::future<std::map<std::string, FundingRate>> AscendexAsync::fetch_funding_rates_async() {
    auto promise = std::make_shared<boost::promise<std::map<std::string, FundingRate>>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("GET", "/api/pro/v2/futures/funding-rates", {});
            auto response = send_request_async(request, yield);
            auto json = nlohmann::json::parse(response);
            auto rates = this->parse_funding_rates(json["data"]);
            promise->set_value(rates);
        } catch (const std::exception& e) {
            promise->set_exception(std::current_exception());
        }
    });

    return future;
}

boost::future<std::vector<LeverageTier>> AscendexAsync::fetch_leverage_tiers_async() {
    auto promise = std::make_shared<boost::promise<std::vector<LeverageTier>>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("GET", "/api/pro/v2/futures/risk-limits", {});
            auto response = send_request_async(request, yield);
            auto json = nlohmann::json::parse(response);
            auto tiers = this->parse_leverage_tiers(json["data"]);
            promise->set_value(tiers);
        } catch (const std::exception& e) {
            promise->set_exception(std::current_exception());
        }
    });

    return future;
}

boost::future<Order> AscendexAsync::create_order_async(
    const std::string& symbol, const std::string& type, const std::string& side,
    double amount, double price, const std::map<std::string, std::string>& params) {
    auto promise = std::make_shared<boost::promise<Order>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, symbol, type, side, amount, price, params, promise](boost::asio::yield_context yield) {
        try {
            auto request_params = params;
            request_params["symbol"] = this->market_id(symbol);
            request_params["orderType"] = type;
            request_params["side"] = side;
            request_params["orderQty"] = std::to_string(amount);
            if (price > 0) {
                request_params["orderPrice"] = std::to_string(price);
            }

            auto request = prepare_request("POST", "/api/pro/v1/cash/order", request_params, true);
            auto response = send_request_async(request, yield);
            auto json = nlohmann::json::parse(response);
            auto order = this->parse_order(json["data"]);
            promise->set_value(order);
        } catch (const std::exception& e) {
            promise->set_exception(std::current_exception());
        }
    });

    return future;
}

// Implementation for other methods follows the same pattern...
// The full implementation would be quite long, so I've included the most important methods
// Let me know if you need the implementation of any specific method

} // namespace ccxt
