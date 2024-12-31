#include "ccxt/base/exchange.h"
#include "ccxt/base/errors.h"
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>
#include <regex>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <curl/curl.h>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <optional>
#include <stdexcept>
#include <future>
#include <boost/beast/version.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/coroutine2/coroutine.hpp>


namespace ccxt {

Exchange::Exchange(boost::asio::io_context& context, const Config& config) : ExchangeBase(context, config) {
    rateLimit= 2000;
    pro = false;
    certified = false;
    lastRestRequestTimestamp = 0;
    curl_ = curl_easy_init();
    init();
}

void Exchange::init() {
    // Default implementation
}

void Exchange::describe() const {
    
}

AsyncPullType Exchange::performHttpRequest(const std::string& host, const std::string& target, const std::string& method) {
            return AsyncPullType(
            [this, host, target, method](boost::coroutines2::coroutine<json>::push_type& yield) {
                try {
                    boost::asio::ip::tcp::resolver resolver(context_);
                    boost::beast::tcp_stream stream(context_);

                    auto const results = resolver.resolve(host, "443");
                    stream.connect(results);

                    boost::beast::http::request<boost::beast::http::string_body> req{
                        boost::beast::http::string_to_verb(method), target, 11};
                    req.set(boost::beast::http::field::host, host);
                    req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

                    boost::beast::http::write(stream, req);

                    boost::beast::flat_buffer buffer;
                    boost::beast::http::response<boost::beast::http::dynamic_body> res;

                    boost::beast::http::read(stream, buffer, res);

                    auto body = boost::beast::buffers_to_string(res.body().data());
                    json response = json::parse(body);

                    yield(response);

                    boost::beast::error_code ec;
                    stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

                    if (ec && ec != boost::beast::errc::not_connected)
                        throw boost::beast::system_error{ec};

                } catch (const std::exception& e) {
                    // Handle exceptions
                    throw;
                }
            });
           
}

// Synchronous REST API methods
json Exchange::fetchMarkets(const json& params) {
    return json::object();
}

json Exchange::fetchTicker(const std::string& symbol, const json& params) {
    return json::object();
}

json Exchange::fetchTickers(const std::vector<std::string>& symbols, const json& params) {
    return json::object();
}

json Exchange::fetchOrderBook(const std::string& symbol, int limit, const json& params) {
    return json::object();
}

json Exchange::fetchTrades(const std::string& symbol, int since, int limit, const json& params) {
    return json::object();
}

json Exchange::fetchOHLCV(const std::string& symbol, const std::string& timeframe,
                         int since, int limit, const json& params) {
    return json::object();
}

json Exchange::fetchBalance(const json& params) {
    return json::object();
}

json Exchange::createOrder(const std::string& symbol, const std::string& type, const std::string& side,
                         double amount, double price, const json& params) {
    return json::object();
}

json Exchange::cancelOrder(const std::string& id, const std::string& symbol, const json& params) {
    return json::object();
}

json Exchange::fetchOrder(const std::string& id, const std::string& symbol, const json& params) {
    return json::object();
}

json Exchange::fetchOrders(const std::string& symbol, int since, int limit, const json& params) {
    return json::object();
}

json Exchange::fetchOpenOrders(const std::string& symbol, int since, int limit, const json& params) {
    return json::object();
}

json Exchange::fetchClosedOrders(const std::string& symbol, int since, int limit, const json& params) {
    return json::object();
}

// Asynchronous REST API methods
AsyncPullType Exchange::fetchMarketsAsync(const json& params) {
    return AsyncPullType(
        [this](boost::coroutines2::coroutine<json>::push_type& yield) {
            try {
                // Call the synchronous fetchOrder method
                
                yield(json::object()); // Yield the result to the caller
            } catch (const std::exception& e) {
                // Handle exceptions and yield an error response if needed
                throw; // or yield an error response
            }
        });
}

AsyncPullType Exchange::fetchTickerAsync(const std::string& symbol, const json& params) {
    return AsyncPullType(
        [this, symbol, params](boost::coroutines2::coroutine<json>::push_type& yield) {
            try {
                // Call the synchronous fetchOrder method
                
                yield(json::object()); // Yield the result to the caller
            } catch (const std::exception& e) {
                // Handle exceptions and yield an error response if needed
                throw; // or yield an error response
            }
        });
}

AsyncPullType Exchange::fetchTickersAsync(const std::vector<std::string>& symbols, const json& params) {
    return AsyncPullType(
        [this](boost::coroutines2::coroutine<json>::push_type& yield) {
            try {
                // Call the synchronous fetchOrder method
                
                yield(json::object()); // Yield the result to the caller
            } catch (const std::exception& e) {
                // Handle exceptions and yield an error response if needed
                throw; // or yield an error response
            }
        });
}

AsyncPullType Exchange::fetchOrderBookAsync(const std::string& symbol, int limit, const json& params) {
    return AsyncPullType(
        [this](boost::coroutines2::coroutine<json>::push_type& yield) {
            try {
                // Call the synchronous fetchOrder method
                
                yield(json::object()); // Yield the result to the caller
            } catch (const std::exception& e) {
                // Handle exceptions and yield an error response if needed
                throw; // or yield an error response
            }
        });
}

AsyncPullType Exchange::fetchTradesAsync(const std::string& symbol, int since, int limit, const json& params) {
    return AsyncPullType(
        [this](boost::coroutines2::coroutine<json>::push_type& yield) {
            try {
                // Call the synchronous fetchOrder method
                
                yield(json::object()); // Yield the result to the caller
            } catch (const std::exception& e) {
                // Handle exceptions and yield an error response if needed
                throw; // or yield an error response
            }
        });
}

AsyncPullType Exchange::fetchOHLCVAsync(const std::string& symbol, const std::string& timeframe,
                                         int since, int limit, const json& params) {
    return AsyncPullType(
        [this](boost::coroutines2::coroutine<json>::push_type& yield) {
            try {
                // Call the synchronous fetchOrder method
                
                yield(json::object()); // Yield the result to the caller
            } catch (const std::exception& e) {
                // Handle exceptions and yield an error response if needed
                throw; // or yield an error response
            }
        });
}

AsyncPullType Exchange::fetchBalanceAsync(const json& params) {
    return AsyncPullType(
        [this](boost::coroutines2::coroutine<json>::push_type& yield) {
            try {
                // Call the synchronous fetchOrder method
                
                yield(json::object()); // Yield the result to the caller
            } catch (const std::exception& e) {
                // Handle exceptions and yield an error response if needed
                throw; // or yield an error response
            }
        });
}

AsyncPullType Exchange::createOrderAsync(const std::string& symbol, const std::string& type, const std::string& side,
                                         double amount, double price, const json& params) {
    return AsyncPullType(
        [this](boost::coroutines2::coroutine<json>::push_type& yield) {
            try {
                // Call the synchronous fetchOrder method
                
                yield(json::object()); // Yield the result to the caller
            } catch (const std::exception& e) {
                // Handle exceptions and yield an error response if needed
                throw; // or yield an error response
            }
        });
}

AsyncPullType Exchange::cancelOrderAsync(const std::string& id, const std::string& symbol, const json& params) {
    return AsyncPullType(
        [this](boost::coroutines2::coroutine<json>::push_type& yield) {
            try {
                // Call the synchronous fetchOrder method
                
                yield(json::object()); // Yield the result to the caller
            } catch (const std::exception& e) {
                // Handle exceptions and yield an error response if needed
                throw; // or yield an error response
            }
        });
}

AsyncPullType Exchange::fetchOrderAsync(const std::string& id, const std::string& symbol, const json& params) {
    return AsyncPullType(
        [this](boost::coroutines2::coroutine<json>::push_type& yield) {
            try {
                // Call the synchronous fetchOrder method
                
                yield(json::object()); // Yield the result to the caller
            } catch (const std::exception& e) {
                // Handle exceptions and yield an error response if needed
                throw; // or yield an error response
            }
        });
}

AsyncPullType Exchange::fetchOrdersAsync(const std::string& symbol, int since, int limit, const json& params) {
    return AsyncPullType(
        [this](boost::coroutines2::coroutine<json>::push_type& yield) {
            try {
                // Call the synchronous fetchOrder method
                
                yield(json::object()); // Yield the result to the caller
            } catch (const std::exception& e) {
                // Handle exceptions and yield an error response if needed
                throw; // or yield an error response
            }
        });
}

AsyncPullType Exchange::fetchOpenOrdersAsync(const std::string& symbol, int since, int limit, const json& params) {
     return AsyncPullType(
        [this](boost::coroutines2::coroutine<json>::push_type& yield) {
            try {
                // Call the synchronous fetchOrder method
                
                yield(json::object()); // Yield the result to the caller
            } catch (const std::exception& e) {
                // Handle exceptions and yield an error response if needed
                throw; // or yield an error response
            }
        });
}

AsyncPullType Exchange::fetchClosedOrdersAsync(const std::string& symbol, int since, int limit, const json& params) {
   return AsyncPullType(
        [this](boost::coroutines2::coroutine<json>::push_type& yield) {
            try {
                // Call the synchronous fetchOrder method
                
                yield(json::object()); // Yield the result to the caller
            } catch (const std::exception& e) {
                // Handle exceptions and yield an error response if needed
                throw; // or yield an error response
            }
        });
}

AsyncPullType Exchange::fetchAsync(const std::string& url, const std::string& method,
                                   const std::map<std::string, std::string>& headers,
                                   const std::string& body) {
     return AsyncPullType(
        [this](boost::coroutines2::coroutine<json>::push_type& yield) {
            try {
                // Call the synchronous fetchOrder method
                
                yield(json::object()); // Yield the result to the caller
            } catch (const std::exception& e) {
                // Handle exceptions and yield an error response if needed
                throw; // or yield an error response
            }
        });
}

// Utility methods
std::string Exchange::sign(const std::string& path, const std::string& api,
                     const std::string& method, const std::map<std::string, std::string>& params,
                     const std::map<std::string, std::string>& headers) {
    return "";
}

std::string Exchange::implodeParams(const std::string& path, const json& params) {
    std::string result = path;
    for (const auto& [key, value] : params.items()) {
        result = std::regex_replace(result, std::regex("\\{" + key + "\\}"),
                                  value.get<std::string>());
    }
    return result;
}

std::vector<std::string> Exchange::extractParams(const std::string& path) {
    std::vector<std::string> result;
    std::regex pattern("\\{([^}]+)\\}");
    auto words_begin = std::sregex_iterator(path.begin(), path.end(), pattern);
    auto words_end = std::sregex_iterator();
    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        std::smatch match = *i;
        result.push_back(match[1].str());
    }
    return result;
}

