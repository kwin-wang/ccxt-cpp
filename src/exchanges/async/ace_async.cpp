#include "ccxt/exchanges/async/ace_async.h"
#include "ccxt/base/json.hpp"
#include "ccxt/base/errors.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/spawn.hpp>

namespace ccxt {

AceAsync::AceAsync(const boost::asio::io_context& context)
    : AsyncExchange(context)
    , Ace()
    , context_(const_cast<boost::asio::io_context&>(context)) {}

boost::future<OrderBook> AceAsync::fetch_order_book_async(const std::string& symbol, int limit) {
    auto promise = std::make_shared<boost::promise<OrderBook>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, symbol, limit, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("GET", "/api/v2/depth", {
                {"symbol", this->market_id(symbol)},
                {"limit", std::to_string(limit ? limit : 100)}
            });
            auto response = send_request_async(request, yield);
            auto json = nlohmann::json::parse(response);
            auto orderbook = this->parse_order_book(json, symbol);
            promise->set_value(orderbook);
        } catch (const std::exception& e) {
            promise->set_exception(std::current_exception());
        }
    });

    return future;
}

boost::future<std::map<std::string, Ticker>> AceAsync::fetch_tickers_async() {
    auto promise = std::make_shared<boost::promise<std::map<std::string, Ticker>>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("GET", "/api/v2/tickers", {});
            auto response = send_request_async(request, yield);
            auto json = nlohmann::json::parse(response);
            auto tickers = this->parse_tickers(json);
            promise->set_value(tickers);
        } catch (const std::exception& e) {
            promise->set_exception(std::current_exception());
        }
    });

    return future;
}

boost::future<Ticker> AceAsync::fetch_ticker_async(const std::string& symbol) {
    auto promise = std::make_shared<boost::promise<Ticker>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, symbol, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("GET", "/api/v2/ticker", {
                {"symbol", this->market_id(symbol)}
            });
            auto response = send_request_async(request, yield);
            auto json = nlohmann::json::parse(response);
            auto ticker = this->parse_ticker(json, symbol);
            promise->set_value(ticker);
        } catch (const std::exception& e) {
            promise->set_exception(std::current_exception());
        }
    });

    return future;
}

boost::future<std::vector<Market>> AceAsync::fetch_markets_async() {
    auto promise = std::make_shared<boost::promise<std::vector<Market>>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("GET", "/api/v2/markets", {});
            auto response = send_request_async(request, yield);
            auto json = nlohmann::json::parse(response);
            auto markets = this->parse_markets(json);
            promise->set_value(markets);
        } catch (const std::exception& e) {
            promise->set_exception(std::current_exception());
        }
    });

    return future;
}

boost::future<std::vector<OHLCV>> AceAsync::fetch_ohlcv_async(
    const std::string& symbol, const std::string& timeframe, long since, int limit) {
    auto promise = std::make_shared<boost::promise<std::vector<OHLCV>>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, symbol, timeframe, since, limit, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("GET", "/api/v2/klines", {
                {"symbol", this->market_id(symbol)},
                {"interval", this->timeframes.at(timeframe)},
                {"since", std::to_string(since)},
                {"limit", std::to_string(limit ? limit : 100)}
            });
            auto response = send_request_async(request, yield);
            auto json = nlohmann::json::parse(response);
            auto ohlcv = this->parse_ohlcvs(json, symbol, timeframe);
            promise->set_value(ohlcv);
        } catch (const std::exception& e) {
            promise->set_exception(std::current_exception());
        }
    });

    return future;
}

boost::future<Order> AceAsync::create_order_async(
    const std::string& symbol, const std::string& type,
    const std::string& side, double amount, double price) {
    auto promise = std::make_shared<boost::promise<Order>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, symbol, type, side, amount, price, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("POST", "/api/v2/orders", {
                {"symbol", this->market_id(symbol)},
                {"type", type},
                {"side", side},
                {"amount", std::to_string(amount)},
                {"price", std::to_string(price)}
            }, true);
            auto response = send_request_async(request, yield);
            auto json = nlohmann::json::parse(response);
            auto order = this->parse_order(json);
            promise->set_value(order);
        } catch (const std::exception& e) {
            promise->set_exception(std::current_exception());
        }
    });

    return future;
}

