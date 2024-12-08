#include "ccxt/exchanges/async/alpaca_async.h"
#include "ccxt/base/json.hpp"
#include "ccxt/base/errors.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/spawn.hpp>

namespace ccxt {

AlpacaAsync::AlpacaAsync(const boost::asio::io_context& context)
    : AsyncExchange(context)
    , Alpaca()
    , context_(const_cast<boost::asio::io_context&>(context)) {}

boost::future<std::vector<Market>> AlpacaAsync::fetch_markets_async() {
    auto promise = std::make_shared<boost::promise<std::vector<Market>>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("GET", "/v2/assets", {}, true);
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

boost::future<OrderBook> AlpacaAsync::fetch_order_book_async(const std::string& symbol, int limit) {
    auto promise = std::make_shared<boost::promise<OrderBook>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, symbol, limit, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("GET", "/v2/stocks/" + this->market_id(symbol) + "/orderbook", {
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

boost::future<std::map<std::string, Ticker>> AlpacaAsync::fetch_tickers_async() {
    auto promise = std::make_shared<boost::promise<std::map<std::string, Ticker>>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("GET", "/v2/stocks/snapshots", {});
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

boost::future<Ticker> AlpacaAsync::fetch_ticker_async(const std::string& symbol) {
    auto promise = std::make_shared<boost::promise<Ticker>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, symbol, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("GET", "/v2/stocks/" + this->market_id(symbol) + "/snapshot", {});
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

boost::future<std::vector<Trade>> AlpacaAsync::fetch_trades_async(
    const std::string& symbol, long since, int limit) {
    auto promise = std::make_shared<boost::promise<std::vector<Trade>>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, symbol, since, limit, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("GET", "/v2/stocks/" + this->market_id(symbol) + "/trades", {
                {"start", std::to_string(since)},
                {"limit", std::to_string(limit ? limit : 100)}
            });
            auto response = send_request_async(request, yield);
            auto json = nlohmann::json::parse(response);
            auto trades = this->parse_trades(json, symbol);
            promise->set_value(trades);
        } catch (const std::exception& e) {
            promise->set_exception(std::current_exception());
        }
    });

    return future;
}

boost::future<std::vector<OHLCV>> AlpacaAsync::fetch_ohlcv_async(
    const std::string& symbol, const std::string& timeframe, long since, int limit) {
    auto promise = std::make_shared<boost::promise<std::vector<OHLCV>>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, symbol, timeframe, since, limit, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("GET", "/v2/stocks/" + this->market_id(symbol) + "/bars", {
                {"timeframe", this->timeframes.at(timeframe)},
                {"start", std::to_string(since)},
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

boost::future<long> AlpacaAsync::fetch_time_async() {
    auto promise = std::make_shared<boost::promise<long>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("GET", "/v2/clock", {});
            auto response = send_request_async(request, yield);
            auto json = nlohmann::json::parse(response);
            promise->set_value(this->parse_8601(json["timestamp"].get<std::string>()));
        } catch (const std::exception& e) {
            promise->set_exception(std::current_exception());
        }
    });

    return future;
}

boost::future<Order> AlpacaAsync::create_order_async(
    const std::string& symbol, const std::string& type, const std::string& side,
    double amount, double price, const std::map<std::string, std::string>& params) {
    auto promise = std::make_shared<boost::promise<Order>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, symbol, type, side, amount, price, params, promise](boost::asio::yield_context yield) {
        try {
            auto request_params = params;
            request_params["symbol"] = this->market_id(symbol);
            request_params["qty"] = std::to_string(amount);
            request_params["side"] = side;
            request_params["type"] = type;
            if (price > 0) {
                request_params["limit_price"] = std::to_string(price);
            }

            auto request = prepare_request("POST", "/v2/orders", request_params, true);
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

boost::future<Order> AlpacaAsync::create_stop_order_async(
    const std::string& symbol, const std::string& type, const std::string& side,
    double amount, double price, const std::map<std::string, std::string>& params) {
    auto promise = std::make_shared<boost::promise<Order>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, symbol, type, side, amount, price, params, promise](boost::asio::yield_context yield) {
        try {
            auto request_params = params;
            request_params["symbol"] = this->market_id(symbol);
            request_params["qty"] = std::to_string(amount);
            request_params["side"] = side;
            request_params["type"] = "stop_" + type;
            if (price > 0) {
                request_params["stop_price"] = std::to_string(price);
            }

            auto request = prepare_request("POST", "/v2/orders", request_params, true);
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

boost::future<Order> AlpacaAsync::edit_order_async(
    const std::string& id, const std::string& symbol, const std::string& type,
    const std::string& side, double amount, double price,
    const std::map<std::string, std::string>& params) {
    auto promise = std::make_shared<boost::promise<Order>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, id, symbol, type, side, amount, price, params, promise](boost::asio::yield_context yield) {
        try {
            auto request_params = params;
            if (amount > 0) {
                request_params["qty"] = std::to_string(amount);
            }
            if (price > 0) {
                request_params["limit_price"] = std::to_string(price);
            }

            auto request = prepare_request("PATCH", "/v2/orders/" + id, request_params, true);
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

boost::future<Order> AlpacaAsync::cancel_order_async(const std::string& id, const std::string& symbol) {
    auto promise = std::make_shared<boost::promise<Order>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, id, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("DELETE", "/v2/orders/" + id, {}, true);
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

boost::future<std::vector<Order>> AlpacaAsync::cancel_all_orders_async(
    const std::string& symbol, const std::map<std::string, std::string>& params) {
    auto promise = std::make_shared<boost::promise<std::vector<Order>>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, symbol, params, promise](boost::asio::yield_context yield) {
        try {
            auto request_params = params;
            if (!symbol.empty()) {
                request_params["symbol"] = this->market_id(symbol);
            }

            auto request = prepare_request("DELETE", "/v2/orders", request_params, true);
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

boost::future<Order> AlpacaAsync::fetch_order_async(const std::string& id, const std::string& symbol) {
    auto promise = std::make_shared<boost::promise<Order>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, id, promise](boost::asio::yield_context yield) {
        try {
            auto request = prepare_request("GET", "/v2/orders/" + id, {}, true);
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

boost::future<std::vector<Order>> AlpacaAsync::fetch_orders_async(
    const std::string& symbol, long since, int limit) {
    auto promise = std::make_shared<boost::promise<std::vector<Order>>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, symbol, since, limit, promise](boost::asio::yield_context yield) {
        try {
            auto params = std::map<std::string, std::string>();
            if (!symbol.empty()) {
                params["symbol"] = this->market_id(symbol);
            }
            if (since > 0) {
                params["after"] = this->iso8601(since);
            }
            if (limit > 0) {
                params["limit"] = std::to_string(limit);
            }

            auto request = prepare_request("GET", "/v2/orders", params, true);
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

boost::future<std::vector<Order>> AlpacaAsync::fetch_open_orders_async(const std::string& symbol) {
    auto promise = std::make_shared<boost::promise<std::vector<Order>>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, symbol, promise](boost::asio::yield_context yield) {
        try {
            auto params = std::map<std::string, std::string>{
                {"status", "open"}
            };
            if (!symbol.empty()) {
                params["symbol"] = this->market_id(symbol);
            }

            auto request = prepare_request("GET", "/v2/orders", params, true);
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

boost::future<std::vector<Order>> AlpacaAsync::fetch_closed_orders_async(
    const std::string& symbol, long since, int limit) {
    auto promise = std::make_shared<boost::promise<std::vector<Order>>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, symbol, since, limit, promise](boost::asio::yield_context yield) {
        try {
            auto params = std::map<std::string, std::string>{
                {"status", "closed"}
            };
            if (!symbol.empty()) {
                params["symbol"] = this->market_id(symbol);
            }
            if (since > 0) {
                params["after"] = this->iso8601(since);
            }
            if (limit > 0) {
                params["limit"] = std::to_string(limit);
            }

            auto request = prepare_request("GET", "/v2/orders", params, true);
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

boost::future<std::vector<Trade>> AlpacaAsync::fetch_my_trades_async(
    const std::string& symbol, long since, int limit) {
    auto promise = std::make_shared<boost::promise<std::vector<Trade>>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, symbol, since, limit, promise](boost::asio::yield_context yield) {
        try {
            auto params = std::map<std::string, std::string>();
            if (!symbol.empty()) {
                params["symbol"] = this->market_id(symbol);
            }
            if (since > 0) {
                params["after"] = this->iso8601(since);
            }
            if (limit > 0) {
                params["limit"] = std::to_string(limit);
            }

            auto request = prepare_request("GET", "/v2/account/activities/FILL", params, true);
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

boost::future<DepositAddress> AlpacaAsync::fetch_deposit_address_async(
    const std::string& code, const std::map<std::string, std::string>& params) {
    auto promise = std::make_shared<boost::promise<DepositAddress>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, code, params, promise](boost::asio::yield_context yield) {
        try {
            auto currency = this->currency(code);
            auto request = prepare_request("GET", "/v2/crypto/wallets/" + currency["id"].get<std::string>() + "/address", params, true);
            auto response = send_request_async(request, yield);
            auto json = nlohmann::json::parse(response);
            auto address = this->parse_deposit_address(json);
            promise->set_value(address);
        } catch (const std::exception& e) {
            promise->set_exception(std::current_exception());
        }
    });

    return future;
}

boost::future<std::vector<Transaction>> AlpacaAsync::fetch_deposits_async(
    const std::string& code, long since, int limit, const std::map<std::string, std::string>& params) {
    auto promise = std::make_shared<boost::promise<std::vector<Transaction>>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, code, since, limit, params, promise](boost::asio::yield_context yield) {
        try {
            auto request_params = params;
            request_params["type"] = "deposit";
            if (!code.empty()) {
                auto currency = this->currency(code);
                request_params["currency"] = currency["id"];
            }
            if (since > 0) {
                request_params["after"] = this->iso8601(since);
            }
            if (limit > 0) {
                request_params["limit"] = std::to_string(limit);
            }

            auto request = prepare_request("GET", "/v2/account/activities", request_params, true);
            auto response = send_request_async(request, yield);
            auto json = nlohmann::json::parse(response);
            auto transactions = this->parse_transactions(json);
            promise->set_value(transactions);
        } catch (const std::exception& e) {
            promise->set_exception(std::current_exception());
        }
    });

    return future;
}

boost::future<std::vector<Transaction>> AlpacaAsync::fetch_withdrawals_async(
    const std::string& code, long since, int limit, const std::map<std::string, std::string>& params) {
    auto promise = std::make_shared<boost::promise<std::vector<Transaction>>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, code, since, limit, params, promise](boost::asio::yield_context yield) {
        try {
            auto request_params = params;
            request_params["type"] = "withdrawal";
            if (!code.empty()) {
                auto currency = this->currency(code);
                request_params["currency"] = currency["id"];
            }
            if (since > 0) {
                request_params["after"] = this->iso8601(since);
            }
            if (limit > 0) {
                request_params["limit"] = std::to_string(limit);
            }

            auto request = prepare_request("GET", "/v2/account/activities", request_params, true);
            auto response = send_request_async(request, yield);
            auto json = nlohmann::json::parse(response);
            auto transactions = this->parse_transactions(json);
            promise->set_value(transactions);
        } catch (const std::exception& e) {
            promise->set_exception(std::current_exception());
        }
    });

    return future;
}

boost::future<std::vector<Transaction>> AlpacaAsync::fetch_deposits_withdrawals_async(
    const std::string& code, long since, int limit, const std::map<std::string, std::string>& params) {
    auto promise = std::make_shared<boost::promise<std::vector<Transaction>>>();
    auto future = promise->get_future();

    boost::asio::spawn(context_, [this, code, since, limit, params, promise](boost::asio::yield_context yield) {
        try {
            auto request_params = params;
            if (!code.empty()) {
                auto currency = this->currency(code);
                request_params["currency"] = currency["id"];
            }
            if (since > 0) {
                request_params["after"] = this->iso8601(since);
            }
            if (limit > 0) {
                request_params["limit"] = std::to_string(limit);
            }

            auto request = prepare_request("GET", "/v2/account/activities", request_params, true);
            auto response = send_request_async(request, yield);
            auto json = nlohmann::json::parse(response);
            auto transactions = this->parse_transactions(json);
            promise->set_value(transactions);
        } catch (const std::exception& e) {
            promise->set_exception(std::current_exception());
        }
    });

    return future;
}

} // namespace ccxt