std::string Exchange::urlencode(const json& params) {
    std::ostringstream result;
    bool first = true;
    for (const auto& [key, value] : params.items()) {
        if (!first) {
            result << "&";
        }
        first = false;
        result << encode(key) << "=" << encode(value.get<std::string>());
    }
    return result.str();
}

std::string Exchange::encode(const std::string& string) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (char c : string) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }
        escaped << std::uppercase;
        escaped << '%' << std::setw(2) << int((unsigned char)c);
        escaped << std::nouppercase;
    }

    return escaped.str();
}

std::string Exchange::hmac(const std::string& message, const std::string& secret,
                     const std::string& algorithm, const std::string& digest) {
    unsigned char* digest_value = nullptr;
    unsigned int digest_len = 0;

    if (algorithm == "sha256") {
        digest_value = HMAC(EVP_sha256(), secret.c_str(), secret.length(),
                          (unsigned char*)message.c_str(), message.length(),
                          nullptr, &digest_len);
    } else if (algorithm == "sha512") {
        digest_value = HMAC(EVP_sha512(), secret.c_str(), secret.length(),
                          (unsigned char*)message.c_str(), message.length(),
                          nullptr, &digest_len);
    } else {
        throw NotSupported("Unsupported hash algorithm: " + algorithm);
    }

    if (!digest_value) {
        throw ExchangeError("HMAC failed");
    }

    std::ostringstream result;
    if (digest == "hex") {
        for (unsigned int i = 0; i < digest_len; i++) {
            result << std::hex << std::setw(2) << std::setfill('0')
                  << (int)digest_value[i];
        }
    } else if (digest == "base64") {
        // TODO: Implement base64 encoding
        throw NotSupported("Base64 encoding not implemented yet");
    } else {
        throw NotSupported("Unsupported digest format: " + digest);
    }

    return result.str();
}