boost::future<Order> AceAsync::cancel_order_async(const std::string& id, const std::string& symbol) {
    auto promise = std::make_shared<boost::promise<Order>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, id, symbol, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("DELETE", "/api/v2/orders/" + id, {}, true);
            auto response = send_request_async(request, yield);
            auto json = nlohmann::json::parse(response);
            auto order = this->parse_order(json);
            promise->set_value(order);
        } catch (const std::exception& e) {
            promise->set_exception(std::current_exception());
        }
    });

    return future;
}

boost::future<Order> AceAsync::fetch_order_async(const std::string& id, const std::string& symbol) {
    auto promise = std::make_shared<boost::promise<Order>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, id, symbol, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("GET", "/api/v2/orders/" + id, {}, true);
            auto response = send_request_async(request, yield);
            auto json = nlohmann::json::parse(response);
            auto order = this->parse_order(json);
            promise->set_value(order);
        } catch (const std::exception& e) {
            promise->set_exception(std::current_exception());
        }
    });

    return future;
}

boost::future<std::vector<Order>> AceAsync::fetch_open_orders_async(const std::string& symbol) {
    auto promise = std::make_shared<boost::promise<std::vector<Order>>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, symbol, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("GET", "/api/v2/orders", {
                {"symbol", symbol.empty() ? "" : this->market_id(symbol)}
            }, true);
            auto response = send_request_async(request, yield);
            auto json = nlohmann::json::parse(response);
            auto orders = this->parse_orders(json);
            promise->set_value(orders);
        } catch (const std::exception& e) {
            promise->set_exception(std::current_exception());
        }
    });

    return future;
}

boost::future<std::vector<Trade>> AceAsync::fetch_my_trades_async(
    const std::string& symbol, long since, int limit) {
    auto promise = std::make_shared<boost::promise<std::vector<Trade>>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, symbol, since, limit, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("GET", "/api/v2/trades/my", {
                {"symbol", symbol.empty() ? "" : this->market_id(symbol)},
                {"since", std::to_string(since)},
                {"limit", std::to_string(limit ? limit : 100)}
            }, true);
            auto response = send_request_async(request, yield);
            auto json = nlohmann::json::parse(response);
            auto trades = this->parse_trades(json);
            promise->set_value(trades);
        } catch (const std::exception& e) {
            promise->set_exception(std::current_exception());
        }
    });

    return future;
}

boost::future<std::vector<Trade>> AceAsync::fetch_order_trades_async(
    const std::string& id, const std::string& symbol, long since, int limit) {
    auto promise = std::make_shared<boost::promise<std::vector<Trade>>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, id, symbol, since, limit, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("GET", "/api/v2/orders/" + id + "/trades", {
                {"since", std::to_string(since)},
                {"limit", std::to_string(limit ? limit : 100)}
            }, true);
            auto response = send_request_async(request, yield);
            auto json = nlohmann::json::parse(response);
            auto trades = this->parse_trades(json);
            promise->set_value(trades);
        } catch (const std::exception& e) {
            promise->set_exception(std::current_exception());
        }
    });

    return future;
}

boost::future<Balance> AceAsync::fetch_balance_async() {
    auto promise = std::make_shared<boost::promise<Balance>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("GET", "/api/v2/accounts/balance", {}, true);
            auto response = send_request_async(request, yield);
            auto json = nlohmann::json::parse(response);
            auto balance = this->parse_balance(json);
            promise->set_value(balance);
        } catch (const std::exception& e) {
            promise->set_exception(std::current_exception());
        }
    });

    return future;
}

} // namespace ccxt
