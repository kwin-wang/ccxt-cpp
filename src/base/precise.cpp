#include "ccxt/base/precise.h"
#include <regex>
#include <sstream>
#include <cmath>
#include <algorithm>

namespace ccxt {

Precise::Precise(const std::string& str) : value(normalize(str)) {}

Precise::Precise(const char* str) : value(normalize(std::string(str))) {}

Precise::Precise(int val) : value(std::to_string(val)) {}

Precise::Precise(long val) : value(std::to_string(val)) {}

Precise::Precise(double val) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(20) << val;
    value = normalize(ss.str());
}

std::string Precise::normalize(const std::string& string) {
    std::string result = string;
    
    // Remove leading zeros
    result = std::regex_replace(result, std::regex("^0+"), "");
    
    // Handle negative numbers
    bool isNegative = false;
    if (!result.empty() && result[0] == '-') {
        isNegative = true;
        result = result.substr(1);
    }
    
    // Handle decimal point
    size_t decimalPos = result.find('.');
    if (decimalPos != std::string::npos) {
        // Remove trailing zeros after decimal point
        result = std::regex_replace(result, std::regex("0+$"), "");
        // Remove decimal point if no decimals
        if (result.back() == '.') {
            result.pop_back();
        }
    }
    
    // Handle empty or zero string
    if (result.empty() || result == "0") {
        return "0";
    }
    
    return isNegative ? "-" + result : result;
}

Precise Precise::string_mul(const std::string& string1, const std::string& string2) {
    return Precise(string1).mul(Precise(string2));
}

Precise Precise::string_div(const std::string& string1, const std::string& string2, int precision) {
    return Precise(string1).div(Precise(string2), precision);
}

Precise Precise::string_add(const std::string& string1, const std::string& string2) {
    return Precise(string1).add(Precise(string2));
}

Precise Precise::string_sub(const std::string& string1, const std::string& string2) {
    return Precise(string1).sub(Precise(string2));
}

Precise Precise::string_mod(const std::string& string1, const std::string& string2) {
    return Precise(string1).mod(Precise(string2));
}

Precise Precise::string_pow(const std::string& string1, const std::string& string2) {
    return Precise(string1).pow(Precise(string2));
}

int Precise::string_eq(const std::string& string1, const std::string& string2) {
    return Precise(string1).eq(Precise(string2));
}

int Precise::string_gt(const std::string& string1, const std::string& string2) {
    return Precise(string1).gt(Precise(string2));
}

int Precise::string_ge(const std::string& string1, const std::string& string2) {
    return Precise(string1).ge(Precise(string2));
}

int Precise::string_lt(const std::string& string1, const std::string& string2) {
    return Precise(string1).lt(Precise(string2));
}

int Precise::string_le(const std::string& string1, const std::string& string2) {
    return Precise(string1).le(Precise(string2));
}

Precise Precise::mul(const Precise& other) const {
    // Implement multiplication for arbitrary precision arithmetic
    // This is a simplified implementation
    double val1 = std::stod(value);
    double val2 = std::stod(other.value);
    return Precise(val1 * val2);
}

Precise Precise::div(const Precise& other, int precision) const {
    // Implement division for arbitrary precision arithmetic
    // This is a simplified implementation
    if (other.value == "0") {
        throw std::runtime_error("Division by zero");
    }
    double val1 = std::stod(value);
    double val2 = std::stod(other.value);
    return Precise(val1 / val2);
}

Precise Precise::add(const Precise& other) const {
    // Implement addition for arbitrary precision arithmetic
    // This is a simplified implementation
    double val1 = std::stod(value);
    double val2 = std::stod(other.value);
    return Precise(val1 + val2);
}

Precise Precise::sub(const Precise& other) const {
    // Implement subtraction for arbitrary precision arithmetic
    // This is a simplified implementation
    double val1 = std::stod(value);
    double val2 = std::stod(other.value);
    return Precise(val1 - val2);
}

Precise Precise::mod(const Precise& other) const {
    // Implement modulo for arbitrary precision arithmetic
    // This is a simplified implementation
    if (other.value == "0") {
        throw std::runtime_error("Modulo by zero");
    }
    double val1 = std::stod(value);
    double val2 = std::stod(other.value);
    return Precise(std::fmod(val1, val2));
}

Precise Precise::pow(const Precise& other) const {
    // Implement power for arbitrary precision arithmetic
    // This is a simplified implementation
    double val1 = std::stod(value);
    double val2 = std::stod(other.value);
    return Precise(std::pow(val1, val2));
}

bool Precise::eq(const Precise& other) const {
    return value == other.value;
}

bool Precise::gt(const Precise& other) const {
    // This is a simplified implementation
    double val1 = std::stod(value);
    double val2 = std::stod(other.value);
    return val1 > val2;
}

bool Precise::ge(const Precise& other) const {
    return gt(other) || eq(other);
}

bool Precise::lt(const Precise& other) const {
    return !ge(other);
}

bool Precise::le(const Precise& other) const {
    return !gt(other);
}

std::string Precise::toString() const {
    return value;
}

double Precise::toDouble() const {
    return std::stod(value);
}

} // namespace ccxt