long long Exchange::milliseconds() const {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

std::string Exchange::uuid() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);

    std::stringstream ss;
    int i;
    ss << std::hex;
    for (i = 0; i < 8; i++) {
        ss << dis(gen);
    }
    ss << "-";
    for (i = 0; i < 4; i++) {
        ss << dis(gen);
    }
    ss << "-4";
    for (i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";
    ss << dis2(gen);
    for (i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";
    for (i = 0; i < 12; i++) {
        ss << dis(gen);
    };
    return ss.str();
}

std::string Exchange::iso8601(long long timestamp) const {
    std::time_t time = timestamp / 1000;
    std::tm* tm = std::gmtime(&time);
    char buffer[30];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", tm);
    int milliseconds = timestamp % 1000;
    sprintf(buffer + strlen(buffer), ".%03dZ", milliseconds);
    return std::string(buffer);
}

long long Exchange::parse8601(const std::string& datetime) {
    std::tm tm = {};
    std::istringstream ss(datetime);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        tp.time_since_epoch()
    ).count();
}

Market Exchange::market(const std::string& symbol) {
    if (markets.find(symbol) == markets.end()) {
        throw ExchangeError("Market '" + symbol + "' does not exist");
    }
    return markets[symbol];
}

void Exchange::loadMarkets(bool reload) {
    if (!markets.empty() && !reload) {
        return;
    }
    json response = fetchMarkets();
    for (const auto& market : response) {
        markets[market["symbol"]] = market;
        markets_by_id[market["id"]] = market;
    }
}

std::string Exchange::marketId(const std::string& symbol) {
    return market(symbol).id;
}

std::string Exchange::safeString(const json& obj, const std::string& key, const std::string& defaultValue) const {
    if (obj.contains(key) && obj[key].is_string()) {
        return obj[key].get<std::string>();
    }
    return defaultValue;
}

double Exchange::safeNumber(const json& obj, const std::string& key, double defaultValue) const {
    if (obj.contains(key)) {
        if (obj[key].is_number()) {
            return obj[key].get<double>();
        }
        if (obj[key].is_string()) {
            try {
                return std::stod(obj[key].get<std::string>());
            } catch (...) {
                return defaultValue;
            }
        }
    }
    return defaultValue;
}

long long Exchange::safeInteger(const json& obj, const std::string& key, long long defaultValue) const {
    if (obj.contains(key)) {
        if (obj[key].is_number_integer()) {
            return obj[key].get<long long>();
        }
        if (obj[key].is_string()) {
            try {
                return std::stoll(obj[key].get<std::string>());
            } catch (...) {
                return defaultValue;
            }
        }
    }
    return defaultValue;
}

bool Exchange::safeBoolean(const json& obj, const std::string& key, bool defaultValue) const {
    if (obj.contains(key)) {
        if (obj[key].is_boolean()) {
            return obj[key].get<bool>();
        }
        if (obj[key].is_string()) {
            std::string value = obj[key].get<std::string>();
            if (value == "true" || value == "1") return true;
            if (value == "false" || value == "0") return false;
        }
        if (obj[key].is_number()) {
            return obj[key].get<int>() != 0;
        }
    }
    return defaultValue;
}

std::string Exchange::symbol(const std::string& marketId) {
    if (markets_by_id.find(marketId) == markets_by_id.end()) {
        throw ExchangeError("Market ID '" + marketId + "' does not exist");
    }
    return markets_by_id[marketId].symbol;
}

std::string Exchange::amountToPrecision(const std::string& symbol, double amount) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(8) << amount;
    return ss.str();
}

std::string Exchange::priceToPrecision(const std::string& symbol, double price) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(8) << price;
    return ss.str();
}

std::string Exchange::feeToPrecision(const std::string& symbol, double fee) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(8) << fee;
    return ss.str();
}

std::string Exchange::currencyToPrecision(const std::string& currency, double fee) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(8) << fee;
    return ss.str();
}

std::string Exchange::costToPrecision(const std::string& symbol, double cost) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(8) << cost;
    return ss.str();
}

json Exchange::fetch(const std::string& url, const std::string& method,
                    const std::map<std::string, std::string>& headers,
                    const std::string& body) {

    std::string readBuffer;
    if(curl_) {
        struct curl_slist* curl_headers = nullptr;
        std::map<std::string, std::string>::const_iterator it = headers.begin();
        for (; it != headers.end(); ++it) {
            curl_headers = curl_slist_append(curl_headers, (it->first + ": " + it->second).c_str());
        }
        curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, curl_headers);
        curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &readBuffer);
        CURLcode res = curl_easy_perform(curl_);
        long httpCode = 0;
        curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &httpCode);

        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        } else if (httpCode != 200) {
            std::cerr << "HTTP request failed with code: " << httpCode << std::endl;
        } else {
            try {
                auto jsonResponse = nlohmann::json::parse(readBuffer);
                // Process the JSON response (e.g., check order status)
                std::cout << jsonResponse.dump(4) << std::endl;
                return jsonResponse; // Pretty print
            } catch (const nlohmann::json::parse_error& e) {
                std::cerr << "JSON parse error: " << e.what() << std::endl;
            }
        }
        curl_easy_cleanup(curl_);
    }
    return json::parse(readBuffer);
}

json Exchange::omit(const json& params, const std::vector<std::string>& keys) {
    json result = params;
    for (const auto& key : keys) {
        result.erase(key);
    }
    return result;
}

} // namespace ccxt
